/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "../../guilib/system.h"
#include "HalManager.h"
#include "RegExp.h"
#ifdef HAS_HAL
#include <libhal-storage.h>

//#define HAL_HANDLEMOUNT

bool CHalManager::NewMessage;
DBusError CHalManager::m_Error;
/* A Removed device, It isn't possible to make a LibHalVolume from a removed device therefor
   we catch the UUID from the udi on the removal */
void CHalManager::DeviceRemoved(LibHalContext *ctx, const char *udi)
{
  NewMessage = true;
  CLog::Log(LOGDEBUG, "HAL: Removed %s", udi);
  CRegExp regUUID;
  regUUID.RegComp("/org/freedesktop/Hal/devices/volume_uuid_([^ ]+)");
  if (regUUID.RegFind(udi) != -1)
  {
    CStdString UUID = regUUID.GetReplaceString("\\1");
    // the udi have UUID with xxx_xxx instead of xxx-xxx
    UUID.Replace('_', '-');
    CLinuxFileSystem::RemoveDevice(UUID);
  }
}

void CHalManager::DeviceNewCapability(LibHalContext *ctx, const char *udi, const char *capability)
{
  NewMessage = true;
  CLog::Log(LOGDEBUG, "HAL: Device got new capability %s", udi);
}

void CHalManager::DeviceLostCapability(LibHalContext *ctx, const char *udi, const char *capability)
{
  NewMessage = true;
  CLog::Log(LOGDEBUG, "HAL: Device lost capability %s", udi);
}

/* HAL Property modified callback. If a device is mounted. This is called. */
void CHalManager::DevicePropertyModified(LibHalContext *ctx, const char *udi, const char *key, dbus_bool_t is_removed, dbus_bool_t is_added)
{
  NewMessage = true;
  CLog::Log(LOGDEBUG, "HAL: Property modified %s", udi);
  LibHalVolume *TryVolume = libhal_volume_from_udi(ctx, udi);

  if (TryVolume)
  {
    CLog::Log(LOGNOTICE, "HAL: Mount found %s", udi);
    libhal_volume_free(TryVolume);
    CDevice dev;
    if (g_HalManager.DeviceFromVolumeUdi(udi, &dev))
      CLinuxFileSystem::AddDevice(dev);
    else
      CLog::Log(LOGERROR, "HAL: Couldn't create device from %s", udi);
  }
}

void CHalManager::DeviceCondition(LibHalContext *ctx, const char *udi, const char *condition_name, const char *condition_details)
{
  CLog::Log(LOGDEBUG, "HAL: Condition %s", udi);
  NewMessage = true;
}
/* HAL Device added. This is before mount. And here is the place to mount the volume in the future */
void CHalManager::DeviceAdded(LibHalContext *ctx, const char *udi)
{
  NewMessage = true;
  CLog::Log(LOGDEBUG, "HAL: Added %s", udi);
#ifdef HAL_HANDLEMOUNT
  char **capabilities;
	if (!(capabilities = libhal_device_get_property_strlist (ctx, udi, "info.capabilities", NULL)))
		return;

  bool volume, block;
  volume = block = false;

/* There probably is a better thing of seeing this using HAL but if the capabilities are both volume and block
   The device i ready to be mounted. */
  for (int i = 0; capabilities[i]; i++)
  {
    if (strcmp(capabilities[i], "volume"))
      volume = true;
    else if (strcmp(capabilities[i], "block"))
      block = true;
  }

  if (volume && block)
    printf("MOUNTABLE!\n"); // TODO Here we CAN run a pmount on the Device
#endif
}

CHalManager g_HalManager;

// Return all volumes that currently are available (Mostly needed at startup, the rest of the volumes comes as events.)
std::vector<CDevice> CHalManager::GetDevices()
{
  std::vector<CDevice> Devices;

  if (g_HalManager.m_Context == NULL)
    return Devices; //Empty..

  char **GDL;
  int i = 0;

  CLog::Log(LOGNOTICE, "HAL: Generating global device list ...");
  GDL = libhal_get_all_devices (g_HalManager.m_Context, &i, &m_Error);

  for (i = 0; GDL[i]; i++)
  {
    std::vector<CDevice> temp = DeviceFromDriveUdi(GDL[i]);

    for (unsigned int j = 0; j < temp.size(); j++)
    {
      CLog::Log(LOGDEBUG, "HAL: %s", temp[j].toString().c_str());
      Devices.push_back(temp[j]);
    }
  }
  CLog::Log(LOGINFO, "HAL: Generated global device list, found %i", i);

  return Devices;
}

CHalManager::CHalManager()
{
}

// Shutdown the connection and free the context
CHalManager::~CHalManager()
{
  dbus_error_free(&m_Error); // Needed?

	if (m_Context != NULL)	
  	libhal_ctx_shutdown(m_Context, NULL);
  if (m_Context != NULL)
  	libhal_ctx_free(m_Context); 
}

// Initialize
void CHalManager::Initialize()
{
  CLog::Log(LOGINFO, "HAL: Starting initializing");
  g_HalManager.m_Context = g_HalManager.InitializeHal();
  if (g_HalManager.m_Context == NULL)
  {
    CLog::Log(LOGERROR, "HAL: no Hal context");
    return;
  }

  std::vector<CDevice> CurrentDevices = CHalManager::GetDevices();

  for (unsigned int i = 0; i < CurrentDevices.size(); i++)
    CLinuxFileSystem::AddDevice(CurrentDevices[i]);

  CLog::Log(LOGINFO, "HAL: Sucessfully initialized");
}

// Initialize basic DBus connection
bool CHalManager::InitializeDBus()
{
	if (m_DBusConnection != NULL)
		return TRUE;

	dbus_error_init (&m_Error);
	if (!(m_DBusConnection = dbus_bus_get (DBUS_BUS_SYSTEM, &m_Error)))
  {
		CLog::Log(LOGERROR, "DBus: Could not get system bus: %s", m_Error.message);
		dbus_error_free (&m_Error);
		return FALSE;
	}

	return TRUE;
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

	if (!libhal_ctx_set_dbus_connection(ctx, m_DBusConnection))
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

// Helper function. creates a CDevice from a HAL udi
bool CHalManager::DeviceFromVolumeUdi(const char *udi, CDevice *device)
{
  if (g_HalManager.m_Context == NULL)
    return false;

  LibHalVolume *tempVolume;
  LibHalDrive  *tempDrive;
  int  Type;
  bool HotPlugged;
  bool Created = false;

  tempVolume = libhal_volume_from_udi(g_HalManager.m_Context, udi);
  if (tempVolume)    
  {
    const char *DriveUdi = libhal_volume_get_storage_device_udi(tempVolume);
    tempDrive = libhal_drive_from_udi(g_HalManager.m_Context, DriveUdi);
    if (tempDrive)
    {
      HotPlugged = (bool)libhal_drive_is_hotpluggable(tempDrive);
      Type = libhal_drive_get_type(tempDrive);

      device->HotPlugged  = HotPlugged;
      device->Type        = Type;
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

// Creates a CDevice for each partition/volume on a Drive.
std::vector<CDevice> CHalManager::DeviceFromDriveUdi(const char *udi)
{
  std::vector<CDevice> Devices;
  if (g_HalManager.m_Context == NULL)
    return Devices; //Empty...

  LibHalDrive *tempDrive;
  LibHalVolume *tempVolume;
  char **AllVolumes;
  bool HotPlugged;
  int  Type;
  int  n;

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
          CDevice dev;

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

  return Devices;
}

// Called from ProcessSlow to trigger the callbacks from DBus
bool CHalManager::Update()
{
  if (g_HalManager.m_Context == NULL)
    return false;

  if (!dbus_connection_read_write_dispatch(g_HalManager.m_DBusConnection, 0)) // We choose 0 that means we won't wait for a message
  {
    CLog::Log(LOGERROR, "DBus: read/write dispatch");
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
#endif // HAS_HAL
