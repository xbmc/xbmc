/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#if defined(HAS_LIBAMCODEC)
#include "AmVideoCap.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/ioctl.h>

// taken from linux/amlogic/amports/amvideocap.h - needs to be synced - no changes expected though
#define AMVIDEOCAP_IOC_MAGIC  'V'
#define AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH      _IOW(AMVIDEOCAP_IOC_MAGIC, 0x02, int)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT     _IOW(AMVIDEOCAP_IOC_MAGIC, 0x03, int)
#define AMVIDEOCAP_IOW_SET_CANCEL_CAPTURE       _IOW(AMVIDEOCAP_IOC_MAGIC, 0x33, int)

#define CAPTURE_DEVICEPATH "/dev/amvideocap0"

CAmVideoCap::CAmVideoCap()
{
  m_deviceOpen = false;
}

CAmVideoCap::~CAmVideoCap()
{
  if (m_deviceOpen)
    close(m_captureFd);
  m_deviceOpen = false;
}

bool CAmVideoCap::CaptureVideoFrame(int destWidth, int destHeight, unsigned char *pixels)
{
  int buffSize = destWidth * destHeight * 3;
  int readSize = 0;
  unsigned char *videoBuffer = new unsigned char[buffSize];

  if (!m_deviceOpen)
  {
    m_captureFd = open(CAPTURE_DEVICEPATH, O_RDWR, 0);
    if (m_captureFd >= 0)
      fcntl(m_captureFd, F_SETFL, fcntl(m_captureFd, F_GETFL) | O_NONBLOCK);
  }

  if (m_captureFd >= 0)
  {
    m_deviceOpen = true;
    // configure destination
    ioctl(m_captureFd, AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH, destWidth);
    ioctl(m_captureFd, AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT, destHeight);
    readSize = pread(m_captureFd, videoBuffer, buffSize, 0);

    unsigned char *videoPtr = videoBuffer;
    for (int processedBytes = 0; processedBytes < buffSize; processedBytes += 3, pixels+=4)
    {
      memcpy(pixels, videoPtr, 3);
      videoPtr += 3;
    }
  }
  delete [] videoBuffer;
  return readSize == buffSize;
}

void CAmVideoCap::CancelCapture()
{
  if (m_deviceOpen)
    ioctl(m_captureFd, AMVIDEOCAP_IOW_SET_CANCEL_CAPTURE, NULL);
}

#endif //defined(HAS_LIBAMCODEC)
