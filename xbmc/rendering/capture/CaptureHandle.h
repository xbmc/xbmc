/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <chrono>
#include <memory>

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

class CCaptureService;
struct CaptureRequest;
struct CaptureResult;

//! RAII owner of a submitted capture request: destruction cancels it.
//! Handles must not outlive the service that issued them.
class CCaptureHandle
{
public:
  CCaptureHandle(CCaptureService& service, std::shared_ptr<CaptureRequest> request);
  ~CCaptureHandle();
  CCaptureHandle(const CCaptureHandle&) = delete;
  CCaptureHandle& operator=(const CCaptureHandle&) = delete;

  //! \return true when the capture completed; false on timeout, failure or cancel
  bool Wait(std::chrono::milliseconds timeout);

  //! Valid only after Wait() returned true.
  const CaptureResult& GetResult() const;

private:
  CCaptureService& m_service;
  std::shared_ptr<CaptureRequest> m_request;
};

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
