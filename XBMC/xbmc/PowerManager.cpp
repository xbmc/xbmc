/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "PowerManager.h"
#include "Application.h"

#ifdef __APPLE__
#include "osx/CocoaPowerSyscall.h"
#elif defined(_LINUX) && defined(HAS_DBUS)
#include "linux/DBusPowerSyscall.h"
#elif defined(_WIN32PC)
#include "win32/Win32PowerSyscall.h"
#endif

CPowerManager g_powerManager;

CPowerManager::CPowerManager()
{
  m_instance = NULL;

#ifdef __APPLE__
  m_instance = new CCocoaPowerSyscall();
#elif defined(_LINUX) && defined(HAS_DBUS)
  m_instance = new CDBusPowerSyscall();
#elif defined(_WIN32PC)
  m_instance = new CWin32PowerSyscall();
#endif

  if (m_instance == NULL)
    m_instance = new CNullPowerSyscall();
}

CPowerManager::~CPowerManager()
{
  delete m_instance;
}

bool CPowerManager::Powerdown()
{
  return CanPowerdown() ? m_instance->Powerdown() : false;
}
bool CPowerManager::Suspend()
{
  if (CanSuspend())
  {
    g_application.m_restartLirc = true;
    g_application.m_restartLCD= true;
    return m_instance->Suspend();
  }
  
  return false;
}
bool CPowerManager::Hibernate()
{
  if (CanHibernate())
  {
    g_application.m_restartLirc = true;
    g_application.m_restartLCD= true;
    return m_instance->Hibernate();
  }

  return false;
}
bool CPowerManager::Reboot()
{
  return CanReboot() ? m_instance->Reboot() : false;
}

bool CPowerManager::CanPowerdown()
{
  return m_instance->CanPowerdown();
}
bool CPowerManager::CanSuspend()
{
  return m_instance->CanSuspend();
}
bool CPowerManager::CanHibernate()
{
  return m_instance->CanHibernate();
}
bool CPowerManager::CanReboot()
{
  return m_instance->CanReboot();
}
