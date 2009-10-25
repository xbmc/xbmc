#include "DeviceKitDisksProvider.h"
#ifdef HAS_DBUS
#include "Util.h"
#include "AdvancedSettings.h"

#ifdef FEDORA_DEVICEKIT

#define PROPERTY_PARTITION_SLAVE "partition-slave"
#define PROPERTY_PARTITION_SIZE  "partition-size"

#define PROPERTY_ID_UUID  "id-uuid"
#define PROPERTY_ID_LABEL "id-label"
#define PROPERTY_ID_TYPE  "id-type"

#define PROPERTY_DEVICE_ISPARTITION "device-is-partition"
#define PROPERTY_DEVICE_MOUNTPATHS  "device-mount-paths"
#define PROPERTY_DEVICE_ISMOUNTED   "device-is-mounted"
#define PROPERTY_DEVICE_ISREMOVABLE "device-is-removable"

#else

#define PROPERTY_PARTITION_SLAVE "PartitionSlave"
#define PROPERTY_PARTITION_SIZE  "PartitionSize"

#define PROPERTY_ID_UUID  "IdUuid"
#define PROPERTY_ID_LABEL "IdLabel"
#define PROPERTY_ID_TYPE  "IdType"

#define PROPERTY_DEVICE_ISPARTITION "DeviceIsPartition"
#define PROPERTY_DEVICE_MOUNTPATHS  "DeviceMountPaths"
#define PROPERTY_DEVICE_ISMOUNTED   "DeviceIsMounted"
#define PROPERTY_DEVICE_ISREMOVABLE "DeviceIsRemovable"

#endif

CDeviceKitDiskDevice::CDeviceKitDiskDevice(const char *DeviceKitUDI)
{
  m_DeviceKitUDI = DeviceKitUDI;
  m_UDI = "";
  m_MountPath = "";
  m_FileSystem = "";
  m_isMounted = false;
  m_isMountedByUs = false;
  m_isRemovable = false;

  m_isPartition = CDBusUtil::GetBoolean("org.freedesktop.DeviceKit.Disks", m_DeviceKitUDI.c_str(), "org.freedesktop.DeviceKit.Disks.Device", PROPERTY_DEVICE_ISPARTITION);

  Update();
}

void CDeviceKitDiskDevice::Update()
{
  PropertyMap properties;

  if (m_isPartition)
  {
    CDBusUtil::GetAll(properties, "org.freedesktop.DeviceKit.Disks", m_DeviceKitUDI.c_str(), "org.freedesktop.DeviceKit.Disks.Device");

    m_UDI         = properties[PROPERTY_ID_UUID];
    m_Label       = properties[PROPERTY_ID_LABEL];
    m_FileSystem  = properties[PROPERTY_ID_TYPE];
    m_MountPath   = properties[PROPERTY_DEVICE_MOUNTPATHS];
    m_isMounted   = properties[PROPERTY_DEVICE_ISMOUNTED].Equals("true");
    m_isRemovable = CDBusUtil::GetBoolean("org.freedesktop.DeviceKit.Disks", properties[PROPERTY_PARTITION_SLAVE].c_str(), "org.freedesktop.DeviceKit.Disks.Device", PROPERTY_DEVICE_ISREMOVABLE);

    m_PartitionSizeGiB = (atol(properties[PROPERTY_PARTITION_SIZE].c_str()) / 1024.0 / 1024.0 / 1024.0);
  }
}

bool CDeviceKitDiskDevice::Mount()
{
  if (!m_isMounted && m_isRemovable && m_isPartition)
  {
    CLog::Log(LOGDEBUG, "DeviceKit.Disks: Mounting %s", m_DeviceKitUDI.c_str());
    CDBusMessage message("org.freedesktop.DeviceKit.Disks", m_DeviceKitUDI.c_str(), "org.freedesktop.DeviceKit.Disks.Device", "FilesystemMount");
    message.AppendArgument("");
    const char *array[1];// = {"sync"};
    array[0] = "sync";
    message.AppendArgument(array, 1);

    DBusMessage *reply = message.SendSystem();
    if (reply)
    {
      char *mountPoint;
      if (dbus_message_get_args (reply, NULL, DBUS_TYPE_STRING, &mountPoint, DBUS_TYPE_INVALID))
      {
        m_MountPath = mountPoint;
        CLog::Log(LOGDEBUG, "DeviceKit.Disks: Sucessfully mounted %s on %s", m_DeviceKitUDI.c_str(), mountPoint);
        m_isMountedByUs = m_isMounted = true;
      }
    }

    return m_isMounted;
  }

  return false;
}

bool CDeviceKitDiskDevice::UnMount()
{
  if (m_isMounted && m_isRemovable && m_isPartition)
  {
    CDBusMessage message("org.freedesktop.DeviceKit.Disks", m_DeviceKitUDI.c_str(), "org.freedesktop.DeviceKit.Disks.Device", "FilesystemUnmount");

    const char *array[0];
    message.AppendArgument(array, 0);

    DBusMessage *reply = message.SendSystem();
    if (reply)
      m_isMountedByUs = m_isMounted = false;

    return !m_isMounted;
  }

  return false;
}

CMediaSource CDeviceKitDiskDevice::ToMediaShare()
{
  CMediaSource source;
  source.strPath = m_MountPath.c_str();
  source.strName = CUtil::GetFileName(m_MountPath.c_str());
  source.m_iDriveType =  m_isRemovable ? CMediaSource::SOURCE_TYPE_REMOVABLE : CMediaSource::SOURCE_TYPE_LOCAL;
  source.m_ignore = true;
  return source;
}

bool CDeviceKitDiskDevice::IsApproved()
{
  return (m_isPartition && m_UDI.length() > 0 && (m_FileSystem.length() > 0 && !m_FileSystem.Equals("swap")) && m_isMounted && !m_MountPath.Equals("/"));
}

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
  DeviceMap::iterator itr;

  for (itr = m_AvailableDevices.begin(); itr != m_AvailableDevices.end(); ++itr)
    delete m_AvailableDevices[itr->first];

  m_AvailableDevices.clear();

  if (m_connection)
  {
    dbus_connection_unref(m_connection);
    m_connection = NULL;
  }

  dbus_error_free (&m_error);
}

void CDeviceKitDisksProvider::Initialize()
{
  CLog::Log(LOGDEBUG, "DeviceKit.Disks: Querying available devices");
  std::vector<CStdString> devices = EnumerateDisks();
  for (unsigned int i = 0; i < devices.size(); i++)
    DeviceAdded(devices[i].c_str(), NULL);
}

bool CDeviceKitDisksProvider::Eject(CStdString mountpath)
{
  DeviceMap::iterator itr;
  CStdString path(mountpath);
  CUtil::RemoveSlashAtEnd(path);

  for (itr = m_AvailableDevices.begin(); itr != m_AvailableDevices.end(); ++itr)
  {
    CDeviceKitDiskDevice *device = itr->second;
    if (device->m_MountPath.Equals(path))
      return device->UnMount();
  }

  return false;
}

std::vector<CStdString> CDeviceKitDisksProvider::GetDiskUsage()
{
  std::vector<CStdString> devices;
  DeviceMap::iterator itr;

  for(itr = m_AvailableDevices.begin(); itr != m_AvailableDevices.end(); ++itr)
  {
    CDeviceKitDiskDevice *device = itr->second;
    if (device->IsApproved())
    {
      CStdString str;
      str.Format("%s %lu GiB", device->m_MountPath.c_str(), device->m_PartitionSizeGiB);
      devices.push_back(str);
    }
  }

  return devices;
}

bool CDeviceKitDisksProvider::PumpDriveChangeEvents(IStorageEventsCallback *callback)
{
  bool result = false;
  if (m_connection)
  {
    dbus_connection_read_write(m_connection, 0);
    DBusMessage *msg = dbus_connection_pop_message(m_connection);

    if (msg)
    {
      char *object;
      if (dbus_message_get_args (msg, NULL, DBUS_TYPE_OBJECT_PATH, &object, DBUS_TYPE_INVALID))
      {
        result = true;
        if (dbus_message_is_signal(msg, "org.freedesktop.DeviceKit.Disks", "DeviceAdded"))
          DeviceAdded(object, callback);
        else if (dbus_message_is_signal(msg, "org.freedesktop.DeviceKit.Disks", "DeviceRemoved"))
          DeviceRemoved(object, callback);
        else if (dbus_message_is_signal(msg, "org.freedesktop.DeviceKit.Disks", "DeviceChanged"))
          DeviceChanged(object, callback);
      }
      dbus_message_unref(msg);
    }
  }
  return result;
}

bool CDeviceKitDisksProvider::HasDeviceKitDisks()
{
  bool hasDeviceKitDisks = false;
  CDBusMessage message("org.freedesktop.DeviceKit.Disks", "/org/freedesktop/DeviceKit/Disks", "org.freedesktop.DeviceKit.Disks", "EnumerateDevices");

  DBusError error;
  dbus_error_init (&error);
  DBusConnection *con = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

  message.Send(con, &error);

  if (!dbus_error_is_set(&error))
    hasDeviceKitDisks = true;
  else
    CLog::Log(LOGDEBUG, "DeviceKit.Disks: %s - %s", error.name, error.message);

  dbus_error_free (&error);
  dbus_connection_unref(con);

  return hasDeviceKitDisks;
}

void CDeviceKitDisksProvider::DeviceAdded(const char *object, IStorageEventsCallback *callback)
{
  CLog::Log(LOGDEBUG, "DeviceKit.Disks: DeviceAdded (%s)", object);

  if (m_AvailableDevices[object])
  {
    CLog::Log(LOGWARNING, "DeviceKit.Disks: Inconsistency found! DeviceAdded on an indexed disk");
    delete m_AvailableDevices[object];
  }

  CDeviceKitDiskDevice *device = new CDeviceKitDiskDevice(object);
  m_AvailableDevices[object] = device;

  if (g_advancedSettings.m_handleMounting)
    device->Mount();

  if (device->m_isMounted && callback)
    callback->OnStorageAdded(device->m_Label, device->m_MountPath);
}

void CDeviceKitDisksProvider::DeviceRemoved(const char *object, IStorageEventsCallback *callback)
{
  CLog::Log(LOGDEBUG, "DeviceKit.Disks: DeviceRemoved (%s)", object);

  CDeviceKitDiskDevice *device = m_AvailableDevices[object];
  if (device)
  {
    if (device->m_isMounted && callback)
      callback->OnStorageUnsafelyRemoved(device->m_Label);

    delete m_AvailableDevices[object];
    m_AvailableDevices.erase(object);
  }
}

void CDeviceKitDisksProvider::DeviceChanged(const char *object, IStorageEventsCallback *callback)
{
  CLog::Log(LOGDEBUG, "DeviceKit.Disks: DeviceChanged (%s)", object);

  CDeviceKitDiskDevice *device = m_AvailableDevices[object];
  if (device == NULL)
  {
    CLog::Log(LOGWARNING, "DeviceKit.Disks: Inconsistency found! DeviceChanged on an unindexed disk");
    DeviceAdded(object, callback);
  }
  else
  {
    bool mounted = device->m_isMounted;
    device->Update();
    if (!mounted && device->m_isMounted && callback)
      callback->OnStorageAdded(device->m_MountPath, device->m_Label);
    else if (mounted && !device->m_isMounted && callback)
      callback->OnStorageSafelyRemoved(device->m_Label);
  }
}

std::vector<CStdString> CDeviceKitDisksProvider::EnumerateDisks()
{
  std::vector<CStdString> devices;
  CDBusMessage message("org.freedesktop.DeviceKit.Disks", "/org/freedesktop/DeviceKit/Disks", "org.freedesktop.DeviceKit.Disks", "EnumerateDevices");
  DBusMessage *reply = message.SendSystem();
  if (reply)
  {
    char** disks  = NULL;
    int    length = 0;
    
    if (dbus_message_get_args (reply, NULL, DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, &disks, &length, DBUS_TYPE_INVALID))
    {
      for (int i = 0; i < length; i++)
        devices.push_back(disks[i]);

      dbus_free_string_array(disks);
    }
  }

  return devices;
}

void CDeviceKitDisksProvider::GetDisks(VECSOURCES& devices, bool EnumerateRemovable)
{
  DeviceMap::iterator itr;

  for (itr = m_AvailableDevices.begin(); itr != m_AvailableDevices.end(); ++itr)
  {
    CDeviceKitDiskDevice *device = itr->second;
    if (device && device->IsApproved() && device->m_isRemovable == EnumerateRemovable)
      devices.push_back(device->ToMediaShare());
  }
}
#endif
