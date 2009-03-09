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

#include "LinuxFileSystem.h"
#include "RegExp.h"
#include "SingleLock.h"
#include "../Util.h"

using namespace std;

CCriticalSection CLinuxFileSystem::m_lock;

#ifdef HAS_HAL
vector<CStorageDevice> CLinuxFileSystem::m_Devices;
bool CLinuxFileSystem::m_DeviceChange;

/* This is never used */
void CLinuxFileSystem::UpdateDevices()
{
  CSingleLock lock(m_lock);
  m_Devices.clear();
  m_Devices = g_HalManager.GetVolumeDevices();
}

/* Here can approved devices be choosen, this is always called for a device in HalManager before it's sent to adddevice. */
bool CLinuxFileSystem::ApproveDevice(CStorageDevice *device)
{
  CSingleLock lock(m_lock);
  bool approve = true;
  //This is only because it's easier to read...
  const char *fs = device->FileSystem.c_str();

  if ( strcmp(fs, "vfat") == 0    || strcmp(fs, "ext2") == 0
       || strcmp(fs, "ext3") == 0 || strcmp(fs, "reiserfs") == 0
       || strcmp(fs, "ntfs") == 0 || strcmp(fs, "ntfs-3g") == 0
       || strcmp(fs, "udf") == 0  || strcmp(fs, "iso9660") == 0
       || strcmp(fs, "xfs") == 0  || strcmp(fs, "hfsplus") == 0)
    approve = true;
  else
    approve = false;

  // Ignore root
  if (strcmp(device->MountPoint, "/") == 0)
    approve = false;

  device->Approved = approve;
  return approve;
}
#endif //HAS_HAL

void CLinuxFileSystem::GetDrives(VECSOURCES &shares)
{
  return GetDrives(NULL, -1, shares);
}

void CLinuxFileSystem::GetLocalDrives(VECSOURCES &shares)
{
  CSingleLock lock(m_lock);
#ifndef HAS_HAL
  GetDrives(shares);
#else
  UpdateDevices();
  for (size_t i = 0; i < m_Devices.size(); i++)
  {
    if (m_Devices[i].Mounted && m_Devices[i].Approved && !m_Devices[i].HotPlugged)
    {
      CMediaSource share;
      m_Devices[i].toMediaSource(&share);
      shares.push_back(share);
    }
  }
#endif
}

void CLinuxFileSystem::GetRemovableDrives(VECSOURCES &shares)
{
  CSingleLock lock(m_lock);
#ifndef HAS_HAL
  GetDrives(shares);
#else
  UpdateDevices();

  for (size_t i = 0; i < m_Devices.size(); i++)
  {
    if (m_Devices[i].Mounted && m_Devices[i].Approved && m_Devices[i].HotPlugged)
    {
      CMediaSource share;
      m_Devices[i].toMediaSource(&share);
      shares.push_back(share);
    }
  }
#endif
}

/* if DeviceType == NULL we return all devices
   To decide wich types types are approved to be returned send for example int DevTypes[] = {0, 5, 6, 7, 8, 9, 10, 13}.
   this will return all removable types like pendrives, memcards and such but NOT removable hdd's have type 1.
   for more information on these for now is in libhal-storage.h. TODO Make defined types that can be common on all O/S */
void CLinuxFileSystem::GetDrives(int *DeviceType, int len, VECSOURCES &shares)
{
  CSingleLock lock(m_lock);
#ifndef HAS_HAL
  std::vector<CStdString> result;
  if (DeviceType == NULL) // -1 is considered all devices, this is only needed in the Browse dialog. The other choices are for VirtualDirectory
  {
    CRegExp reMount;
#ifdef __APPLE__
    reMount.RegComp("on (.+) \\(([^,]+)");
#else
    reMount.RegComp("on (.+) type ([^ ]+)");
#endif
    char line[1024];

    FILE* pipe = popen("mount", "r");

    if (pipe)
    {
      while (fgets(line, sizeof(line) - 1, pipe))
      {
        if (reMount.RegFind(line) != -1)
        {
          char* mount = reMount.GetReplaceString("\\1");
          char* fs    = reMount.GetReplaceString("\\2");
#ifdef __APPLE__
          // Ignore the stuff that doesn't make sense.
        if (strcmp(fs, "devfs") == 0 || strcmp(fs, "fdesc") == 0 || strcmp(fs, "autofs") == 0 || strcmp(fs, "dev") == 0 || strcmp(fs, "mnt") == 0)
          continue;

        // Skip this for now, until we can figure out the name of the root volume.
        if (strcmp(mount, "/") == 0)
          continue;

        result.push_back(mount);
#else
          // Ignore root
          if (strcmp(mount, "/") == 0)
            continue;
          // Here we choose wich filesystems are approved
          if (strcmp(fs, "fuseblk") == 0 || strcmp(fs, "vfat") == 0
              || strcmp(fs, "ext2") == 0 || strcmp(fs, "ext3") == 0
              || strcmp(fs, "reiserfs") == 0 || strcmp(fs, "xfs") == 0
              || strcmp(fs, "ntfs-3g") == 0 || strcmp(fs, "iso9660") == 0)
            result.push_back(mount);
#endif
          free(fs);
          free(mount);
        }
      }
      pclose(pipe);
    }
  }
  for (unsigned int i = 0; i < result.size(); i++)
  {
    CMediaSource share;
    share.strPath = result[i];
    share.strName = CUtil::GetFileName(result[i]);
    share.m_ignore = true;
    shares.push_back(share);
  }
#else //#ifdef HAS_HAL
  for (unsigned int i = 0; i < m_Devices.size(); i++)
  {
    if (m_Devices[i].Mounted && m_Devices[i].Approved)
    {
      if (DeviceType == NULL)
      {
        CMediaSource share;
        m_Devices[i].toMediaSource(&share);
        shares.push_back(share);
      }
      else
      {
        for (int j = 0; j < len; j++)
        {
          if (DeviceType[j] == m_Devices[i].Type)
          {
            CMediaSource share;
            m_Devices[i].toMediaSource(&share);
            shares.push_back(share);
          }
        }
      }
    }
  }
#endif
}

std::vector<CStdString> CLinuxFileSystem::GetDiskUsage()
{
  vector<CStdString> result;
  char line[1024];

#ifdef __APPLE__
  FILE* pipe = popen("df -hT ufs,cd9660,hfs,udf", "r");
#else
  FILE* pipe = popen("df -hx tmpfs", "r");
#endif

  if (pipe)
  {
    while (fgets(line, sizeof(line) - 1, pipe))
    {
      result.push_back(line);
    }
    pclose(pipe);
  }

  return result;
}

#ifdef HAS_HAL
/* Remove a device based on the UUID for the partition. Hal Cannot make a CStorageDevice from something removed that is why we need UUID */
bool CLinuxFileSystem::RemoveDevice(const char *UUID)
{
  CSingleLock lock(m_lock);
  int remove = -1;
  for (unsigned int i = 0; i < m_Devices.size(); i++)
  {
    if (strcmp(m_Devices[i].UUID.c_str(), UUID) == 0)
    {
      CLog::Log(LOGNOTICE, "LFS: Removed - %s | %s", CHalManager::StorageTypeToString(m_Devices[i].Type), m_Devices[i].FriendlyName.c_str());
      remove = i;
    }
  }
  if (remove != -1)
  {
    m_Devices.erase(m_Devices.begin() + remove);
    m_DeviceChange = true;
    return true;
  }
  else
  {
    CLog::Log(LOGWARNING, "LSF: Storage list inconsistancy detected. Rebuilding device list");
    UpdateDevices();
    return false;
  }
}

/* Add a device that LinuxFileSystem can use. Approved or not it is sent here. */
bool CLinuxFileSystem::AddDevice(CStorageDevice Device)
{
  CSingleLock lock(m_lock);
  int add = -1;
  for (unsigned int i = 0; i < m_Devices.size(); i++)
  {
    if (strcmp(m_Devices[i].UUID.c_str(), Device.UUID.c_str()) == 0)
    {
      add = i;
      break;
    }
  }

  if (add == -1)
  {
    CLog::Log(LOGNOTICE, "LFS: Added - %s | %s", CHalManager::StorageTypeToString(Device.Type), Device.FriendlyName.c_str());
    m_Devices.push_back(Device);
    m_DeviceChange = true;
  }
  else
  {
    CLog::Log(LOGNOTICE, "LFS: Updated - %s | %s", CHalManager::StorageTypeToString(Device.Type), Device.FriendlyName.c_str());
    m_Devices[add] = Device;
    m_DeviceChange = true;
  }

  return m_DeviceChange;
}

/* If any device have been added since the last call, this will return true */
bool CLinuxFileSystem::AnyDeviceChange()
{
  CSingleLock lock(m_lock);
  if (m_DeviceChange)
  {
    m_DeviceChange = false;
    return true;
  }
  else
    return false;
}
#endif // HAS_HAL
