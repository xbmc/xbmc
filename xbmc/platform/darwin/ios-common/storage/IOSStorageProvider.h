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
#include <vector>

class CIOSStorageProvider : public IStorageProvider
{
public:
  CIOSStorageProvider() {}
  ~CIOSStorageProvider() override {}

  void Initialize() override {}
  void Stop() override {}

  void GetLocalDrives(VECSOURCES& localDrives) override;
  void GetRemovableDrives(VECSOURCES& removableDrives) override {}

  std::vector<std::string> GetDiskUsage(void) override;

  bool Eject(const std::string& mountpath) override { return false; }

  bool PumpDriveChangeEvents(IStorageEventsCallback* callback) override { return false; }
};
