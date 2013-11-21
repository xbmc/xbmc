/*
 *      Copyright (C) 2012-2013 Team XBMC
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
 
#if defined (TARGET_ANDROID)

#include "AndroidPowerSyscall.h"
#include "android/activity/XBMCApp.h"
#include "filesystem/File.h"
#include <unistd.h>

CAndroidPowerSyscall::CAndroidPowerSyscall()
{
  m_isRooted = false;
  m_su_path = "/system/bin/su";

  if (XFILE::CFile::Exists(m_su_path))
    m_isRooted = true;
  else
  {
    m_su_path = "/system/xbin/su";
    if (XFILE::CFile::Exists(m_su_path))
      m_isRooted = true;
  }
}

CAndroidPowerSyscall::~CAndroidPowerSyscall()
{ }

bool CAndroidPowerSyscall::Powerdown()
{
  if (!m_isRooted)
    return false;

  int rc = system((m_su_path + " -c \"reboot -p\"").c_str());
  return (rc == 0);
}

bool CAndroidPowerSyscall::Reboot()
{
  if (!m_isRooted)
    return false;

  int rc = system((m_su_path + " -c reboot").c_str());
  return (rc == 0);
}

int CAndroidPowerSyscall::BatteryLevel(void)
{
  return CXBMCApp::GetBatteryLevel();
}

bool CAndroidPowerSyscall::PumpPowerEvents(IPowerEventsCallback *callback)
{    
  return true;
}

#endif
