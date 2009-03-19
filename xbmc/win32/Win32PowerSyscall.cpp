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
#include "Win32PowerSyscall.h"
#ifdef _WIN32PC
#include "WIN32Util.h"
CWin32PowerSyscall::CWin32PowerSyscall()
{
}

bool CWin32PowerSyscall::Powerdown()
{
  return CWIN32Util::PowerManagement(POWERSTATE_SHUTDOWN);
}
bool CWin32PowerSyscall::Suspend()
{
  return CWIN32Util::PowerManagement(POWERSTATE_SUSPEND);
}
bool CWin32PowerSyscall::Hibernate()
{
  return CWIN32Util::PowerManagement(POWERSTATE_HIBERNATE);
}
bool CWin32PowerSyscall::Reboot()
{
  return CWIN32Util::PowerManagement(POWERSTATE_REBOOT);
}

bool CWin32PowerSyscall::CanPowerdown()
{
  return true;
}
bool CWin32PowerSyscall::CanSuspend()
{
  return true;
}
bool CWin32PowerSyscall::CanHibernate()
{
  return true;
}
bool CWin32PowerSyscall::CanReboot()
{
  return true;
}
#endif
