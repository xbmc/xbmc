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

class CDarwinStorageProvider : public IStorageProvider
{
public:
  CDarwinStorageProvider();
  virtual ~CDarwinStorageProvider() { }

  virtual void Initialize() { }
  virtual void Stop() { }

  virtual void GetLocalDrives(VECSOURCES &localDrives);
  virtual void GetRemovableDrives(VECSOURCES &removableDrives);

  virtual std::vector<std::string> GetDiskUsage(void);

  virtual bool Eject(const std::string& mountpath);

  virtual bool PumpDriveChangeEvents(IStorageEventsCallback *callback);

  static void SetEvent(void);

private:
  static bool m_event;
};
