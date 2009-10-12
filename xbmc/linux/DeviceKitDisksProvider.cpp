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

void CDeviceKitDisksProvider::HandleDisk(VECSOURCES& devices, const char *device, bool EnumerateRemovable)
{
  PropertyMap properties;

  if (CDBusUtil::GetBoolean("org.freedesktop.DeviceKit.Disks", device, "org.freedesktop.DeviceKit.Disks.Device", "DeviceIsPartition"))
  {
    CDBusUtil::GetAll(properties, "org.freedesktop.DeviceKit.Disks", device, "org.freedesktop.DeviceKit.Disks.Device");

    bool isRemovable  = CDBusUtil::GetBoolean("org.freedesktop.DeviceKit.Disks", properties["PartitionSlave"].c_str(), "org.freedesktop.DeviceKit.Disks.Device", "DeviceIsRemovable");
    bool isMounted    = strcmp(properties["DeviceIsMounted"].c_str(), "true") == 0;

    printf("IsRemovable %s mount %s\n", isRemovable ? "true" : "false", properties["DeviceMountPaths"].c_str());

    if (strlen(properties["IdUuid"].c_str()) > 0 && strcmp(properties["IdType"].c_str(), "swap") && EnumerateRemovable == isRemovable && isMounted)
    {
      CMediaSource source;
      source.strPath = properties["DeviceMountPaths"].c_str();
      source.strName = CUtil::GetFileName(properties["DeviceMountPaths"].c_str());
      source.m_ignore = true;
      devices.push_back(source);
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
