#include "DeviceKitDisksProvider.h"
#include "Util.h"
#ifdef HAS_DBUS

CDeviceKitDisksProvider::CDeviceKitDisksProvider()
{
  dbus_error_init (&m_error);
  m_connection = dbus_bus_get(DBUS_BUS_SYSTEM, &m_error);

  dbus_bus_add_match(m_connection, "type='signal',interface='org.freedesktop.DeviceKit.Disks'", &m_error);
  dbus_connection_flush(m_connection);
  if (dbus_error_is_set(&m_error))
  {
    CLog::Log(LOGERROR, "DeviceKit.Disks: Failed to attach to signal %s", m_error.message);
    dbus_connection_unref(m_connection);
    m_connection = NULL;
  }
}

CDeviceKitDisksProvider::~CDeviceKitDisksProvider()
{
  if (m_connection)
  {
    dbus_connection_unref(m_connection);
    m_connection = NULL;
  }

  dbus_error_free (&m_error);
}

std::vector<CStdString> CDeviceKitDisksProvider::GetDiskUsage()
{
  return std::vector<CStdString>();
}

bool CDeviceKitDisksProvider::PumpDriveChangeEvents()
{
  if (m_connection)
  {
    dbus_connection_read_write(m_connection, 0);
    DBusMessage *msg = dbus_connection_pop_message(m_connection);

    if (msg)
    {
      if (dbus_message_is_signal(msg, "org.freedesktop.DeviceKit.Disks", "DeviceAdded"))
      {
        CLog::Log(LOGDEBUG, "DeviceKit.Disks: Got \"DeviceAdded\"-signal");
        return true;
      }
      if (dbus_message_is_signal(msg, "org.freedesktop.DeviceKit.Disks", "DeviceRemoved"))
      {
        CLog::Log(LOGDEBUG, "DeviceKit.Disks: Got \"DeviceRemoved\"-signal");
        return true;
      }
      if (dbus_message_is_signal(msg, "org.freedesktop.DeviceKit.Disks", "DeviceChanged"))
      {
        CLog::Log(LOGDEBUG, "DeviceKit.Disks: Got \"DeviceChanged\"-signal");
        return true;
      }
    }
  }
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
