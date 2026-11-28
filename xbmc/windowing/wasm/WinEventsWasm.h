/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "windowing/WinEvents.h"
#include "windowing/XBMC_events.h"

#include <atomic>
#include <deque>
#include <mutex>
#include <unordered_set>

class CWinEventsWasm : public IWinEvents
{
public:
  CWinEventsWasm();
  ~CWinEventsWasm() override;

  bool MessagePump() override;

  void MessagePush(const XBMC_Event& newEvent);

  //! \brief Remembers a key whose press was not passed to Kodi.
  void AddSwallowedKey(unsigned long keyCode);
  //! \return whether the key's press was swallowed
  bool RemoveSwallowedKey(unsigned long keyCode);

  //! \return the browser's device pixels per CSS pixel
  static double DevicePixelRatio();

  //! \brief Scales pointer positions, which arrive in CSS pixels, by scale from now on, to
  //! match the render resolution.
  void SetPointerScale(double scale);
  double GetPointerScale() const { return m_pointerScale; }

private:
  void ProcessProxyCallbacks();
  bool DispatchQueuedEvents();

  std::mutex m_mutex;
  std::deque<XBMC_Event> m_events;

  std::atomic<double> m_pointerScale{1.0};

  // Keys whose press was not passed to Kodi. Their release must be dropped too,
  // or Kodi's long-press handling acts on a release it never saw the press for.
  // Only used on the browser thread.
  std::unordered_set<unsigned long> m_swallowedKeys;
};
