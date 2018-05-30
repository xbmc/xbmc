/*
 *      Copyright (C) 2017 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <queue>

#include "threads/CriticalSection.h"
#include "../WinEvents.h"

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
