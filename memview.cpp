#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <conio.h>
#include <cctype>

#pragma comment(lib, "psapi.lib")

struct ProcessInfo {
    DWORD pid;
    std::string name;
    SIZE_T workingSet;
    SIZE_T privateBytes;
    SIZE_T peakWorkingSet;
    DWORD pageFaults;
    DWORD threadCount;
    DWORD handleCount;
};

void setColor(WORD color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void clearScreen() {
    COORD topLeft = {0, 0};
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO screen;
    DWORD written;
    GetConsoleScreenBufferInfo(console, &screen);
    FillConsoleOutputCharacterA(console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
    FillConsoleOutputAttribute(console, screen.wAttributes, screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
    SetConsoleCursorPosition(console, topLeft);
}

std::string formatBytes(SIZE_T bytes) {
    std::ostringstream oss;
    if (bytes >= 1024ULL * 1024 * 1024)
        oss << std::fixed << std::setprecision(2) << (double)bytes / (1024.0 * 1024 * 1024) << " GB";
    else if (bytes >= 1024 * 1024)
        oss << std::fixed << std::setprecision(2) << (double)bytes / (1024.0 * 1024) << " MB";
    else if (bytes >= 1024)
        oss << std::fixed << std::setprecision(2) << (double)bytes / 1024.0 << " KB";
    else
        oss << bytes << " B";
    return oss.str();
}

std::vector<ProcessInfo> getProcesses() {
    std::vector<ProcessInfo> procs;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return procs;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    if (!Process32First(snap, &pe)) { CloseHandle(snap); return procs; }

    do {
        ProcessInfo info;
        info.pid = pe.th32ProcessID;
        info.name = pe.szExeFile;
        info.workingSet = 0;
        info.privateBytes = 0;
        info.peakWorkingSet = 0;
        info.pageFaults = 0;
        info.threadCount = pe.cntThreads;
        info.handleCount = 0;

        HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
        if (proc) {
            PROCESS_MEMORY_COUNTERS_EX pmc;
            pmc.cb = sizeof(pmc);
            if (GetProcessMemoryInfo(proc, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
                info.workingSet    = pmc.WorkingSetSize;
                info.privateBytes  = pmc.PrivateUsage;
                info.peakWorkingSet = pmc.PeakWorkingSetSize;
                info.pageFaults    = pmc.PageFaultCount;
            }
            GetProcessHandleCount(proc, &info.handleCount);
            CloseHandle(proc);
        }
        procs.push_back(info);
    } while (Process32Next(snap, &pe));

    CloseHandle(snap);
    return procs;
}

void drawHeader() {
    setColor(0x0A);
    std::cout << "  memview";
    setColor(0x08);
    std::cout << " by enivoss\n";
    setColor(0x08);
    std::cout << "  --------------------------------------------------------------------------------\n";
    setColor(0x07);
    std::cout << "  " << std::left << std::setw(8) << "PID"
              << std::setw(30) << "PROCESS"
              << std::setw(16) << "WORKING SET"
              << std::setw(16) << "PRIVATE"
              << std::setw(8)  << "THREADS"
              << "\n";
    setColor(0x08);
    std::cout << "  --------------------------------------------------------------------------------\n";
}

void drawProcesses(std::vector<ProcessInfo>& procs, int selected, int offset) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int visibleRows = csbi.dwSize.Y - 9;

    for (int i = offset; i < (int)procs.size() && i < offset + visibleRows; i++) {
        if (i == selected) {
            setColor(0x20);
        } else if (procs[i].workingSet > 500 * 1024 * 1024) {
            setColor(0x0C);
        } else if (procs[i].workingSet > 100 * 1024 * 1024) {
            setColor(0x0E);
        } else {
            setColor(0x07);
        }

        std::string name = procs[i].name;
        if (name.length() > 28) name = name.substr(0, 25) + "...";

        std::cout << "  " << std::left << std::setw(8) << procs[i].pid
                  << std::setw(30) << name
                  << std::setw(16) << formatBytes(procs[i].workingSet)
                  << std::setw(16) << formatBytes(procs[i].privateBytes)
                  << std::setw(8)  << procs[i].threadCount
                  << "\n";
    }
    setColor(0x07);
}

void drawFooter(int total, const std::string& sortMode) {
    setColor(0x08);
    std::cout << "\n  --------------------------------------------------------------------------------\n";
    setColor(0x07);
    std::cout << "  [UP/DOWN] navigate  [ENTER] detail  [M] sort mem  [N] sort name  [R] refresh  [Q] quit\n";
    std::cout << "  " << total << " processes  |  sort: " << sortMode << "\n";
}

void drawDetailView(const ProcessInfo& p) {
    clearScreen();
    setColor(0x0A);
    std::cout << "  memview";
    setColor(0x08);
    std::cout << " by enivoss  ";
    setColor(0x0A);
    std::cout << "process detail\n";
    setColor(0x08);
    std::cout << "  --------------------------------------------------------------------------------\n";
    setColor(0x07);

    std::string name = p.name;
    if (name.length() > 60) name = name.substr(0, 57) + "...";

    std::cout << "\n";
    setColor(0x0A);
    std::cout << "  " << name << "\n";
    setColor(0x08);
    std::cout << "  --------------------------------\n";
    setColor(0x07);

    auto row = [](const std::string& label, const std::string& value) {
        setColor(0x08);
        std::cout << "  " << std::left << std::setw(22) << label;
        setColor(0x07);
        std::cout << value << "\n";
    };

    row("PID",              std::to_string(p.pid));
    row("Working Set",      formatBytes(p.workingSet));
    row("Peak Working Set", formatBytes(p.peakWorkingSet));
    row("Private Bytes",    formatBytes(p.privateBytes));
    row("Page Faults",      std::to_string(p.pageFaults));
    row("Threads",          std::to_string(p.threadCount));
    row("Handles",          std::to_string(p.handleCount));

    std::cout << "\n";
    setColor(0x08);
    std::cout << "  --------------------------------------------------------------------------------\n";
    setColor(0x07);
    std::cout << "  [ESC] back\n";
}

int main() {
    SetConsoleTitleA("memview");

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    int selected = 0;
    int offset   = 0;
    std::string sortMode = "memory";
    bool inDetail = false;

    auto procs = getProcesses();
    std::sort(procs.begin(), procs.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
        return a.workingSet > b.workingSet;
    });

    while (true) {
        if (!inDetail) {
            clearScreen();
            drawHeader();
            drawProcesses(procs, selected, offset);
            drawFooter((int)procs.size(), sortMode);
        } else {
            drawDetailView(procs[selected]);
        }

        if (_kbhit()) {
            int key = _getch();

            if (inDetail) {
                if (key == 27) inDetail = false;
                continue;
            }

            if (key == 0 || key == 224) {
                key = _getch();
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                GetConsoleScreenBufferInfo(hConsole, &csbi);
                int visibleRows = csbi.dwSize.Y - 9;
                if (key == 72 && selected > 0) {
                    selected--;
                    if (selected < offset) offset--;
                } else if (key == 80 && selected < (int)procs.size() - 1) {
                    selected++;
                    if (selected >= offset + visibleRows) offset++;
                }
            } else {
                char c = (char)tolower(key);
                if (c == 'q') break;
                if (key == 13) {
                    inDetail = true;
                }
                if (c == 'r') {
                    procs = getProcesses();
                    selected = 0; offset = 0;
                    if (sortMode == "memory") {
                        std::sort(procs.begin(), procs.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
                            return a.workingSet > b.workingSet;
                        });
                    } else {
                        std::sort(procs.begin(), procs.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
                            return a.name < b.name;
                        });
                    }
                }
                if (c == 'm') {
                    sortMode = "memory";
                    std::sort(procs.begin(), procs.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
                        return a.workingSet > b.workingSet;
                    });
                    selected = 0; offset = 0;
                }
                if (c == 'n') {
                    sortMode = "name";
                    std::sort(procs.begin(), procs.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
                        return a.name < b.name;
                    });
                    selected = 0; offset = 0;
                }
            }
        }
        Sleep(16);
    }

    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    setColor(0x07);
    clearScreen();
    return 0;
}
