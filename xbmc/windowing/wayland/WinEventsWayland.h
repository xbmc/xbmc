/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../WinEvents.h"
#include "threads/CriticalSection.h"

#include <queue>

namespace wayland
{
class event_queue_t;
class display_t;
}

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class CWinEventsWayland : public IWinEvents
{
public:
  bool MessagePump() override;
  void MessagePush(XBMC_Event* ev);
  /// Write buffered messages to the compositor
  static void Flush();
  /// Do a roundtrip on the specified queue from the event processing thread
  static void RoundtripQueue(wayland::event_queue_t const& queue);

private:
  friend class CWinSystemWayland;
  static void SetDisplay(wayland::display_t* display);

  CCriticalSection m_queueMutex;
  std::queue<XBMC_Event> m_queue;
};

}
}
}
