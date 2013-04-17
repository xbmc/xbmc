/*
 *      Copyright (C) 2013 Team XBMC
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

#ifndef _POWER_SYSCALL_VIRTUAL_SLEEP_H_
#define _POWER_SYSCALL_VIRTUAL_SLEEP_H_
#include "IPowerSyscall.h"

// Systems that have no native standby mode, can base their
// IPowerSyscall implementation on this class, and need only
// implement VirtualSleep()/VirtualWake().
class CPowerSyscallVirtualSleep : public IPowerSyscall
{
public:
  CPowerSyscallVirtualSleep() : m_virtualSleepState(VIRTUAL_SLEEP_STATE_AWAKE) {}
  virtual ~CPowerSyscallVirtualSleep() {}

  virtual bool CanSuspend() { return true; }
  virtual bool Suspend();

  virtual bool PumpPowerEvents(IPowerEventsCallback *callback);

  virtual bool ProcessAction(const CAction& action);

  virtual bool VirtualSleep() = 0;
  virtual bool VirtualWake()  = 0;

protected:
  // keep track of virtual sleep state for devices that support it
  typedef enum {
    VIRTUAL_SLEEP_STATE_AWAKE = 0,
    VIRTUAL_SLEEP_STATE_ASLEEP,
    VIRTUAL_SLEEP_STATE_WILL_WAKE,
    VIRTUAL_SLEEP_STATE_WILL_SLEEP,
  } VirtualSleepState;

  VirtualSleepState m_virtualSleepState;
};

#endif // _POWER_SYSCALL_VIRTUAL_SLEEP_H_
