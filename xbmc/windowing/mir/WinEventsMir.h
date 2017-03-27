/*
 *      Copyright (C) 2016 Canonical Ltd.
 *      brandon.schaefer@canonical.com
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
  size_t GetQueueSize() override;
  void  MessagePush(XBMC_Event* ev);

private:
  std::queue<XBMC_Event> m_events;
  std::mutex m_mutex;
};
