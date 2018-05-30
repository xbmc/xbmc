/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include <atomic>
#include <string>
#include <vector>

#include "storage/IStorageProvider.h"
#include "utils/Job.h"

class CStorageProvider : public ::IStorageProvider
{
public:
  virtual ~CStorageProvider();

  void Initialize() override;
  void Stop() override { }

  void GetLocalDrives(VECSOURCES &localDrives) override;
  void GetRemovableDrives(VECSOURCES &removableDrives) override;
  std::string GetFirstOpticalDeviceFileName() override;
  bool Eject(const std::string& mountpath) override;
  std::vector<std::string> GetDiskUsage() override;
  bool PumpDriveChangeEvents(IStorageEventsCallback *callback) override;

private:
  enum Drive_Types
  {
    ALL_DRIVES = 0,
    LOCAL_DRIVES,
    REMOVABLE_DRIVES,
    DVD_DRIVES
  };
  static void GetDrivesByType(VECSOURCES &localDrives, Drive_Types eDriveType = ALL_DRIVES, bool bonlywithmedia = false);

  winrt::Windows::Devices::Enumeration::DeviceWatcher m_watcher{ nullptr };
  std::atomic<bool> m_changed;
};
