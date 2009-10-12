#pragma once
#include "IStorageProvider.h"
#ifdef HAS_DBUS
#include "DBusUtil.h"

class CDeviceKitDisksProvider : public IStorageProvider
{
public:
  CDeviceKitDisksProvider();
  virtual ~CDeviceKitDisksProvider();

  virtual void GetLocalDrives(VECSOURCES &localDrives) { EnumerateDisks(localDrives, false); }
  virtual void GetRemovableDrives(VECSOURCES &removableDrives) { EnumerateDisks(removableDrives, true); }

  virtual std::vector<CStdString> GetDiskUsage();

  virtual bool PumpDriveChangeEvents();

  static bool HasDeviceKitDisks();
private:
  bool IsAllowedType(const CStdString& type) const;

  void HandleDisk(VECSOURCES& devices, const char *device, bool EnumerateRemovable);
  void EnumerateDisks(VECSOURCES& devices, bool EnumerateRemovable);

  DBusConnection *m_connection;
  DBusError m_error;
};
#endif
