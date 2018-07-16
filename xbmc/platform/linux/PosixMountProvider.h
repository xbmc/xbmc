/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

#include "storage/IStorageProvider.h"

class CPosixMountProvider : public IStorageProvider
{
public:
  CPosixMountProvider();
  ~CPosixMountProvider() override = default;

  void Initialize() override;
  void Stop() override { }

  void GetLocalDrives(VECSOURCES &localDrives) override { GetDrives(localDrives); }
  void GetRemovableDrives(VECSOURCES &removableDrives) override { /*GetDrives(removableDrives);*/ }

  std::vector<std::string> GetDiskUsage() override;

  bool Eject(const std::string& mountpath) override;

  bool PumpDriveChangeEvents(IStorageEventsCallback *callback) override;
private:
  void GetDrives(VECSOURCES &drives);

  unsigned int m_removableLength;
};
