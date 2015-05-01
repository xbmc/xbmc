/*
 *      Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *      Copyright (C) 2002-2013 Team XBMC
 *      http://xbmc.org
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

// IoSupport.cpp: implementation of the CIoSupport class.
//
//////////////////////////////////////////////////////////////////////

#include "system.h"
#include "IoSupport.h"
#include "utils/log.h"
#ifdef TARGET_WINDOWS
#include "my_ntddcdrm.h"
#endif
#if defined(TARGET_LINUX)
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/cdrom.h>
#endif
#if defined(TARGET_DARWIN)
#include <sys/param.h>
#include <mach-o/dyld.h>
#if defined(TARGET_DARWIN_OSX)
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOCDTypes.h>
#include <IOKit/storage/IODVDTypes.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOCDMedia.h>
#include <IOKit/storage/IODVDMedia.h>
#include <IOKit/storage/IOCDMediaBSDClient.h>
#include <IOKit/storage/IODVDMediaBSDClient.h>
#include <IOKit/storage/IOStorageDeviceCharacteristics.h>
#endif
#endif
#ifdef TARGET_FREEBSD
#include <sys/syslimits.h>
#endif
#include "cdioSupport.h"
#include "MediaManager.h"
#ifdef TARGET_POSIX
#include "XHandle.h"
#endif

#ifdef HAS_DVD_DRIVE
using namespace MEDIA_DETECT;
#endif

PVOID CIoSupport::m_rawXferBuffer;

HANDLE CIoSupport::OpenCDROM()
{
  HANDLE hDevice = 0;

#ifdef HAS_DVD_DRIVE
#if defined(TARGET_POSIX)
  int fd = open(CLibcdio::GetInstance()->GetDeviceFileName(), O_RDONLY | O_NONBLOCK);
  hDevice = new CXHandle(CXHandle::HND_FILE);
  hDevice->fd = fd;
  hDevice->m_bCDROM = true;
#elif defined(TARGET_WINDOWS)
  hDevice = CreateFile(g_mediaManager.TranslateDevicePath("",true).c_str(), GENERIC_READ, FILE_SHARE_READ,
                       NULL, OPEN_EXISTING,
                       FILE_FLAG_RANDOM_ACCESS, NULL );
#else

  hDevice = CreateFile("\\\\.\\Cdrom0", GENERIC_READ, FILE_SHARE_READ,
                       NULL, OPEN_EXISTING,
                       FILE_FLAG_RANDOM_ACCESS, NULL );

#endif
#endif
  return hDevice;
}

void CIoSupport::AllocReadBuffer()
{
#ifndef TARGET_POSIX
  m_rawXferBuffer = GlobalAlloc(GPTR, RAW_SECTOR_SIZE);
#endif
}

void CIoSupport::FreeReadBuffer()
{
#ifndef TARGET_POSIX
  GlobalFree(m_rawXferBuffer);
#endif
}

INT CIoSupport::ReadSector(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)

{
  DWORD dwRead;
  DWORD dwSectorSize = 2048;

#if defined(TARGET_DARWIN) && defined(HAS_DVD_DRIVE)
  dk_cd_read_t cd_read;
  memset( &cd_read, 0, sizeof(cd_read) );

  cd_read.sectorArea  = kCDSectorAreaUser;
  cd_read.buffer      = lpczBuffer;

  cd_read.sectorType  = kCDSectorTypeMode1;
  cd_read.offset      = dwSector * kCDSectorSizeMode1;

  cd_read.bufferLength = 2048;

  if( ioctl(hDevice->fd, DKIOCCDREAD, &cd_read ) == -1 )
  {
    return -1;
  }
  return 2048;
#elif defined(TARGET_POSIX)
  if (hDevice->m_bCDROM)
  {
    int fd = hDevice->fd;

    // seek to requested sector
    off_t offset = (off_t)dwSector * (off_t)MODE1_DATA_SIZE;

    if (lseek(fd, offset, SEEK_SET) < 0)
    {
      CLog::Log(LOGERROR, "CD: ReadSector Request to read sector %d\n", (int)dwSector);
      CLog::Log(LOGERROR, "CD: ReadSector error: %s\n", strerror(errno));
      OutputDebugString("CD Read error\n");
      return (-1);
    }

    // read data block of this sector
    while (read(fd, lpczBuffer, MODE1_DATA_SIZE) < 0)
    {
      // read was interrupted - try again
      if (errno == EINTR)
        continue;

      // error reading sector
      CLog::Log(LOGERROR, "CD: ReadSector Request to read sector %d\n", (int)dwSector);
      CLog::Log(LOGERROR, "CD: ReadSector error: %s\n", strerror(errno));
      OutputDebugString("CD Read error\n");
      return (-1);
    }

    return MODE1_DATA_SIZE;
  }
#endif
  LARGE_INTEGER Displacement;
  Displacement.QuadPart = ((INT64)dwSector) * dwSectorSize;

  for (int i = 0; i < 5; i++)
  {
    if (SetFilePointer(hDevice, Displacement.u.LowPart, &Displacement.u.HighPart, FILE_BEGIN) != (DWORD)-1)
    {
      if (ReadFile(hDevice, m_rawXferBuffer, dwSectorSize, &dwRead, NULL))
      {
        memcpy(lpczBuffer, m_rawXferBuffer, dwSectorSize);
        return dwRead;
      }
    }
  }

  CLog::Log(LOGERROR, "%s: CD Read error", __FUNCTION__);
  return -1;
}


INT CIoSupport::ReadSectorMode2(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
#ifdef HAS_DVD_DRIVE
#if defined(TARGET_DARWIN)
  dk_cd_read_t cd_read;

  memset( &cd_read, 0, sizeof(cd_read) );

  cd_read.sectorArea = kCDSectorAreaUser;
  cd_read.buffer = lpczBuffer;

  cd_read.offset       = dwSector * kCDSectorSizeMode2Form2;
  cd_read.sectorType   = kCDSectorTypeMode2Form2;
  cd_read.bufferLength = kCDSectorSizeMode2Form2;

  if( ioctl( hDevice->fd, DKIOCCDREAD, &cd_read ) == -1 )
  {
    return -1;
  }
  return MODE2_DATA_SIZE;
#elif defined(TARGET_FREEBSD)
  // NYI
#elif defined(TARGET_POSIX)
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
    }
  }
#endif
#endif
  return -1;
}

INT CIoSupport::ReadSectorCDDA(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
  return -1;
}

VOID CIoSupport::CloseCDROM(HANDLE hDevice)
{
  CloseHandle(hDevice);
}

