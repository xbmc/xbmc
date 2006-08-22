#include "../stdafx.h"

#include "../xbox/Undocumented.h"
#include "MemoryUnitManager.h"
#include "../FileSystem/MemoryUnits/FatXDevice.h"
#include "../FileSystem/MemoryUnits/FatXFileSystem.h"

// Undocumented stuff

typedef STRING OBJECT_STRING;
typedef PSTRING POBJECT_STRING;

#define IRP_MJ_READ                     0x02

#define FAT_VOLUME_NAME_LENGTH          32
#define FAT_ONLINE_DATA_LENGTH          2048

extern "C"
{
	XBOXAPI NTSTATUS WINAPI 
MU_CreateDeviceObject(
    IN  ULONG            Port,
    IN  ULONG            Slot,
    IN  POBJECT_STRING  DeviceName
    );

	XBOXAPI VOID WINAPI 
MU_CloseDeviceObject(
    IN  ULONG  Port,
    IN  ULONG  Slot
    );

	XBOXAPI PDEVICE_OBJECT WINAPI
MU_GetExistingDeviceObject(
    IN  ULONG  Port,
    IN  ULONG  Slot
    );

	XBOXAPI
NTSTATUS WINAPI
IoSynchronousFsdRequest(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL
    );

XBOXAPI
DWORD
WINAPI
XMountMURootA(
   IN DWORD dwPort,
   IN DWORD dwSlot,
   OUT PCHAR pchDrive
   );
};

typedef struct _PARTITION_INFORMATION {
    LARGE_INTEGER StartingOffset;
    LARGE_INTEGER PartitionLength;
    ULONG HiddenSectors;
    ULONG PartitionNumber;
    UCHAR PartitionType;
    BOOLEAN BootIndicator;
    BOOLEAN RecognizedPartition;
    BOOLEAN RewritePartition;
} PARTITION_INFORMATION, *PPARTITION_INFORMATION;

typedef struct _DISK_GEOMETRY {
    LARGE_INTEGER Cylinders;
    DWORD MediaType;
    DWORD TracksPerCylinder;
    DWORD SectorsPerTrack;
    DWORD BytesPerSector;
} DISK_GEOMETRY, *PDISK_GEOMETRY;

#define IOCTL_DISK_BASE                 FILE_DEVICE_DISK
#define IOCTL_DISK_GET_DRIVE_GEOMETRY   CTL_CODE(IOCTL_DISK_BASE, 0x0000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_GET_PARTITION_INFO   CTL_CODE(IOCTL_DISK_BASE, 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS)

CMemoryUnitManager g_memoryUnitManager;

CMemoryUnitManager::CMemoryUnitManager()
{
  m_initialized = false;
}

bool CMemoryUnitManager::Update()
{
  DWORD newdev = 0;
  DWORD deaddev = 0;
  if (!m_initialized)
  {
    newdev = XGetDevices(XDEVICE_TYPE_MEMORY_UNIT);
    m_initialized = true;
  }
  else if (!XGetDeviceChanges(XDEVICE_TYPE_MEMORY_UNIT, &newdev, &deaddev))
    return false;

  if (!newdev && !deaddev)
    return false;

  CLog::Log(LOGDEBUG, __FUNCTION__" Reports added device %08x and removed device %08x", newdev, deaddev);

  if (deaddev && deaddev != newdev)
    UnMountUnits(deaddev);
  if (newdev && newdev != deaddev)
    MountUnits(newdev);
  return (newdev || deaddev);
}

void CMemoryUnitManager::UnMountUnits(unsigned long device)
{
  CLog::Log(LOGNOTICE, "Attempting to unmount memory unit device %08x", device);
  for (DWORD i = 0; i < 4; ++i)
  {
    if (device & (1 << i))
    {
      if (HasDevice(XDEVICE_PORT0 + i, XDEVICE_TOP_SLOT))
      {
        UnMountDevice(XDEVICE_PORT0 + i, XDEVICE_TOP_SLOT);
      }
    }
    if (device & (1 << (i + 16)))
    {
      if (HasDevice(XDEVICE_PORT0 + i, XDEVICE_BOTTOM_SLOT))
      {
        UnMountDevice(XDEVICE_PORT0 + i, XDEVICE_BOTTOM_SLOT);
      }
    }
  }
}

void CMemoryUnitManager::MountUnits(unsigned long device)
{
  CLog::Log(LOGNOTICE, "Attempting to mount memory unit device %08x", device);
  for (DWORD i = 0; i < 4; ++i)
  {
    if (device & (1 << i))
    {
      MountDevice(XDEVICE_PORT0 + i, XDEVICE_TOP_SLOT);
    }
    if (device & (1 << (i + 16)))
    {
      MountDevice(XDEVICE_PORT0 + i, XDEVICE_BOTTOM_SLOT);
    }
  }
}

bool CMemoryUnitManager::IsDriveValid(char Drive)
{
  for (unsigned int i = 0; i < m_memUnits.size(); i++)
  {
    IDevice *device = m_memUnits[i];
    if (strcmpi(device->GetFileSystem(), "fatx") == 0)
    {
      if (((CFatXDevice *)device)->GetDrive() == Drive)
        return true;
    }
  }
  return false;
}

bool CMemoryUnitManager::HasDevice(unsigned long port, unsigned long slot)
{
  for (unsigned int i = 0; i < m_memUnits.size(); i++)
    if (m_memUnits[i]->IsInPort(port, slot))
      return true;
  return false;
}

bool CMemoryUnitManager::MountDevice(unsigned long port, unsigned long slot)
{
  // check whether we already have one mounted here...
  if (HasDevice(port, slot))
  {
    CLog::Log(LOGERROR, __FUNCTION__" Attempt to mount already mounted MU device");
    return false;
  }

  NTSTATUS Status;
	CHAR szDeviceName[64];
	OBJECT_STRING DeviceName;
	PDEVICE_OBJECT DeviceObject;

	// setup the string buffer - leave room for NULL and '\\'
	DeviceName.Length = 0;
	DeviceName.MaximumLength = sizeof(szDeviceName)/sizeof(CHAR)-2;
	DeviceName.Buffer = szDeviceName;

	// create the device object
  CLog::Log(LOGDEBUG, __FUNCTION__" Creating MU device (port %i, slot %i)", port, slot);
	Status = MU_CreateDeviceObject(port, slot, &DeviceName);

	if(NT_SUCCESS(Status))
	{
    CLog::Log(LOGDEBUG, __FUNCTION__" Getting MU device (port %i, slot %i)", port, slot);
		DeviceObject = MU_GetExistingDeviceObject(port, slot);

    CFatXDevice *device = new CFatXDevice(port, slot, DeviceObject);
    if (device->Mount(DeviceName.Buffer))
    {
      m_memUnits.push_back(device);
      return true;
    }
    delete device;
  }
  return false;
}

void CMemoryUnitManager::UnMountDevice(unsigned long port, unsigned long slot)
{
  CLog::Log(LOGDEBUG, __FUNCTION__" Unmounting MU device (port %i, slot %i)", port, slot);
  for (vector<IDevice *>::iterator it = m_memUnits.begin(); it != m_memUnits.end(); ++it)
  {
    IDevice *device = *it;
    if (device->IsInPort(port, slot))
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" Found MU device to unmount (port %i, slot %i)", port, slot);
      device->UnMount();
      CLog::Log(LOGDEBUG, __FUNCTION__" Closing device object (port %i, slot %i)", port, slot);
    	MU_CloseDeviceObject(port, slot);
      CLog::Log(LOGDEBUG, __FUNCTION__" Device object is closed");
      delete device;
      CLog::Log(LOGDEBUG, __FUNCTION__" Device is deleted");
      m_memUnits.erase(it);
      return;
    }
  }
}

IDevice *CMemoryUnitManager::GetDevice(unsigned char unit)
{
  if (unit >= m_memUnits.size())
  {
    CLog::Log(LOGERROR, __FUNCTION__" Attempt to access memory device %i when it doesn't exist", unit);
    return NULL;
  }
  return m_memUnits[unit];
}

bool CMemoryUnitManager::IsDriveWriteable(const CStdString &path)
{
  CURL url(path);
  IDevice *device = GetDevice(url.GetProtocol()[3] - '0');
  if (device && strcmpi(device->GetFileSystem(), "fatx"))
    return true;
  return false;
}

void CMemoryUnitManager::GetMemoryUnitShares(VECSHARES &shares)
{
  for (unsigned int i = 0; i < m_memUnits.size(); i++)
  {
    CShare share;
    CStdString volumeName = m_memUnits[i]->GetVolumeName();
    volumeName.TrimRight(' ');
    if (volumeName.IsEmpty())
      share.strName.Format("%s %i (%s)", "Memory Unit", i + 1, m_memUnits[i]->GetFileSystem());
    else
      share.strName.Format("%s %i (%s) (%s)", "Memory Unit", i + 1, volumeName.c_str(), m_memUnits[i]->GetFileSystem());
    share.strPath.Format("mem%i://", i);
    shares.push_back(share);
  }
}

IFileSystem *CMemoryUnitManager::GetFileSystem(unsigned char unit)
{
  IDevice *device = GetDevice(unit);
  if (!device)
    return NULL;
  return new CFatXFileSystem(unit);
}