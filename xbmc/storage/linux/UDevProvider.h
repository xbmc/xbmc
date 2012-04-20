#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "storage/IStorageProvider.h"

#ifdef HAVE_LIBUDEV

struct udev;
struct udev_monitor;

class CUDevProvider : public IStorageProvider
{
public:
  CUDevProvider();
  virtual ~CUDevProvider() { }

  virtual void Initialize();
  virtual void Stop();

  virtual void GetLocalDrives(VECSOURCES &localDrives);
  virtual void GetRemovableDrives(VECSOURCES &removableDrives);

  virtual bool Eject(CStdString mountpath);

  virtual std::vector<CStdString> GetDiskUsage();

  virtual bool PumpDriveChangeEvents(IStorageEventsCallback *callback);

private:
  void GetDisks(VECSOURCES& devices, bool removable);

  struct udev         *m_udev;
  struct udev_monitor *m_udevMon;
};

#endif
