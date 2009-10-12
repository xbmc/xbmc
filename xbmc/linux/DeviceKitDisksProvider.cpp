#include "DeviceKitDisksProvider.h"
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
  return false;
}

void CDeviceKitDisksProvider::HandleDisk(VECSOURCES& devices, const char *device, bool EnumerateRemovable)
{
  PropertyMap properties;

  if (CDBusUtil::GetBoolean("org.freedesktop.DeviceKit.Disks", device, "org.freedesktop.DeviceKit.Disks.Device", "DeviceIsPartition"))
  {
    CDBusUtil::GetAll(properties, "org.freedesktop.DeviceKit.Disks", device, "org.freedesktop.DeviceKit.Disks.Device");

//    CStdString parent = CDBusUtil::GetVariant("org.freedesktop.DeviceKit.Disks", device, "org.freedesktop.DeviceKit.Disks.Device", "PartitionSlave");
    bool isRemovable  = CDBusUtil::GetBoolean("org.freedesktop.DeviceKit.Disks", properties["PartitionSlave"].c_str(), "org.freedesktop.DeviceKit.Disks.Device", "DeviceIsRemovable");
    bool isMounted    = strcmp(properties["DeviceIsMounted"].c_str(), "true") == 0;

    if (strlen(properties["IdUuid"].c_str()) > 0 && strcmp(properties["IdType"].c_str(), "swap") && EnumerateRemovable == isRemovable)
    {
      CMediaSource source;
      //devices.push_back(DiskDevice(device, properties["IdUuid"].c_str(), properties["DeviceMountPaths"].c_str(), isMounted, isRemovable));
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
    
    if (dbus_message_get_args (reply, NULL,
				DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, &disks, &length, DBUS_TYPE_INVALID))
	  {


      for (int i = 0; i < length; i++)
        HandleDisk(devices, disks[i], EnumerateRemovable);

      dbus_free_string_array(disks);
    }
  }
}
#endif
