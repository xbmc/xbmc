/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoInfoScannerLookups.h"

#include "threads/Thread.h"
#include "utils/log.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <iterator>
#include <mutex>
#include <vector>

namespace KODI::VIDEO
{
namespace
{
//! A thread of a scan, at the lowest priority, as CJobManager runs the scan's own
class CScanThread : public CThread
{
public:
  CScanThread(const char* name, std::function<void()> work) : CThread(name), m_work(std::move(work))
  {
    Create();
  }
  ~CScanThread() override { StopThread(); }

protected:
  void Process() override
  {
    SetPriority(ThreadPriority::LOWEST);
    m_work();
  }

private:
  const std::function<void()> m_work;
};
} // namespace

void CVideoLookups::Run(size_t count,
                        size_t workers,
                        const std::function<void(size_t)>& lookUp,
                        const std::function<bool(size_t)>& handBack,
                        const std::atomic<bool>& stop)
{
  CRun run{std::min(workers, count), stop};
  run.Add(count, [&lookUp](size_t i, size_t) { lookUp(i); });
  run.HandBack(count, handBack);
}

CVideoLookups::CRun::CRun(size_t workers, const std::atomic<bool>& stop)
  : m_workers(workers),
    m_stop(stop),
    m_window(workers * 8)
{
  // With one, each video is looked up on the calling thread as it is handed back
  if (m_workers <= 1)
    return;

  try
  {
    m_threads.reserve(m_workers);
    for (size_t worker = 0; worker < m_workers; ++worker)
      m_threads.push_back(std::make_unique<CScanThread>("VideoLookUp", [this] { Work(); }));
  }
  catch (...)
  {
    // A thread that can't be made ends those that were
    End();
    throw;
  }
}

CVideoLookups::CRun::~CRun()
{
  End();
}

size_t CVideoLookups::CRun::Add(size_t count, std::function<void(size_t, size_t)> lookUp)
{
  size_t first;
  {
    std::unique_lock guard(m_lock);
    first = m_done.size();
    if (count == 0)
      return first;
    m_additions.push_back({.first = first, .lookUp = std::move(lookUp)});
    m_done.resize(first + count, 0);
  }
  m_changed.notify_all();
  return first;
}

bool CVideoLookups::CRun::HandBack(size_t end, const std::function<bool(size_t)>& handBack)
{
  if (m_workers <= 1)
  {
    for (; m_handedBack < std::min(end, m_done.size()); ++m_handedBack)
    {
      if (m_ended || m_stop)
      {
        End();
        return false;
      }
      LookUp(m_handedBack);
      if (!handBack(m_handedBack))
      {
        End();
        return false;
      }
    }
    return !m_ended;
  }

  try
  {
    while (true)
    {
      size_t index;
      bool lookedUp;
      {
        std::unique_lock guard(m_lock);
        if (m_ended)
          return false;
        if (m_handedBack >= std::min(end, m_done.size()))
          return true;
        index = m_handedBack;
        // A video not yet looked up when the scan stopped is never looked up. The stop flag is
        // set without waking anyone, so it is looked at again now and then.
        const auto ready{[this, index] { return m_done[index] || (m_stop && index >= m_next); }};
        while (!m_changed.wait_for(guard, std::chrono::milliseconds(100), ready))
          ;
        lookedUp = m_done[index];
      }
      if (!lookedUp || !handBack(index))
      {
        End();
        return false;
      }

      {
        std::unique_lock guard(m_lock);
        ++m_handedBack;
      }
      m_changed.notify_all();
    }
  }
  catch (...)
  {
    End();
    throw;
  }
}

void CVideoLookups::CRun::Work()
{
  while (true)
  {
    size_t index;
    {
      std::unique_lock guard(m_lock);
      m_changed.wait(guard,
                     [this] {
                       return m_ended || m_stop ||
                              (m_next < m_done.size() && m_next < m_handedBack + m_window);
                     });
      if (m_ended || m_stop)
      {
        // The calling thread may be waiting for a video no one is to look up
        guard.unlock();
        m_changed.notify_all();
        return;
      }
      index = m_next++;
    }

    LookUp(index);

    {
      std::unique_lock guard(m_lock);
      m_done[index] = 1;
    }
    m_changed.notify_all();
  }
}

void CVideoLookups::CRun::LookUp(size_t index)
{
  const Addition* added;
  {
    std::unique_lock guard(m_lock);
    added = &*std::prev(std::ranges::upper_bound(m_additions, index, {}, &Addition::first));
  }

  // A video whose look-up throws is still handed back, on one thread as on several
  try
  {
    added->lookUp(index - added->first, index);
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Looking up video {} failed", index + 1);
  }
}

void CVideoLookups::CRun::End()
{
  {
    std::unique_lock guard(m_lock);
    m_ended = true;
  }
  m_changed.notify_all();
  m_threads.clear();
}
} // namespace KODI::VIDEO
