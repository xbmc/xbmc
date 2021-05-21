/*
 *  Copyright (C) 2012-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TVOSPowerSyscall.h"

#include "utils/log.h"

IPowerSyscall* CTVOSPowerSyscall::CreateInstance()
{
  return new CTVOSPowerSyscall;
}

void CTVOSPowerSyscall::Register()
{
  IPowerSyscall::RegisterPowerSyscall(CTVOSPowerSyscall::CreateInstance);
}

bool CTVOSPowerSyscall::Powerdown()
{
  return false;
}

bool CTVOSPowerSyscall::Suspend()
{
  return false;
}

bool CTVOSPowerSyscall::Hibernate()
{
  return false;
}

bool CTVOSPowerSyscall::Reboot()
{
  return false;
}

bool CTVOSPowerSyscall::CanPowerdown()
{
  return false;
}

bool CTVOSPowerSyscall::CanSuspend()
{
  return false;
}

bool CTVOSPowerSyscall::CanHibernate()
{
  return false;
}

bool CTVOSPowerSyscall::CanReboot()
{
  return false;
}

int CTVOSPowerSyscall::BatteryLevel()
{
  return 0;
}

bool CTVOSPowerSyscall::PumpPowerEvents(IPowerEventsCallback* callback)
{
  switch (m_state)
  {
    case SUSPENDED:
      callback->OnSleep();
      CLog::Log(LOGDEBUG, "{}: OnSleep called", __FUNCTION__);
      break;
    case RESUMED:
      callback->OnWake();
      CLog::Log(LOGDEBUG, "{}: OnWake called", __FUNCTION__);
      break;
    default:
      return false;
  }
  m_state = REPORTED;
  return true;
}

void CTVOSPowerSyscall::SetOnPause()
{
  m_state = SUSPENDED;
}

void CTVOSPowerSyscall::SetOnResume()
{
  m_state = RESUMED;
}