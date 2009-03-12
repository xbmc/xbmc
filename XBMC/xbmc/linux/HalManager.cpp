/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "../../guilib/system.h"
#ifdef HAS_HAL
#include "HalManager.h"
#include "Application.h"
#include <libhal-storage.h>
#include "LinuxFileSystem.h"
#include "SingleLock.h"
#include "../Util.h"
#include "../../guilib/LocalizeStrings.h"
//#define HAL_HANDLEMOUNT

bool CHalManager::NewMessage;
DBusError CHalManager::m_Error;
CCriticalSection CHalManager::m_lock;

/* A Removed device, It isn't possible to make a LibHalVolume from a removed device therefor
   we catch the UUID from the udi on the removal */
void CHalManager::DeviceRemoved(LibHalContext *ctx, const char *udi)
{
  NewMessage = true;
  CLog::Log(LOGDEBUG, "HAL: Device (%s) Removed", udi);
  g_HalManager.RemoveDevice(udi);
}

void CHalManager::DeviceNewCapability(LibHalContext *ctx, const char *udi, const char *capability)
{
  NewMessage = true;
  CLog::Log(LOGDEBUG, "HAL: Device (%s) gained capability %s", udi, capability);
  g_HalManager.ParseDevice(udi);
}

void CHalManager::DeviceLostCapability(LibHalContext *ctx, const char *udi, const char *capability)
{
  NewMessage = true;
  CLog::Log(LOGDEBUG, "HAL: Device (%s) lost capability %s", udi, capability);
  g_HalManager.ParseDevice(udi);
}

/* HAL Property modified callback. If a device is mounted. This is called. */
void CHalManager::DevicePropertyModified(LibHalContext *ctx, const char *udi, const char *key, dbus_bool_t is_removed, dbus_bool_t is_added)
{
  NewMessage = true;
  CLog::Log(LOGDEBUG, "HAL: Device (%s) Property %s modified", udi, key);
  g_HalManager.ParseDevice(udi);
}

void CHalManager::DeviceCondition(LibHalContext *ctx, const char *udi, const char *condition_name, const char *condition_details)
{
  CLog::Log(LOGDEBUG, "HAL: Device (%s) Condition %s | %s", udi, condition_name, condition_details);
  if (!strcmp(condition_name, "ButtonPressed") && !strcmp(condition_details, "power"))
  {
    CUtil::ExecBuiltIn("XBMC.ActivateWindow(ShutdownMenu)");
    return;
  }
  NewMessage = true;
  g_HalManager.ParseDevice(udi);
}

/* HAL Device added. This is before mount. And here is the place to mount the volume in the future */
void CHalManager::DeviceAdded(LibHalContext *ctx, const char *udi)
{
  NewMessage = true;
  CLog::Log(LOGDEBUG, "HAL: Device (%s) Added", udi);
  g_HalManager.ParseDevice(udi);
}

CHalManager g_HalManager;

/* Iterate through all devices currently on the computer. Needed mostly at startup */
void CHalManager::GenerateGDL()
{
  if (m_Context == NULL)
    return;

  char **GDL;
  int i = 0;
  CLog::Log(LOGDEBUG, "HAL: Clearing old global device list, if any");
  m_Volumes.clear();

  CLog::Log(LOGNOTICE, "HAL: Generating global device list");
  GDL = libhal_get_all_devices(g_HalManager.m_Context, &i, &m_Error);

  for (i = 0; GDL[i]; i++)
  {
    ParseDevice(GDL[i]);
  }
  CLog::Log(LOGINFO, "HAL: Generated global device list, found %i", i);

  libhal_free_string_array(GDL);
}

// Return all volumes that currently are available (Mostly needed at startup, the rest of the volumes comes as events.)
std::vector<CStorageDevice> CHalManager::GetVolumeDevices()
{
  CSingleLock lock(m_lock);
  return m_Volumes;
}

CHalManager::CHalManager()
{
  m_Notifications = false;
  m_Context = NULL;
  m_DBusSystemConnection = NULL;
}

// Shutdown the connection and free the context
CHalManager::~CHalManager()
{
  if (g_advancedSettings.m_useHalMount)
  {
// Unmount all media XBMC have mounted
    for (unsigned int i = 0; i < m_Volumes.size(); i++)
    {
      if (m_Volumes[i].MountedByXBMC && m_Volumes[i].Mounted)
      {
        CLog::Log(LOGNOTICE, "HAL: Unmounts %s", m_Volumes[i].FriendlyName.c_str());
        UnMount(m_Volumes[i]);
      }
    }
  }

  if (m_Context != NULL)
    libhal_ctx_shutdown(m_Context, NULL);
  if (m_Context != NULL)
    libhal_ctx_free(m_Context);

  if (m_DBusSystemConnection != NULL)
  {
    dbus_connection_unref(m_DBusSystemConnection);
    m_DBusSystemConnection = NULL;
  }
  dbus_error_free(&m_Error); // Needed?
}

// Initialize
void CHalManager::Initialize()
{
  m_Notifications = false;
  CLog::Log(LOGINFO, "HAL: Starting initializing");
  g_HalManager.m_Context = g_HalManager.InitializeHal();
  if (g_HalManager.m_Context == NULL)
  {
    CLog::Log(LOGERROR, "HAL: no Hal context");
    return;
  }

  GenerateGDL();

  CLog::Log(LOGINFO, "HAL: Sucessfully initialized");
  m_Notifications = true;
}

// Initialize basic DBus connection
bool CHalManager::InitializeDBus()
{
  if (m_DBusSystemConnection != NULL)
    return true;

  dbus_error_init (&m_Error);
  if (m_DBusSystemConnection == NULL && !(m_DBusSystemConnection = dbus_bus_get (DBUS_BUS_SYSTEM, &m_Error)))
  {
    CLog::Log(LOGERROR, "DBus: Could not get system bus: %s", m_Error.message);
    dbus_error_free (&m_Error);
  }

  if (m_DBusSystemConnection != NULL)
    return true;
  else
    return false;
}

// Initialize basic HAL connection
LibHalContext *CHalManager::InitializeHal()
{
  LibHalContext *ctx;
  char **devices;
  int nr;

  if (!InitializeDBus())
    return NULL;

  if (!(ctx = libhal_ctx_new()))
  {
    CLog::Log(LOGERROR, "HAL: failed to create a HAL context!");
    return NULL;
  }

  if (!libhal_ctx_set_dbus_connection(ctx, m_DBusSystemConnection))
    CLog::Log(LOGERROR, "HAL: Failed to connect with dbus");

  libhal_ctx_set_device_added(ctx, DeviceAdded);
  libhal_ctx_set_device_removed(ctx, DeviceRemoved);
  libhal_ctx_set_device_new_capability(ctx, DeviceNewCapability);
  libhal_ctx_set_device_lost_capability(ctx, DeviceLostCapability);
  libhal_ctx_set_device_property_modified(ctx, DevicePropertyModified);
  libhal_ctx_set_device_condition(ctx, DeviceCondition);

  if (!libhal_device_property_watch_all(ctx, &m_Error))
  {
    CLog::Log(LOGERROR, "HAL: Failed to set property watch %s", m_Error.message);
    dbus_error_free(&m_Error);
    libhal_ctx_free(ctx);
    return NULL;
  }

  if (!libhal_ctx_init(ctx, &m_Error))
  {
    CLog::Log(LOGERROR, "HAL: Failed to initialize hal context: %s", m_Error.message);
    dbus_error_free(&m_Error);
    libhal_ctx_free(ctx);
    return NULL;
  }

  /*
 * Do something to ping the HAL daemon - the above functions will
 * succeed even if hald is not running, so long as DBUS is.  But we
 * want to exit silently if hald is not running, to behave on
 * pre-2.6 systems.
 */
  if (!(devices = libhal_get_all_devices(ctx, &nr, &m_Error)))
  {
    CLog::Log(LOGERROR, "HAL: seems that Hal daemon is not running: %s", m_Error.message);
    dbus_error_free(&m_Error);

    libhal_ctx_shutdown(ctx, NULL);
    libhal_ctx_free(ctx);
    return NULL;
  }

  libhal_free_string_array(devices);

  return ctx;
}

// Helper function. creates a CStorageDevice from a HAL udi
bool CHalManager::DeviceFromVolumeUdi(const char *udi, CStorageDevice *device)
{
  if (g_HalManager.m_Context == NULL)
    return false;

  LibHalVolume *tempVolume;
  LibHalDrive  *tempDrive;
  bool Created = false;

  tempVolume = libhal_volume_from_udi(g_HalManager.m_Context, udi);
  if (tempVolume)
  {
    const char *DriveUdi = libhal_volume_get_storage_device_udi(tempVolume);
    tempDrive = libhal_drive_from_udi(g_HalManager.m_Context, DriveUdi);

    if (tempDrive)
    {
      char * FriendlyName   = libhal_device_get_property_string(g_HalManager.m_Context, udi, "info.product", NULL);
      device->FriendlyName  = FriendlyName;
      libhal_free_string(FriendlyName);
      char *block = libhal_device_get_property_string(g_HalManager.m_Context, udi, "block.device", NULL);
      device->DevID         = block;
      libhal_free_string(block);

      device->HotPlugged  = (bool)libhal_drive_is_hotpluggable(tempDrive);
      device->Type        = libhal_drive_get_type(tempDrive);
      device->Mounted     = (bool)libhal_volume_is_mounted(tempVolume);
      device->MountPoint  = libhal_volume_get_mount_point(tempVolume);
      device->Label       = libhal_volume_get_label(tempVolume);
      device->UUID        = libhal_volume_get_uuid(tempVolume);
      device->FileSystem  = libhal_volume_get_fstype(tempVolume);
      CLinuxFileSystem::ApproveDevice(device);

      libhal_drive_free(tempDrive);
      Created = true;
    }
    else
      CLog::Log(LOGERROR, "HAL: Couldn't create a Drive even if we had a volume - %s", udi);

    libhal_volume_free(tempVolume);
  }

  return Created;
}

// Creates a CStorageDevice for each partition/volume on a Drive.
std::vector<CStorageDevice> CHalManager::DeviceFromDriveUdi(const char *udi)
{
  std::vector<CStorageDevice> Devices;
  if (g_HalManager.m_Context == NULL)
    return Devices; //Empty...

  LibHalDrive *tempDrive;
  LibHalVolume *tempVolume;
  char **AllVolumes;
  bool HotPlugged;
  int  Type;
  int  n;

  AllVolumes = NULL;
  tempDrive = libhal_drive_from_udi(g_HalManager.m_Context, udi);

  if (tempDrive)
  {
    HotPlugged = (bool)libhal_drive_is_hotpluggable(tempDrive);

    Type = libhal_drive_get_type(tempDrive);

    AllVolumes = libhal_drive_find_all_volumes(g_HalManager.m_Context, tempDrive, &n);
    if (AllVolumes)
    {
      for (n = 0; AllVolumes[n]; n++)
      {
        tempVolume = libhal_volume_from_udi(g_HalManager.m_Context, AllVolumes[n]);

        if (tempVolume)
        {
          CStorageDevice dev(AllVolumes[n]);
          char * FriendlyName = libhal_device_get_property_string(g_HalManager.m_Context, AllVolumes[n], "info.product", NULL);
          dev.FriendlyName    = FriendlyName;
          libhal_free_string(FriendlyName);
          char *block = libhal_device_get_property_string(g_HalManager.m_Context, AllVolumes[n], "block.device", NULL);
          dev.DevID           = block;
          libhal_free_string(block);

          dev.HotPlugged  = HotPlugged;
          dev.Type        = Type;
          dev.Mounted     = (bool)libhal_volume_is_mounted(tempVolume);
          dev.MountPoint  = libhal_volume_get_mount_point(tempVolume);
          dev.Label       = libhal_volume_get_label(tempVolume);
          dev.UUID        = libhal_volume_get_uuid(tempVolume);
          dev.FileSystem  = libhal_volume_get_fstype(tempVolume);
          CLinuxFileSystem::ApproveDevice(&dev);

          Devices.push_back(dev);
          libhal_volume_free(tempVolume);
        }
        else
          CLog::Log(LOGERROR, "HAL: Couldn't get a volume from Drive %s", udi);
      }
    }
    libhal_drive_free(tempDrive);
  }

  libhal_free_string_array(AllVolumes);
  return Devices;
}

// Called from ProcessSlow to trigger the callbacks from DBus
bool CHalManager::Update()
{
  CSingleLock lock(m_lock);
  if (m_Context == NULL)
    return false;

  if (!dbus_connection_read_write_dispatch(m_DBusSystemConnection, 0)) // We choose 0 that means we won't wait for a message
  {
    CLog::Log(LOGERROR, "DBus: System - read/write dispatch");
    return false;
  }
  if (NewMessage)
  {
    NewMessage = false;
    return true;
  }
  else
    return false;
}

// Makes a temporary DBus connection and sends a PowerManagement call
bool CHalManager::PowerManagement(PowerState State)
{
  DBusMessage* msg;
  DBusMessageIter args;
  DBusError error;
  dbus_error_init (&error);
  DBusConnection *connection = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
  dbus_int32_t int32 = 0;
  if (connection)
  {
    switch (State)
    {
    case POWERSTATE_HIBERNATE:
      msg = dbus_message_new_method_call("org.freedesktop.Hal", "/org/freedesktop/Hal/devices/computer", "org.freedesktop.Hal.Device.SystemPowerManagement", "Hibernate");
      g_application.m_restartLirc = true;
      g_application.m_restartLCD = true;
      break;
    case POWERSTATE_SUSPEND:
      msg = dbus_message_new_method_call("org.freedesktop.Hal", "/org/freedesktop/Hal/devices/computer", "org.freedesktop.Hal.Device.SystemPowerManagement", "Suspend");
      dbus_message_iter_init_append(msg, &args);
      if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &int32))
        CLog::Log(LOGERROR, "DBus: Failed to append arguments");
      g_application.m_restartLirc = true;
      g_application.m_restartLCD= true;
      break;
    case POWERSTATE_SHUTDOWN:
      msg = dbus_message_new_method_call("org.freedesktop.Hal", "/org/freedesktop/Hal/devices/computer", "org.freedesktop.Hal.Device.SystemPowerManagement", "Shutdown");
      break;
    case POWERSTATE_REBOOT:
      msg = dbus_message_new_method_call("org.freedesktop.Hal", "/org/freedesktop/Hal/devices/computer", "org.freedesktop.Hal.Device.SystemPowerManagement", "Reboot");
      break;
    default:
      CLog::Log(LOGERROR, "DBus: Unrecognised State");
      return false;
      break;
    }

    if (msg == NULL)
      CLog::Log(LOGERROR, "DBus: Create PowerManagement Message failed");
    else
    {
      DBusMessage *reply;
      reply = dbus_connection_send_with_reply_and_block(connection, msg, -1, &error); //The reply timout might be bad to have as -1
      if (dbus_error_is_set(&error))
      {
        CLog::Log(LOGERROR, "DBus: %s - %s", error.name, error.message);
        if (strcmp(error.name, "org.freedesktop.Hal.Device.PermissionDeniedByPolicy") == 0)
          g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(257), g_localizeStrings.Get(13020));

        return false;
      }
      // Need to create a reader for the Message
      dbus_message_unref (reply);
      dbus_message_unref(msg);
      msg = NULL;
    }

    dbus_connection_unref(connection);
    connection = NULL;
    return true;
  }

  return false;
}

/* libhal-storage type to readable form */
const char *CHalManager::StorageTypeToString(int DeviceType)
{
  switch (DeviceType)
  {
  case 0:  return "removable disk";
  case 1:  return "disk";
  case 2:  return "cdrom";
  case 3:  return "floppy";
  case 4:  return "tape";
  case 5:  return "compact flash";
  case 6:  return "memory stick";
  case 7:  return "smart media";
  case 8:  return "sd mmc";
  case 9:  return "camera";
  case 10: return "audio player";
  case 11: return "zip";
  case 12: return "jaz";
  case 13: return "flashkey";
  case 14: return "magneto-optical";
  default: return NULL;
  }
}

/* Readable libhal-storage type to int type */
int CHalManager::StorageTypeFromString(const char *DeviceString)
{
  if      (strcmp(DeviceString, "removable disk") == 0)  return 0;
  else if (strcmp(DeviceString, "disk") == 0)            return 1;
  else if (strcmp(DeviceString, "cdrom") == 0)           return 2;
  else if (strcmp(DeviceString, "floppy") == 0)          return 3;
  else if (strcmp(DeviceString, "tape") == 0)            return 4;
  else if (strcmp(DeviceString, "compact flash") == 0)   return 5;
  else if (strcmp(DeviceString, "memory stick") == 0)    return 6;
  else if (strcmp(DeviceString, "smart media") == 0)     return 7;
  else if (strcmp(DeviceString, "sd mmc") == 0)          return 8;
  else if (strcmp(DeviceString, "camera") == 0)          return 9;
  else if (strcmp(DeviceString, "audio player") == 0)    return 10;
  else if (strcmp(DeviceString, "zip") == 0)             return 11;
  else if (strcmp(DeviceString, "jaz") == 0)             return 12;
  else if (strcmp(DeviceString, "flashkey") == 0)        return 13;
  else if (strcmp(DeviceString, "magneto-optical") == 0) return 14;
  return -1;
}

/* Parse newly found device and add it to our remembered devices */
void CHalManager::ParseDevice(const char *udi)
{
  CSingleLock lock(m_lock);
  char *category;
  category = libhal_device_get_property_string(m_Context, udi, "info.category", NULL);
  if (category == NULL)
    return;

  else if (strcmp(category, "volume") == 0)
  {
    CStorageDevice dev(udi);
    if (!DeviceFromVolumeUdi(udi, &dev))
      return;
    if (g_advancedSettings.m_useHalMount)
    {
/* Here it can be checked if the device isn't mounted and then mount */
//TODO Have mountpoints be other than in /media/*
      if (!dev.Mounted && (dev.HotPlugged || dev.Type == 2) && dev.Approved)
      {
        char **capability;
        capability =libhal_device_get_property_strlist (m_Context, udi, "info.capabilities", NULL);

        bool Mountable = false;
        if (dev.Type == 2 && (strcmp(capability[0], "volume.disc") == 0 && strcmp(capability[1], "volume") == 0)) // CD/DVD
          Mountable = true;
        else if ((strcmp(capability[0], "volume") == 0 && strcmp(capability[1], "block") == 0)) // HDD
          Mountable = true;

        if (Mountable)
        {
          CLog::Log(LOGNOTICE, "HAL: Trying to mount %s", dev.FriendlyName.c_str());
          CStdString MountPoint;
          CStdString TestPath;
          if (dev.Label.size() > 0)
          {
            MountPoint = dev.Label.c_str();
            TestPath.Format("/media/%s", MountPoint.c_str());
            struct stat St;
            if (stat("/media", &St) != 0)
              return; //If /media doesn't exist something is wrong.
            while(stat (TestPath.c_str(), &St) == 0 && S_ISDIR (St.st_mode))
            {
              CLog::Log(LOGDEBUG, "HAL: Proposed Mountpoint already existed");
              MountPoint.append("_");
              TestPath.Format("/media/%s", MountPoint.c_str());
            }
          }
          else
          {
            MountPoint = StorageTypeToString(dev.Type);
            TestPath.Format("/media/%s", MountPoint.c_str());
            int Nbr = 0;
            struct stat St;
            if (stat("/media", &St) != 0)
              return; //If /media doesn't exist something is wrong.
            while(stat (TestPath.c_str(), &St) == 0 && S_ISDIR (St.st_mode))
            {
              CLog::Log(LOGDEBUG, "HAL: Proposed Mountpoint already existed");
              Nbr++;
              MountPoint.Format("%s%i", StorageTypeToString(dev.Type), Nbr);
              TestPath.Format("/media/%s", MountPoint.c_str());
            }
          }
          Mount(dev, MountPoint);
          // Reload some needed things.
          if (!DeviceFromVolumeUdi(udi, &dev))
            return;

          if (dev.Mounted)
          {
            dev.MountedByXBMC = true;
            CLog::Log(LOGINFO, "HAL: mounted %s on %s", dev.FriendlyName.c_str(), dev.MountPoint.c_str());
            if (m_Notifications)
              g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(13021), dev.FriendlyName.c_str());
          }
        }
        libhal_free_string_array(capability);
      }
    }
    int update = -1;
    for (unsigned int i = 0; i < m_Volumes.size(); i++)
    {
      if (strcmp(m_Volumes[i].UDI.c_str(), udi) == 0)
      {
        update = i;
        break;
      }
    }
    if (update == -1)
    {
      CLog::Log(LOGDEBUG, "HAL: Added - %s | %s", CHalManager::StorageTypeToString(dev.Type),  dev.toString().c_str());
      m_Volumes.push_back(dev);
    }
    else
    {
      CLog::Log(LOGDEBUG, "HAL: Update - %s | %s", CHalManager::StorageTypeToString(dev.Type),  dev.toString().c_str());
      if (g_advancedSettings.m_useHalMount)
      {
// If the device was mounted by XBMC before and it is still mounted then it mounted by XBMC still.
        if (dev.Mounted == m_Volumes[update].Mounted)
          dev.MountedByXBMC = m_Volumes[update].MountedByXBMC;
      }
      m_Volumes[update] = dev;
    }
    CLinuxFileSystem::AddDevice(dev);
  }
/*
  else if (strcmp(category, "camera") == 0)
  { // PTP-Devices }
  else if (strcmp(category, "bluetooth_hci") == 0)
  { // Bluetooth-Devices }
  else if (strcmp(category, "portable audio player") == 0)
  { // MTP-Devices }
  else if (strcmp(category, "alsa") == 0)
  { //Alsa Devices }
*/

  libhal_free_string(category);
}
/* Here we should iterate through our remembered devices if any of them are removed */
bool CHalManager::RemoveDevice(const char *udi)
{
  CSingleLock lock(m_lock);
  for (unsigned int i = 0; i < m_Volumes.size(); i++)
  {
    if (strcmp(m_Volumes[i].UDI.c_str(), udi) == 0)
    {
      CLog::Log(LOGNOTICE, "HAL: Removed - %s | %s", CHalManager::StorageTypeToString(m_Volumes[i].Type), m_Volumes[i].toString().c_str());
      CLinuxFileSystem::RemoveDevice(m_Volumes[i].UUID.c_str());

      if (m_Volumes[i].Mounted && g_advancedSettings.m_useHalMount)
      {
        UnMount(m_Volumes[i]);
        if (m_Notifications)
          g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(13022), m_Volumes[i].FriendlyName.c_str());
      }
      m_Volumes.erase(m_Volumes.begin() + i);
      return true;
    }
  }
  return false;
}

bool CHalManager::UnMount(CStorageDevice volume)
{
  CLog::Log(LOGNOTICE, "HAL: UnMounting %s (%s)", volume.UDI.c_str(), volume.toString().c_str());
  DBusMessage* msg;
  DBusMessageIter args;
  DBusError error;
  dbus_error_init (&error);
  DBusConnection *connection = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
  if (connection)
  {
    msg = dbus_message_new_method_call("org.freedesktop.Hal", volume.UDI.c_str(), "org.freedesktop.Hal.Device.Volume", "Unmount");
    dbus_message_iter_init_append(msg, &args);
    DBusMessageIter sub;
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING, &sub);
    const char *s = "lazy";
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &s);
    dbus_message_iter_close_container(&args, &sub);

    if (msg == NULL)
        CLog::Log(LOGERROR, "DBus: Create PowerManagement Message failed");
    else
    {
      DBusMessage *reply;
      reply = dbus_connection_send_with_reply_and_block(connection, msg, -1, &error); //The reply timout might be bad to have as -1
      if (dbus_error_is_set(&error))
      {
        CLog::Log(LOGERROR, "DBus: %s - %s", error.name, error.message);
        return false;
      }
      // Need to create a reader for the Message
      dbus_message_unref (reply);
      dbus_message_unref(msg);
      msg = NULL;
    }

    dbus_connection_unref(connection);
    connection = NULL;
    return true;
  }
  else
  {
    CLog::Log(LOGERROR, "DBus: Failed to connect to Systembus");
    return false;
  }
}

bool CHalManager::Mount(CStorageDevice volume, CStdString mountpath)
{
  CLog::Log(LOGNOTICE, "HAL: Mounting %s (%s) at %s", volume.UDI.c_str(), volume.toString().c_str(), mountpath.c_str());
  DBusMessage* msg;
  DBusMessageIter args;
  DBusError error;
  dbus_error_init (&error);
  DBusConnection *connection = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
  const char *s;
  if (connection)
  {
    msg = dbus_message_new_method_call("org.freedesktop.Hal", volume.UDI.c_str(), "org.freedesktop.Hal.Device.Volume", "Mount");
    dbus_message_iter_init_append(msg, &args);
    s = mountpath.c_str();
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &s))
      CLog::Log(LOGERROR, "DBus: Failed to append arguments");
    s = ""; //FileSystem
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &s))
      CLog::Log(LOGERROR, "DBus: Failed to append arguments");
    DBusMessageIter sub;
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING, &sub);
    s = "sync";
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &s);
    s = "ro"; //Mount readonly as we dont have an eject yet
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &s);
    dbus_message_iter_close_container(&args, &sub);

    if (msg == NULL)
        CLog::Log(LOGERROR, "DBus: Create PowerManagement Message failed");
    else
    {
      DBusMessage *reply;
      reply = dbus_connection_send_with_reply_and_block(connection, msg, -1, &error); //The reply timout might be bad to have as -1
      if (dbus_error_is_set(&error))
      {
        CLog::Log(LOGERROR, "DBus: %s - %s", error.name, error.message);
        return false;
      }
      // Need to create a reader for the Message
      dbus_message_unref (reply);
      dbus_message_unref(msg);
      msg = NULL;
    }

    dbus_connection_unref(connection);
    connection = NULL;
    return true;
  }
  else
  {
    CLog::Log(LOGERROR, "DBus: Failed to connect to Systembus");
    return false;
  }
}
#endif // HAS_HAL
