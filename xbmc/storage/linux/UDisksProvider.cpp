/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "UDisksProvider.h"
#ifdef HAS_DBUS
#include "settings/AdvancedSettings.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "PosixMountProvider.h"

CUDiskDevice::CUDiskDevice(const char *DeviceKitUDI):
  m_DeviceKitUDI(DeviceKitUDI)
{
  m_isMounted = false;
  m_isMountedByUs = false;
  m_isRemovable = false;
  m_isPartition = false;
  m_isFileSystem = false;
  m_isSystemInternal = false;
  m_isOptical = false;
  m_PartitionSize = 0;
  Update();
}

void CUDiskDevice::Update()
{
  CVariant properties = CDBusUtil::GetAll("org.freedesktop.UDisks", m_DeviceKitUDI.c_str(), "org.freedesktop.UDisks.Device");

  m_isFileSystem = properties["IdUsage"].asString() == "filesystem";
  if (m_isFileSystem)
  {
    m_UDI         = properties["IdUuid"].asString();
    m_Label       = properties["IdLabel"].asString();
    m_FileSystem  = properties["IdType"].asString();
  }
  else
  {
    m_UDI.clear();
    m_Label.clear();
    m_FileSystem.clear();
  }

  m_isMounted   = properties["DeviceIsMounted"].asBoolean();
  if (m_isMounted && !properties["DeviceMountPaths"].empty())
    m_MountPath   = properties["DeviceMountPaths"][0].asString();
  else
    m_MountPath.clear();

  m_PartitionSize = properties["PartitionSize"].asUnsignedInteger();
  m_isPartition = properties["DeviceIsPartition"].asBoolean();
  m_isSystemInternal = properties["DeviceIsSystemInternal"].asBoolean();
  m_isOptical = properties["DeviceIsOpticalDisc"].asBoolean();
  if (m_isPartition)
  {
    CVariant isRemovable = CDBusUtil::GetVariant("org.freedesktop.UDisks", properties["PartitionSlave"].asString().c_str(), "org.freedesktop.UDisks.Device", "DeviceIsRemovable");

    if ( !isRemovable.isNull() )
      m_isRemovable = isRemovable.asBoolean();
    else
      m_isRemovable = false;
  }
  else
    m_isRemovable = properties["DeviceIsRemovable"].asBoolean();
}

bool CUDiskDevice::Mount()
{
  if (!m_isMounted && !m_isSystemInternal && m_isFileSystem)
  {
    CLog::Log(LOGDEBUG, "UDisks: Mounting %s", m_DeviceKitUDI.c_str());
    CDBusMessage message("org.freedesktop.UDisks", m_DeviceKitUDI.c_str(), "org.freedesktop.UDisks.Device", "FilesystemMount");
    message.AppendArgument("");
    const char *array[] = {};
    message.AppendArgument(array, 0);

    DBusMessage *reply = message.SendSystem();
    if (reply)
    {
      char *mountPoint;
      if (dbus_message_get_args (reply, NULL, DBUS_TYPE_STRING, &mountPoint, DBUS_TYPE_INVALID))
      {
        m_MountPath = mountPoint;
        CLog::Log(LOGDEBUG, "UDisks: Successfully mounted %s on %s", m_DeviceKitUDI.c_str(), mountPoint);
        m_isMountedByUs = m_isMounted = true;
      }
    }

    return m_isMounted;
  }
  else
    CLog::Log(LOGDEBUG, "UDisks: Is not able to mount %s", toString().c_str());

  return false;
}

bool CUDiskDevice::UnMount()
{
  if (m_isMounted && !m_isSystemInternal && m_isFileSystem)
  {
    CDBusMessage message("org.freedesktop.UDisks", m_DeviceKitUDI.c_str(), "org.freedesktop.UDisks.Device", "FilesystemUnmount");

    const char *array[1];
    message.AppendArgument(array, 0);

    DBusMessage *reply = message.SendSystem();
    if (reply)
      m_isMountedByUs = m_isMounted = false;

    return !m_isMounted;
  }
  else
    CLog::Log(LOGDEBUG, "UDisks: Is not able to unmount %s", toString().c_str());

  return false;
}

CMediaSource CUDiskDevice::ToMediaShare()
{
  CMediaSource source;
  source.strPath = m_MountPath;
  if (m_Label.empty())
  {
    std::string strSize = StringUtils::SizeToString(m_PartitionSize);
    source.strName = StringUtils::Format("%s %s", strSize.c_str(), g_localizeStrings.Get(155).c_str());
  }
  else
    source.strName = m_Label;
  if (m_isOptical)
    source.m_iDriveType = CMediaSource::SOURCE_TYPE_DVD;
  else if (m_isSystemInternal)
    source.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  else
    source.m_iDriveType = CMediaSource::SOURCE_TYPE_REMOVABLE;
  source.m_ignore = true;
  return source;
}

bool CUDiskDevice::IsApproved()
{
  return (m_isFileSystem && m_isMounted && m_UDI.length() > 0 && (m_FileSystem.length() > 0 && m_FileSystem != "swap") 
      && m_MountPath != "/" && m_MountPath != "/boot") || m_isOptical;
}

#define BOOL2SZ(b) ((b) ? "true" : "false")

std::string CUDiskDevice::toString()
{
  return StringUtils::Format("DeviceUDI %s: IsFileSystem %s HasFileSystem %s "
      "IsSystemInternal %s IsMounted %s IsRemovable %s IsPartition %s "
      "IsOptical %s",
      m_DeviceKitUDI.c_str(), BOOL2SZ(m_isFileSystem), m_FileSystem.c_str(),
      BOOL2SZ(m_isSystemInternal), BOOL2SZ(m_isMounted),
      BOOL2SZ(m_isRemovable), BOOL2SZ(m_isPartition), BOOL2SZ(m_isOptical));
}

CUDisksProvider::CUDisksProvider()
{
  dbus_error_init (&m_error);
  //! @todo do not use dbus_connection_pop_message() that requires the use of a
  //! private connection
  m_connection = dbus_bus_get_private(DBUS_BUS_SYSTEM, &m_error);

  if (m_connection)
  {
    dbus_connection_set_exit_on_disconnect(m_connection, false);

    dbus_bus_add_match(m_connection, "type='signal',interface='org.freedesktop.UDisks'", &m_error);
    dbus_connection_flush(m_connection);
  }

  if (dbus_error_is_set(&m_error))
  {
    CLog::Log(LOGERROR, "UDisks: Failed to attach to signal %s", m_error.message);
    dbus_connection_close(m_connection);
    dbus_connection_unref(m_connection);
    m_connection = NULL;
  }
}

CUDisksProvider::~CUDisksProvider()
{
  DeviceMap::iterator itr;

  for (itr = m_AvailableDevices.begin(); itr != m_AvailableDevices.end(); ++itr)
    delete m_AvailableDevices[itr->first];

  m_AvailableDevices.clear();

  if (m_connection)
  {
    dbus_connection_close(m_connection);
    dbus_connection_unref(m_connection);
    m_connection = NULL;
  }

  dbus_error_free (&m_error);
}

void CUDisksProvider::Initialize()
{
  CLog::Log(LOGDEBUG, "Selected UDisks as storage provider");
  m_DaemonVersion = atoi(CDBusUtil::GetVariant("org.freedesktop.UDisks", "/org/freedesktop/UDisks", "org.freedesktop.UDisks", "DaemonVersion").asString().c_str());
  CLog::Log(LOGDEBUG, "UDisks: DaemonVersion %i", m_DaemonVersion);

  CLog::Log(LOGDEBUG, "UDisks: Querying available devices");
  std::vector<std::string> devices = EnumerateDisks();
  for (unsigned int i = 0; i < devices.size(); i++)
    DeviceAdded(devices[i].c_str(), NULL);
}

bool CUDisksProvider::Eject(const std::string& mountpath)
{
  DeviceMap::iterator itr;
  std::string path(mountpath);
  URIUtils::RemoveSlashAtEnd(path);

  for (itr = m_AvailableDevices.begin(); itr != m_AvailableDevices.end(); ++itr)
  {
    CUDiskDevice *device = itr->second;
    if (device->m_MountPath == path)
      return device->UnMount();
  }

  return false;
}

std::vector<std::string> CUDisksProvider::GetDiskUsage()
{
  CPosixMountProvider legacy;
  return legacy.GetDiskUsage();
}

bool CUDisksProvider::PumpDriveChangeEvents(IStorageEventsCallback *callback)
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
        if (dbus_message_is_signal(msg, "org.freedesktop.UDisks", "DeviceAdded"))
          DeviceAdded(object, callback);
        else if (dbus_message_is_signal(msg, "org.freedesktop.UDisks", "DeviceRemoved"))
          DeviceRemoved(object, callback);
        else if (dbus_message_is_signal(msg, "org.freedesktop.UDisks", "DeviceChanged"))
          DeviceChanged(object, callback);
      }
      dbus_message_unref(msg);
    }
  }
  return result;
}

bool CUDisksProvider::HasUDisks()
{
  bool hasUDisks = false;
  CDBusMessage message("org.freedesktop.UDisks", "/org/freedesktop/UDisks", "org.freedesktop.UDisks", "EnumerateDevices");

  DBusError error;
  dbus_error_init (&error);
  DBusConnection *con = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

  if (con)
    message.Send(con, &error);

  if (!dbus_error_is_set(&error))
    hasUDisks = true;
  else
    CLog::Log(LOGDEBUG, "UDisks: %s - %s", error.name, error.message);

  dbus_error_free (&error);
  if (con)
    dbus_connection_unref(con);

  return hasUDisks;
}

void CUDisksProvider::DeviceAdded(const char *object, IStorageEventsCallback *callback)
{
  if (g_advancedSettings.CanLogComponent(LOGDBUS))
    CLog::Log(LOGDEBUG, "UDisks: DeviceAdded (%s)", object);

  if (m_AvailableDevices[object])
  {
    CLog::Log(LOGWARNING, "UDisks: Inconsistency found! DeviceAdded on an indexed disk");
    delete m_AvailableDevices[object];
  }

  CUDiskDevice *device = NULL;
    device = new CUDiskDevice(object);
  m_AvailableDevices[object] = device;

  if (g_advancedSettings.m_handleMounting)
    device->Mount();

  if (g_advancedSettings.CanLogComponent(LOGDBUS))
    CLog::Log(LOGDEBUG, "UDisks: DeviceAdded - %s", device->toString().c_str());

  if (device->m_isMounted && device->IsApproved())
  {
    CLog::Log(LOGINFO, "UDisks: Added %s", device->m_MountPath.c_str());
    if (callback)
      callback->OnStorageAdded(device->m_Label, device->m_MountPath);
  }
}

void CUDisksProvider::DeviceRemoved(const char *object, IStorageEventsCallback *callback)
{
  if (g_advancedSettings.CanLogComponent(LOGDBUS))
    CLog::Log(LOGDEBUG, "UDisks: DeviceRemoved (%s)", object);

  CUDiskDevice *device = m_AvailableDevices[object];
  if (device)
  {
    if (device->m_isMounted && callback)
      callback->OnStorageUnsafelyRemoved(device->m_Label);

    delete m_AvailableDevices[object];
    m_AvailableDevices.erase(object);
  }
}

void CUDisksProvider::DeviceChanged(const char *object, IStorageEventsCallback *callback)
{
  if (g_advancedSettings.CanLogComponent(LOGDBUS))
    CLog::Log(LOGDEBUG, "UDisks: DeviceChanged (%s)", object);

  CUDiskDevice *device = m_AvailableDevices[object];
  if (device == NULL)
  {
    CLog::Log(LOGWARNING, "UDisks: Inconsistency found! DeviceChanged on an unindexed disk");
    DeviceAdded(object, callback);
  }
  else
  {
    bool mounted = device->m_isMounted;
    /* make sure to not silently remount ejected usb thumb drives
       that user wants to eject, but make sure to mount blurays */
    if (!mounted && g_advancedSettings.m_handleMounting && device->m_isOptical)
      device->Mount();

    device->Update();
    if (!mounted && device->m_isMounted && callback)
      callback->OnStorageAdded(device->m_Label, device->m_MountPath);
    else if (mounted && !device->m_isMounted && callback)
      callback->OnStorageSafelyRemoved(device->m_Label);

    if (g_advancedSettings.CanLogComponent(LOGDBUS))
      CLog::Log(LOGDEBUG, "UDisks: DeviceChanged - %s", device->toString().c_str());
  }
}

std::vector<std::string> CUDisksProvider::EnumerateDisks()
{
  std::vector<std::string> devices;
  CDBusMessage message("org.freedesktop.UDisks", "/org/freedesktop/UDisks", "org.freedesktop.UDisks", "EnumerateDevices");
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

void CUDisksProvider::GetDisks(VECSOURCES& devices, bool EnumerateRemovable)
{
  DeviceMap::iterator itr;

  for (itr = m_AvailableDevices.begin(); itr != m_AvailableDevices.end(); ++itr)
  {
    CUDiskDevice *device = itr->second;
    if (device && device->IsApproved() && device->m_isSystemInternal != EnumerateRemovable)
      devices.push_back(device->ToMediaShare());
  }
}
#endif
