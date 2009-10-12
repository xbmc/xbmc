#include "DeviceKitDisksProvider.h"
#include "Util.h"
#ifdef HAS_DBUS
#include "DBusUtil.h"

std::vector<CStdString> CDeviceKitDisksProvider::GetDiskUsage()
{
  return std::vector<CStdString>();
}

bool CDeviceKitDisksProvider::PumpDriveChangeEvents()
{
  return false;
}

bool CDeviceKitDisksProvider::HasDeviceKitDisks()
{
  return true;
}

bool CDeviceKitDisksProvider::IsAllowedType(const CStdString& type) const
{
 return (!type.Equals("swap") && type.length() > 0);
}

void CDeviceKitDisksProvider::HandleDisk(VECSOURCES& devices, const char *device, bool EnumerateRemovable)
{
  PropertyMap properties;

  if (CDBusUtil::GetBoolean("org.freedesktop.DeviceKit.Disks", device, "org.freedesktop.DeviceKit.Disks.Device", "DeviceIsPartition"))
  {
    CDBusUtil::GetAll(properties, "org.freedesktop.DeviceKit.Disks", device, "org.freedesktop.DeviceKit.Disks.Device");

    bool isRemovable  = CDBusUtil::GetBoolean("org.freedesktop.DeviceKit.Disks", properties["PartitionSlave"].c_str(), "org.freedesktop.DeviceKit.Disks.Device", "DeviceIsRemovable");
    bool isMounted    = strcmp(properties["DeviceIsMounted"].c_str(), "true") == 0;
    CStdString path   = properties["DeviceMountPaths"];

    CLog::Log(LOGDEBUG, "DeviceKit.Disks: Found %s disk %s with type %s. ID is %s", isRemovable ? "removable" : "local", isMounted ? path.c_str() : "unmounted", properties["IdType"].c_str(), properties["IdUuid"].c_str());

    if (properties["IdUuid"].length() > 0 && IsAllowedType(properties["IdType"]) && EnumerateRemovable == isRemovable && isMounted && !path.Equals("/"))
    {
      CMediaSource source;
      source.strPath = path.c_str();
      source.strName = CUtil::GetFileName(path.c_str());
      source.m_ignore = true;
      devices.push_back(source);

      CLog::Log(LOGDEBUG, "DeviceKit.Disks: Approved disk %s", properties["IdUuid"].c_str());
    }
  }
}

void CDeviceKitDisksProvider::EnumerateDisks(VECSOURCES& devices, bool EnumerateRemovable)
{
  CDBusMessage message("org.freedesktop.DeviceKit.Disks", "/org/freedesktop/DeviceKit/Disks", "org.freedesktop.DeviceKit.Disks", "EnumerateDevices");
  DBusMessage *reply = message.SendSystem();
  if (reply)
  {
    char** disks = NULL;
    int    length     = 0;
    
    if (dbus_message_get_args (reply, NULL, DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, &disks, &length, DBUS_TYPE_INVALID))
	  {
      for (int i = 0; i < length; i++)
        HandleDisk(devices, disks[i], EnumerateRemovable);

      dbus_free_string_array(disks);
    }
  }
}
#endif
