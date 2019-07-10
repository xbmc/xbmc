/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Win10PowerSyscall.h"

#include <winrt/Windows.Devices.Power.h>

using namespace winrt::Windows::Devices::Power;

IPowerSyscall* CPowerSyscall::CreateInstance()
{
  return new CPowerSyscall();
}

void CPowerSyscall::Register()
{
  IPowerSyscall::RegisterPowerSyscall(CPowerSyscall::CreateInstance);
}

CPowerSyscall::CPowerSyscall() = default;

CPowerSyscall::~CPowerSyscall() = default;

bool CPowerSyscall::Powerdown()
{
  return false;
}
bool CPowerSyscall::Suspend()
{
  return false;
}
bool CPowerSyscall::Hibernate()
{
  return false;
}
bool CPowerSyscall::Reboot()
{
  return false;
}

bool CPowerSyscall::CanPowerdown()
{
  return false;
}
bool CPowerSyscall::CanSuspend()
{
  return false;
}
bool CPowerSyscall::CanHibernate()
{
  return false;
}
bool CPowerSyscall::CanReboot()
{
  return false;
}

int CPowerSyscall::BatteryLevel()
{
  int result = 0;
  auto aggBattery = Battery::AggregateBattery();
  auto report = aggBattery.GetReport();

  int remaining = 0;
  if (report.RemainingCapacityInMilliwattHours())
    remaining = report.RemainingCapacityInMilliwattHours().GetInt32();
  int full = 0;
  if (report.FullChargeCapacityInMilliwattHours())
    full = report.FullChargeCapacityInMilliwattHours().GetInt32();

  if (full != 0 && remaining != 0)
  {
    float percent = static_cast<float>(remaining) / static_cast<float>(full);
    result = static_cast<int> (percent * 100.0f);
  }
  return result;
}

bool CPowerSyscall::PumpPowerEvents(IPowerEventsCallback *callback)
{
  return true;
}
