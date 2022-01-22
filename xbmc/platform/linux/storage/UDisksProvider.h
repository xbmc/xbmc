/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DBusUtil.h"
#include "storage/IStorageProvider.h"

#include <string>
#include <vector>

class CUDiskDevice
{
public:
  CUDiskDevice(const char *DeviceKitUDI);
  ~CUDiskDevice() = default;

  void Update();

  bool Mount();
  bool UnMount();

  /*! \brief Check if the device is approved/whitelisted
    * @return true if the device is approved/whitelisted, false otherwise
  */
  bool IsApproved() const;

  /*! \brief Get the storage type of this device
   * @return the storage type (e.g. OPTICAL) or UNKNOWN if
   * the type couldn't be detected
  */
  MEDIA_DETECT::STORAGE::Type GetStorageType() const;

  /*! \brief Check if the device is optical
    * @return true if the device is optical, false otherwise
  */
  bool IsOptical() const;

  /*! \brief Check if the device is mounted
    * @return true if the device is mounted, false otherwise
  */
  bool IsMounted() const;

  /*! \brief Check if the device is internal to the system
    * @return true if the device is internal to the system, false otherwise
  */
  bool IsSystemInternal() const;

  /*! \brief Get the device display name/label
    * @return the device display name/label
  */
  std::string GetDisplayName() const;

  /*! \brief Get the device mount point
    * @return the device mount point
  */
  std::string GetMountPoint() const;

  /*! \brief Get a representation of the device as a readable string
    * @return device as a string
  */
  std::string ToString() const;

  /*! \brief Get a representation of the device as a media share
    * @return the media share
  */
  CMediaSource ToMediaShare() const;

  /*! \brief Get a representation of the device as a storage device abstraction
    * @return the storage device abstraction of the device
  */
  MEDIA_DETECT::STORAGE::StorageDevice ToStorageDevice() const;

private:
  std::string m_UDI;
  std::string m_DeviceKitUDI;
  std::string m_MountPath;
  std::string m_FileSystem;
  std::string m_Label;
  bool m_isMounted;
  bool m_isMountedByUs;
  bool m_isRemovable;
  bool m_isPartition;
  bool m_isFileSystem;
  bool m_isSystemInternal;
  bool m_isOptical;
  int64_t m_PartitionSize;
};

class CUDisksProvider : public IStorageProvider
{
public:
  CUDisksProvider();
  ~CUDisksProvider() override;

  void Initialize() override;
  void Stop() override { }

  void GetLocalDrives(VECSOURCES &localDrives) override { GetDisks(localDrives, false); }
  void GetRemovableDrives(VECSOURCES &removableDrives) override { GetDisks(removableDrives, true); }

  bool Eject(const std::string& mountpath) override;

  std::vector<std::string> GetDiskUsage() override;

  bool PumpDriveChangeEvents(IStorageEventsCallback *callback) override;

  static bool HasUDisks();
private:
  typedef std::map<std::string, CUDiskDevice *> DeviceMap;
  typedef std::pair<std::string, CUDiskDevice *> DevicePair;

  void DeviceAdded(const char *object, IStorageEventsCallback *callback);
  void DeviceRemoved(const char *object, IStorageEventsCallback *callback);
  void DeviceChanged(const char *object, IStorageEventsCallback *callback);

  std::vector<std::string> EnumerateDisks();

  void GetDisks(VECSOURCES& devices, bool EnumerateRemovable);

  int m_DaemonVersion;

  DeviceMap m_AvailableDevices;

  CDBusConnection m_connection;
};
