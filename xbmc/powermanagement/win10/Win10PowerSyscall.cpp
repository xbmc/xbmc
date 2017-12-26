/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "Win10PowerSyscall.h"

using namespace Windows::Devices::Power;

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
  auto aggBattery = Battery::AggregateBattery;
  auto report = aggBattery->GetReport();

  int remaining = 0;
  if (report->RemainingCapacityInMilliwattHours)
    remaining = report->RemainingCapacityInMilliwattHours->Value;
  int full = 0;
  if (report->FullChargeCapacityInMilliwattHours)
    full = report->FullChargeCapacityInMilliwattHours->Value;

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
