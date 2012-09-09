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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "HALProvider.h"
#ifdef HAS_HAL
#include "HALManager.h"
#include "utils/log.h"
#include "PosixMountProvider.h"

CHALProvider::CHALProvider()
{
  m_removableLength = 0;
}

void CHALProvider::Initialize()
{
  CLog::Log(LOGDEBUG, "Selected HAL as storage provider");
  g_HalManager.Initialize();
  PumpDriveChangeEvents(NULL);
}

void CHALProvider::Stop()
{
  g_HalManager.Stop();
}

void CHALProvider::GetLocalDrives(VECSOURCES &localDrives)
{
  std::vector<CStorageDevice> devices = g_HalManager.GetVolumeDevices();

  for (size_t i = 0; i < devices.size(); i++)
  {
    if (devices[i].Mounted && devices[i].Approved && !devices[i].HotPlugged)
    {
      CMediaSource share;
      devices[i].toMediaSource(&share);
      localDrives.push_back(share);
    }
  }
}

void CHALProvider::GetRemovableDrives(VECSOURCES &removableDrives)
{
  std::vector<CStorageDevice> devices = g_HalManager.GetVolumeDevices();

  for (size_t i = 0; i < devices.size(); i++)
  {
    if (devices[i].Mounted && devices[i].Approved && devices[i].HotPlugged)
    {
      CMediaSource share;
      devices[i].toMediaSource(&share);
      removableDrives.push_back(share);
    }
  }
}

bool CHALProvider::Eject(CStdString mountpath)
{
  return g_HalManager.Eject(mountpath);
}

std::vector<CStdString> CHALProvider::GetDiskUsage()
{
  CPosixMountProvider legacy;
  return legacy.GetDiskUsage();
}

// TODO Use HALs events for this instead.
bool CHALProvider::PumpDriveChangeEvents(IStorageEventsCallback *callback)
{
//Pump HalManager dry of events
  bool changed = false;
  while (g_HalManager.Update())
    changed = true;

  return changed;
}
#endif
