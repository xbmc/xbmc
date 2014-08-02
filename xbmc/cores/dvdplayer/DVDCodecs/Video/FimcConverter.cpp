/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include "FimcConverter.h"

#include <sys/ioctl.h>
#include <dirent.h>

#ifdef CLASSNAME
#undef CLASSNAME
#endif
#define CLASSNAME "FimcConverter"

FimcConverter::FimcConverter()
{
  m_iConverterHandle = -1;
  m_FIMCCaptureBuffersCount = 0;
  m_v4l2FIMCCaptureBuffers = NULL;
}

FimcConverter::~FimcConverter()
{
  Dispose();
}

void FimcConverter::Dispose()
{
  CLog::Log(LOGDEBUG, "%s::%s - Freeing memory allocated for FIMC buffers", CLASSNAME, __func__);
  if (m_v4l2FIMCCaptureBuffers)
    m_v4l2FIMCCaptureBuffers = CLinuxV4l2::FreeBuffers(m_FIMCCaptureBuffersCount, m_v4l2FIMCCaptureBuffers);
  CLog::Log(LOGDEBUG, "%s::%s - Closing FIMC device", CLASSNAME, __func__);
  if (m_iConverterHandle >= 0)
  {
    if (CLinuxV4l2::StreamOn(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, VIDIOC_STREAMOFF))
      CLog::Log(LOGDEBUG, "%s::%s - FIMC OUTPUT Stream OFF", CLASSNAME, __func__);
    if (CLinuxV4l2::StreamOn(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, VIDIOC_STREAMOFF))
      CLog::Log(LOGDEBUG, "%s::%s - FIMC CAPTURE Stream OFF", CLASSNAME, __func__);
    close(m_iConverterHandle);
  }
  m_FIMCCaptureBuffersCount = 0;
  m_v4l2FIMCCaptureBuffers = NULL;
  m_iConverterHandle = -1;
}

bool FimcConverter::OpenDevice()
{
  DIR *dir;
  struct dirent *ent;

  if ((dir = opendir ("/sys/class/video4linux/")) != NULL)
  {
    while ((ent = readdir (dir)) != NULL)
    {
      if (strncmp(ent->d_name, "video", 5) == 0)
      {
        char *p;
        char name[64];
        char devname[64];
        char sysname[64];
        char drivername[32];
        char target[1024];
        int ret;

        snprintf(sysname, sizeof(sysname), "/sys/class/video4linux/%s", ent->d_name);
        snprintf(name, sizeof(name), "/sys/class/video4linux/%s/name", ent->d_name);

        FILE* fp = fopen(name, "r");
        if (fp == NULL)
          continue;

        if (fgets(drivername, 32, fp) != NULL)
        {
          p = strchr(drivername, '\n');
          if (p != NULL)
            *p = '\0';
        }
        else
        {
          fclose(fp);
          continue;
        }
        fclose(fp);

        ret = readlink(sysname, target, sizeof(target));
        if (ret < 0)
          continue;
        target[ret] = '\0';
        p = strrchr(target, '/');
        if (p == NULL)
          continue;

        snprintf(devname, sizeof(devname), "/dev/%s", ++p);

        if (m_iConverterHandle < 0 && strstr(drivername, "fimc") != NULL && strstr(drivername, "m2m") != NULL)
        {
          struct v4l2_capability cap;
          int fd = open(devname, O_RDWR, 0);
          if (fd > 0) {
            memset(&(cap), 0, sizeof (cap));
            ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
            if (ret == 0)
              if ((cap.capabilities & V4L2_CAP_VIDEO_M2M_MPLANE ||
                ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) && (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT_MPLANE))) &&
                (cap.capabilities & V4L2_CAP_STREAMING))
              {
                m_iConverterHandle = fd;
                CLog::Log(LOGDEBUG, "%s::%s - Found %s %s", CLASSNAME, __func__, drivername, devname);
              }
          }
          if (m_iConverterHandle < 0)
            close(fd);
        }
        if (m_iConverterHandle >= 0)
          return true;
      }
    }
    closedir (dir);
  }
  return false;
}

bool FimcConverter::RequestBuffers(int buffers)
{
  // Request FIMC OUTPUT buffers
  int ret = CLinuxV4l2::RequestBuffer(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_USERPTR, buffers);
  if (ret == V4L2_ERROR)
  {
    CLog::Log(LOGERROR, "%s::%s - FIMC OUTPUT REQBUFS failed", CLASSNAME, __func__);
    return false;
  }

  CLog::Log(LOGDEBUG, "%s::%s - FIMC OUTPUT REQBUFS Number of buffers is %d", CLASSNAME, __func__, ret);
  return true;
}

bool FimcConverter::SetOutputFormat(struct v4l2_format* fmt)
{
  // Setup FIMC OUTPUT fmt with data from MFC CAPTURE received on previous step
  fmt->type                        = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
  if (ioctl(m_iConverterHandle, VIDIOC_S_FMT, fmt))
  {
    CLog::Log(LOGERROR, "%s::%s - FIMC OUTPUT S_FMT Failed", CLASSNAME, __func__);
    return false;
  }

  CLog::Log(LOGDEBUG, "%s::%s - FIMC OUTPUT S_FMT (%dx%d)", CLASSNAME, __func__, fmt->fmt.pix_mp.width, fmt->fmt.pix_mp.height);
  return true;
}

bool FimcConverter::SetOutputCrop(struct v4l2_crop* crop)
{
  // Setup FIMC OUTPUT crop with data from MFC CAPTURE received on previous step
  crop->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
  if (ioctl(m_iConverterHandle, VIDIOC_S_CROP, crop))
  {
    CLog::Log(LOGERROR, "%s::%s - FIMC OUTPUT S_CROP Failed to set crop information", CLASSNAME, __func__);
    return false;
  }

  CLog::Log(LOGDEBUG, "%s::%s - FIMC OUTPUT S_CROP (%dx%d)", CLASSNAME, __func__, crop->c.width, crop->c.height);
  return true;
}

bool FimcConverter::SetCaptureFormat(struct v4l2_format *fmt)
{
  // Setup FIMC capture
  fmt->fmt.pix_mp.pixelformat   = V4L2_PIX_FMT_NV12M;
  fmt->type                     = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  fmt->fmt.pix_mp.field         = V4L2_FIELD_ANY;

  if (ioctl(m_iConverterHandle, VIDIOC_S_FMT, fmt))
  {
    CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE S_FMT Failed", CLASSNAME, __func__);
    return false;
  }

  CLog::Log(LOGDEBUG, "%s::%s - FIMC CAPTURE S_FMT %dx%d", CLASSNAME, __func__, fmt->fmt.pix_mp.width, fmt->fmt.pix_mp.height);
  return true;
}

bool FimcConverter::SetCaptureCrop(struct v4l2_crop *crop)
{
  // Setup FIMC CAPTURE crop
  crop->type      = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  crop->c.left    = 0;
  crop->c.top     = 0;
  if (ioctl(m_iConverterHandle, VIDIOC_S_CROP, crop))
  {
    CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE S_CROP Failed", CLASSNAME, __func__);
    return false;
  }

  CLog::Log(LOGDEBUG, "%s::%s - FIMC CAPTURE S_CROP (%dx%d)", CLASSNAME, __func__, crop->c.width, crop->c.height);
  return true;
}

bool FimcConverter::SetupCaptureBuffers()
{
  // Get FIMC produced picture details to adjust output buffer parameters with these values
  struct v4l2_format fmt = {};
  fmt.type               = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  if (ioctl(m_iConverterHandle, VIDIOC_G_FMT, &fmt))
  {
    CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE G_FMT Failed", CLASSNAME, __func__);
    return false;
  }

  // Request FIMC capture buffers
  m_FIMCCaptureBuffersCount = CLinuxV4l2::RequestBuffer(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP, FIMC_CAPTURE_BUFFERS_CNT);
  if (m_FIMCCaptureBuffersCount == V4L2_ERROR)
  {
    CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE REQBUFS failed", CLASSNAME, __func__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s::%s - FIMC CAPTURE REQBUFS Number of buffers is %d", CLASSNAME, __func__, m_FIMCCaptureBuffersCount);

  // Allocate, Memory Map and queue fimc capture buffers
  m_v4l2FIMCCaptureBuffers = (V4L2Buffer *)calloc(m_FIMCCaptureBuffersCount, sizeof(V4L2Buffer));
  if (!m_v4l2FIMCCaptureBuffers)
  {
    CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE Cannot allocate memory for buffers", CLASSNAME, __func__);
    return false;
  }
  if(!CLinuxV4l2::MmapBuffers(m_iConverterHandle, m_FIMCCaptureBuffersCount, m_v4l2FIMCCaptureBuffers, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP, true))
  {
    CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE Cannot mmap for capture buffers", CLASSNAME, __func__);
    return false;
  }

  CLog::Log(LOGDEBUG, "%s::%s - FIMC CAPTURE Succesfully allocated, mmapped and queued %d buffers", CLASSNAME, __func__, m_FIMCCaptureBuffersCount);

  // STREAMON on FIMC OUTPUT and CAPTURE
  if (!CLinuxV4l2::StreamOn(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, VIDIOC_STREAMON))
  {
    CLog::Log(LOGERROR, "%s::%s - FIMC OUTPUT Failed to Stream ON", CLASSNAME, __func__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s::%s - FIMC OUTPUT Stream ON", CLASSNAME, __func__);
  if (!CLinuxV4l2::StreamOn(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, VIDIOC_STREAMON))
  {
    CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE Failed to Stream ON", CLASSNAME, __func__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s::%s - FIMC CAPTURE Stream ON", CLASSNAME, __func__);

  return true;
}


V4L2Buffer* FimcConverter::GetCaptureBuffer(int index)
{
  return m_v4l2FIMCCaptureBuffers != NULL && index < m_FIMCCaptureBuffersCount ? &(m_v4l2FIMCCaptureBuffers[index]) : NULL;
}

bool FimcConverter::QueueCaptureBuffer(int index)
{
  int ret = CLinuxV4l2::QueueBuffer(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP, &m_v4l2FIMCCaptureBuffers[index]);
  if (ret == V4L2_ERROR)
  {
    CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE Failed to queue buffer with index %d, errno = %d", CLASSNAME, __func__, index, errno);
    return false;
  }
  return true;
}

bool FimcConverter::DequeueCaptureBuffer(int *status, double *timestamp)
{
  int index = CLinuxV4l2::DequeueBuffer(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP, timestamp);
  if (index < 0)
  {
    if (errno == EAGAIN)
    {
      // Dequeue buffer not ready, need more data on input. EAGAIN = 11
      *status = 1; // false with result = 1 indicates that we return VC_BUFFER
      return false;
    }
    CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE error dequeue output buffer, got number %d, errno %d", CLASSNAME, __func__, index, errno);
    *status = 0;
    return false; // false with result = 0 indicates we return VC_FLUSHED
  }
  *status = index;
  return true;
}

bool FimcConverter::QueueOutputBuffer(int index, V4L2Buffer* m_v4l2MFCCaptureBuffer, int *status)
{
  int ret = CLinuxV4l2::QueueBuffer(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_USERPTR, m_v4l2MFCCaptureBuffer);
  if (ret == V4L2_ERROR)
  {
    CLog::Log(LOGERROR, "%s::%s - FIMC OUTPUT Failed to queue buffer with index %d, errno %d", CLASSNAME, __func__, ret, errno);
    return false;
  }
  *status = ret;
  return true; // *status contains the queued buffer index
}

bool FimcConverter::DequeueOutputBuffer(int *status, double *timestamp)
{
  // Dequeue frame from fimc output
  int index = CLinuxV4l2::DequeueBuffer(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_USERPTR, timestamp);
  if (index < 0) {
    if (errno == EAGAIN) // Dequeue buffer not ready, need more data on input. EAGAIN = 11
    {
      *status = 1;
      return false;
    }
    CLog::Log(LOGERROR, "%s::%s - FIMC OUTPUT error dequeue output buffer, got number %d", CLASSNAME, __func__, index);
    *status = 0;
    return false;
  }
  *status = index; // *status contains the buffer index;
  return true;
}
