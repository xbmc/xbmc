/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoInfoScannerLookups.h"

#include "utils/log.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
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

  // Look-ups run this far at most ahead of the video handed back
  const size_t window{workers * 4};

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
} // namespace KODI::VIDEO
