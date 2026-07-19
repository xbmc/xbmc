/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CaptureHandle.h"

#include "rendering/capture/CaptureRequest.h"
#include "rendering/capture/CaptureService.h"
#include "threads/SystemClock.h"
#include "utils/log.h"

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

CCaptureHandle::CCaptureHandle(CCaptureService& service, std::shared_ptr<CaptureRequest> request)
  : m_service(service),
    m_request(std::move(request))
{
}

CCaptureHandle::~CCaptureHandle()
{
  if (m_request)
    m_service.Cancel(m_request);
}

bool CCaptureHandle::Wait(std::chrono::milliseconds timeout)
{
  if (!m_request || !m_request->event.Wait(timeout))
    return false;

  return m_service.GetState(*m_request) == CaptureState::DONE;
}

const CaptureResult& CCaptureHandle::GetResult() const
{
  if (!m_request)
  {
    CLog::LogF(LOGERROR, "called on a detached handle");
    static const CaptureResult empty;
    return empty;
  }
  return m_request->result;
}

bool CCaptureHandle::WaitNext(std::chrono::milliseconds timeout)
{
  if (!m_request)
    return false;

  XbmcThreads::EndTime<> deadline{timeout};
  while (true)
  {
    const uint64_t deliveries = m_service.GetDeliveries(*m_request);
    if (deliveries != m_lastDelivery)
    {
      m_lastDelivery = deliveries;
      return true;
    }

    const CaptureState state = m_service.GetState(*m_request);
    if (state == CaptureState::DONE || state == CaptureState::FAILED ||
        state == CaptureState::CANCELLED)
    {
      // Complete() raises the count and finishes a ONESHOT under one lock, so
      // a delivery can land between the two reads above. Nothing is delivered
      // after these states, so one re-read settles it.
      const uint64_t settled = m_service.GetDeliveries(*m_request);
      if (settled == m_lastDelivery)
        return false;
      m_lastDelivery = settled;
      return true;
    }

    if (deadline.IsTimePast() || !m_request->event.Wait(deadline.GetTimeLeft()))
      return false;
  }
}

CaptureResult CCaptureHandle::CopyResult() const
{
  if (!m_request)
  {
    CLog::LogF(LOGERROR, "called on a detached handle");
    return {};
  }
  return m_service.CopyResult(*m_request);
}

CaptureState CCaptureHandle::GetState() const
{
  if (!m_request)
    return CaptureState::CANCELLED;
  return m_service.GetState(*m_request);
}

void CCaptureHandle::Detach()
{
  m_request.reset();
}

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
