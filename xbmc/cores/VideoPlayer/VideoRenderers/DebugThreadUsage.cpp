/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DebugThreadUsage.h"

#include "ServiceBroker.h"
#include "utils/CPUInfo.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
constexpr auto UPDATE_INTERVAL = std::chrono::seconds(1);
// Longer gaps mean the overlay was hidden, so the old sample would average over that time
constexpr auto MAX_SAMPLE_GAP = std::chrono::seconds(3);

struct ThreadGroup
{
  std::string name;
  double seconds{0.0};
  int count{0};
};
} // namespace

const std::string& CDebugThreadUsage::GetSummary(int maxEntries)
{
  // Also limit attempts that return nothing, e.g. where the platform cannot tell
  const auto now = std::chrono::steady_clock::now();
  if (now - m_lastAttempt < UPDATE_INTERVAL)
    return m_summary;
  m_lastAttempt = now;

  if (now - m_lastRead > MAX_SAMPLE_GAP)
  {
    m_lastRead = {};
    m_threadSeconds.clear();
    m_summary.clear();
  }

  Update(static_cast<size_t>(maxEntries));
  return m_summary;
}

void CDebugThreadUsage::Update(size_t maxEntries)
{
  const auto cpuInfo = CServiceBroker::GetCPUInfo();
  if (!cpuInfo)
    return;

  const std::vector<ThreadCpuTime> threads = cpuInfo->GetThreadCpuTimes();
  if (threads.empty())
    return;

  const auto now = std::chrono::steady_clock::now();
  const bool haveBaseline = m_lastRead != std::chrono::steady_clock::time_point{};
  const double elapsed = std::chrono::duration<double>(now - m_lastRead).count();

  std::unordered_map<int, double> threadSeconds;
  std::vector<ThreadGroup> groups;
  for (const auto& thread : threads)
  {
    threadSeconds[thread.id] = thread.seconds;

    // A thread without a previous sample started within the interval
    const auto prev = m_threadSeconds.find(thread.id);
    const double delta = prev != m_threadSeconds.end() && prev->second <= thread.seconds
                             ? thread.seconds - prev->second
                             : thread.seconds;

    auto group = std::find_if(groups.begin(), groups.end(),
                              [&thread](const ThreadGroup& g) { return g.name == thread.name; });
    if (group == groups.end())
    {
      groups.push_back({thread.name, 0.0, 0});
      group = groups.end() - 1;
    }
    group->seconds += delta;
    group->count++;
  }

  m_threadSeconds = std::move(threadSeconds);
  m_lastRead = now;

  // The first read only establishes a baseline
  if (!haveBaseline || elapsed <= 0)
    return;

  std::sort(groups.begin(), groups.end(),
            [](const ThreadGroup& a, const ThreadGroup& b) { return a.seconds > b.seconds; });

  std::vector<std::string> entries;
  for (const auto& group : groups)
  {
    const double usage = group.seconds * 100.0 / elapsed;
    if (entries.size() >= maxEntries || usage < 1.0)
      break;

    const long percent = std::lround(usage);

    if (group.count > 1)
      entries.push_back(StringUtils::Format("{} x{} {}%", group.name, group.count, percent));
    else
      entries.push_back(StringUtils::Format("{} {}%", group.name, percent));
  }
  m_summary = StringUtils::Join(entries, ", ");
}
