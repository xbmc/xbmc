/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "storage/IStorageProvider.h"

#include <string>
#include <utility>
#include <vector>

class COSXStorageProvider : public IStorageProvider
{
public:
  COSXStorageProvider();
  ~COSXStorageProvider() override = default;

  void Initialize() override {}
  void Stop() override {}

  void GetLocalDrives(VECSOURCES& localDrives) override;
  void GetRemovableDrives(VECSOURCES& removableDrives) override;

  std::vector<std::string> GetDiskUsage() override;

  bool Eject(const std::string& mountpath) override;

  bool PumpDriveChangeEvents(IStorageEventsCallback* callback) override;

  static void VolumeMountNotification(const char* label, const char* mountpoint);
  static void VolumeUnmountNotification(const char* label, const char* mountpoint);

private:
  static std::vector<std::pair<std::string, std::string>> m_mountsToNotify; // label, mountpoint
  static std::vector<std::pair<std::string, std::string>> m_unmountsToNotify; // label, mountpoint
};
