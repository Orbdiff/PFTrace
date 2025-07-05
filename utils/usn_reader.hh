#pragma once

#include <Windows.h>
#include <winioctl.h>
#include <chrono>
#include <string>
#include <format>
#include <iostream>
#include <vector>
#include "time_utils.h"

class USNJournalReader {
public:
    USNJournalReader(const std::wstring& volumeLetter)
        : volumeLetter_(volumeLetter) {
    }

    int Run()
    {
        std::wcout << L"[*] Starting USN Journal analysis...\n\n";

        if (!Dump()) {
            std::wcerr << L"[-] Failed to read the USN Journal.\n";
            return 1;
        }

        return 0;
    }

private:
    std::wstring volumeLetter_;
    HANDLE volumeHandle_ = INVALID_HANDLE_VALUE;
    BYTE* buffer_ = nullptr;
    USN_JOURNAL_DATA_V0 journalData_{};

    bool Dump()
    {
        if (!OpenVolume()) return false;
        if (!QueryJournal()) { CloseVolume(); return false; }
        if (!AllocateBuffer()) { CloseVolume(); return false; }

        bool result = ReadJournalAndPrint();
        Cleanup();
        return result;
    }

    bool OpenVolume()
    {
        std::wstring devicePath = L"\\\\.\\" + volumeLetter_;
        volumeHandle_ = CreateFileW(
            devicePath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );
        return volumeHandle_ != INVALID_HANDLE_VALUE;
    }

    bool QueryJournal()
    {
        DWORD bytesReturned = 0;
        return DeviceIoControl(
            volumeHandle_,
            FSCTL_QUERY_USN_JOURNAL,
            nullptr, 0,
            &journalData_, sizeof(journalData_),
            &bytesReturned,
            nullptr);
    }

    bool AllocateBuffer()
    {
        const DWORD bufferSize = 32 * 1024 * 1024; // 32 MB
        buffer_ = (BYTE*)VirtualAlloc(nullptr, bufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        return buffer_ != nullptr;
    }

    bool ReadJournalAndPrint()
    {
        READ_USN_JOURNAL_DATA_V0 readData = {};
        readData.StartUsn = journalData_.FirstUsn;
        readData.ReasonMask = 0xFFFFFFFF;
        readData.UsnJournalID = journalData_.UsnJournalID;

        const DWORD bufferSize = 32 * 1024 * 1024;
        DWORD bytesReturned = 0;
        size_t totalRecords = 0;

        time_t logonTime = GetCurrentUserLogonTime();

        while (DeviceIoControl(
            volumeHandle_,
            FSCTL_READ_USN_JOURNAL,
            &readData, sizeof(readData),
            buffer_, bufferSize,
            &bytesReturned,
            nullptr)) {

            if (bytesReturned <= sizeof(USN)) break;

            BYTE* ptr = buffer_ + sizeof(USN);
            BYTE* end = buffer_ + bytesReturned;

            while (ptr < end) {
                USN_RECORD_V2* record = reinterpret_cast<USN_RECORD_V2*>(ptr);
                if (record->RecordLength == 0) break;

                if ((record->Reason & 0x00000200) != 0) { // FILE_DELETE
                    std::wstring filenameW(record->FileName, record->FileNameLength / sizeof(WCHAR));
                    if (filenameW.size() >= 3 &&
                        _wcsicmp(filenameW.substr(filenameW.size() - 3).c_str(), L".pf") == 0) {

                        FILETIME ft;
                        ft.dwLowDateTime = record->TimeStamp.LowPart;
                        ft.dwHighDateTime = record->TimeStamp.HighPart;

                        time_t usnTime = FileTimeToTimeT(ft);
                        if (usnTime > logonTime) {
                            char filenameUtf8[1024] = {};
                            int len = WideCharToMultiByte(CP_UTF8, 0, record->FileName,
                                record->FileNameLength / sizeof(WCHAR),
                                filenameUtf8, sizeof(filenameUtf8) - 1, nullptr, nullptr);
                            filenameUtf8[len] = '\0';

                            std::cout << "Filename: " << filenameUtf8 << "\n";
                            std::cout << "Reason: File Delete\n";
                            std::cout << "Time: ";
                            print_time(usnTime);
                            std::cout << "\n";

                            totalRecords++;
                        }
                    }
                }

                ptr += record->RecordLength;
            }

            readData.StartUsn = *(USN*)buffer_;
        }

        std::wcout << std::format(L"[+] Total .pf deleted after logon time: {}\n", totalRecords);
        return true;
    }

    void Cleanup()
    {
        FreeBuffer();
        CloseVolume();
    }

    void FreeBuffer()
    {
        if (buffer_) {
            VirtualFree(buffer_, 0, MEM_RELEASE);
            buffer_ = nullptr;
        }
    }

    void CloseVolume()
    {
        if (volumeHandle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(volumeHandle_);
            volumeHandle_ = INVALID_HANDLE_VALUE;
        }
    }
};