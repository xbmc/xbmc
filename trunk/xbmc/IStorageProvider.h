#pragma once
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
#include "system.h"
#include "MediaSource.h"

class IStorageEventsCallback
{
public:
  virtual ~IStorageEventsCallback() { }

  virtual void OnStorageAdded(const CStdString &label, const CStdString &path) = 0;
  virtual void OnStorageSafelyRemoved(const CStdString &label) = 0;
  virtual void OnStorageUnsafelyRemoved(const CStdString &label) = 0;
};

class IStorageProvider
{
public:
  virtual ~IStorageProvider() { }

  virtual void Initialize() = 0;
  virtual void Stop() = 0;

  virtual void GetLocalDrives(VECSOURCES &localDrives) = 0;
  virtual void GetRemovableDrives(VECSOURCES &removableDrives) = 0;

  virtual bool Eject(CStdString mountpath) = 0;

  virtual std::vector<CStdString> GetDiskUsage() = 0;

  virtual bool PumpDriveChangeEvents(IStorageEventsCallback *callback) = 0;
};
