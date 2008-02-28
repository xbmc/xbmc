#include "LinuxFileSystem.h"
#include "RegExp.h"
#ifdef HAS_HAL
#include "HalManager.h"
#endif

using namespace std;

#ifdef HAS_HAL
vector<CDevice> CLinuxFileSystem::m_Devices;
bool CLinuxFileSystem::m_DeviceChange;

/* This is never used */
void CLinuxFileSystem::UpdateDevices()
{
  m_Devices.clear();
  m_Devices = CHalManager::GetDevices();
}

/* Here can approved devices be choosen, this is always called for a device in HalManager before it's sent to adddevice. */
bool CLinuxFileSystem::ApproveDevice(CDevice *device)
{
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
/* if DeviceType == NULL we return all devices */
vector<CStdString> CLinuxFileSystem::GetDevices(int *DeviceType, int len)
{
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
  if (m_DeviceChange)
  {
    m_DeviceChange = false;
    return true;
  }
  else
    return false;
}
#endif // HAS_HAL
