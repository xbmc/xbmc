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
#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "LinuxV4l2.h"

#include "xbmc/utils/log.h"

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>
#include <linux/media.h>

#ifdef CLASSNAME
#undef CLASSNAME
#endif
#define CLASSNAME "CLinuxV4l2"

CLinuxV4l2::CLinuxV4l2() 
{
}

CLinuxV4l2::~CLinuxV4l2()
{
}

int CLinuxV4l2::RequestBuffer(int device, enum v4l2_buf_type type, enum v4l2_memory memory, int numBuffers)
{
  struct v4l2_requestbuffers reqbuf;
  int ret = 0;

  if(device < 0)
    return false;

  memset(&reqbuf, 0, sizeof(struct v4l2_requestbuffers));

  reqbuf.type     = type;
  reqbuf.memory   = memory;
  reqbuf.count    = numBuffers;

  ret = ioctl(device, VIDIOC_REQBUFS, &reqbuf);
  if (ret)
  {
    CLog::Log(LOGERROR, "%s::%s - Request buffers", CLASSNAME, __func__);
    return V4L2_ERROR;
  }

  return reqbuf.count;
}

bool CLinuxV4l2::StreamOn(int device, enum v4l2_buf_type type, int onoff)
{
  int ret = 0;
  enum v4l2_buf_type setType = type;

  if(device < 0)
    return false;

  ret = ioctl(device, onoff, &setType);
  if(ret)
    return false;

  return true;
}

bool CLinuxV4l2::MmapBuffers(int device, int count, V4L2Buffer *v4l2Buffers, enum v4l2_buf_type type, enum v4l2_memory memory, bool queue)
{
  struct v4l2_buffer buf;
  struct v4l2_plane planes[V4L2_NUM_MAX_PLANES];
  int ret;
  int i, j;

  if(device < 0 || !v4l2Buffers || count == 0)
    return false;

  for(i = 0; i < count; i++)
  {
    memset(&buf, 0, sizeof(struct v4l2_buffer));
    memset(&planes, 0, sizeof(struct v4l2_plane) * V4L2_NUM_MAX_PLANES);
    buf.type      = type;
    buf.memory    = memory;
    buf.index     = i;
    buf.m.planes  = planes;
    buf.length    = V4L2_NUM_MAX_PLANES;

    ret = ioctl(device, VIDIOC_QUERYBUF, &buf);
    if (ret)
    {
      CLog::Log(LOGERROR, "%s::%s - Query buffer", CLASSNAME, __func__);
      return false;
    }

    V4L2Buffer *buffer = &v4l2Buffers[i];

    buffer->iNumPlanes = 0;
    for (j = 0; j < V4L2_NUM_MAX_PLANES; j++) 
    {
      //printf("%s::%s - plane %d %d size %d 0x%08x\n", CLASSNAME, __func__, i, j, buf.m.planes[j].length,
      //    buf.m.planes[j].m.userptr);
      buffer->iSize[j]       = buf.m.planes[j].length;
      buffer->iBytesUsed[j]  = buf.m.planes[j].bytesused;
      if(buffer->iSize[j])
      {
        buffer->cPlane[j] = mmap(NULL, buf.m.planes[j].length, PROT_READ | PROT_WRITE,
                       MAP_SHARED, device, buf.m.planes[j].m.mem_offset);
        if(buffer->cPlane[j] == MAP_FAILED)
        {
          CLog::Log(LOGERROR, "%s::%s - Mmapping buffer", CLASSNAME, __func__);
          return false;
        }
        memset(buffer->cPlane[j], 0, buf.m.planes[j].length);
        buffer->iNumPlanes++;
      }
    }
    buffer->iIndex = i;

    if(queue)
    {
      ret = ioctl(device, VIDIOC_QBUF, &buf);
      if (ret)
      {
        CLog::Log(LOGERROR, "%s::%s - Queue buffer", CLASSNAME, __func__);
        return false;
      }
      buffer->bQueue = true;
    }
  }

  return true;
}

V4L2Buffer *CLinuxV4l2::FreeBuffers(int count, V4L2Buffer *v4l2Buffers)
{
  int i, j;

  if(v4l2Buffers != NULL)
  {
    for(i = 0; i < count; i++)
    {
      V4L2Buffer *buffer = &v4l2Buffers[i];

      for (j = 0; j < buffer->iNumPlanes; j++)
      {
        if(buffer->cPlane[j] && buffer->cPlane[j] != MAP_FAILED)
        {
          munmap(buffer->cPlane[j], buffer->iSize[j]);
          CLog::Log(LOGDEBUG, "%s::%s - unmap convert buffer", CLASSNAME, __func__);
        }
      }
    }
    free(v4l2Buffers);
  }
  return NULL;
}

int CLinuxV4l2::DequeueBuffer(int device, enum v4l2_buf_type type, enum v4l2_memory memory, int planes)
{
  struct v4l2_buffer vbuf;
  struct v4l2_plane  vplanes[V4L2_NUM_MAX_PLANES];
  int ret = 0;

  if(device < 0)
    return V4L2_ERROR;

  memset(&vplanes, 0, sizeof(struct v4l2_plane) * V4L2_NUM_MAX_PLANES);
  memset(&vbuf, 0, sizeof(struct v4l2_buffer));
  vbuf.type     = type;
  vbuf.memory   = memory;
  vbuf.m.planes = vplanes;
  vbuf.length   = planes;

  ret = ioctl(device, VIDIOC_DQBUF, &vbuf);
  if (ret) {
    if (errno != EAGAIN)
      CLog::Log(LOGERROR, "%s::%s - Dequeue buffer", CLASSNAME, __func__);
    return V4L2_ERROR;
  }
  
  return vbuf.index;
}

int CLinuxV4l2::QueueBuffer(int device, enum v4l2_buf_type type, 
    enum v4l2_memory memory, int planes, int index, V4L2Buffer *buffer)
{
  struct v4l2_buffer vbuf;
  struct v4l2_plane  vplanes[V4L2_NUM_MAX_PLANES];
  int ret = 0;

  if(!buffer || device <0)
    return V4L2_ERROR;

  memset(&vplanes, 0, sizeof(struct v4l2_plane) * V4L2_NUM_MAX_PLANES);
  memset(&vbuf, 0, sizeof(struct v4l2_buffer));
  vbuf.type     = type;
  vbuf.memory   = memory;
  vbuf.index    = index;
  vbuf.m.planes = vplanes;
  vbuf.length   = buffer->iNumPlanes;

  for (int i = 0; i < buffer->iNumPlanes; i++) 
  {
    vplanes[i].m.userptr   = (unsigned long)buffer->cPlane[i];
    vplanes[i].length      = buffer->iSize[i];
    vplanes[i].bytesused   = buffer->iBytesUsed[i];
  }

  ret = ioctl(device, VIDIOC_QBUF, &vbuf);
  if (ret)
  {
    CLog::Log(LOGERROR, "%s::%s - Queue buffer", CLASSNAME, __func__);
    return V4L2_ERROR;
  }
  buffer->bQueue = true;

  return index;
}

int CLinuxV4l2::PollInput(int device, int timeout)
{
  int ret = 0;
  struct pollfd p;
  p.fd = device;
  p.events = POLLIN | POLLERR;

  ret = poll(&p, 1, timeout);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "%s::%s - Polling input", CLASSNAME, __func__);
    return V4L2_ERROR;
  }
  else if (ret == 0)
  {
    return V4L2_BUSY;
  }

  return V4L2_READY;
}

int CLinuxV4l2::PollOutput(int device, int timeout)
{
  int ret = 0;
  struct pollfd p;
  p.fd = device;
  p.events = POLLOUT | POLLERR;

  ret = poll(&p, 1, timeout);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "%s::%s - Polling output", CLASSNAME, __func__);
    return V4L2_ERROR;
  }
  else if (ret == 0)
  {
    return V4L2_BUSY;
  }

  return V4L2_READY;
}

int CLinuxV4l2::SetControllValue(int device, int id, int value)
{
  struct v4l2_control control;
  int ret;

  control.id    = id;
  control.value = value;

  ret = ioctl(device, VIDIOC_S_CTRL, &control);

  if(ret < 0) 
  {
    CLog::Log(LOGERROR, "%s::%s - Set controll if %d value %d\n", CLASSNAME, __func__, id, value);
    return V4L2_ERROR;
  }

  return V4L2_OK;
}
