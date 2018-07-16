/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <set>
#include <string>
#include <vector>

#include "storage/IStorageProvider.h"

class CAndroidStorageProvider : public IStorageProvider
{
public:
  CAndroidStorageProvider();
  virtual ~CAndroidStorageProvider() { }

  virtual void Initialize() { }
  virtual void Stop() { }
  virtual bool Eject(const std::string& mountpath) { return false; }

  virtual void GetLocalDrives(VECSOURCES &localDrives);
  virtual void GetRemovableDrives(VECSOURCES &removableDrives);
  virtual std::vector<std::string> GetDiskUsage();

  virtual bool PumpDriveChangeEvents(IStorageEventsCallback *callback);

private:
  std::string unescape(const std::string& str);
  VECSOURCES m_removableDrives;
  unsigned int m_removableLength;

  static std::set<std::string> GetRemovableDrives();
  static std::set<std::string> GetRemovableDrivesLinux();
};
