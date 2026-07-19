/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

enum class CaptureState;
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

  //! Block until a delivery newer than the last one seen by this handle.
  //! Deliveries between calls collapse to the latest one.
  //! \return true on a new delivery; false on timeout, failure or cancel
  bool WaitNext(std::chrono::milliseconds timeout);

  //! Locked copy of the latest delivery; pair with WaitNext().
  CaptureResult CopyResult() const;

  //! Current request state, so pump loops can fail fast.
  CaptureState GetState() const;

  //! Release ownership without cancelling: the request completes or fails on
  //! its own and its callback still runs. Wait() is unavailable afterwards.
  void Detach();

private:
  CCaptureService& m_service;
  std::shared_ptr<CaptureRequest> m_request;
  //! Delivery count already consumed through WaitNext()
  uint64_t m_lastDelivery{0};
};

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
