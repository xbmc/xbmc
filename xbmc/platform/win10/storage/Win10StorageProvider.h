/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "storage/IStorageProvider.h"
#include "utils/Job.h"

#include <atomic>
#include <string>
#include <vector>

class CStorageProvider : public IStorageProvider
{
public:
  virtual ~CStorageProvider();

  void Initialize() override;
  void Stop() override { }

  void GetLocalDrives(std::vector<CMediaSource>& localDrives) override;
  void GetRemovableDrives(std::vector<CMediaSource>& removableDrives) override;
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
  static void GetDrivesByType(std::vector<CMediaSource>& localDrives,
                              Drive_Types eDriveType = ALL_DRIVES,
                              bool bonlywithmedia = false);

  winrt::Windows::Devices::Enumeration::DeviceWatcher m_watcher{ nullptr };
  std::atomic<bool> m_changed;
};
