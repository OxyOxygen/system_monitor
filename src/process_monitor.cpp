#include "process_monitor.h"
#include <algorithm>
#include <psapi.h>
#include <tlhelp32.h>

ProcessMonitor::ProcessMonitor()
    : numProcessors(1), sortMode(SortMode::ByMemory) {
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  numProcessors = sysInfo.dwNumberOfProcessors;
  if (numProcessors < 1)
    numProcessors = 1;

  lastSystemTime.QuadPart = 0;
  lastIdleTime.QuadPart = 0;
  prevSystemTime.QuadPart = 0;

  // Initialize system time for first delta
  FILETIME ftIdle, ftKernel, ftUser;
  if (GetSystemTimes(&ftIdle, &ftKernel, &ftUser)) {
    ULARGE_INTEGER kernel, user;
    kernel.LowPart = ftKernel.dwLowDateTime;
    kernel.HighPart = ftKernel.dwHighDateTime;
    user.LowPart = ftUser.dwLowDateTime;
    user.HighPart = ftUser.dwHighDateTime;
    prevSystemTime.QuadPart = kernel.QuadPart + user.QuadPart;
  }
}

std::vector<ProcessInfo> ProcessMonitor::getTopProcesses(int count) {
  std::vector<ProcessInfo> processes;

  // Get current system time for CPU% calculation
  FILETIME ftIdle, ftKernel, ftUser;
  ULARGE_INTEGER currentSystemTime = {};
  if (GetSystemTimes(&ftIdle, &ftKernel, &ftUser)) {
    ULARGE_INTEGER kernel, user;
    kernel.LowPart = ftKernel.dwLowDateTime;
    kernel.HighPart = ftKernel.dwHighDateTime;
    user.LowPart = ftUser.dwLowDateTime;
    user.HighPart = ftUser.dwHighDateTime;
    currentSystemTime.QuadPart = kernel.QuadPart + user.QuadPart;
  }

  uint64_t systemDelta = currentSystemTime.QuadPart - prevSystemTime.QuadPart;

  // Take a snapshot of all processes
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) {
    return processes;
  }

  PROCESSENTRY32 pe32;
  pe32.dwSize = sizeof(PROCESSENTRY32);

  if (!Process32First(snapshot, &pe32)) {
    CloseHandle(snapshot);
    return processes;
  }

  std::unordered_map<DWORD, ProcessCpuData> currentProcessTimes;

  do {
    // Skip System Idle Process (PID 0)
    if (pe32.th32ProcessID == 0)
      continue;

    ProcessInfo info = {};
    info.pid = pe32.th32ProcessID;
    info.name = pe32.szExeFile;
    info.cpuUsage = 0.0;
    info.memoryUsed = 0;

    // Open process to get memory info and CPU times
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                  FALSE, pe32.th32ProcessID);
    if (hProcess != nullptr) {
      // Memory info
      PROCESS_MEMORY_COUNTERS pmc;
      if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        info.memoryUsed = pmc.WorkingSetSize;
      }

      // CPU times for CPU% calculation
      FILETIME ftCreate, ftExit, ftProcKernel, ftProcUser;
      if (GetProcessTimes(hProcess, &ftCreate, &ftExit, &ftProcKernel,
                          &ftProcUser)) {
        ULARGE_INTEGER procKernel, procUser;
        procKernel.LowPart = ftProcKernel.dwLowDateTime;
        procKernel.HighPart = ftProcKernel.dwHighDateTime;
        procUser.LowPart = ftProcUser.dwLowDateTime;
        procUser.HighPart = ftProcUser.dwHighDateTime;

        ProcessCpuData current;
        current.lastKernel = procKernel;
        current.lastUser = procUser;
        currentProcessTimes[info.pid] = current;

        // Calculate CPU% from delta
        if (systemDelta > 0) {
          auto it = prevProcessTimes.find(info.pid);
          if (it != prevProcessTimes.end()) {
            uint64_t procDelta =
                (procKernel.QuadPart - it->second.lastKernel.QuadPart) +
                (procUser.QuadPart - it->second.lastUser.QuadPart);
            info.cpuUsage = (static_cast<double>(procDelta) /
                             static_cast<double>(systemDelta)) *
                            100.0 * numProcessors;
            if (info.cpuUsage > 100.0 * numProcessors)
              info.cpuUsage = 100.0 * numProcessors;
            if (info.cpuUsage < 0.0)
              info.cpuUsage = 0.0;
          }
        }
      }
      CloseHandle(hProcess);
    }

    processes.push_back(info);
  } while (Process32Next(snapshot, &pe32));

  CloseHandle(snapshot);

  // Update stored times for next call
  prevProcessTimes = std::move(currentProcessTimes);
  prevSystemTime = currentSystemTime;

  // Sort based on current mode
  if (sortMode == SortMode::ByCpu) {
    std::sort(processes.begin(), processes.end(),
              [](const ProcessInfo &a, const ProcessInfo &b) {
                return a.cpuUsage > b.cpuUsage;
              });
  } else {
    std::sort(processes.begin(), processes.end(),
              [](const ProcessInfo &a, const ProcessInfo &b) {
                return a.memoryUsed > b.memoryUsed;
              });
  }

  // Limit to requested count
  if (static_cast<int>(processes.size()) > count) {
    processes.resize(count);
  }

  return processes;
}
