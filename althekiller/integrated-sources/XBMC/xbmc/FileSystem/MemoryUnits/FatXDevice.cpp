#include "stdafx.h"
#include "FatXDevice.h"
#include "xbox/Undocumented.h"

typedef STRING OBJECT_STRING;
typedef PSTRING POBJECT_STRING;

#define IRP_MJ_READ                     0x02
#define IRP_MJ_WRITE                    0x03

struct FAT_VOLUME_HEADER {
    ULONG Signature;
    ULONG SerialNumber;
    ULONG SectorsPerCluster;  
    ULONG RootDirFirstCluster;
    WCHAR VolumeName[32];
};

#define FSCTL_DISMOUNT_VOLUME           CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  8, METHOD_BUFFERED, FILE_ANY_ACCESS)

CFatXDevice::CFatXDevice(unsigned long port, unsigned long slot, void *device)
:IDevice(port, slot, device)
{
  m_drive = 'H' + (char)((port << 1) | slot);
}

void CFatXDevice::LogInfo()
{
}

const char *CFatXDevice::GetFileSystem()
{
  return "FATX";
}

void CFatXDevice::UnMount()
{
  CLog::Log(LOGDEBUG, __FUNCTION__ " attempting unmount of drive %c:", m_drive);

  // grab the symbolic link
  char mountPoint[64];
  sprintf(mountPoint, "\\??\\%c:" , m_drive);
  STRING symLink =
    {
      strlen(mountPoint),
      strlen(mountPoint) + 1,
      mountPoint
    };

  // open our symbolic link to grab a handle on the filesystem
  // we want to dismount
  OBJECT_ATTRIBUTES obja;
  HANDLE handle;
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS status;

  InitializeObjectAttributes(
      &obja,
      (POBJECT_STRING)&symLink,
      OBJ_CASE_INSENSITIVE,
      NULL
      );

  CLog::Log(LOGDEBUG, __FUNCTION__ " opening symlink %c:", m_drive);
  status = NtCreateFile(
                  &handle,
                  SYNCHRONIZE,
                  &obja,
                  &IoStatusBlock,
                  NULL,
                  FILE_ATTRIBUTE_NORMAL,
                  0,
                  FILE_OPEN,
                  FILE_SYNCHRONOUS_IO_NONALERT
                  );

  // if successful, we close the filesystem
  if (NT_SUCCESS(status))
  {
    CLog::Log(LOGDEBUG, __FUNCTION__ " success opening symlink, unmounting");
    status = NtFsControlFile(handle, NULL, NULL, NULL, &IoStatusBlock, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0);
    CLog::Log(LOGDEBUG, __FUNCTION__ " filesystem dismount returned %08x", status);
    NtClose(handle);
  }

  CLog::Log(LOGDEBUG, __FUNCTION__ " Deleting symbolic link");
  // and now we unlink our symbolic link
  IoDeleteSymbolicLink(&symLink);
}

bool CFatXDevice::ReadVolumeName()
{
  LARGE_INTEGER StartingOffset;
  BYTE buffer[PAGE_SIZE];
  FAT_VOLUME_HEADER *VolumeHeader = (FAT_VOLUME_HEADER *)buffer;

  // the first sector contains the FATX drive information
  StartingOffset.QuadPart = 0;
  NTSTATUS Status = IoSynchronousFsdRequest(IRP_MJ_READ, (PDEVICE_OBJECT)m_device,
      VolumeHeader, PAGE_SIZE, &StartingOffset);

  if (NT_SUCCESS(Status))
  {
    // Check we have a FATX drive, and if so, read the metadata
    if (VolumeHeader->Signature == 'XTAF')
    {
      g_charsetConverter.wToUTF8(VolumeHeader->VolumeName, m_volumeName);
      return true;
    }
  }
  return false;
}

bool CFatXDevice::Mount(const char *device)
{
  char name[64];

  OBJECT_STRING DeviceName;
  DeviceName.Length = strlen(device)+1;
	DeviceName.MaximumLength = sizeof(name)/sizeof(CHAR)-1;
	DeviceName.Buffer = name;

  strcpy(name, device);

  //Add a back slash to so we can open the root directory
  name[DeviceName.Length - 1] = '\\';
  name[DeviceName.Length] = '\0';

  //Attempt to open the root directory (this effectively mounts the drive).
  OBJECT_ATTRIBUTES Obja;
  HANDLE DirHandle;
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;

  InitializeObjectAttributes(
      &Obja,
      (POBJECT_STRING)&DeviceName,
      OBJ_CASE_INSENSITIVE,
      NULL
      );

  CLog::Log(LOGDEBUG, __FUNCTION__" Attempting to open as FATX volume");
  Status = NtCreateFile(
              &DirHandle,
              FILE_LIST_DIRECTORY | SYNCHRONIZE,
              &Obja,
              &IoStatusBlock,
              NULL,
              FILE_ATTRIBUTE_NORMAL,
              FILE_SHARE_READ | FILE_SHARE_WRITE,
              FILE_OPEN_IF,
              FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
              );

  if (NT_SUCCESS(Status))
  { // yes, we have a fatx volume
    CLog::Log(LOGDEBUG, __FUNCTION__" Success opening FATX volume");
    NtClose(DirHandle);  //Close the handle, we no longer need it

    // attempt to create the symbolic link
    // we need this so that we can just use the built in filesystem stuff.

    // only way around it would be to use the NtQueryFile() stuff, which is
    // much uglier than FindFirstFile() et. al.

    char szDosDevice[64];
    sprintf(szDosDevice, "\\??\\%c:" , m_drive);
    STRING DosDevice =
      {
        strlen(szDosDevice),
        strlen(szDosDevice) + 1,
        szDosDevice
      };
    DeviceName.Length--;
    name[DeviceName.Length] = '\0';
    Status = IoCreateSymbolicLink(&DosDevice, &DeviceName);

    if (NT_SUCCESS(Status))
    {
      if (!ReadVolumeName())
      {
        UnMount();
        return false;
      }
      return true;
    }
  }
  return false;
}
