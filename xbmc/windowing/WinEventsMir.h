/*
*      Copyright (C) 2005-2013 Team XBMC
*      http://www.xbmc.org
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

#ifndef WINDOW_EVENTS_MIR_H
#define WINDOW_EVENTS_MIR_H

#pragma once
#include "windowing/WinEvents.h"

class IDllMirClient;
class IDllXKBCommon;

namespace xbmc
{
namespace mir
{
class IEventPipe
{
public:

  virtual ~IEventPipe() {};
  virtual void Pump() = 0;
};
}
}

class CWinEventsMir : public CWinEventsBase
{
public:
  CWinEventsMir();
  static bool MessagePump();
  static void RefreshDevices();
  static bool IsRemoteLowBattery();
  
  static void SetEventPipe(IDllMirClient &,
                           IDllXKBCommon &,
                           xbmc::mir::IEventPipe &);
  static void RemoveEventPipe();
};

#endif
