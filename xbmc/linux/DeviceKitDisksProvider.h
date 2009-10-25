#pragma once
#include "IStorageProvider.h"
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

  CMediaSource ToMediaShare();

  CStdString m_UDI, m_DeviceKitUDI, m_MountPath, m_FileSystem, m_Label;
  bool m_isMounted, m_isMountedByUs, m_isRemovable, m_isPartition;
  long int m_PartitionSizeGiB;
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
