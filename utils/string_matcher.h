#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <fstream>

bool ContainsTerm(const std::string& content, const std::vector<std::string>& terms)
{
    return std::any_of(terms.begin(), terms.end(), [&](const std::string& term) {
        return content.find(term) != std::string::npos;
        });
}

bool ContainsTerm(const std::wstring& content, const std::vector<std::wstring>& terms)
{
    return std::any_of(terms.begin(), terms.end(), [&](const std::wstring& term) {
        return content.find(term) != std::wstring::npos;
        });
}

bool CheckFileForFlags(const std::wstring& file_path, bool& typeA, bool& typeB, bool& typeC, bool& typeD)
{
    typeA = typeB = typeC = typeD = false;
    std::ifstream file(file_path, std::ios::binary);
    if (!file.good()) return false;
    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), {});
    std::string ascii(buffer.begin(), buffer.end());

    std::vector<std::string> typeA_ascii = {
        "MouseLeft", "MouseRight", "MouseX", "MouseY",
        "mouse_event", "autoclick", "clicker",
        "Reach->", "AutoClicker->",
        "C:\\Users\\DeathZ\\source\\repos\\StarDLL\\x64\\Release\\MoonDLL",
        "C:\\Users\\mella\\source\\repos\\Fox v2\\x64\\Release\\Fox.pdb"
    };
    std::vector<std::string> typeB_ascii = { "minecraft/client" };
    std::vector<std::string> typeC_ascii = { "imgui.ini", "imgui_", "imgui_impl" };

    typeA = ContainsTerm(ascii, typeA_ascii);
    typeB = ContainsTerm(ascii, typeB_ascii);
    typeC = ContainsTerm(ascii, typeC_ascii);

    if (buffer.size() % 2 == 1) buffer.push_back(0);
    std::wstring wcontent(reinterpret_cast<wchar_t*>(buffer.data()), buffer.size() / 2);

    std::vector<std::wstring> typeA_utf16 = {
        L"MouseLeft", L"MouseRight", L"MouseX", L"MouseY",
        L"mouse_event", L"autoclick", L"clicker",
        L"Reach->", L"AutoClicker->",
        L"C:\\Users\\DeathZ\\source\\repos\\StarDLL\\x64\\Release\\MoonDLL",
        L"C:\\Users\\mella\\source\\repos\\Fox v2\\x64\\Release\\Fox.pdb"
    };
    std::vector<std::wstring> typeB_utf16 = { L"minecraft/client" };
    std::vector<std::wstring> typeC_utf16 = { L"imgui.ini", L"imgui_", L"imgui_impl" };

    typeA |= ContainsTerm(wcontent, typeA_utf16);
    typeB |= ContainsTerm(wcontent, typeB_utf16);
    typeC |= ContainsTerm(wcontent, typeC_utf16);

    {
        HANDLE hFile = CreateFileW(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile != INVALID_HANDLE_VALUE) {
            HANDLE hMapping = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
            if (hMapping) {
                LPVOID base = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
                if (base) {
                    IMAGE_DOS_HEADER* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
                    if (dos->e_magic == IMAGE_DOS_SIGNATURE) {
                        IMAGE_NT_HEADERS* nt = reinterpret_cast<IMAGE_NT_HEADERS*>((BYTE*)base + dos->e_lfanew);
                        if (nt->Signature == IMAGE_NT_SIGNATURE) {
                            WORD nsec = nt->FileHeader.NumberOfSections;
                            IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);

                            for (WORD i = 0; i < nsec; ++i) {
                                char name[9] = { 0 };
                                memcpy(name, sec[i].Name, 8);
                                std::string sname(name);
                                std::string low = sname;
                                std::transform(low.begin(), low.end(), low.begin(), ::tolower);

                                if (low.find(".vmp") != std::string::npos) {
                                    typeD = true; // Flag D (VM PROTECT)
                                    break;
                                }
                            }
                        }
                    }
                    UnmapViewOfFile(base);
                }
                CloseHandle(hMapping);
            }
            CloseHandle(hFile);
        }
    }

    return typeA || typeB || typeC || typeD;
}