/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CaptureService.h"

#include "rendering/capture/CaptureHandle.h"
#include "utils/log.h"

#include <mutex>

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

CCaptureService::~CCaptureService()
{
  FailAll();
}

std::unique_ptr<CCaptureHandle> CCaptureService::Submit(const CaptureSpec& spec,
                                                        CaptureCallback callback)
{
  auto request = std::make_shared<CaptureRequest>();
  request->spec = spec;
  request->callback = std::move(callback);

  {
    std::unique_lock lock(m_lock);
    m_pending.push_back(request);
    m_hasPending.store(true, std::memory_order_release);
  }

  return std::make_unique<CCaptureHandle>(*this, std::move(request));
}

bool CCaptureService::LatchFrame()
{
  if (!m_hasPending.load(std::memory_order_acquire) && m_stack.empty())
    return false;

  bool needsFrame = false;

  {
    std::unique_lock lock(m_lock);
    m_hasPending.store(false, std::memory_order_relaxed);

    std::erase_if(m_stack, [](const std::shared_ptr<CaptureRequest>& request) {
      return request->state == CaptureState::CANCELLED ||
             request->state == CaptureState::FAILED;
    });

    // arriving requests push onto the stack top in submission order; each
    // forces one frame so an idle GUI still renders something to tap
    for (auto& request : m_pending)
    {
      if (request->state != CaptureState::PENDING)
        continue;
      m_stack.push_back(request);
      needsFrame = true;
    }
    m_pending.clear();

    // one consumer per render loop: the most recent request wins whatever its
    // cadence, so a one-shot submitted just before a continuous is buried
    // until the continuous is gone (a rare late or lost screenshot, accepted)
    for (auto& request : m_stack)
    {
      if (request->state == CaptureState::LATCHED)
        request->state = CaptureState::PENDING;
    }
    std::shared_ptr<CaptureRequest> chosen;
    if (!m_stack.empty())
      chosen = m_stack.back();

    if (chosen)
    {
      chosen->served = false; // FrameComplete judges this at the end of the frame
      chosen->state = CaptureState::LATCHED;
    }

    // the chosen one-shot forces frames until it serves or FrameComplete fails
    // it; a buried one-shot waits quietly and a lone CONTINUOUS never forces,
    // so unchanged frames are still skipped during a steady continuous capture
    if (chosen && chosen->spec.cadence == CaptureCadence::ONESHOT)
      needsFrame = true;
  }

  return needsFrame;
}

std::vector<std::shared_ptr<CaptureRequest>> CCaptureService::TakeActive(CaptureContent content)
{
  std::vector<std::shared_ptr<CaptureRequest>> matches;
  std::unique_lock lock(m_lock);
  // exactly one request is LATCHED per frame; return it wherever it sits
  for (const auto& request : m_stack)
  {
    if (request->state == CaptureState::LATCHED && request->spec.content == content)
    {
      matches.push_back(request);
      break;
    }
  }
  return matches;
}

void CCaptureService::Complete(const std::shared_ptr<CaptureRequest>& request, CaptureResult result)
{
  bool dispatch = false;
  bool finished = false;

  {
    std::unique_lock lock(m_lock);
    if (request->state == CaptureState::LATCHED)
    {
      request->result = std::move(result);
      request->deliveries++;
      request->served = true;
      dispatch = true;
      if (request->spec.cadence == CaptureCadence::ONESHOT)
      {
        request->state = CaptureState::DONE;
        finished = true;
      }
    }
    else
    {
      // cancelled while a tap was servicing it
      finished = true;
    }
  }

  request->event.Set();

  if (finished)
    RemoveActive(request);
  if (dispatch && request->callback)
    Dispatch(request);
}

void CCaptureService::Fail(const std::shared_ptr<CaptureRequest>& request)
{
  bool failed = false;

  {
    std::unique_lock lock(m_lock);
    if (request->state == CaptureState::LATCHED || request->state == CaptureState::PENDING)
    {
      request->state = CaptureState::FAILED;
      failed = true;
    }
  }

  if (failed)
    CLog::LogF(LOGERROR, "capture request failed (content {}, {}x{})",
               static_cast<int>(request->spec.content), request->spec.width,
               request->spec.height);

  request->event.Set();
  if (failed)
    RemoveActive(request);
}

void CCaptureService::FrameComplete()
{
  std::shared_ptr<CaptureRequest> unserved;

  {
    std::unique_lock lock(m_lock);
    for (const auto& request : m_stack)
    {
      if (request->state != CaptureState::LATCHED)
        continue;
      if (!request->served)
        unserved = request;
      break; // at most one request is latched per frame
    }
  }

  if (unserved)
    Fail(unserved);
}

void CCaptureService::Cancel(const std::shared_ptr<CaptureRequest>& request)
{
  {
    std::unique_lock lock(m_lock);
    if (request->state == CaptureState::DONE || request->state == CaptureState::FAILED)
      return;
    request->state = CaptureState::CANCELLED;
  }

  request->event.Set();
}

CaptureState CCaptureService::GetState(const CaptureRequest& request) const
{
  std::unique_lock lock(m_lock);
  return request.state;
}

uint64_t CCaptureService::GetDeliveries(const CaptureRequest& request) const
{
  std::unique_lock lock(m_lock);
  return request.deliveries;
}

CaptureResult CCaptureService::CopyResult(const CaptureRequest& request) const
{
  std::unique_lock lock(m_lock);
  return request.result;
}

void CCaptureService::FailAll()
{
  std::vector<std::shared_ptr<CaptureRequest>> all;

  {
    std::unique_lock lock(m_lock);
    for (auto& request : m_pending)
    {
      if (request->state == CaptureState::PENDING)
        request->state = CaptureState::FAILED;
    }
    all = std::move(m_pending);
    m_pending.clear();

    for (auto& request : m_stack)
    {
      if (request->state == CaptureState::LATCHED || request->state == CaptureState::PENDING)
        request->state = CaptureState::FAILED;
    }
    all.insert(all.end(), m_stack.begin(), m_stack.end());
    m_stack.clear();
  }

  for (auto& request : all)
    request->event.Set();

  m_callbackQueue.CancelJobs();
}

void CCaptureService::Dispatch(const std::shared_ptr<CaptureRequest>& request)
{
  m_callbackQueue.Submit([request] { request->callback(request->result); });
}

void CCaptureService::RemoveActive(const std::shared_ptr<CaptureRequest>& request)
{
  std::erase(m_stack, request);
}

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
