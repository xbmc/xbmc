/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#ifdef HAS_HAL
#include "HALManager.h"
#include "interfaces/Builtins.h"
#include <libhal-storage.h>
#include "threads/SingleLock.h"
#include "utils/URIUtils.h"
#include "guilib/LocalizeStrings.h"
#include "powermanagement/PowerManager.h"
#include "settings/AdvancedSettings.h"
#include "dialogs/GUIDialogKaiToast.h"

#ifdef HAS_SDL_JOYSTICK
#include <SDL/SDL.h>
#include <SDL/SDL_version.h>
#include "input/SDLJoystick.h"
#endif

bool CHALManager::NewMessage;
DBusError CHALManager::m_Error;
CCriticalSection CHALManager::m_lock;

/* A Removed device, It isn't possible to make a LibHalVolume from a removed device therefor
   we catch the UUID from the udi on the removal */
void CHALManager::DeviceRemoved(LibHalContext *ctx, const char *udi)
{
  NewMessage = true;
  CLog::Log(LOGDEBUG, "HAL: Device (%s) Removed", udi);
  g_HalManager.RemoveDevice(udi);
}

void CHALManager::DeviceNewCapability(LibHalContext *ctx, const char *udi, const char *capability)
{
  NewMessage = true;
  CLog::Log(LOGDEBUG, "HAL: Device (%s) gained capability %s", udi, capability);
  g_HalManager.UpdateDevice(udi);
}

void CHALManager::DeviceLostCapability(LibHalContext *ctx, const char *udi, const char *capability)
{
  NewMessage = true;
  CLog::Log(LOGDEBUG, "HAL: Device (%s) lost capability %s", udi, capability);
  g_HalManager.UpdateDevice(udi);
}

/* HAL Property modified callback. If a device is mounted. This is called. */
void CHALManager::DevicePropertyModified(LibHalContext *ctx, const char *udi, const char *key, dbus_bool_t is_removed, dbus_bool_t is_added)
{
  NewMessage = true;
  CLog::Log(LOGDEBUG, "HAL: Device (%s) Property %s modified", udi, key);
  g_HalManager.UpdateDevice(udi);
}

void CHALManager::DeviceCondition(LibHalContext *ctx, const char *udi, const char *condition_name, const char *condition_details)
{
  NewMessage = true;
  CLog::Log(LOGDEBUG, "HAL: Device (%s) Condition %s | %s", udi, condition_name, condition_details);
  if (!strcmp(condition_name, "ButtonPressed") && !strcmp(condition_details, "power"))
    CBuiltins::Execute("XBMC.ShutDown()");
  else
    g_HalManager.UpdateDevice(udi);
}

/* HAL Device added. This is before mount. And here is the place to mount the volume in the future */
void CHALManager::DeviceAdded(LibHalContext *ctx, const char *udi)
{
  NewMessage = true;
  CLog::Log(LOGDEBUG, "HAL: Device (%s) Added", udi);
  g_HalManager.AddDevice(udi);
}

CHALManager g_HalManager;

/* Iterate through all devices currently on the computer. Needed mostly at startup */
void CHALManager::GenerateGDL()
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
    AddDevice(GDL[i]);
  }
  CLog::Log(LOGINFO, "HAL: Generated global device list, found %i", i);

  libhal_free_string_array(GDL);
}

// Return all volumes that currently are available (Mostly needed at startup, the rest of the volumes comes as events.)
std::vector<CStorageDevice> CHALManager::GetVolumeDevices()
{
  CSingleLock lock(m_lock);
  return m_Volumes;
}

CHALManager::CHALManager()
{
  m_Notifications = false;
  m_Context = NULL;
  m_DBusSystemConnection = NULL;
#if defined(HAS_SDL_JOYSTICK)
  const SDL_version *sdl_version = SDL_Linked_Version();
  m_bMultipleJoysticksSupport = (sdl_version->major >= 1 && sdl_version->minor >= 3)?true:false;
#endif
}

void CHALManager::Stop()
{
  if (g_advancedSettings.m_handleMounting)
  { // Unmount all media XBMC have mounted
    for (unsigned int i = 0; i < m_Volumes.size(); i++)
    {
      if (m_Volumes[i].MountedByXBMC && m_Volumes[i].Mounted)
      {
        CLog::Log(LOGNOTICE, "HAL: Unmounts %s", m_Volumes[i].FriendlyName.c_str());
        UnMount(m_Volumes[i]);
      }
    }
  }

  m_Volumes.clear();

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
void CHALManager::Initialize()
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
bool CHALManager::InitializeDBus()
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
LibHalContext *CHALManager::InitializeHal()
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
bool CHALManager::DeviceFromVolumeUdi(const char *udi, CStorageDevice *device)
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
      if (device->Mounted)
        URIUtils::AddSlashAtEnd(device->MountPoint);
      device->Label       = libhal_volume_get_label(tempVolume);
      device->UUID        = libhal_volume_get_uuid(tempVolume);
      device->FileSystem  = libhal_volume_get_fstype(tempVolume);
      device->HalIgnore   = libhal_device_get_property_bool(g_HalManager.m_Context, udi, "volume.ignore", NULL);
      ApproveDevice(device);

      libhal_drive_free(tempDrive);
      Created = true;
    }
    else
      CLog::Log(LOGERROR, "HAL: Couldn't create a Drive even if we had a volume - %s", udi);

    libhal_volume_free(tempVolume);
  }

  return Created;
}

// Called from ProcessSlow to trigger the callbacks from DBus
bool CHALManager::Update()
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

/* libhal-storage type to readable form */
const char *CHALManager::StorageTypeToString(int DeviceType)
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
int CHALManager::StorageTypeFromString(const char *DeviceString)
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

void CHALManager::UpdateDevice(const char *udi)
{
  CSingleLock lock(m_lock);
  char *category;
  category = libhal_device_get_property_string(m_Context, udi, "info.category", NULL);
  if (category == NULL)
    return;

  if (strcmp(category, "volume") == 0)
  {
    CStorageDevice dev(udi);
    if (!DeviceFromVolumeUdi(udi, &dev))
      return;
    for (unsigned int i = 0; i < m_Volumes.size(); i++)
    {
      if (strcmp(m_Volumes[i].UDI.c_str(), udi) == 0)
      {
        CLog::Log(LOGDEBUG, "HAL: Update - %s | %s", CHALManager::StorageTypeToString(dev.Type),  dev.toString().c_str());
        if (g_advancedSettings.m_handleMounting)  // If the device was mounted by XBMC before it's still mounted by XBMC.
            dev.MountedByXBMC = m_Volumes[i].MountedByXBMC;
        if (!dev.Mounted && m_Volumes[i].Mounted)
          CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(13023), dev.FriendlyName.c_str(), TOAST_DISPLAY_TIME, false);
        m_Volumes[i] = dev;

        break;
      }
    }
  }

  libhal_free_string(category);
}
void CHALManager::HandleNewVolume(CStorageDevice *dev)
{
  if (g_advancedSettings.m_handleMounting)
  {
/* Here it can be checked if the device isn't mounted and then mount */
//TODO Have mountpoints be other than in /media/*
    if (!dev->Mounted && (dev->HotPlugged || dev->Type == 2) && dev->Approved)
    {
      char **capability;
      capability =libhal_device_get_property_strlist (m_Context, dev->UDI.c_str(), "info.capabilities", NULL);

      bool Mountable = false;
      if (dev->Type == 2 && (strcmp(capability[0], "volume.disc") == 0 && strcmp(capability[1], "volume") == 0)) // CD/DVD
        Mountable = true;
      else if ((strcmp(capability[0], "volume") == 0 && strcmp(capability[1], "block") == 0)) // HDD
        Mountable = true;

      if (Mountable)
      {
        CLog::Log(LOGNOTICE, "HAL: Trying to mount %s", dev->FriendlyName.c_str());
        CStdString MountPoint;
        CStdString TestPath;
        if (dev->Label.size() > 0)
        {
          MountPoint = dev->Label.c_str();
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
          MountPoint = StorageTypeToString(dev->Type);
          TestPath.Format("/media/%s", MountPoint.c_str());
          int Nbr = 0;
          struct stat St;
          if (stat("/media", &St) != 0)
            return; //If /media doesn't exist something is wrong.
          while(stat (TestPath.c_str(), &St) == 0 && S_ISDIR (St.st_mode))
          {
            CLog::Log(LOGDEBUG, "HAL: Proposed Mountpoint already existed");
            Nbr++;
            MountPoint.Format("%s%i", StorageTypeToString(dev->Type), Nbr);
            TestPath.Format("/media/%s", MountPoint.c_str());
          }
        }
        if (Mount(dev, MountPoint))
        {
          CLog::Log(LOGINFO, "HAL: mounted %s on %s", dev->FriendlyName.c_str(), dev->MountPoint.c_str());
          if (m_Notifications)
            CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(13021), dev->FriendlyName.c_str(), TOAST_DISPLAY_TIME, false);
        }
      }
      libhal_free_string_array(capability);
    }
  }
}

/* Parse newly found device and add it to our remembered devices */
void CHALManager::AddDevice(const char *udi)
{
  CSingleLock lock(m_lock);
  char *category;
  category = libhal_device_get_property_string(m_Context, udi, "info.category", NULL);
  if (category == NULL)
    return;

  if (strcmp(category, "volume") == 0)
  {
    CStorageDevice dev(udi);
    if (DeviceFromVolumeUdi(udi, &dev))
    {
      CLog::Log(LOGDEBUG, "HAL: Added - %s | %s", CHALManager::StorageTypeToString(dev.Type),  dev.toString().c_str());
      HandleNewVolume(&dev);
      m_Volumes.push_back(dev);
    }
  }
#if defined(HAS_SDL_JOYSTICK)
  // Scan input devices
  else if (strcmp(category, "input") == 0)
  {
    DBusError dbusError;
    dbus_error_init(&dbusError);

    char **capability;
    capability =libhal_device_get_property_strlist (m_Context, udi, "info.capabilities", &dbusError);
    for(char **ptr = capability; *ptr != NULL;ptr++)
    {
      // Reload joysticks
      if(strcmp(*ptr, "input.joystick") == 0)
      {
        CLog::Log(LOGINFO, "HAL: Joystick plugged in");
        CHALDevice dev = CHALDevice(udi);
        dev.FriendlyName = libhal_device_get_property_string(m_Context, udi, "info.product", &m_Error);
        m_Joysticks.push_back(dev);

        if(m_Joysticks.size() < 2 || m_bMultipleJoysticksSupport)
        {
          // Restart SDL joystick subsystem
          if (!g_Joystick.Reinitialize())
            break;

          if (m_Notifications)
            CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(13024), dev.FriendlyName.c_str(), TOAST_DISPLAY_TIME, false);
        }
      }
    }
    libhal_free_string_array(capability);
  }
#endif
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
bool CHALManager::RemoveDevice(const char *udi)
{
  CSingleLock lock(m_lock);
  for (unsigned int i = 0; i < m_Volumes.size(); i++)
  {
    if (strcmp(m_Volumes[i].UDI.c_str(), udi) == 0)
    {
      CLog::Log(LOGNOTICE, "HAL: Removed - %s | %s", CHALManager::StorageTypeToString(m_Volumes[i].Type), m_Volumes[i].toString().c_str());

      if (m_Volumes[i].Mounted)
      {
        if (g_advancedSettings.m_handleMounting)
          UnMount(m_Volumes[i]);
        if (m_Notifications)
          CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(13022), m_Volumes[i].FriendlyName.c_str());
        CLog::Log(LOGNOTICE, "HAL: Unsafe drive removal");
      }
      m_Volumes.erase(m_Volumes.begin() + i);
      return true;
    }
  }
#if defined(HAS_SDL_JOYSTICK)
  for(uint i = 0; i < m_Joysticks.size(); i++)
  {
    if (strcmp(m_Joysticks[i].UDI.c_str(), udi) == 0)
    {
      if(m_Joysticks.size() < 3 || m_bMultipleJoysticksSupport)
      {
        // Restart SDL joystick subsystem
        if (!g_Joystick.Reinitialize())
          return false;

        if (m_Notifications)
          CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(13025), m_Joysticks[i].FriendlyName.c_str(), TOAST_DISPLAY_TIME, false);
      }
      m_Joysticks.erase(m_Joysticks.begin() + i);
      return true;
    }
  }
#endif
  return false;
}

bool CHALManager::ApproveDevice(CStorageDevice *device)
{
  bool approve = true;
  //This is only because it's easier to read...
  const char *fs = device->FileSystem.c_str();

  if ( strcmp(fs, "vfat") == 0    || strcmp(fs, "ext2") == 0
       || strcmp(fs, "ext3") == 0 || strcmp(fs, "reiserfs") == 0
       || strcmp(fs, "ntfs") == 0 || strcmp(fs, "ntfs-3g") == 0
       || strcmp(fs, "udf") == 0  || strcmp(fs, "iso9660") == 0
       || strcmp(fs, "xfs") == 0  || strcmp(fs, "hfsplus") == 0
       || strcmp(fs, "ext4") == 0)
    approve = true;
  else
    approve = false;

  // Ignore some mountpoints, unless a weird setup these should never contain anything usefull for an enduser.
  if (strcmp(device->MountPoint, "/") == 0 || strcmp(device->MountPoint, "/boot/") == 0 || strcmp(device->MountPoint, "/mnt/") == 0 || strcmp(device->MountPoint, "/home/") == 0)
    approve = false;

  if (device->HalIgnore)
    approve = false;

  device->Approved = approve;
  return approve;
}

bool CHALManager::Eject(CStdString path)
{
  for (unsigned int i = 0; i < m_Volumes.size(); i++)
  {
    if (m_Volumes[i].MountPoint.Equals(path))
      return m_Volumes[i].HotPlugged ? UnMount(m_Volumes[i]) : false;
  }

  return false;
}

bool CHALManager::UnMount(CStorageDevice volume)
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
        CLog::Log(LOGERROR, "DBus: Create UnMount Message failed");
    else
    {
      DBusMessage *reply;
      reply = dbus_connection_send_with_reply_and_block(connection, msg, -1, &error); //The reply timout might be bad to have as -1
      if (dbus_error_is_set(&error))
      {
        CLog::Log(LOGERROR, "DBus: %s - %s", error.name, error.message);
        dbus_error_free(&error);
        return false;
      }
      // Need to create a reader for the Message
      dbus_message_unref (reply);
      dbus_message_unref(msg);
      msg = NULL;
    }

    volume.MountPoint = "";
    volume.Mounted    = false;
    dbus_connection_unref(connection);
    connection = NULL;
    return true;
  }
  else
  {
    CLog::Log(LOGERROR, "DBus: Failed to connect to Systembus");
    dbus_error_free(&error);
    return false;
  }
}

bool CHALManager::Mount(CStorageDevice *volume, CStdString mountpath)
{
  CLog::Log(LOGNOTICE, "HAL: Mounting %s (%s) at %s with umask=%u", volume->UDI.c_str(), volume->toString().c_str(), mountpath.c_str(), umask (0));
  DBusMessage* msg;
  DBusMessageIter args;
  DBusError error;
  dbus_error_init (&error);
  DBusConnection *connection = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
  const char *s;
  if (connection)
  {
    msg = dbus_message_new_method_call("org.freedesktop.Hal", volume->UDI.c_str(), "org.freedesktop.Hal.Device.Volume", "Mount");
    dbus_message_iter_init_append(msg, &args);
    s = mountpath.c_str();
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &s))
      CLog::Log(LOGERROR, "DBus: Failed to append arguments");
    s = ""; //FileSystem
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &s))
      CLog::Log(LOGERROR, "DBus: Failed to append arguments");
    DBusMessageIter sub;
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING, &sub);

    CStdString temporaryString;

    if (volume->FileSystem.Equals("vfat"))
    {
      int mask = umask (0);
      temporaryString.Format("umask=%#o", mask);
      s = temporaryString.c_str();
      dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &s);
      temporaryString.Format("uid=%u", getuid());
      s = temporaryString.c_str();
      dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &s);
      s = "shortname=mixed";
      dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &s);
      s = "utf8";
      dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &s);
      // 'sync' option will slow down transfer speed significantly for FAT filesystems. We prefer 'flush' instead.
      s = "flush";
      dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &s);
    }
    else
    {
      s = "sync";
      dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &s);
    }

    dbus_message_iter_close_container(&args, &sub);

    if (msg == NULL)
        CLog::Log(LOGERROR, "DBus: Create Mount Message failed");
    else
    {
      DBusMessage *reply;
      reply = dbus_connection_send_with_reply_and_block(connection, msg, -1, &error); //The reply timout might be bad to have as -1
      if (dbus_error_is_set(&error))
      {
        CLog::Log(LOGERROR, "DBus: %s - %s", error.name, error.message);
        dbus_error_free(&error);
        return false;
      }
      // Need to create a reader for the Message
      dbus_message_unref (reply);
      dbus_message_unref(msg);
      msg = NULL;
    }

    volume->Mounted = true;
    volume->MountedByXBMC = true;
    volume->MountPoint = mountpath;
    dbus_connection_unref(connection);
    connection = NULL;
    return true;
  }
  else
  {
    CLog::Log(LOGERROR, "DBus: Failed to connect to Systembus");
    dbus_error_free(&error);
    return false;
  }
}
#endif // HAS_HAL
