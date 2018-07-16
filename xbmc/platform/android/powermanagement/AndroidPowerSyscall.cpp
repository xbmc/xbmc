/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#if defined (TARGET_ANDROID)

#include "AndroidPowerSyscall.h"
#include "platform/android/activity/XBMCApp.h"

IPowerSyscall* CAndroidPowerSyscall::CreateInstance()
{
  return new CAndroidPowerSyscall();
}

void CAndroidPowerSyscall::Register()
{
  IPowerSyscall::RegisterPowerSyscall(CAndroidPowerSyscall::CreateInstance);
}

CAndroidPowerSyscall::CAndroidPowerSyscall()
{ }

CAndroidPowerSyscall::~CAndroidPowerSyscall()
{ }

int CAndroidPowerSyscall::BatteryLevel(void)
{
  return CXBMCApp::GetBatteryLevel();
}

bool CAndroidPowerSyscall::PumpPowerEvents(IPowerEventsCallback *callback)
{
  return true;
}

#endif
