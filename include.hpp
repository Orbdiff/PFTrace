#pragma once

#include <iostream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>
#include <mutex>

#include "prefetch_parser.hh"
#include "signature_utils.h"
#include "color_utils.h"
#include "string_matcher.h"
#include "volume_utils.h"
#include "time_utils.h"
#include "threading_utils.h"
#include "usn_reader.hh"

struct PrefetchInfo
{
    std::string filename;
    prefetch_parser parser;
    time_t latest_exec;
};

inline void RunAnalyzer() {
    static std::mutex print_mutex;

    std::string prefetch_dir = "C:\\Windows\\Prefetch";
    time_t logon_time = GetCurrentUserLogonTime();

    char view_rundll, view_regsvr, view_usn;
    std::cout << "Do you want to scan RUNDLL32 files? (y/n): ";
    std::cin >> view_rundll;
    std::cout << "Do you want to scan REGSVR32 files? (y/n): ";
    std::cin >> view_regsvr;
    std::cout << "Do you want to analyze the USN Journal? (y/n): ";
    std::cin >> view_usn;

    bool scan_rundll = tolower(view_rundll) == 'y';
    bool scan_regsvr = tolower(view_regsvr) == 'y';
    bool analyze_usn = tolower(view_usn) == 'y';

    if (!scan_rundll && !scan_regsvr && !analyze_usn) {
        std::cout << "[!] No analysis option selected. Exiting in 2 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return;
    }

    size_t thread_count = GetOptimalThreadCount();
    std::vector<PrefetchInfo> results;

    if (scan_rundll || scan_regsvr) {
        for (const auto& entry : std::filesystem::directory_iterator(prefetch_dir)) {
            if (entry.path().extension() != ".pf") continue;
            std::string filename = entry.path().filename().string();

            if ((scan_rundll && filename.find("RUNDLL32") != std::string::npos) ||
                (scan_regsvr && filename.find("REGSVR32") != std::string::npos)) {

                prefetch_parser pf(entry.path().string());
                if (!pf.success()) continue;

                time_t max_exec = 0;
                for (auto t : pf.last_eight_execution_times())
                    if (t >= logon_time && t > max_exec) max_exec = t;

                if (max_exec > 0)
                    results.push_back({ filename, std::move(pf), max_exec });
            }
        }
    }

    if (results.empty() && !analyze_usn) {
        system("cls");
        std::cout << "[!] No RUNDLL32 or REGSVR32 files detected. Exiting in 2 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return;
    }

    system("cls");
    std::cout << "[*] Logon time: ";
    print_time(logon_time);
    std::cout << "[*] Using " << thread_count << " threads\n";

    auto start_time = std::chrono::high_resolution_clock::now();

    if (!results.empty()) {
        std::sort(results.begin(), results.end(), [](const PrefetchInfo& a, const PrefetchInfo& b) {
            return a.latest_exec > b.latest_exec;
            });

        for (const auto& info : results) {
            std::cout << "\n>>> File: " << info.filename << "\n";
            std::cout << "Last executions:\n";
            for (auto t : info.parser.last_eight_execution_times())
                if (t >= logon_time) print_time(t);

            std::wcout << L"\nUsed paths (Unsigned only):\n";

            std::vector<std::pair<std::wstring, std::wstring>> paths;
            for (const auto& s : info.parser.get_filenames_strings()) {
                std::wstring dummy;
                std::wstring resolved = ReplaceVolumeWithDrive(s, dummy);
                paths.emplace_back(resolved, dummy);
            }

            ParallelWithLimit(paths, thread_count, [&](const std::pair<std::wstring, std::wstring>& item) {
                const std::wstring& resolved = item.first;
                const std::wstring& drive_letter = item.second;

                bool isUnsigned = !VerifySignatureForPath(resolved);
                bool typeA, typeB, typeC, typeD;
                bool isFlagged = CheckFileForFlags(resolved, typeA, typeB, typeC, typeD);

                if (isUnsigned || isFlagged) {
                    std::lock_guard<std::mutex> lock(print_mutex);
                    std::wcout << L"  - " << resolved;

                    if (!drive_letter.empty() && drive_letter != L"C:\\") {
                        std::wcout << L"  "; SetColorRed(); std::wcout << L" | Suspicious file"; ResetColor();
                    }
                    if (typeA) { std::wcout << L"  "; SetColorRed(); std::wcout << L" | Flag Type A"; ResetColor(); }
                    if (typeB) { std::wcout << L"  "; SetColorRed(); std::wcout << L" | Flag Type B"; ResetColor(); }
                    if (typeC) { std::wcout << L"  "; SetColorRed(); std::wcout << L" | Flag Type C"; ResetColor(); }
                    if (typeD) { std::wcout << L"  "; SetColorRed(); std::wcout << L" | Flag Type D (VMProtect)"; ResetColor(); }

                    std::wcout << std::endl;
                }
                });

            std::cout << "\n";
        }
    }

    if (analyze_usn) {
        USNJournalReader reader(L"C:");
        reader.Run();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    std::cout << "[*] Total analysis time: " << (elapsed_ms / 1000.0) << " seconds\n\n";
    system("pause");
}