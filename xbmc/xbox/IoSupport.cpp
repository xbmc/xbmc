/*
* XBoxMediaPlayer
* Copyright (c) 2002 d7o3g4q and RUNTiME
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// IoSupport.cpp: implementation of the CIoSupport class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "IoSupport.h"
#ifdef HAS_UNDOCUMENTED
#ifdef _XBOX
#include "Undocumented.h"
#include "../xbox/XKExports.h"
#else
#include "ntddcdrm.h"
#endif
#endif
#ifdef _LINUX
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/cdrom.h>
#include "../FileSystem/cdioSupport.h"
#endif

#define NT_STATUS_OBJECT_NAME_NOT_FOUND long(0xC0000000 | 0x0034)
#define NT_STATUS_VOLUME_DISMOUNTED     long(0xC0000000 | 0x026E)

typedef struct
{
  char cDriveLetter;
  char* szDevice;
  int iPartition;
}
stDriveMapping;

#ifdef _XBOX
stDriveMapping driveMapping[] =
  {
    { 'C', "Harddisk0\\Partition2", 2},
    { 'D', "Cdrom0", -1},
    { 'E', "Harddisk0\\Partition1", 1},
    { 'X', "Harddisk0\\Partition3", 3},
    { 'Y', "Harddisk0\\Partition4", 4},
    { 'Z', "Harddisk0\\Partition5", 5},
  };
#elif defined(WIN32)
stDriveMapping driveMapping[] =
  {
    { 'C', "C:", 2},
    { 'D', "D:", -1},
    { 'E', "E:", 1},
    { 'X', "X:", 3},
    { 'Y', "Y:", 4},
    { 'Z', "Z:", 5},
  };
#include "../../Tools/Win32/XBMC_PC.h"

#else
stDriveMapping driveMapping[] =
  {
     {'P', new char[MAX_PATH], 0},
     {'Q', new char[MAX_PATH], 0},
     {'T', new char[MAX_PATH], 0},
     {'Z', new char[MAX_PATH], 0},
     {'C', new char[MAX_PATH], 0},
     {'E', new char[MAX_PATH], 0}
  };
#endif

#define NUM_OF_DRIVES ( sizeof( driveMapping) / sizeof( driveMapping[0] ) )


PVOID CIoSupport::m_rawXferBuffer;
PARTITION_TABLE CIoSupport::m_partitionTable;
bool CIoSupport::m_fPartitionTableIsValid;


// cDriveLetter e.g. 'D'
// szDevice e.g. "Cdrom0" or "Harddisk0\Partition6"
HRESULT CIoSupport::MapDriveLetter(char cDriveLetter, char * szDevice)
{
#ifdef _XBOX
  char szSourceDevice[MAX_PATH+32];
  char szDestinationDrive[16];
  NTSTATUS status;

  CLog::Log(LOGNOTICE, "Mapping drive %c to %s", cDriveLetter, szDevice);

  sprintf(szSourceDevice, "\\Device\\%s", szDevice);
  sprintf(szDestinationDrive, "\\??\\%c:", cDriveLetter);

  ANSI_STRING DeviceName, LinkName;

  RtlInitAnsiString(&DeviceName, szSourceDevice);
  RtlInitAnsiString(&LinkName, szDestinationDrive);

  status = IoCreateSymbolicLink(&LinkName, &DeviceName);

  if (!NT_SUCCESS(status))
    CLog::Log(LOGERROR, "Failed to create symbolic link!  (status=0x%08x)", status);

  return status;
#elif WIN32
  if ((strnicmp(szDevice, "Harddisk0", 9) == 0) ||
      (strnicmp(szDevice, "Cdrom", 5) == 0))
    return S_OK;
  return E_FAIL;
#else
  char upperLetter = toupper(cDriveLetter);
  for (unsigned int i=0; i < NUM_OF_DRIVES; i++)
    if (driveMapping[i].cDriveLetter == upperLetter)
    {
      strcpy(driveMapping[i].szDevice, szDevice);
      CLog::Log(LOGNOTICE, "Mapping drive %c to %s", cDriveLetter, szDevice);
      return S_OK;
    }
  return E_FAIL;
#endif
}

// cDriveLetter e.g. 'D'
HRESULT CIoSupport::UnmapDriveLetter(char cDriveLetter)
{
#ifdef _XBOX
  char szDestinationDrive[16];
  ANSI_STRING LinkName;
  NTSTATUS status;

  sprintf(szDestinationDrive, "\\??\\%c:", cDriveLetter);
  RtlInitAnsiString(&LinkName, szDestinationDrive);

  status =  IoDeleteSymbolicLink(&LinkName);

  if (NT_SUCCESS(status))
    CLog::Log(LOGNOTICE, "Unmapped drive %c", cDriveLetter);
  else if(status != NT_STATUS_OBJECT_NAME_NOT_FOUND)
    CLog::Log(LOGERROR, "Failed to delete symbolic link!  (status=0x%08x)", status);

  return status;
#else
  return S_OK;
#endif
}

HRESULT CIoSupport::RemapDriveLetter(char cDriveLetter, char * szDevice)
{
  UnmapDriveLetter(cDriveLetter);

  return MapDriveLetter(cDriveLetter, szDevice);
}
// to be used with CdRom devices.
HRESULT CIoSupport::Dismount(char * szDevice)
{
#ifdef _XBOX
  char szSourceDevice[MAX_PATH+32];
  ANSI_STRING DeviceName;
  NTSTATUS status;

  sprintf(szSourceDevice, "\\Device\\%s", szDevice);

  RtlInitAnsiString(&DeviceName, szSourceDevice);

  status = IoDismountVolumeByName(&DeviceName);

  if (NT_SUCCESS(status))
    CLog::Log(LOGNOTICE, "Dismounted %s", szDevice);
  else if(status != NT_STATUS_VOLUME_DISMOUNTED)
    CLog::Log(LOGERROR, "Failed to dismount volume!  (status=0x%08x)", status);

  return status;
#else
  return S_OK;
#endif
}

void CIoSupport::GetPartition(char cDriveLetter, char * szPartition)
{
  char upperLetter = toupper(cDriveLetter);
  if (upperLetter >= 'F' && upperLetter <= 'O')
  {
    sprintf(szPartition, "Harddisk0\\Partition%u", upperLetter - 'A' + 1);
    return;
  }
  for (unsigned int i=0; i < NUM_OF_DRIVES; i++)
    if (driveMapping[i].cDriveLetter == upperLetter)
    {
      strcpy(szPartition, driveMapping[i].szDevice);
      return;
    }
  *szPartition = 0;
}

const char* CIoSupport::GetPartition(char cDriveLetter)
{
  char upperLetter = toupper(cDriveLetter);
  for (unsigned int i=0; i < NUM_OF_DRIVES; i++)
    if (driveMapping[i].cDriveLetter == upperLetter)
      return driveMapping[i].szDevice;
  return NULL;
}

void CIoSupport::GetDrive(char * szPartition, char * cDriveLetter)
{
  int part_str_len = strlen(szPartition);
  int part_num;

  if (part_str_len < 19)
  {
    *cDriveLetter = 0;
    return;
  }

  part_num = atoi(szPartition + 19);

  if (part_num >= 6)
  {
    *cDriveLetter = part_num + 'A' - 1;
    return;
  }
  for (unsigned int i=0; i < NUM_OF_DRIVES; i++)
    if (strnicmp(driveMapping[i].szDevice, szPartition, strlen(driveMapping[i].szDevice)) == 0)
    {
      *cDriveLetter = driveMapping[i].cDriveLetter;
      return;
    }
  *cDriveLetter = 0;
}

HRESULT CIoSupport::EjectTray()
{
#ifdef _XBOX
  HalWriteSMBusValue(0x20, 0x0C, FALSE, 0);  // eject tray
#endif
  return S_OK;
}

HRESULT CIoSupport::CloseTray()
{
#ifdef _XBOX
  HalWriteSMBusValue(0x20, 0x0C, FALSE, 1);  // close tray
#endif
  return S_OK;
}

DWORD CIoSupport::GetTrayState()
{
#ifdef _XBOX
  DWORD dwTrayState, dwTrayCount;
  if (g_advancedSettings.m_usePCDVDROM)
  {
    dwTrayState = TRAY_CLOSED_MEDIA_PRESENT;
  }
  else
  {
    HalReadSMCTrayState(&dwTrayState, &dwTrayCount);
  }

  return dwTrayState;
#endif
  return DRIVE_NOT_READY;
}

HRESULT CIoSupport::Shutdown()
{
#ifdef _XBOX
  // fails assertion on debug bios (symptom lockup unless running dr watson)
  // so you can continue past the failed assertion).
  if (IsDebug())
    return E_FAIL;
  KeRaiseIrqlToDpcLevel();
  HalInitiateShutdown();
#endif
  return S_OK;
}

HANDLE CIoSupport::OpenCDROM()
{
  HANDLE hDevice;

#ifdef _XBOX
  IO_STATUS_BLOCK status;
  ANSI_STRING filename;
  OBJECT_ATTRIBUTES attributes;
  RtlInitAnsiString(&filename, "\\Device\\Cdrom0");
  InitializeObjectAttributes(&attributes, &filename, OBJ_CASE_INSENSITIVE, NULL);
  if (!NT_SUCCESS(NtOpenFile(&hDevice,
                             GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                             &attributes,
                             &status,
                             FILE_SHARE_READ,
                             FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT)))
  {
    return NULL;
  }
#elif defined(_LINUX)
  hDevice = CreateFile(MEDIA_DETECT::CCdIoSupport::GetDeviceFileName(), 
                       GENERIC_READ, FILE_SHARE_READ,
                       NULL, OPEN_EXISTING,
                       FILE_FLAG_RANDOM_ACCESS, NULL );
#else

  hDevice = CreateFile("\\\\.\\Cdrom0", GENERIC_READ, FILE_SHARE_READ,
                       NULL, OPEN_EXISTING,
                       FILE_FLAG_RANDOM_ACCESS, NULL );

#endif
  return hDevice;
}

void CIoSupport::AllocReadBuffer()
{
#ifndef _LINUX
  m_rawXferBuffer = GlobalAlloc(GPTR, RAW_SECTOR_SIZE);
#endif
}

void CIoSupport::FreeReadBuffer()
{
#ifndef _LINUX
  GlobalFree(m_rawXferBuffer);
#endif
}

INT CIoSupport::ReadSector(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)

{
  DWORD dwRead;
  DWORD dwSectorSize = 2048;

#ifdef _LINUX
  if (hDevice->m_bCDROM)
  {    
    int fd = hDevice->fd;
    int lba = (dwSector + CD_MSF_OFFSET) ;
    int m,s,f;
    union 
    {
      struct cdrom_msf msf;
      char buffer[2356];
    } arg;

    // convert sector offset to minute, second, frame format
    // since that is what the 'ioctl' requires as input
    f = lba % CD_FRAMES;
    lba /= CD_FRAMES;
    s = lba % CD_SECS;
    lba /= CD_SECS;
    m = lba;

    arg.msf.cdmsf_min0 = m;
    arg.msf.cdmsf_sec0 = s;
    arg.msf.cdmsf_frame0 = f;
    
    int ret = ioctl(fd, CDROMREADMODE1, &arg);
    if (ret==0)
    {
      memcpy(lpczBuffer, arg.buffer, 2048);
      return 2048;
    }
    CLog::Log(LOGERROR, "CD: ReadSector Request to read sector %d\n", (int)dwSector);
    CLog::Log(LOGERROR, "CD: ReadSector error: %s\n", strerror(errno));
    CLog::Log(LOGERROR, "CD: ReadSector minute %d, second %d, frame %d\n", m, s, f);
    OutputDebugString("CD Read error\n");
    return -1;    
  }
#endif
  LARGE_INTEGER Displacement;
  Displacement.QuadPart = ((INT64)dwSector) * dwSectorSize;

  for (int i = 0; i < 5; i++)
  {
    SetFilePointer(hDevice, Displacement.LowPart, &Displacement.HighPart, FILE_BEGIN);

    if (ReadFile(hDevice, m_rawXferBuffer, dwSectorSize, &dwRead, NULL))
    {
      memcpy(lpczBuffer, m_rawXferBuffer, dwSectorSize);
      return dwRead;
    }
  }

  OutputDebugString("CD Read error\n");
  return -1;
}


INT CIoSupport::ReadSectorMode2(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
#ifdef HAS_DVD_DRIVE
#ifdef _LINUX
  if (hDevice->m_bCDROM)
  {    
    int fd = hDevice->fd;
    int lba = (dwSector + CD_MSF_OFFSET) ;
    int m,s,f;
    union 
    {
      struct cdrom_msf msf;
      char buffer[2356];
    } arg;

    // convert sector offset to minute, second, frame format
    // since that is what the 'ioctl' requires as input
    f = lba % CD_FRAMES;
    lba /= CD_FRAMES;
    s = lba % CD_SECS;
    lba /= CD_SECS;
    m = lba;

    arg.msf.cdmsf_min0 = m;
    arg.msf.cdmsf_sec0 = s;
    arg.msf.cdmsf_frame0 = f;
    
    int ret = ioctl(fd, CDROMREADMODE2, &arg);
    if (ret==0)
    {
      memcpy(lpczBuffer, arg.buffer, MODE2_DATA_SIZE); // don't think offset is needed here
      return MODE2_DATA_SIZE;
    }
    CLog::Log(LOGERROR, "CD: ReadSectorMode2 Request to read sector %d\n", (int)dwSector);
    CLog::Log(LOGERROR, "CD: ReadSectorMode2 error: %s\n", strerror(errno));
    CLog::Log(LOGERROR, "CD: ReadSectorMode2 minute %d, second %d, frame %d\n", m, s, f);
    OutputDebugString("CD Read error\n");
    return -1;    
  }
#else
  DWORD dwBytesReturned;
  RAW_READ_INFO rawRead = {0};

  // Oddly enough, DiskOffset uses the Red Book sector size
  rawRead.DiskOffset.QuadPart = 2048 * dwSector;
  rawRead.SectorCount = 1;
  rawRead.TrackMode = XAForm2;


  for (int i = 0; i < 5; i++)
  {
    if ( DeviceIoControl( hDevice,
                          IOCTL_CDROM_RAW_READ,
                          &rawRead,
                          sizeof(RAW_READ_INFO),
                          m_rawXferBuffer,
                          RAW_SECTOR_SIZE,
                          &dwBytesReturned,
                          NULL ) != 0 )
    {
      memcpy(lpczBuffer, (char*)m_rawXferBuffer+MODE2_DATA_START, MODE2_DATA_SIZE);
      return MODE2_DATA_SIZE;
    }
    else
    {
      int iErr = GetLastError();
      //   printf("%i\n", iErr);
    }
  }
#endif
#endif
  return -1;
}

INT CIoSupport::ReadSectorCDDA(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
#ifdef HAS_DVD_DRIVE
#ifndef _LINUX
  DWORD dwBytesReturned;
  RAW_READ_INFO rawRead;

  // Oddly enough, DiskOffset uses the Red Book sector size
  rawRead.DiskOffset.QuadPart = 2048 * dwSector;
  rawRead.SectorCount = 1;
  rawRead.TrackMode = CDDA;

  for (int i = 0; i < 5; i++)
  {
    if ( DeviceIoControl( hDevice,
                          IOCTL_CDROM_RAW_READ,
                          &rawRead,
                          sizeof(RAW_READ_INFO),
                          m_rawXferBuffer,
                          sizeof(RAW_SECTOR_SIZE),
                          &dwBytesReturned,
                          NULL ) != 0 )
    {
      memcpy(lpczBuffer, m_rawXferBuffer, RAW_SECTOR_SIZE);
      return RAW_SECTOR_SIZE;
    }
  }
#endif
#endif
  return -1;
}

VOID CIoSupport::CloseCDROM(HANDLE hDevice)
{
  CloseHandle(hDevice);
}

// returns true if this is a debug machine
BOOL CIoSupport::IsDebug()
{
#ifdef _XBOX
  return (XboxKrnlVersion->Qfe & 0x8000) || ((DWORD)XboxHardwareInfo & 0x10);
#else
  return FALSE;
#endif
}


VOID CIoSupport::GetXbePath(char* szDest)
{
#ifdef _XBOX
  //Function to get the XBE Path like:
  //E:\DevKit\xbplayer\xbplayer.xbe

  char szTemp[MAX_PATH];
  char cDriveLetter = 0;

  strncpy(szTemp, XeImageFileName->Buffer + 8, XeImageFileName->Length - 8);
  szTemp[20] = 0;
  GetDrive(szTemp, &cDriveLetter);

  strncpy(szTemp, XeImageFileName->Buffer + 29, XeImageFileName->Length - 29);
  szTemp[XeImageFileName->Length - 29] = 0;

  sprintf(szDest, "%c:\\%s", cDriveLetter, szTemp);

#elif WIN32
  GetCurrentDirectory(XBMC_MAX_PATH, szDest);
  strcat(szDest, "\\XBMC_PC.exe");
#else
  /* Get our PID and build the name of the link in /proc */
  pid_t pid = getpid();
  char linkname[64]; /* /proc/<pid>/exe */
  snprintf(linkname, sizeof(linkname), "/proc/%i/exe", pid);

  /* Now read the symbolic link */
  char buf[1024];
  int ret = readlink(linkname, buf, 1024);
  buf[ret] = 0;
	
  strcpy(szDest, buf);
#endif
}

bool CIoSupport::DriveExists(char cDriveLetter)
{
  cDriveLetter = toupper(cDriveLetter);
#ifdef _XBOX
  // new kernel detection method
  if (m_fPartitionTableIsValid)
  {
    char szDrive[32];
    ANSI_STRING drive_string;
    NTSTATUS status;
    HANDLE hTemp;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;

    sprintf(szDrive, "\\??\\%c:", cDriveLetter);
    RtlInitAnsiString(&drive_string, szDrive);

    oa.Attributes = OBJ_CASE_INSENSITIVE;
    oa.ObjectName = &drive_string;
    oa.RootDirectory = 0;

    status = NtOpenFile(&hTemp, GENERIC_READ | GENERIC_WRITE, &oa, &iosb, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_ALERT);

    if (NT_SUCCESS(status))
    {
      CloseHandle(hTemp);
      return true;
    }

    return false;
  }
  else
  {
    LARGE_INTEGER drive_size = GetDriveSize();
    // old kernel detection method
    if (cDriveLetter == 'F')
    {
      // if the drive is bigger than the old 8gb drive (plus a bit of room for error),
      // the F drive can exist
      if (drive_size.QuadPart >= 9000000000)
        return true;
    }

    if (cDriveLetter == 'G')
    {
      // if the kernel is set to use partitions 6 and 7 by default
      // the g drive can exist
      if(((XboxKrnlVersion->Qfe & 67) == 67))
        return true;
      // not all kernel versions return 67, if the drive is bigger than
      // 137 gb drive (plus a bit of room for error), the G drive can exist
      else if ( drive_size.QuadPart >= 150000000000 )
        return true;
    }

    return false;
  }
#elif defined(WIN32)
  if (cDriveLetter < 'A' || cDriveLetter > 'Z')
    return false;

  DWORD drivelist;
  DWORD bitposition = cDriveLetter - 'A';

  drivelist = GetLogicalDrives();

  if (!drivelist)
    return false;

  return (drivelist >> bitposition) & 1;
#else
  return false;
#endif
}

bool CIoSupport::PartitionExists(int nPartition)
{
#ifdef _XBOX
  char szPartition[32];
  ANSI_STRING part_string;
  NTSTATUS status;
  HANDLE hTemp;
  OBJECT_ATTRIBUTES oa;
  IO_STATUS_BLOCK iosb;

  sprintf(szPartition, "\\Device\\Harddisk0\\Partition%u", nPartition);
  RtlInitAnsiString(&part_string, szPartition);

  oa.Attributes = OBJ_CASE_INSENSITIVE;
  oa.ObjectName = &part_string;
  oa.RootDirectory = 0;

  status = NtOpenFile(&hTemp, GENERIC_READ | GENERIC_WRITE, &oa, &iosb, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_ALERT);

  if (NT_SUCCESS(status))
  {
    CloseHandle(hTemp);
    return true;
  }

  return false;
#else
  return false;
#endif
}

LARGE_INTEGER CIoSupport::GetDriveSize()
{
  LARGE_INTEGER drive_size;
#ifdef _XBOX
  HANDLE hDevice;
  char szHardDrive[32] = "\\Device\\Harddisk0\\Partition0";
  ANSI_STRING hd_string;
  OBJECT_ATTRIBUTES oa;
  IO_STATUS_BLOCK iosb;
  DISK_GEOMETRY disk_geometry;
  NTSTATUS status;

  RtlInitAnsiString(&hd_string, szHardDrive);
  drive_size.QuadPart = 0;

  oa.Attributes = OBJ_CASE_INSENSITIVE;
  oa.ObjectName = &hd_string;
  oa.RootDirectory = 0;

  status = NtOpenFile(&hDevice, GENERIC_READ | GENERIC_WRITE, &oa, &iosb, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_ALERT);
  if (!NT_SUCCESS(status))
    return drive_size;

  status = NtDeviceIoControlFile(hDevice, NULL, NULL, NULL, &iosb,
        IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &disk_geometry, sizeof(disk_geometry));

  CloseHandle(hDevice);

  if (!NT_SUCCESS(status))
    return drive_size;

  drive_size.QuadPart = disk_geometry.BytesPerSector *
                        disk_geometry.SectorsPerTrack *
                        disk_geometry.TracksPerCylinder *
                        disk_geometry.Cylinders.QuadPart;

  return drive_size;
#endif
  drive_size.QuadPart = 0;
  return drive_size;
}

bool CIoSupport::ReadPartitionTable(){

#ifdef _XBOX
  unsigned int retval;

  retval = ReadPartitionTable(&m_partitionTable);
  if (retval == STATUS_SUCCESS)
  {
    m_fPartitionTableIsValid = true;
    return true;
  }
  else
  {
    m_fPartitionTableIsValid = false;
    return false;
  }
#else
  return false;
#endif
}

bool CIoSupport::HasPartitionTable()
{
  return m_fPartitionTableIsValid;
}

void CIoSupport::MapExtendedPartitions()
{
#ifdef _XBOX
  if (!m_fPartitionTableIsValid)
    return;
  char szDevice[32] = "\\Harddisk0\\Partition0";
  char driveletter;
  // we start at 5 - the first 5 partitions are the mandatory standard Xbox partitions
  // we don't deal with those here.
  for (int i = EXTEND_PARTITION_BEGIN; i <= EXTEND_PARTITION_END; i++)
  {
    if (m_partitionTable.pt_entries[i - 1].pe_flags & PE_PARTFLAGS_IN_USE)
    {
      driveletter = 'A' + i - 1;
      CLog::Log(LOGINFO, "  map drive %c:", driveletter);
      szDevice[20] = '1' + i - 1;
      MapDriveLetter(driveletter, szDevice);
    }
  }
#endif
}

unsigned int CIoSupport::ReadPartitionTable(PARTITION_TABLE *p_table)
{
#ifdef _XBOX
  ANSI_STRING a_file;
  OBJECT_ATTRIBUTES obj_attr;
  IO_STATUS_BLOCK io_stat_block;
  HANDLE handle;
  unsigned int stat;
  unsigned int ioctl_cmd_in_buf[100];
  unsigned int ioctl_cmd_out_buf[100];
  unsigned int partition_table_addr;

  memset(p_table, 0, sizeof(PARTITION_TABLE));

  RtlInitAnsiString(&a_file, "\\Device\\Harddisk0\\partition0");
  obj_attr.RootDirectory = 0;
  obj_attr.ObjectName = &a_file;
  obj_attr.Attributes = OBJ_CASE_INSENSITIVE;

  stat = NtOpenFile(&handle, (GENERIC_READ | 0x00100000), &obj_attr, &io_stat_block, (FILE_SHARE_READ | FILE_SHARE_WRITE), 0x10);

  if (stat != STATUS_SUCCESS)
  {
    return stat;
  }

  memset(ioctl_cmd_out_buf, 0, sizeof(ioctl_cmd_out_buf));
  memset(ioctl_cmd_in_buf, 0, sizeof(ioctl_cmd_in_buf));
  ioctl_cmd_in_buf[0] = IOCTL_SUBCMD_GET_INFO;


  stat = NtDeviceIoControlFile(handle, 0, 0, 0, &io_stat_block,
                               IOCTL_CMD_LBA48_ACCESS,
                               ioctl_cmd_in_buf, sizeof(ioctl_cmd_in_buf),
                               ioctl_cmd_out_buf, sizeof(ioctl_cmd_out_buf));

  NtClose(handle);
  if (stat != STATUS_SUCCESS)
  {
    return stat;
  }

  if ((ioctl_cmd_out_buf[LBA48_GET_INFO_MAGIC1_IDX] != LBA48_GET_INFO_MAGIC1_VAL) ||
      (ioctl_cmd_out_buf[LBA48_GET_INFO_MAGIC2_IDX] != LBA48_GET_INFO_MAGIC2_VAL))
  {

    return STATUS_UNSUCCESSFUL;
  }

  partition_table_addr = ioctl_cmd_out_buf[LBA48_GET_INFO_LOWCODE_BASE_IDX];
  partition_table_addr += ioctl_cmd_out_buf[LBA48_GET_INFO_PART_TABLE_OFS_IDX];

  memcpy(p_table, (void *)partition_table_addr, sizeof(PARTITION_TABLE));

  return STATUS_SUCCESS;
#else
  return (unsigned int) -1;
#endif
}
