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
  virtual ~CIOSStorageProvider() {}

  virtual void Initialize() {}
  virtual void Stop() {}

  virtual void GetLocalDrives(VECSOURCES& localDrives);
  virtual void GetRemovableDrives(VECSOURCES& removableDrives) {}

  virtual std::vector<std::string> GetDiskUsage(void);

  virtual bool Eject(const std::string& mountpath) { return false; }

  virtual bool PumpDriveChangeEvents(IStorageEventsCallback* callback) { return false; }
};
