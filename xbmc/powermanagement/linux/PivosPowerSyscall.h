/*
 *      Copyright (C) 2012 Team XBMC
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

#include "powermanagement/IPowerSyscall.h"

class CPivosPowerSyscall : public CPowerSyscallWithoutEvents
{
public:
  CPivosPowerSyscall();
  virtual ~CPivosPowerSyscall() {}

  virtual bool Powerdown();
  virtual bool Suspend();
  virtual bool Hibernate();
  virtual bool Reboot();

  virtual bool CanPowerdown();
  virtual bool CanSuspend();
  virtual bool CanHibernate();
  virtual bool CanReboot();
  virtual int  BatteryLevel();

  static bool HasPivosPowerSyscall();

private:
  bool m_CanPowerdown;
  bool m_CanSuspend;
  bool m_CanHibernate;
  bool m_CanReboot;
};
