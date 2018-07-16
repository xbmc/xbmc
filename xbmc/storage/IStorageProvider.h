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

#include "MediaSource.h"
#ifdef HAS_DVD_DRIVE
#include "cdioSupport.h"
#endif

class IStorageEventsCallback
{
public:
  virtual ~IStorageEventsCallback() = default;

  virtual void OnStorageAdded(const std::string &label, const std::string &path) = 0;
  virtual void OnStorageSafelyRemoved(const std::string &label) = 0;
  virtual void OnStorageUnsafelyRemoved(const std::string &label) = 0;
};

class IStorageProvider
{
public:
  virtual ~IStorageProvider() = default;

  virtual void Initialize() = 0;
  virtual void Stop() = 0;

  virtual void GetLocalDrives(VECSOURCES &localDrives) = 0;
  virtual void GetRemovableDrives(VECSOURCES &removableDrives) = 0;
  virtual std::string GetFirstOpticalDeviceFileName()
  {
#ifdef HAS_DVD_DRIVE
    return std::string(MEDIA_DETECT::CLibcdio::GetInstance()->GetDeviceFileName());
#else
    return "";
#endif
  }

  virtual bool Eject(const std::string& mountpath) = 0;

  virtual std::vector<std::string> GetDiskUsage() = 0;

  virtual bool PumpDriveChangeEvents(IStorageEventsCallback *callback) = 0;

  /**\brief Called by media manager to create platform storage provider
  *
  * This method used to create platfrom specified storage provider
  */
  static IStorageProvider* CreateInstance();
};
