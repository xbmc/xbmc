/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidPowerSyscall.h"

#include "utils/log.h"

#include "platform/android/activity/XBMCApp.h"

IPowerSyscall* CAndroidPowerSyscall::CreateInstance()
{
  return new CAndroidPowerSyscall();
}

void CAndroidPowerSyscall::Register()
{
  IPowerSyscall::RegisterPowerSyscall(CAndroidPowerSyscall::CreateInstance);
}

int CAndroidPowerSyscall::BatteryLevel(void)
{
  return CXBMCApp::GetBatteryLevel();
}

bool CAndroidPowerSyscall::PumpPowerEvents(IPowerEventsCallback *callback)
{
  switch (m_state)
  {
    case SUSPENDED:
      callback->OnSleep();
      CLog::Log(LOGINFO, "{}: OnSleep called", __FUNCTION__);
      break;
    case RESUMED:
      callback->OnWake();
      CLog::Log(LOGINFO, "{}: OnWake called", __FUNCTION__);
      break;
    default:
      return false;
  }
  m_state = REPORTED;
  return true;
}
