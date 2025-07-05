#pragma once

#include <windows.h>
#include <string>
#include "prefetch_parser.hh"

std::wstring GetDriveLetterFromSerial(DWORD serial)
{
    wchar_t volumeName[] = L"A:\\";
    for (wchar_t drive = L'A'; drive <= L'Z'; ++drive) {
        volumeName[0] = drive;
        DWORD volSerial = 0;
        if (GetVolumeInformationW(volumeName, nullptr, 0, &volSerial, nullptr, nullptr, nullptr, 0)) {
            if (volSerial == serial) return std::wstring(1, drive) + L":\\";
        }
    }
    return L"";
}

std::wstring ReplaceVolumeWithDrive(const std::wstring& path, std::wstring& out_drive_letter)
{
    const std::wstring volume_prefix = L"\\VOLUME{";
    size_t start = path.find(volume_prefix);
    if (start == std::wstring::npos) return path;
    size_t end = path.find(L'}', start);
    if (end == std::wstring::npos) return path;
    std::wstring volume_id = path.substr(start + volume_prefix.size(), end - start - volume_prefix.size());
    size_t dash = volume_id.find_last_of(L'-');
    if (dash == std::wstring::npos) return path;
    std::wstring serial_str = volume_id.substr(dash + 1);
    DWORD serial = std::wcstoul(serial_str.c_str(), nullptr, 16);
    std::wstring drive = GetDriveLetterFromSerial(serial);
    if (!drive.empty()) {
        out_drive_letter = drive;
        std::wstring new_path = path;
        new_path.replace(start, (end - start) + 2, drive);
        return new_path;
    }
    return path;
}