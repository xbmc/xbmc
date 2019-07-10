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

  bool IsApproved();

  std::string toString();

  CMediaSource ToMediaShare();

  std::string m_UDI, m_DeviceKitUDI, m_MountPath, m_FileSystem, m_Label;
  bool m_isMounted, m_isMountedByUs, m_isRemovable, m_isPartition, m_isFileSystem, m_isSystemInternal, m_isOptical;
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
