/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CaptureService.h"

#include "rendering/capture/CaptureHandle.h"

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
  if (!m_hasPending.load(std::memory_order_acquire) && m_active.empty())
    return false;

  bool needsFrame = false;

  {
    std::unique_lock lock(m_lock);
    m_hasPending.store(false, std::memory_order_relaxed);

    std::erase_if(m_active, [](const std::shared_ptr<CaptureRequest>& request)
                  { return request->state == CaptureState::CANCELLED; });

    for (auto& request : m_pending)
    {
      if (request->state != CaptureState::PENDING)
        continue;
      request->state = CaptureState::LATCHED;
      m_active.push_back(request);
      needsFrame = true;
    }
    m_pending.clear();
  }

  return needsFrame;
}

std::vector<std::shared_ptr<CaptureRequest>> CCaptureService::TakeActive(CaptureContent content)
{
  std::vector<std::shared_ptr<CaptureRequest>> matches;
  for (const auto& request : m_active)
  {
    if (request->spec.content == content)
      matches.push_back(request);
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
  {
    std::unique_lock lock(m_lock);
    if (request->state == CaptureState::LATCHED || request->state == CaptureState::PENDING)
      request->state = CaptureState::FAILED;
  }

  request->event.Set();
  RemoveActive(request);
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

    for (auto& request : m_active)
    {
      if (request->state == CaptureState::LATCHED)
        request->state = CaptureState::FAILED;
    }
    all.insert(all.end(), m_active.begin(), m_active.end());
    m_active.clear();
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
  std::erase(m_active, request);
}

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
