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
  m_service.Cancel(m_request);
}

bool CCaptureHandle::Wait(std::chrono::milliseconds timeout)
{
  if (!m_request->event.Wait(timeout))
    return false;

  return m_service.GetState(*m_request) == CaptureState::DONE;
}

const CaptureResult& CCaptureHandle::GetResult() const
{
  return m_request->result;
}

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
