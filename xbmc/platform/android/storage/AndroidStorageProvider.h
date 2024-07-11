/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "storage/IStorageProvider.h"

#include <set>
#include <string>
#include <vector>

class CAndroidStorageProvider : public IStorageProvider
{
public:
  CAndroidStorageProvider();
  ~CAndroidStorageProvider() override = default;

  void Initialize() override {}
  void Stop() override {}
  bool Eject(const std::string& mountpath) override { return false; }

  void GetLocalDrives(VECSOURCES& localDrives) override;
  void GetRemovableDrives(VECSOURCES& removableDrives) override;
  std::vector<std::string> GetDiskUsage() override;

  bool PumpDriveChangeEvents(IStorageEventsCallback* callback) override;

private:
  std::string unescape(const std::string& str);
  VECSOURCES m_removableDrives;
  unsigned int m_removableLength;

  static std::set<std::string> GetRemovableDrives();
  static std::set<std::string> GetRemovableDrivesLinux();

  bool GetStorageUsage(const std::string& path, std::string& usage);
};
