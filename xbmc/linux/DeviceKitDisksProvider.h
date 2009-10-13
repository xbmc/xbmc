#pragma once
#include "IStorageProvider.h"
#ifdef HAS_DBUS
#include "DBusUtil.h"

class CDeviceKitDiskDevice
{
public:
  CDeviceKitDiskDevice(const char *DeviceKitUDI);

  void Update();

  bool Mount();
  bool UnMount();

  bool IsApproved();

  CMediaSource ToMediaShare();

  CStdString m_UDI, m_DeviceKitUDI, m_MountPath, m_FileSystem;
  bool m_isMounted, m_isMountedByUs, m_isRemovable, m_isPartition;
};

class CDeviceKitDisksProvider : public IStorageProvider
{
public:
  CDeviceKitDisksProvider();
  virtual ~CDeviceKitDisksProvider();

  virtual void GetLocalDrives(VECSOURCES &localDrives) { GetDisks(localDrives, false); }
  virtual void GetRemovableDrives(VECSOURCES &removableDrives) { GetDisks(removableDrives, true); }

  virtual std::vector<CStdString> GetDiskUsage();

  virtual bool PumpDriveChangeEvents();

  static bool HasDeviceKitDisks();
private:
  typedef std::map<CStdString, CDeviceKitDiskDevice *> DeviceMap;
  typedef std::pair<CStdString, CDeviceKitDiskDevice *> DevicePair;

  void DeviceAdded(const char *object);
  void DeviceRemoved(const char *object);
  void DeviceChanged(const char *object);

  std::vector<CStdString> EnumerateDisks();

  void GetDisks(VECSOURCES& devices, bool EnumerateRemovable);

  DeviceMap m_AvailableDevices;

  DBusConnection *m_connection;
  DBusError m_error;
};
#endif
