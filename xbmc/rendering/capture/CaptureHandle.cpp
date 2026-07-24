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

void CCaptureHandle::Detach()
{
  m_request.reset();
}

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
