/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "utils/log.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <string>

namespace OVERLAY
{

/*!
 * \brief Generic, type-agnostic single-worker async render pipeline, shared
 *  by both the libass (TEXT/SSA) and bitmap (PGS/DVB/DVD-SPU) decode paths
 *  in CRenderer. The decode logic itself (what a "job" means, how to turn
 *  it into a "result") stays entirely separate per type - this class only
 *  provides the threading discipline both paths need identically:
 *
 *  - RequestRender() is cheap and non-blocking, called from the GUI/render
 *    thread. It keeps only the single newest not-yet-started job (a
 *    one-slot mailbox): if the worker falls behind, older requests are
 *    simply overwritten rather than queued, so the worker always works on
 *    the most current job instead of working through a growing backlog.
 *    A request identical to the last one submitted is dropped rather than
 *    resubmitted, since re-running an unchanged job wastes worker time -
 *    this matters because the caller may submit the same lookahead target
 *    on several consecutive GUI ticks before a presentation flip actually
 *    happens (e.g. video frame rate below display refresh rate).
 *
 *  - GetBestResult() is also non-blocking and never waits on the worker.
 *    It keeps the last two completed results and returns whichever has
 *    the newest target pts that does not exceed the caller-supplied
 *    'displayPts' - i.e. it will never return a result for a subtitle
 *    that is not due to be shown yet (which would mean showing it
 *    prematurely - the pause-transition case, see design notes at the
 *    top of OverlayRenderer.cpp). Staleness in the other direction is
 *    accepted without limit: an old-but-still-valid result is exactly
 *    the degraded-but-smooth behaviour this class exists to provide
 *    when the worker cannot keep up (e.g. a fast run of complex cues),
 *    so it is always preferred over showing nothing while the worker
 *    catches up. Returns nullptr only if every stored result is still
 *    ahead of 'displayPts' (nothing suitable has ever been rendered yet).
 *
 *  - GetLatestResult() returns just the most recently completed result,
 *    with no pts matching. Used by the bitmap path, where a result is
 *    valid for as long as the source CDVDOverlay it was decoded from is
 *    on screen, rather than needing to match a specific per-frame pts.
 *
 *  - Flush() clears the mailbox and both result history slots and bumps
 *    an internal epoch counter. libass has no cancellation point, so a
 *    render already in flight when Flush() is called cannot be
 *    interrupted; it is left to finish, but its result is silently
 *    dropped instead of published, since it carries a now-stale epoch.
 *    The epoch check happens inside the same critical section that
 *    Flush() uses to bump the epoch and clear history, so a render that
 *    finishes concurrently with a Flush() call cannot slip its result in
 *    between the check and the publish - whichever of the two reaches
 *    the lock first fully completes before the other proceeds. This
 *    keeps Flush() itself instant (never blocks on the worker) while
 *    guaranteeing a post-flush read can never observe pre-flush content.
 *
 *  TJob must be equality-comparable (operator==), used for request
 *  deduplication. TResult must expose a 'double pts' member, used by
 *  GetBestResult(); GetLatestResult() has no such requirement.
 */
template<typename TJob, typename TResult>
class CAsyncSubtitleRenderer : private CThread
{
public:
  using RenderFunc = std::function<std::shared_ptr<const TResult>(const TJob&)>;

  CAsyncSubtitleRenderer(const std::string& threadName, RenderFunc renderFunc)
    : CThread(threadName.c_str()),
      m_threadName(threadName),
      m_renderFunc(std::move(renderFunc))
  {
    Create();
  }

  ~CAsyncSubtitleRenderer() override { StopThread(true); }

  void RequestRender(TJob job)
  {
    std::unique_lock lock(m_requestLock);
    if (m_lastSubmittedJob && *m_lastSubmittedJob == job)
      return;

    if (m_pendingJob)
    {
      // The worker hasn't even started the previous job yet - it is about
      // to be overwritten and will never be rendered. This, not a slow
      // render itself, is what "skips a cue entirely" looks like.
      CLog::Log(LOGDEBUG, "ASYNC_SUB[{}]: dropping never-started job (#{} dropped total)",
                m_threadName, ++m_droppedPendingJobs);
    }

    m_lastSubmittedJob = std::make_unique<TJob>(job);
    m_pendingJob = std::make_unique<TJob>(std::move(job));
    m_pendingEpoch = m_epoch.load();
    lock.unlock();
    m_requestEvent.Set();
  }

  std::shared_ptr<const TResult> GetBestResult(double displayPts) const
  {
    std::unique_lock lock(m_resultLock);
    std::shared_ptr<const TResult> best;
    double bestPts = -std::numeric_limits<double>::infinity();
    for (const auto& entry : m_results)
    {
      if (!entry || entry->pts > displayPts)
        continue;

      if (entry->pts > bestPts)
      {
        bestPts = entry->pts;
        best = entry;
      }
    }
    return best;
  }

  std::shared_ptr<const TResult> GetLatestResult() const
  {
    std::unique_lock lock(m_resultLock);
    return m_results[m_newest];
  }

  void Flush()
  {
    std::unique_lock lock1(m_requestLock);
    std::unique_lock lock2(m_resultLock);
    m_pendingJob.reset();
    m_lastSubmittedJob.reset();
    m_epoch.fetch_add(1);
    m_results[0].reset();
    m_results[1].reset();
    m_newest = 0;
  }

private:
  void Process() override
  {
    using namespace std::chrono_literals;

    while (!m_bStop)
    {
      std::unique_ptr<TJob> job;
      uint64_t epoch;
      {
        std::unique_lock lock(m_requestLock);
        if (!m_pendingJob)
        {
          lock.unlock();
          AbortableWait(m_requestEvent, 500ms);
          continue;
        }
        job = std::move(m_pendingJob);
        epoch = m_pendingEpoch;
      }

      // The expensive part: may take anywhere from <1ms to 100+ms
      // depending on the source. The GUI/render thread is never blocked
      // by this call; it only ever reads whatever was last published.
      const auto t0 = std::chrono::steady_clock::now();
      std::shared_ptr<const TResult> result = m_renderFunc(*job);
      const double renderMs =
          std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
      CLog::Log(LOGDEBUG, "ASYNC_SUB[{}]: render for pts={:.3f} took {:.1f}ms", m_threadName,
                result->pts / DVD_TIME_BASE, renderMs);
 
      std::unique_lock lock(m_resultLock);
      // Deciding "is this still valid" and "publish it" must happen under
      // the same lock Flush() uses to bump the epoch, otherwise a Flush()
      // landing between an outside-the-lock check and this write could
      // publish a result the flush was supposed to have discarded.
      if (epoch == m_epoch.load())
      {
        if (m_results[m_newest])
          CLog::Log(LOGDEBUG, "ASYNC_SUB[{}]: superseding unread result pts={:.3f}", m_threadName,
                    m_results[m_newest]->pts / DVD_TIME_BASE);
        m_newest = 1 - m_newest;
        m_results[m_newest] = std::move(result);
      }
      else
      {
        // Flush() happened while rendering; discard silently.
        CLog::Log(LOGDEBUG, "ASYNC_SUB[{}]: discarding stale-epoch result for pts={:.3f}",
                  m_threadName, result->pts / DVD_TIME_BASE);
      }
    }
  }

  std::string m_threadName;
  RenderFunc m_renderFunc;
  uint64_t m_droppedPendingJobs{0};

  mutable CCriticalSection m_requestLock;
  CEvent m_requestEvent;
  std::unique_ptr<TJob> m_pendingJob;
  std::unique_ptr<TJob> m_lastSubmittedJob;
  uint64_t m_pendingEpoch{0};

  mutable CCriticalSection m_resultLock;
  std::shared_ptr<const TResult> m_results[2];
  int m_newest{0};

  std::atomic<uint64_t> m_epoch{0};
};

} // namespace OVERLAY
