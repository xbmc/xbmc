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
#include "storage/IStorageProvider.h"
#include "HALProvider.h"
#include "DeviceKitDisksProvider.h"
#include "UDisksProvider.h"
#include "PosixMountProvider.h"

class CLinuxStorageProvider : public IStorageProvider
{
public:
  CLinuxStorageProvider()
  {
    m_instance = NULL;

#ifdef HAS_DBUS
    if (CUDisksProvider::HasUDisks())
      m_instance = new CUDisksProvider();
    else if (CDeviceKitDisksProvider::HasDeviceKitDisks())
      m_instance = new CDeviceKitDisksProvider();
#endif
#ifdef HAS_HAL
    if (m_instance == NULL)
      m_instance = new CHALProvider();
#endif

    if (m_instance == NULL)
      m_instance = new CPosixMountProvider();
  }

  virtual ~CLinuxStorageProvider()
  {
    delete m_instance;
  }

  virtual void Initialize()
  {
    m_instance->Initialize();
  }

  virtual void Stop()
  {
    m_instance->Stop();
  }

  virtual void GetLocalDrives(VECSOURCES &localDrives)
  {
    // Home directory
    CMediaSource share;
    share.strPath = getenv("HOME");
    share.strName = g_localizeStrings.Get(21440);
    share.m_ignore = true;
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    localDrives.push_back(share);
    share.strPath = "/";
    share.strName = g_localizeStrings.Get(21453);
    localDrives.push_back(share);

    m_instance->GetLocalDrives(localDrives);
  }

  virtual void GetRemovableDrives(VECSOURCES &removableDrives)
  {
    m_instance->GetRemovableDrives(removableDrives);
  }

  virtual bool Eject(CStdString mountpath)
  {
    return m_instance->Eject(mountpath);
  }

  virtual std::vector<CStdString> GetDiskUsage()
  {
    return m_instance->GetDiskUsage();
  }

  virtual bool PumpDriveChangeEvents(IStorageEventsCallback *callback)
  {
    return m_instance->PumpDriveChangeEvents(callback);
  }

private:
  IStorageProvider *m_instance;
};
