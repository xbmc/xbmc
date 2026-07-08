/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "jobs/JobQueue.h"
#include "rendering/capture/CaptureRequest.h"
#include "threads/CriticalSection.h"

#include <atomic>
#include <memory>
#include <vector>

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

class CCaptureHandle;

/*!
 \brief Registry of pixel-capture requests serviced by render-thread taps.

 Threading contract:
 - Submit()/Cancel(): any thread; they never touch the graphics context.
 - LatchFrame()/TakeActive()/Complete()/Fail(): render thread only.
 - Completion callbacks run on the service's job queue, never on the
   render thread and never under the registry lock.
 - One lock guards the registry and all request state; the atomic pending
   flag is checked before locking, so the steady-state per-frame cost of
   an empty registry is a single atomic load.
 - FailAll() requires the render loop to have stopped servicing taps.
 */
class CCaptureService
{
public:
  CCaptureService() = default;
  ~CCaptureService();

  //! Register a request; the returned handle owns its lifetime.
  //! \param callback optional; runs on a worker thread after SUCCESSFUL
  //! completion only; failures and cancels just wake the waiter
  std::unique_ptr<CCaptureHandle> Submit(const CaptureSpec& spec, CaptureCallback callback = {});

  //! Render thread, frame boundary: make newly submitted requests visible
  //! to this frame's taps and prune cancelled ones.
  //! \return true when a newly latched request needs a forced frame; the
  //! caller owns the GUI-side dirty marking.
  bool LatchFrame();

  //! Render thread: latched requests a tap of this content must service.
  std::vector<std::shared_ptr<CaptureRequest>> TakeActive(CaptureContent content);

  //! Render thread: deliver a serviced request. ONESHOT requests finish;
  //! CONTINUOUS requests stay latched for the next frame.
  void Complete(const std::shared_ptr<CaptureRequest>& request, CaptureResult result);

  //! Render thread: fail a serviced request and wake its waiter.
  void Fail(const std::shared_ptr<CaptureRequest>& request);

  //! Any thread: abandon a request; no callback fires after this returns.
  void Cancel(const std::shared_ptr<CaptureRequest>& request);

  CaptureState GetState(const CaptureRequest& request) const;

  //! Teardown: fail every request, wake every waiter, drop queued callbacks.
  void FailAll();

private:
  void Dispatch(const std::shared_ptr<CaptureRequest>& request);
  void RemoveActive(const std::shared_ptr<CaptureRequest>& request);

  mutable CCriticalSection m_lock;
  std::atomic<bool> m_hasPending{false};
  //! guarded by m_lock
  std::vector<std::shared_ptr<CaptureRequest>> m_pending;
  //! owned by the render thread; taps iterate it with no lock held
  std::vector<std::shared_ptr<CaptureRequest>> m_active;
  //! FIFO with one job at a time: completions are delivered in order
  CJobQueue m_callbackQueue;
};

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
