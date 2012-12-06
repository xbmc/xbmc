#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "storage/IStorageProvider.h"
#ifdef HAS_DBUS
#include "DBusUtil.h"

class CDeviceKitDiskDevice
{
public:
  CDeviceKitDiskDevice(const char *DeviceKitUDI);
  virtual ~CDeviceKitDiskDevice() { }

  virtual void Update() = 0;

  bool Mount();
  bool UnMount();

  bool IsApproved();

  CStdString toString();

  CMediaSource ToMediaShare();

  CStdString m_UDI, m_DeviceKitUDI, m_MountPath, m_FileSystem, m_Label;
  bool m_isMounted, m_isMountedByUs, m_isRemovable, m_isPartition, m_isFileSystem, m_isSystemInternal, m_isOptical;
  float m_PartitionSizeGiB;
};

class CDeviceKitDiskDeviceNewAPI : public CDeviceKitDiskDevice
{
public:
  CDeviceKitDiskDeviceNewAPI(const char *DeviceKitUDI) : CDeviceKitDiskDevice(DeviceKitUDI) { Update(); }
  virtual ~CDeviceKitDiskDeviceNewAPI() { }

  virtual void Update();
};

class CDeviceKitDiskDeviceOldAPI : public CDeviceKitDiskDevice
{
public:
  CDeviceKitDiskDeviceOldAPI(const char *DeviceKitUDI) : CDeviceKitDiskDevice(DeviceKitUDI) { Update(); }
  virtual ~CDeviceKitDiskDeviceOldAPI() { }

  virtual void Update();
};

class CDeviceKitDisksProvider : public IStorageProvider
{
public:
  CDeviceKitDisksProvider();
  virtual ~CDeviceKitDisksProvider();

  virtual void Initialize();
  virtual void Stop() { }

  virtual void GetLocalDrives(VECSOURCES &localDrives) { GetDisks(localDrives, false); }
  virtual void GetRemovableDrives(VECSOURCES &removableDrives) { GetDisks(removableDrives, true); }

  virtual bool Eject(CStdString mountpath);

  virtual std::vector<CStdString> GetDiskUsage();

  virtual bool PumpDriveChangeEvents(IStorageEventsCallback *callback);

  static bool HasDeviceKitDisks();
private:
  typedef std::map<CStdString, CDeviceKitDiskDevice *> DeviceMap;
  typedef std::pair<CStdString, CDeviceKitDiskDevice *> DevicePair;

  void DeviceAdded(const char *object, IStorageEventsCallback *callback);
  void DeviceRemoved(const char *object, IStorageEventsCallback *callback);
  void DeviceChanged(const char *object, IStorageEventsCallback *callback);

  std::vector<CStdString> EnumerateDisks();

  void GetDisks(VECSOURCES& devices, bool EnumerateRemovable);

  int m_DaemonVersion;

  DeviceMap m_AvailableDevices;

  DBusConnection *m_connection;
  DBusError m_error;
};
#endif
