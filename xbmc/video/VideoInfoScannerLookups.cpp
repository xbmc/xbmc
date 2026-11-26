/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoInfoScannerLookups.h"

#include "URL.h"
#include "interfaces/generic/ScriptRunner.h"
#include "threads/Thread.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/Episode.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <iterator>
#include <mutex>
#include <utility>
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
                        const std::atomic<bool>& stop,
                        size_t window /* = 0 */)
{
  CRun run{std::min(workers, count), stop, window};
  run.Add(count, [&lookUp](size_t i, size_t) { lookUp(i); });
  run.HandBack(count, handBack);
}

CVideoLookups::CRun::CRun(size_t workers, const std::atomic<bool>& stop, size_t window /* = 0 */)
  : m_workers(workers),
    m_stop(stop),
    m_window(window > 0 ? window : workers * 8)
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

  // A video whose look-up throws is still handed back
  try
  {
    const CScriptRunner::CAbandonWhen abandonOnStop{m_stop};
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

namespace
{
//! The place in its run, from 1, of the video the thread looks up, or 0 for the video being added
//! (CVideoLookups::CPlace)
thread_local size_t lookUpPlace{0};
} // namespace

CVideoLookups::CPlace::CPlace(size_t place) : m_previous(std::exchange(lookUpPlace, place + 1))
{
}

CVideoLookups::CPlace::~CPlace()
{
  lookUpPlace = m_previous;
}

size_t CVideoLookups::Place()
{
  return lookUpPlace;
}

bool IsDiscByName(const std::string& path)
{
  return URIUtils::IsDiscImage(path) || URIUtils::IsBDFile(path);
}

CDiscReadAhead::CDiscReadAhead(size_t ahead, size_t readers) : m_ahead(ahead), m_readers(readers)
{
}

CDiscReadAhead::~CDiscReadAhead()
{
  Stop();
}

void CDiscReadAhead::Queue(const std::string& disc, std::function<void()> read, bool held)
{
  std::unique_lock lock(m_lock);
  if (m_stopping || std::ranges::find(m_discs, disc, &Disc::path) != m_discs.end())
    return;

  m_discs.push_back(
      {.path = disc, .read = std::move(read), .state = held ? State::HELD : State::QUEUED});
  if (m_threads.size() < m_readers)
    m_threads.push_back(std::make_unique<CScanThread>("VideoDiscReader", [this] { Work(); }));
  m_changed.notify_all();
}

void CDiscReadAhead::Allow(const std::string& disc)
{
  std::unique_lock lock(m_lock);
  if (const auto held{std::ranges::find(m_discs, disc, &Disc::path)};
      held != m_discs.end() && held->state == State::HELD)
  {
    held->state = State::QUEUED;
    m_changed.notify_all();
  }
}

void CDiscReadAhead::Drop(const std::string& disc)
{
  std::unique_lock lock(m_lock);
  std::erase_if(m_discs,
                [&disc](const Disc& queued) { return queued.path == disc && queued.Waiting(); });
}

bool CDiscReadAhead::Take(const std::string& disc)
{
  std::unique_lock lock(m_lock);
  const auto taken{std::ranges::find(m_discs, disc, &Disc::path)};
  if (taken == m_discs.end())
    return false;

  for (auto passed{m_discs.begin()}; passed != taken;)
  {
    if (passed->state == State::READING)
    {
      passed->passed = true;
      ++passed;
      continue;
    }
    if (passed->state == State::READ)
      --m_held;
    passed = m_discs.erase(passed);
  }

  if (taken->Waiting())
  {
    m_discs.erase(taken);
    m_changed.notify_all();
    return false;
  }
  // One passed by a disc taken before it, which the scan has come back to, is kept for it
  taken->passed = false;
  // Those let go of above leave room for another to be read meanwhile
  m_changed.notify_all();
  m_changed.wait(lock, [&taken] { return taken->state == State::READ; });
  return true;
}

void CDiscReadAhead::Release(const std::string& disc)
{
  std::unique_lock lock(m_lock);
  if (const auto taken{std::ranges::find(m_discs, disc, &Disc::path)};
      taken != m_discs.end() && taken->state == State::READ)
  {
    m_discs.erase(taken);
    --m_held;
    m_changed.notify_all();
  }
}

void CDiscReadAhead::Abandon()
{
  std::unique_lock lock(m_lock);
  m_stopping = true;
  std::erase_if(m_discs, [](const Disc& queued) { return queued.Waiting(); });
  m_changed.notify_all();
}

void CDiscReadAhead::Stop()
{
  Abandon();
  m_threads.clear();

  std::unique_lock lock(m_lock);
  m_discs.clear();
  m_held = 0;
  m_stopping = false;
}

void CDiscReadAhead::Work()
{
  std::unique_lock lock(m_lock);
  while (true)
  {
    auto next{m_discs.end()};
    m_changed.wait(lock,
                   [this, &next]
                   {
                     if (m_stopping)
                       return true;
                     next = std::ranges::find_if(m_discs, [](const Disc& queued)
                                                 { return queued.state == State::QUEUED; });
                     return next != m_discs.end() && m_held < m_ahead;
                   });
    if (m_stopping)
      return;

    // Being read, it stays in the list until read, so the iterator does too
    next->state = State::READING;
    ++m_held;
    const std::function<void()> read{std::move(next->read)};
    lock.unlock();
    try
    {
      read();
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Reading disc {} ahead failed", CURL::GetRedacted(next->path));
    }
    lock.lock();

    next->state = State::READ;
    if (next->passed)
    {
      m_discs.erase(next);
      --m_held;
    }
    m_changed.notify_all();
  }
}

std::vector<std::string> EpisodeDiscs(const std::vector<const EPISODE*>& files)
{
  std::vector<std::string> discs;
  for (const EPISODE* file : files)
  {
    if (IsDiscByName(file->strPath) && std::ranges::find(discs, file->strPath) == discs.end())
      discs.push_back(file->strPath);
  }
  return discs;
}
} // namespace KODI::VIDEO
