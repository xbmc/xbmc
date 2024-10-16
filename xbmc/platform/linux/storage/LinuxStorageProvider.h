/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "storage/IStorageProvider.h"

#include <vector>

class CLinuxStorageProvider : public IStorageProvider
{
public:
  CLinuxStorageProvider();
  ~CLinuxStorageProvider() override;

  void Initialize() override;
  void Stop() override;
  void GetLocalDrives(std::vector<CMediaSource>& localDrives) override;
  void GetRemovableDrives(std::vector<CMediaSource>& removableDrives) override;
  bool Eject(const std::string& mountpath) override;
  std::vector<std::string> GetDiskUsage() override;
  bool PumpDriveChangeEvents(IStorageEventsCallback *callback) override;

private:
  IStorageProvider *m_instance;
};
