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

  /*!
   * \brief Start making the media already in the drives at startup available as sources.
   *
   * Reading a disc spins the drive up and opens it with two libraries in turn, so a platform
   * that answers here does the reading in the background and the sources appear once it has.
   * Platforms whose media detection runs on a thread already have nothing to do here.
   */
  virtual void ScanForPresentMedia() {}

  /*!
   * \brief Wait for any scan started by ScanForPresentMedia() to finish.
   *
   * Called before the services the scan uses (GUI, localized strings, disc caches) go away.
   */
  virtual void StopScanForPresentMedia() {}

  virtual void GetLocalDrives(std::vector<CMediaSource>& localDrives) = 0;
  virtual void GetRemovableDrives(std::vector<CMediaSource>& removableDrives) = 0;
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
