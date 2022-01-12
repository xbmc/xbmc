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

namespace MEDIA_DETECT
{
/*! \brief Abstracts a generic storage device */
struct StorageDevice
{
  /*! Device name/label */
  std::string label;
  /*! Device mountpoint/path */
  std::string path;
  /*! If the device is optical */
  bool optical;
};
} // namespace MEDIA_DETECT

class IStorageEventsCallback
{
public:
  virtual ~IStorageEventsCallback() = default;

  /*! \brief Callback executed when a new storage device is added
    * @param device the storage device
    */
  virtual void OnStorageAdded(const MEDIA_DETECT::StorageDevice& device) = 0;

  /*! \brief Callback executed when a new storage device is safely removed
    * @param device the storage device
    */
  virtual void OnStorageSafelyRemoved(const MEDIA_DETECT::StorageDevice& device) = 0;

  /*! \brief Callback executed when a new storage device is unsafely removed
    * @param device the storage device
    */
  virtual void OnStorageUnsafelyRemoved(const MEDIA_DETECT::StorageDevice& device) = 0;
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
  * This method used to create platform specified storage provider
  */
  static IStorageProvider* CreateInstance();
};
