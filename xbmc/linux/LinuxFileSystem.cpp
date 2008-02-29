#include "LinuxFileSystem.h"
#include "RegExp.h"
#ifdef HAS_HAL
#include "HalManager.h"
#endif

#include "SingleLock.h"

using namespace std;

#ifdef HAS_HAL
vector<CDevice> CLinuxFileSystem::m_Devices;
bool CLinuxFileSystem::m_DeviceChange;
CCriticalSection CLinuxFileSystem::m_lock;

/* This is never used */
void CLinuxFileSystem::UpdateDevices()
{
  CSingleLock lock(m_lock);
  m_Devices.clear();
  m_Devices = CHalManager::GetDevices();
}

/* Here can approved devices be choosen, this is always called for a device in HalManager before it's sent to adddevice. */
bool CLinuxFileSystem::ApproveDevice(CDevice *device)
{
  CSingleLock lock(m_lock);
  bool approve = true;
  //This is only because it's easier to read...
  const char *fs = device->FileSystem.c_str();

  if (strcmp(fs, "vfat") == 0 || strcmp(fs, "ext2") == 0 || strcmp(fs, "ext3") == 0 || strcmp(fs, "reiserfs") == 0 || strcmp(fs, "xfs") == 0 || strcmp(fs, "ntfs") == 0)
    approve = true;
  else
    approve = false;

  // Ignore root
  if (strcmp(device->MountPoint, "/") == 0)
    approve = false;

  if (device->Type == 3) //We don't approve CD/DVD it's handled elsewere
    approve = false;

  device->Approved = approve;
  return approve;
}
#endif //HAS_HAL

vector<CStdString> CLinuxFileSystem::GetDevices()
{
  return GetDevices(NULL, -1);
}

vector<CStdString> CLinuxFileSystem::GetRemovableDevices()
{
  CSingleLock lock(m_lock);
#ifndef HAS_HAL
  return GetDevices();
#else
  UpdateDevices();
  vector<CStdString> result;
  for (size_t i = 0; i < m_Devices.size(); i++)
  {
    if (m_Devices[i].Mounted && m_Devices[i].Approved && (m_Devices[i].Removable || m_Devices[i].HotPlugged))
    {
      result.push_back(m_Devices[i].MountPoint);
    }
  }
  return result;
#endif
}

/* if DeviceType == NULL we return all devices 
   To decide wich types types are approved to be returned send for example int DevTypes[] = {0, 5, 6, 7, 8, 9, 10, 13}.
   this will return all removable types like pendrives, memcards and such but NOT removable hdd's have type 1.
   for more information on these for now is in libhal-storage.h. TODO Make defined types that can be common on all O/S */
vector<CStdString> CLinuxFileSystem::GetDevices(int *DeviceType, int len)
{
  CSingleLock lock(m_lock);
  vector<CStdString> result;
#ifndef HAS_HAL
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
        if (strcmp(fs, "devfs") == 0 || strcmp(fs, "fdesc") == 0 || strcmp(fs, "autofs") == 0)
          continue;
              
        // Skip this for now, until we can figure out the name of the root volume.
        if (strcmp(mount, "/") == 0)
          continue;
#else 
	        // Ignore root 
	        if (strcmp(mount, "/") == 0) 
	          continue; 
	        // Here we choose wich filesystems are approved 
	        if (strcmp(fs, "fuseblk") == 0 || strcmp(fs, "vfat") == 0 || strcmp(fs, "ext2") == 0 || strcmp(fs, "ext3") == 0 || strcmp(fs, "reiserfs") == 0 || strcmp(fs, "xfs") == 0 || strcmp(fs, "ntfs-3g") == 0) 
	          result.push_back(mount); 
#endif 
          free(fs);
          free(mount);
        }
      }
      pclose(pipe);
    }
  }
#else //#ifdef HAS_HAL
  for (unsigned int i = 0; i < m_Devices.size(); i++)
  {
    if (m_Devices[i].Mounted && m_Devices[i].Approved)
    {
      if (DeviceType == NULL)
        result.push_back(m_Devices[i].MountPoint);
      else
      {
        for (int j = 0; j < len; j++)
        {
          if (DeviceType[j] == m_Devices[i].Type)
            result.push_back(m_Devices[i].MountPoint);
        }
      }
    }
  }
#endif
  return result;
}

#ifdef HAS_HAL
/* Remove a device based on the UUID for the partition. Hal Cannot make a CDevice from something removed that is why we need UUID */
bool CLinuxFileSystem::RemoveDevice(CStdString UUID)
{
  CSingleLock lock(m_lock);
  int remove = -1;
  for (unsigned int i = 0; i < m_Devices.size(); i++)
  {
    if (strcmp(m_Devices[i].UUID.c_str(), UUID.c_str()) == 0)
    {
      CLog::Log(LOGNOTICE, "LFS: Removed - %s", m_Devices[i].toString().c_str());
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
    return false;
}

/* Add a device that LinuxFileSystem can use. Approved or not it is sent here. */
bool CLinuxFileSystem::AddDevice(CDevice Device)
{
  CSingleLock lock(m_lock);
  bool add = true;
  for (unsigned int i = 0; i < m_Devices.size(); i++)
  {
    if (strcmp(m_Devices[i].UUID.c_str(), Device.UUID.c_str()) == 0)
      add = false;
  }

  if (add)
  {
    CLog::Log(LOGNOTICE, "LFS: Added - %s", Device.toString().c_str());
    m_Devices.push_back(Device);
    m_DeviceChange = true;
  }

  return add;
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

