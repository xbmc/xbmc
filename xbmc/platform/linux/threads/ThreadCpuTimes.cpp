/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ThreadCpuTimes.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#include <dirent.h>
#include <unistd.h>

std::vector<ThreadCpuTime> KODI::LINUX::GetThreadCpuTimes()
{
  std::vector<ThreadCpuTime> threads;

  DIR* dir = opendir("/proc/self/task");
  if (!dir)
    return threads;

  const double ticksPerSecond = static_cast<double>(sysconf(_SC_CLK_TCK));

  while (const dirent* entry = readdir(dir))
  {
    if (entry->d_name[0] == '.')
      continue;

    std::ifstream file(std::string("/proc/self/task/") + entry->d_name + "/stat");
    std::string line;
    if (!std::getline(file, line))
      continue;

    // The thread name (field 2) is in parentheses and may itself contain them
    const auto open = line.find('(');
    const auto close = line.rfind(')');
    if (open == std::string::npos || close == std::string::npos || close < open)
      continue;

    std::istringstream ss(line.substr(close + 1));
    std::string field;
    unsigned long long ticks = 0;
    for (int i = 3; i <= 15 && ss >> field; i++)
    {
      if (i == 14 || i == 15) // utime, stime
        ticks += std::strtoull(field.c_str(), nullptr, 10);
    }

    threads.push_back({std::atoi(entry->d_name), line.substr(open + 1, close - open - 1),
                       static_cast<double>(ticks) / ticksPerSecond});
  }
  closedir(dir);

  return threads;
}
