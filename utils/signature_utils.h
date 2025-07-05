#pragma once

#include <windows.h>
#include <wintrust.h>
#include <softpub.h>
#include <string>
#include <unordered_map>

#pragma comment(lib, "wintrust.lib")

std::unordered_map<std::wstring, bool> signatureCache;

bool VerifyFileSignature(const wchar_t* filePath)
{
    WINTRUST_FILE_INFO fileInfo = { sizeof(WINTRUST_FILE_INFO), filePath, nullptr, nullptr };
    WINTRUST_DATA winTrustData = { sizeof(WINTRUST_DATA) };
    winTrustData.dwUIChoice = WTD_UI_NONE;
    winTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    winTrustData.dwUnionChoice = WTD_CHOICE_FILE;
    winTrustData.dwStateAction = WTD_STATEACTION_IGNORE;
    winTrustData.pFile = &fileInfo;
    GUID policyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    return WinVerifyTrust(NULL, &policyGUID, &winTrustData) == ERROR_SUCCESS;
}

bool VerifySignatureForPath(const std::wstring& path)
{
    auto it = signatureCache.find(path);
    if (it != signatureCache.end()) return it->second;
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES)
        return (signatureCache[path] = false);
    return (signatureCache[path] = VerifyFileSignature(path.c_str()));
}