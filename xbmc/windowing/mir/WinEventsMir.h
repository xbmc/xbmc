/*
 *      Copyright (C) 2016 Canonical Ltd.
 *      brandon.schaefer@canonical.com
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */


#pragma once

#include <mutex>
#include <queue>
#include <mir_toolkit/mir_client_library.h>

#include "../WinEvents.h"

extern void MirHandleEvent(MirWindow* window, MirEvent const* ev, void* context);

class CWinEventsMir : public IWinEvents
{
public:
  virtual ~CWinEventsMir() {};
  bool MessagePump() override;
  void  MessagePush(XBMC_Event* ev);

private:
  size_t GetQueueSize();

  std::queue<XBMC_Event> m_events;
  std::mutex m_mutex;
};
