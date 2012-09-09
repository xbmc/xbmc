#ifdef HAS_HAL
#ifndef HALMANAGER_H
#define HALMANAGER_H

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
#include <string.h>
#include <stdio.h>
#include <dbus/dbus.h>
#include <libhal.h>
#include <vector>

#define BYTE char
#include "utils/log.h"
#include "threads/CriticalSection.h"
#include "utils/URIUtils.h"
#include "MediaSource.h"
#include "settings/GUISettings.h"

class CHALDevice
{
public:
  CStdString UDI;
  CStdString FriendlyName;
  CHALDevice(const char *udi) { UDI = udi; }
};

class CStorageDevice : public CHALDevice
{
public:
  CStorageDevice(const char *udi) : CHALDevice(udi) { HotPlugged = false; Mounted = false; Approved = false; MountedByXBMC = false; }
  bool MountedByXBMC;
  bool Mounted;
  bool Approved;
  bool HotPlugged;
  bool HalIgnore;
  CStdString MountPoint;
  CStdString Label;
  CStdString UUID;
  CStdString DevID;
  int  Type;
  CStdString FileSystem;

  CStdString toString()
  { // Not the prettiest but it's better than having to reproduce it elsewere in the code...
    CStdString rtn, tmp1, tmp2, tmp3, tmp4;
    if (UUID.size() > 0)
      tmp1.Format("UUID %s | ", UUID.c_str());
    if (FileSystem.size() > 0)
      tmp2.Format("FileSystem %s | ", FileSystem.c_str());
    if (MountPoint.size() > 0)
      tmp3.Format("Mounted on %s | ", MountPoint.c_str());
    if (HotPlugged)
      tmp4.Format("HotPlugged YES | ");
    else
      tmp4.Format("HotPlugged NO  | ");

    if (Approved)
      rtn.Format("%s%s%s%sType %i |Approved YES ", tmp1.c_str(), tmp2.c_str(), tmp3.c_str(), tmp4.c_str(), Type);
    else
      rtn.Format("%s%s%s%sType %i |Approved NO  ", tmp1.c_str(), tmp2.c_str(), tmp3.c_str(), tmp4.c_str(), Type);

    return  rtn;
  }
  void toMediaSource(CMediaSource *share)
  {
    share->strPath = MountPoint;
    if (Label.size() > 0)
      share->strName = Label;
    else
    {
      share->strName = MountPoint;
      URIUtils::RemoveSlashAtEnd(share->strName);
      share->strName = URIUtils::GetFileName(share->strName);
    }

    share->m_ignore = true;
    if (HotPlugged)
      share->m_iDriveType = CMediaSource::SOURCE_TYPE_REMOVABLE;
    else if(strcmp(FileSystem.c_str(), "iso9660") == 0 || strcmp(FileSystem.c_str(), "udf") == 0)
      share->m_iDriveType = CMediaSource::SOURCE_TYPE_DVD;
    else
      share->m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  }

};


class CHALManager
{
public:
  static const char *StorageTypeToString(int DeviceType);
  static int StorageTypeFromString(const char *DeviceString);
  bool Update();

  void Initialize();
  CHALManager();
  void Stop();
  std::vector<CStorageDevice> GetVolumeDevices();
  bool Eject(CStdString path);
protected:
  DBusConnection *m_DBusSystemConnection;
  LibHalContext  *m_Context;
  static DBusError m_Error;
  static bool NewMessage;


  void UpdateDevice(const char *udi);
  void AddDevice(const char *udi);
  bool RemoveDevice(const char *udi);

private:
  bool m_Notifications;
  LibHalContext *InitializeHal();
  bool InitializeDBus();
  void GenerateGDL();

  bool UnMount(CStorageDevice volume);
  bool Mount(CStorageDevice *volume, CStdString mountpath);
  void HandleNewVolume(CStorageDevice *dev);
  static bool ApproveDevice(CStorageDevice *device);

  static bool DeviceFromVolumeUdi(const char *udi, CStorageDevice *device);
  static CCriticalSection m_lock;

#if defined(HAS_SDL_JOYSTICK)
  bool m_bMultipleJoysticksSupport;
#endif

  //Callbacks HAL
  static void DeviceRemoved(LibHalContext *ctx, const char *udi);
  static void DeviceNewCapability(LibHalContext *ctx, const char *udi, const char *capability);
  static void DeviceLostCapability(LibHalContext *ctx, const char *udi, const char *capability);
  static void DevicePropertyModified(LibHalContext *ctx, const char *udi, const char *key, dbus_bool_t is_removed, dbus_bool_t is_added);
  static void DeviceCondition(LibHalContext *ctx, const char *udi, const char *condition_name, const char *condition_details);
  static void DeviceAdded(LibHalContext *ctx, const char *udi);

  //Remembered Devices
  std::vector<CStorageDevice> m_Volumes;
#if defined(HAS_SDL_JOYSTICK)
  std::vector<CHALDevice> m_Joysticks;
#endif
};

extern CHALManager g_HalManager;
#endif
#endif // HAS_HAL
