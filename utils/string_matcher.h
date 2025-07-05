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

bool CheckFileForFlags(const std::wstring& file_path, bool& typeA, bool& typeB, bool& typeC)
{
    typeA = typeB = typeC = false;
    std::ifstream file(file_path, std::ios::binary);
    if (!file.good()) return false;
    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), {});
    std::string ascii(buffer.begin(), buffer.end());

    std::vector<std::string> typeA_ascii = { "MouseLeft", "MouseRight", "MouseX", "MouseY", "mouse_event", "autoclick", "clicker", "Reach->", "AutoClciker->", "C:\\Users\\DeathZ\\source\\repos\\StarDLL\\x64\\Release\\MoonDLL" };
    std::vector<std::string> typeB_ascii = { "minecraft/client" };
    std::vector<std::string> typeC_ascii = { "imgui.ini", "imgui_", "imgui_impl" };

    typeA = ContainsTerm(ascii, typeA_ascii);
    typeB = ContainsTerm(ascii, typeB_ascii);
    typeC = ContainsTerm(ascii, typeC_ascii);

    if (buffer.size() % 2 == 1) buffer.push_back(0);
    std::wstring wcontent(reinterpret_cast<wchar_t*>(buffer.data()), buffer.size() / 2);

    std::vector<std::wstring> typeA_utf16 = { L"MouseLeft", L"MouseRight", L"MouseX", L"MouseY", L"mouse_event", L"autoclick", L"clicker", L"Reach->", L"AutoClciker->", L"C:\\Users\\DeathZ\\source\\repos\\StarDLL\\x64\\Release\\MoonDLL" };
    std::vector<std::wstring> typeB_utf16 = { L"minecraft/client" };
    std::vector<std::wstring> typeC_utf16 = { L"imgui.ini", L"imgui_", L"imgui_impl" };

    typeA |= ContainsTerm(wcontent, typeA_utf16);
    typeB |= ContainsTerm(wcontent, typeB_utf16);
    typeC |= ContainsTerm(wcontent, typeC_utf16);

    return typeA || typeB || typeC;
}