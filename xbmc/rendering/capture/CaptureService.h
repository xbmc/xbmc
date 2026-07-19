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
#include <cstdint>
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

 Arbitration: registered requests form a stack (last submitted on top) and
 exactly ONE consumer is serviced per render loop, the most recent one (pure
 LIFO). A one-shot submitted just before a continuous can be buried under it
 and waits until the continuous is gone (a rare late/lost screenshot,
 accepted). FrameComplete() fails any request left unserved, so an unservable
 one cannot force frames or hold the stack top forever. Effectively one
 continuous capture at a time.

 Threading contract:
 - Submit()/Cancel(): any thread; they never touch the graphics context.
 - LatchFrame()/TakeActive()/Complete()/Fail()/FrameComplete(): render thread.
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

  //! Render thread, frame boundary: push newly submitted requests onto the
  //! stack, prune cancelled ones, and latch the stack top as this frame's
  //! serviced consumer.
  //! \return true when a frame must be forced: once per arriving request, and
  //! while the latched consumer is a ONESHOT (until it serves or FrameComplete
  //! fails it); a lone CONTINUOUS request never forces one, so unchanged frames
  //! are still skipped. The caller owns the dirty marking.
  bool LatchFrame();

  //! Render thread: the latched request this content's tap must service, if
  //! any (at most one request is latched per frame).
  std::vector<std::shared_ptr<CaptureRequest>> TakeActive(CaptureContent content);

  //! Render thread: deliver a serviced request. ONESHOT requests finish;
  //! CONTINUOUS requests stay registered for the next frame.
  void Complete(const std::shared_ptr<CaptureRequest>& request, CaptureResult result);

  //! Render thread: fail a serviced request and wake its waiter, whatever its
  //! cadence. Use this when the capture cannot succeed for the current
  //! configuration (video on a hardware plane, no capture surface, a failed
  //! readback): those are fixed properties, so retrying cannot help.
  void Fail(const std::shared_ptr<CaptureRequest>& request);

  //! Render thread: the frame is drawn and every tap has run. A request still
  //! latched and unserved is failed, whatever its cadence: a whole frame went
  //! by without any tap serving it, so none will in this configuration. Call
  //! this only on a frame that really drew, otherwise an idle GUI would fail
  //! requests that are working.
  void FrameComplete();

  //! Any thread: abandon a request; no callback fires after this returns.
  void Cancel(const std::shared_ptr<CaptureRequest>& request);

  CaptureState GetState(const CaptureRequest& request) const;

  //! Any thread: completed deliveries so far; waiters compare against the last one seen.
  uint64_t GetDeliveries(const CaptureRequest& request) const;

  //! Any thread: locked copy of the latest result; per-frame pixel buffers are immutable.
  CaptureResult CopyResult(const CaptureRequest& request) const;

  //! Teardown: fail every request, wake every waiter, drop queued callbacks.
  void FailAll();

private:
  void Dispatch(const std::shared_ptr<CaptureRequest>& request);
  void RemoveActive(const std::shared_ptr<CaptureRequest>& request);

  mutable CCriticalSection m_lock;
  std::atomic<bool> m_hasPending{false};
  //! guarded by m_lock
  std::vector<std::shared_ptr<CaptureRequest>> m_pending;
  //! the consumer stack, submission order, back() is the top; the vector
  //! itself is owned by the render thread
  std::vector<std::shared_ptr<CaptureRequest>> m_stack;
  //! FIFO with one job at a time: completions are delivered in order
  CJobQueue m_callbackQueue;
};

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
