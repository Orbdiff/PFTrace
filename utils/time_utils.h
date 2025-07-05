#pragma once

#include <windows.h>
#include <ntsecapi.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>

#pragma comment(lib, "Secur32.lib")

time_t FileTimeToTimeT(const FILETIME& ft)
{
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;
    return static_cast<time_t>((ull.QuadPart - 116444736000000000ULL) / 10000000ULL);
}

time_t GetCurrentUserLogonTime()
{
    wchar_t username[256]; DWORD size = sizeof(username) / sizeof(wchar_t);
    if (!GetUserNameW(username, &size)) return 0;

    ULONG count = 0; PLUID sessions = nullptr;
    if (LsaEnumerateLogonSessions(&count, &sessions) != 0) return 0;

    time_t result = 0;
    for (ULONG i = 0; i < count; i++) {
        PSECURITY_LOGON_SESSION_DATA pData = nullptr;
        if (LsaGetLogonSessionData(&sessions[i], &pData) == 0 && pData) {
            if (pData->UserName.Buffer && pData->LogonType == Interactive &&
                _wcsicmp(pData->UserName.Buffer, username) == 0) {
                FILETIME ft{ pData->LogonTime.LowPart, pData->LogonTime.HighPart };
                result = FileTimeToTimeT(ft);
                LsaFreeReturnBuffer(pData);
                break;
            }
            LsaFreeReturnBuffer(pData);
        }
    }
    if (sessions) LsaFreeReturnBuffer(sessions);
    return result;
}

void print_time(time_t t)
{
    if (t == 0) return;
    struct tm timeinfo; localtime_s(&timeinfo, &t);
    std::cout << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S") << "\n";
}