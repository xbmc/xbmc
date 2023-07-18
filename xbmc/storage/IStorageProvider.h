/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "MediaSource.h"

#include <memory>
#include <string>
#include <vector>
#ifdef HAS_OPTICAL_DRIVE
#include "cdioSupport.h"
#endif

namespace MEDIA_DETECT
{
namespace STORAGE
{
/*! \brief Abstracts a generic storage device type*/
enum class Type
{
  UNKNOWN, /*!< the storage type is unknown */
  OPTICAL /*!< an optical device (e.g. DVD or Bluray) */
};

/*! \brief Abstracts a generic storage device */
struct StorageDevice
{
  /*! Device name/label */
  std::string label{};
  /*! Device mountpoint/path */
  std::string path{};
  /*! The storage type (e.g. OPTICAL) */
  STORAGE::Type type{STORAGE::Type::UNKNOWN};
};
} // namespace STORAGE
} // namespace MEDIA_DETECT

class IStorageEventsCallback
{
public:
  virtual ~IStorageEventsCallback() = default;

  /*! \brief Callback executed when a new storage device is added
    * @param device the storage device
    */
  virtual void OnStorageAdded(const MEDIA_DETECT::STORAGE::StorageDevice& device) = 0;

  /*! \brief Callback executed when a new storage device is safely removed
    * @param device the storage device
    */
  virtual void OnStorageSafelyRemoved(const MEDIA_DETECT::STORAGE::StorageDevice& device) = 0;

  /*! \brief Callback executed when a new storage device is unsafely removed
    * @param device the storage device
    */
  virtual void OnStorageUnsafelyRemoved(const MEDIA_DETECT::STORAGE::StorageDevice& device) = 0;
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
#ifdef HAS_OPTICAL_DRIVE
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
  static std::unique_ptr<IStorageProvider> CreateInstance();
};
