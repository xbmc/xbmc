/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoInfoScannerLookups.h"

#include "URL.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace KODI::VIDEO
{
void CVideoLookups::Run(size_t count,
                        size_t workers,
                        const std::function<void(size_t)>& lookUp,
                        const std::function<bool(size_t)>& handBack,
                        const std::atomic<bool>& stop)
{
  // A video whose look-up throws is still handed back, on one thread as on several
  const auto lookUpOrLog{[&lookUp, count](size_t i)
                         {
                           try
                           {
                             lookUp(i);
                           }
                           catch (...)
                           {
                             CLog::LogF(LOGERROR, "Looking up video {} of {} failed", i + 1, count);
                           }
                         }};

  workers = std::min(workers, count);
  if (workers <= 1)
  {
    for (size_t i = 0; i < count && !stop; ++i)
    {
      lookUpOrLog(i);
      if (!handBack(i))
        break;
    }
    return;
  }

  // Look-ups run this far at most ahead of the video handed back: far enough to keep the scraper
  // busy while the calling thread adds a slow one, such as a movie of several discs
  const size_t window{workers * 8};

  std::mutex lock;
  std::condition_variable changed;
  size_t next{0}; // the next video to look up
  size_t handedBack{0}; // the videos handed back so far
  bool stopping{false};
  std::vector<char> done(count, 0);

  const auto work{[&]
                  {
                    while (true)
                    {
                      size_t i;
                      {
                        std::unique_lock guard(lock);
                        changed.wait(guard,
                                     [&] {
                                       return stopping || stop || next >= count ||
                                              next < handedBack + window;
                                     });
                        if (stopping || stop || next >= count)
                        {
                          // The calling thread may be waiting for a video no one is to look up
                          guard.unlock();
                          changed.notify_all();
                          return;
                        }
                        i = next++;
                      }

                      lookUpOrLog(i);

                      {
                        std::unique_lock guard(lock);
                        done[i] = 1;
                      }
                      changed.notify_all();
                    }
                  }};

  std::vector<std::thread> threads;
  threads.reserve(workers);

  const auto finish{[&]
                    {
                      {
                        std::unique_lock guard(lock);
                        stopping = true;
                      }
                      changed.notify_all();
                      for (std::thread& thread : threads)
                        thread.join();
                    }};

  try
  {
    // Made in here, so that a thread that can't be made ends those that were
    for (size_t worker = 0; worker < workers; ++worker)
      threads.emplace_back(work);

    for (size_t i = 0; i < count; ++i)
    {
      bool lookedUp;
      {
        std::unique_lock guard(lock);
        // A video not yet looked up when the scan stopped is never looked up. The stop flag is
        // set without waking anyone, so it is looked at again now and then.
        const auto ready{[&] { return done[i] || ((stopping || stop) && i >= next); }};
        while (!changed.wait_for(guard, std::chrono::milliseconds(100), ready))
          ;
        lookedUp = done[i];
      }
      if (!lookedUp || !handBack(i))
        break;

      {
        std::unique_lock guard(lock);
        ++handedBack;
      }
      changed.notify_all();
    }
  }
  catch (...)
  {
    finish();
    throw;
  }
  finish();
}

bool IsDiscByName(const std::string& path)
{
  return URIUtils::IsDiscImage(path) || URIUtils::IsBDFile(path);
}

CDiscReadAhead::~CDiscReadAhead()
{
  Stop();
}

void CDiscReadAhead::Queue(const std::string& disc, std::function<void()> read, bool held)
{
  std::unique_lock lock(m_lock);
  if (m_stopping ||
      std::ranges::any_of(m_discs, [&disc](const Disc& queued) { return queued.path == disc; }))
    return;

  m_discs.push_back(
      {.path = disc, .read = std::move(read), .state = held ? State::HELD : State::QUEUED});
  if (m_threads.size() < m_readers)
    m_threads.emplace_back([this] { Work(); });
  m_changed.notify_all();
}

void CDiscReadAhead::Allow(const std::string& disc)
{
  std::unique_lock lock(m_lock);
  if (const auto held{std::ranges::find_if(m_discs, [&disc](const Disc& queued)
                                           { return queued.path == disc; })};
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
                [&disc](const Disc& queued) {
                  return queued.path == disc &&
                         (queued.state == State::HELD || queued.state == State::QUEUED);
                });
}

bool CDiscReadAhead::Take(const std::string& disc)
{
  std::unique_lock lock(m_lock);
  const auto taken{
      std::ranges::find_if(m_discs, [&disc](const Disc& queued) { return queued.path == disc; })};
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

  if (taken->state == State::HELD || taken->state == State::QUEUED)
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
  if (const auto taken{std::ranges::find_if(m_discs, [&disc](const Disc& queued)
                                            { return queued.path == disc; })};
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
  std::erase_if(m_discs, [](const Disc& queued)
                { return queued.state == State::HELD || queued.state == State::QUEUED; });
  m_changed.notify_all();
}

void CDiscReadAhead::Stop()
{
  Abandon();
  for (std::thread& thread : m_threads)
    thread.join();
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
} // namespace KODI::VIDEO
