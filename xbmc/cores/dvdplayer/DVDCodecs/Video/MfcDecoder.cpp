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

#include "MfcDecoder.h"

#include <utils/fastmemcpy.h>
#include <sys/ioctl.h>
#include <dirent.h>

#ifdef CLASSNAME
#undef CLASSNAME
#endif
#define CLASSNAME "MfcDecoder"

MfcDecoder::MfcDecoder()
{
  m_iDecoderHandle = -1;
  hasNV12Support = false;
  m_bVideoConvert = false;
  m_MFCOutputBuffersCount = 0;
  m_v4l2MFCOutputBuffers = NULL;
  m_MFCCaptureBuffersCount = 0;
  m_v4l2MFCCaptureBuffers = NULL;
  m_iMFCCapturePlane1Size = -1;
  m_iMFCCapturePlane2Size = -1;
  memset(&(m_v4l2OutputBuffer), 0, sizeof (m_v4l2OutputBuffer));
}

MfcDecoder::~MfcDecoder()
{
  Dispose();
}

bool MfcDecoder::OpenDevice()
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

        snprintf(sysname, 64, "/sys/class/video4linux/%s", ent->d_name);
        snprintf(name, 64, "/sys/class/video4linux/%s/name", ent->d_name);

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

        snprintf(devname, 64, "/dev/%s", ++p);

        if (m_iDecoderHandle < 0 && strncmp(drivername, "s5p-mfc-dec", 11) == 0)
        {
          struct v4l2_capability cap;
          int fd = open(devname, O_RDWR | O_NONBLOCK, 0);
          if (fd > 0)
          {
            memset(&(cap), 0, sizeof (cap));
            ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
            if (ret == 0)
              if ((cap.capabilities & V4L2_CAP_VIDEO_M2M_MPLANE ||
                ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) && (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT_MPLANE))) &&
                (cap.capabilities & V4L2_CAP_STREAMING))
              {
                m_iDecoderHandle = fd;
                CLog::Log(LOGDEBUG, "%s::%s - Found %s %s", CLASSNAME, __func__, drivername, devname);
              }
          }
          if (m_iDecoderHandle < 0)
            close(fd);
        }
        if (m_iDecoderHandle >= 0)
        {
          // MFC should at least support NV12MT format
          return HasNV12MTSupport();
        }
      }
    }
    closedir (dir);
  }
  return false;
}

bool MfcDecoder::HasNV12MTSupport()
{
  // we enumerate all the supported formats looking for NV12MT and NV12
  int index = 0;
  bool hasNV12MTSupport = false;
  while (true)
  {
    struct v4l2_fmtdesc vid_fmtdesc = {};
    vid_fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    vid_fmtdesc.index = index++;

    ret = ioctl(m_iDecoderHandle, VIDIOC_ENUM_FMT, &vid_fmtdesc);
    if (ret != 0)
      break;
    CLog::Log(LOGDEBUG, "%s::%s - Decoder format %d: %c%c%c%c (%s)", CLASSNAME, __func__, vid_fmtdesc.index,
        vid_fmtdesc.pixelformat & 0xFF, (vid_fmtdesc.pixelformat >> 8) & 0xFF,
        (vid_fmtdesc.pixelformat >> 16) & 0xFF, (vid_fmtdesc.pixelformat >> 24) & 0xFF,
        vid_fmtdesc.description);
    if (vid_fmtdesc.pixelformat == V4L2_PIX_FMT_NV12MT)
      hasNV12MTSupport = true;
    if (vid_fmtdesc.pixelformat == V4L2_PIX_FMT_NV12)
      m_hasNV12Support = true;
  }
  return hasNV12MTSupport;
}

bool MfcDecoder::SetupOutputFormat(CDVDStreamInfo &hints)
{
  struct v4l2_format fmt = {};
  switch (hints.codec)
  {
    case AV_CODEC_ID_VC1:
      fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_VC1_ANNEX_G;
      m_name = "mfc-vc1";
      break;
    case AV_CODEC_ID_MPEG1VIDEO:
      fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_MPEG1;
      m_name = "mfc-mpeg1";
      break;
    case AV_CODEC_ID_MPEG2VIDEO:
      fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_MPEG2;
      m_name = "mfc-mpeg2";
      break;
    case AV_CODEC_ID_MPEG4:
      fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_MPEG4;
      m_name = "mfc-mpeg4";
      break;
    case AV_CODEC_ID_H263:
      fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H263;
      m_name = "mfc-h263";
      break;
    case AV_CODEC_ID_H264:
      fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
      m_name = "mfc-h264";
      break;
    default:
      return false;
  }
  
  fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
  fmt.fmt.pix_mp.plane_fmt[0].sizeimage = STREAM_BUFFER_SIZE;
  fmt.fmt.pix_mp.num_planes = V4L2_NUM_MAX_PLANES;

  if (ioctl(m_iDecoderHandle, VIDIOC_S_FMT, &fmt) != 0)
  {
    CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT S_FMT failed", CLASSNAME, __func__);
    return false;
  }
  
  return true;
}

bool MfcDecoder::RequestBuffers()
{
  // Get mfc needed number of buffers
  struct v4l2_control ctrl = {};
  ctrl.id = V4L2_CID_MIN_BUFFERS_FOR_CAPTURE;
  if (ioctl(m_iDecoderHandle, VIDIOC_G_CTRL, &ctrl))
  {
    CLog::Log(LOGERROR, "%s::%s - MFC CAPTURE Failed to get the number of buffers required", CLASSNAME, __func__);
    return false;
  }
  m_MFCCaptureBuffersCount = ctrl.value + MFC_CAPTURE_EXTRA_BUFFER_CNT;
  return true;
}

bool MfcDecoder::SetupOutputBuffers()
{
  // Request mfc output buffers
  m_MFCOutputBuffersCount = CLinuxV4l2::RequestBuffer(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_MMAP, MFC_OUTPUT_BUFFERS_CNT);
  if (m_MFCOutputBuffersCount == V4L2_ERROR)
  {
    CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT REQBUFS failed", CLASSNAME, __func__);
    return false;
  } 
  CLog::Log(LOGDEBUG, "%s::%s - MFC OUTPUT REQBUFS Number of MFC buffers is %d (requested %d)", CLASSNAME, __func__, m_MFCOutputBuffersCount, MFC_OUTPUT_BUFFERS_CNT);

  // Memory Map mfc output buffers
  m_v4l2MFCOutputBuffers = (V4L2Buffer *) calloc(m_MFCOutputBuffersCount, sizeof(V4L2Buffer));
  if (!m_v4l2MFCOutputBuffers)
  {
    CLog::Log(LOGERROR, "%s::%s - MFC cannot allocate OUTPUT buffers in memory", CLASSNAME, __func__);
    return false;
  }
  if (!CLinuxV4l2::MmapBuffers(m_iDecoderHandle, m_MFCOutputBuffersCount, m_v4l2MFCOutputBuffers, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_MMAP, false))
  {
    CLog::Log(LOGERROR, "%s::%s - MFC Cannot mmap OUTPUT buffers", CLASSNAME, __func__);
    return false;
  }
  
  CLog::Log(LOGDEBUG, "%s::%s - MFC OUTPUT Succesfully mmapped %d buffers", CLASSNAME, __func__, m_MFCOutputBuffersCount);
  return true;
}

bool MfcDecoder::QueueHeader(CDVDStreamInfo &hints)
{
  unsigned int extraSize = 0;
  uint8_t *extraData = NULL;
  
  m_bVideoConvert = m_converter.Open(hints.codec, (uint8_t *)hints.extradata, hints.extrasize, true);
  if (m_bVideoConvert)
  {
    if(m_converter.GetExtraData() != NULL && m_converter.GetExtraSize() > 0)
    {
      extraSize = m_converter.GetExtraSize();
      extraData = m_converter.GetExtraData();
    }
  }
  else
  {
    if(hints.extrasize > 0 && hints.extradata != NULL)
    {
      extraSize = hints.extrasize;
      extraData = (uint8_t*)hints.extradata;
    }
  }
  
  // Prepare header
  m_v4l2MFCOutputBuffers[0].iBytesUsed[0] = extraSize;
  fast_memcpy((uint8_t *)m_v4l2MFCOutputBuffers[0].cPlane[0], extraData, extraSize);

  // Queue header to mfc output queue
  int ret = CLinuxV4l2::QueueBuffer(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_MMAP, m_v4l2MFCOutputBuffers[0].iNumPlanes, 0, &m_v4l2MFCOutputBuffers[0]);
  if (ret == V4L2_ERROR)
  {
    CLog::Log(LOGERROR, "%s::%s - MFC Error queuing header", CLASSNAME, __func__);
    return false;
  }
  m_v4l2MFCOutputBuffers[ret].bQueue = true;
  CLog::Log(LOGDEBUG, "%s::%s - MFC OUTPUT <- %d header of size %d", CLASSNAME, __func__, ret, extraSize);

  // STREAMON on mfc OUTPUT
  if (!CLinuxV4l2::StreamOn(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, VIDIOC_STREAMON))
  {
    CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT Failed to Stream ON", CLASSNAME, __func__);
    return false;
  }
  
  CLog::Log(LOGDEBUG, "%s::%s - MFC OUTPUT Stream ON", CLASSNAME, __func__);
  return true;
}

bool MfcDecoder::SetCaptureFormat()
{
  struct v4l2_format fmt = {};
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12M;
  int ret = ioctl(m_iDecoderHandle, VIDIOC_S_FMT, &fmt);
  if (ret != 0)
  {
    CLog::Log(LOGERROR, "%s::%s - MFC CAPTURE S_FMT Failed on CAPTURE", CLASSNAME, __func__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s::%s - MFC CAPTURE S_FMT 0x%x",  CLASSNAME, __func__, fmt.fmt.pix_mp.pixelformat);
  return true;
}

bool MfcDecoder::GetCaptureFormat(struct v4l2_format* fmt)
{
  fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  if (ioctl(m_iDecoderHandle, VIDIOC_G_FMT, fmt))
  {
    CLog::Log(LOGERROR, "%s::%s - MFC CAPTURE G_FMT Failed", CLASSNAME, __func__);
    return false;
  }
  m_iMFCCapturePlane1Size = fmt->fmt.pix_mp.plane_fmt[0].sizeimage;
  m_iMFCCapturePlane2Size = fmt->fmt.pix_mp.plane_fmt[1].sizeimage;
  
  CLog::Log(LOGDEBUG, "%s::%s - MFC CAPTURE G_FMT: fmt (%dx%d), plane[0]=%d plane[1]=%d", CLASSNAME, __func__, fmt->fmt.pix_mp.width, fmt->fmt.pix_mp.height, m_iMFCCapturePlane1Size, m_iMFCCapturePlane2Size);
  return true;
}

bool MfcDecoder::GetCaptureCrop(struct v4l2_crop* crop)
{
  crop->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  if (ioctl(m_iDecoderHandle, VIDIOC_G_CROP, crop))
  {
    CLog::Log(LOGERROR, "%s::%s - MFC CAPTURE G_CROP Failed to get crop information", CLASSNAME, __func__);
    return false;
  }

  CLog::Log(LOGDEBUG, "%s::%s - MFC CAPTURE G_CROP (%dx%d)", CLASSNAME, __func__, crop->c.width, crop->c.height);
  return true;
}

bool MfcDecoder::SetupCaptureBuffers()
{
  // Request mfc capture buffers
  m_MFCCaptureBuffersCount = CLinuxV4l2::RequestBuffer(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP, m_MFCCaptureBuffersCount);
  if (m_MFCCaptureBuffersCount == V4L2_ERROR)
  {
    CLog::Log(LOGERROR, "%s::%s - MFC CAPTURE REQBUFS failed", CLASSNAME, __func__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s::%s - MFC CAPTURE REQBUFS Number of buffers is %d (extra %d)", CLASSNAME, __func__, m_MFCCaptureBuffersCount, MFC_CAPTURE_EXTRA_BUFFER_CNT);

  // Allocate, Memory Map and queue mfc capture buffers
  m_v4l2MFCCaptureBuffers = (V4L2Buffer *)calloc(m_MFCCaptureBuffersCount, sizeof(V4L2Buffer));
  if (!m_v4l2MFCCaptureBuffers)
  {
    CLog::Log(LOGERROR, "%s::%s - MFC CAPTURE Cannot allocate memory for buffers", CLASSNAME, __func__);
    return false;
  }
  if (!CLinuxV4l2::MmapBuffers(m_iDecoderHandle, m_MFCCaptureBuffersCount, m_v4l2MFCCaptureBuffers, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP, true))
  {
    CLog::Log(LOGERROR, "%s::%s - MFC CAPTURE Cannot mmap memory for buffers", CLASSNAME, __func__);
    return false;
  }
  
  for (int n = 0; n < m_MFCCaptureBuffersCount; n++)
  {
    m_v4l2MFCCaptureBuffers[n].iBytesUsed[0] = m_iMFCCapturePlane1Size;
    m_v4l2MFCCaptureBuffers[n].iBytesUsed[1] = m_iMFCCapturePlane2Size;
    m_v4l2MFCCaptureBuffers[n].bQueue = true;
  }
  CLog::Log(LOGDEBUG, "%s::%s - MFC CAPTURE Succesfully allocated, mmapped and queued %d buffers", CLASSNAME, __func__, m_MFCCaptureBuffersCount);

  // STREAMON on mfc CAPTURE
  if (!CLinuxV4l2::StreamOn(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, VIDIOC_STREAMON))
  {
    CLog::Log(LOGERROR, "%s::%s - MFC CAPTURE Failed to Stream ON", CLASSNAME, __func__);
    return false;
  }
  
  CLog::Log(LOGDEBUG, "%s::%s - MFC CAPTURE Stream ON", CLASSNAME, __func__);
  return true;
}

bool MfcDecoder::DequeueHeader()
{
  // Dequeue header from MFC, we don't need it anymore
  int ret = CLinuxV4l2::DequeueBuffer(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_MMAP, V4L2_NUM_MAX_PLANES);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT error dequeue header, got number %d, errno %d", CLASSNAME, __func__, ret, errno);
    return false;
  }
  
  m_v4l2MFCOutputBuffers[ret].bQueue = false;
  CLog::Log(LOGDEBUG, "%s::%s - MFC OUTPUT -> %d header", CLASSNAME, __func__, ret);

  CLog::Log(LOGNOTICE, "%s::%s - MFC Setup succesfull, start streaming", CLASSNAME, __func__);
  return true;
}

bool MfcDecoder::IsOutputBufferEmpty(int index)
{
  return (m_v4l2MFCOutputBuffers != NULL && index < m_MFCOutputBuffersCount ) ? !m_v4l2MFCOutputBuffers[index].bQueue : false;
}

void MfcDecoder::SetCaptureBufferEmpty(int index)
{
  if (m_v4l2MFCCaptureBuffers != NULL && index < m_MFCCaptureBuffersCount)
    m_v4l2MFCCaptureBuffers[index].bQueue = false;
}

void MfcDecoder::SetCaptureBufferBusy(int index)
{
  if (m_v4l2MFCCaptureBuffers != NULL && index < m_MFCCaptureBuffersCount)
    m_v4l2MFCCaptureBuffers[index].bQueue = true;
}

V4L2Buffer* GetCaptureBuffer(int index)
{
  return m_v4l2MFCCaptureBuffers != NULL && index < m_MFCCaptureBuffersCount ? &(m_v4l2MFCCaptureBuffers[index]) : NULL;
}

void MfcDecoder::Dispose()
{
  CLog::Log(LOGDEBUG, "%s::%s - Freeing memory allocated for buffers", CLASSNAME, __func__);
  if (m_v4l2MFCOutputBuffers)
    m_v4l2MFCOutputBuffers = CLinuxV4l2::FreeBuffers(m_MFCOutputBuffersCount, m_v4l2MFCOutputBuffers);
  if (m_v4l2MFCCaptureBuffers)
    m_v4l2MFCCaptureBuffers = CLinuxV4l2::FreeBuffers(m_MFCCaptureBuffersCount, m_v4l2MFCCaptureBuffers);
  CLog::Log(LOGDEBUG, "%s::%s - Closing MFC device", CLASSNAME, __func__);
  if (m_iDecoderHandle >= 0)
  {
    if (CLinuxV4l2::StreamOn(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, VIDIOC_STREAMOFF))
      CLog::Log(LOGDEBUG, "%s::%s - MFC OUTPUT Stream OFF", CLASSNAME, __func__);
    if (CLinuxV4l2::StreamOn(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, VIDIOC_STREAMOFF))
      CLog::Log(LOGDEBUG, "%s::%s - MFC CAPTURE Stream OFF", CLASSNAME, __func__);
    close(m_iDecoderHandle);
  }
  m_MFCOutputBuffersCount = 0;
  m_v4l2MFCOutputBuffers = NULL;
  m_MFCCaptureBuffersCount = 0;
  m_v4l2MFCCaptureBuffers = NULL;
  m_iDecoderHandle = -1;
}

bool MfcDecoder::DequeueOutputBuffer(int *result)
{
  int ret = CLinuxV4l2::PollOutput(m_iDecoderHandle, 1000); // POLLIN - Capture, POLLOUT - Output
  if (ret == V4L2_ERROR)
  {
    CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT PollOutput Error", CLASSNAME, __func__);
    *result = VC_ERROR;
    return false;
  }
  else
  if (ret == V4L2_READY)
  {
    int index = CLinuxV4l2::DequeueBuffer(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_MMAP, V4L2_NUM_MAX_PLANES);
    if (index < 0)
    {
      CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT error dequeue output buffer, got number %d, errno %d", CLASSNAME, __func__, index, errno);
      *result = VC_ERROR;
      return false;
    }
    m_v4l2MFCOutputBuffers[index].bQueue = false;
    *result = index;
  }
  else
  if (ret == V4L2_BUSY)
  { // buffer is still busy
    CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT All buffers are queued and busy, no space for new frame to decode. Very broken situation.", CLASSNAME, __func__);
    // FIXME This should be handled as abnormal situation that should be addressed, otherwise decoding could stuck here forever
    *result = VC_FLUSHED;
    return false;
  }
  else
  {
    CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT\e[0m PollOutput error %d, errno %d", CLASSNAME, __func__, ret, errno);
    *result = VC_ERROR;
    return false;
  }
  return true; // *result contains the buffer index
}

bool MfcDecoder::SendBuffer(int index, uint8_t* demuxer_content, int demuxer_bytes)
{
  if (m_bVideoConvert)
  {
    m_converter.Convert(demuxer_content, demuxer_bytes);
    demuxer_bytes = m_converter.GetConvertSize();
    demuxer_content = m_converter.GetConvertBuffer();
  }

  if (demuxer_bytes < m_v4l2MFCOutputBuffers[index].iSize[0])
  {
    fast_memcpy((uint8_t *)m_v4l2MFCOutputBuffers[index].cPlane[0], demuxer_content, demuxer_bytes);
    m_v4l2MFCOutputBuffers[index].iBytesUsed[0] = demuxer_bytes;

    int ret = CLinuxV4l2::QueueBuffer(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_MMAP, m_v4l2MFCOutputBuffers[index].iNumPlanes, index, &m_v4l2MFCOutputBuffers[index]);
    if (ret == V4L2_ERROR)
    {
      CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT Failed to queue buffer with index %d, errno %d", CLASSNAME, __func__, index, errno);
      return false;
    }
    m_v4l2MFCOutputBuffers[index].bQueue = true;
  }
  else
  {
    CLog::Log(LOGERROR, "%s::%s - Packet to big for streambuffer", CLASSNAME, __func__);
    return false;
  }
  return true;
}

void MfcDecoder::AddDecodedCaptureBuffer(int index)
{
  if (m_MFCDecodedCaptureBuffers != NULL)
    m_MFCDecodedCaptureBuffers.push(index);
}

void MfcDecoder::RemoveFirstDecodedCaptureBuffer()
{
  if (m_MFCDecodedCaptureBuffers != NULL && !m_MFCDecodedCaptureBuffers.empty())
    m_MFCDecodedCaptureBuffers.pop();
}

bool MfcDecoder::GetFirstDecodedCaptureBuffer(int *index)
{
  if (m_MFCDecodedCaptureBuffers.empty())
    return false;
  *index = m_MFCDecodedCaptureBuffers.front();
  return true; // *index contains the first decoded buffer
}

bool MfcDecoder::DequeueDecodedFrame(int *result)
{
  // Dequeue decoded frame
  int index = CLinuxV4l2::DequeueBuffer(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP, V4L2_NUM_MAX_PLANES);
  if (index < 0)
  {
    if (errno == EAGAIN)
    {
      // Buffer is still busy, queue more
      *result = VC_BUFFER;
      return false;
    }
    CLog::Log(LOGERROR, "%s::%s - MFC CAPTURE error dequeue output buffer, got number %d, errno %d", CLASSNAME, __func__, index, errno);
    *result = VC_ERROR;
    return false;
  }
  m_v4l2MFCCaptureBuffers[index].bQueue = false;
  *result = index;
  return true; // *result contains the buffer index
}

bool MfcDecoder::QueueOutputBuffer(int index)
{
  // Queue dequeued from FIMC OUPUT frame back to MFC CAPTURE
  if (&m_v4l2MFCCaptureBuffers[index] && !m_v4l2MFCCaptureBuffers[index].bQueue)
  {
    int ret = CLinuxV4l2::QueueBuffer(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP, m_v4l2MFCCaptureBuffers[index].iNumPlanes, index, &m_v4l2MFCCaptureBuffers[index]);
    if (ret < 0)
    {
      CLog::Log(LOGERROR, "%s::%s - queue output buffer\n", CLASSNAME, __func__);
      return false;
    }
    m_v4l2MFCCaptureBuffers[index].bQueue = true;
  }
  return true;
}

// FIXME: maybe this function can be better integrated in DVDVideoCodecMfc...
void MfcDecoder::SetOutputBufferPlanes(int m_iDecodedWidth, int m_iDecodedHeight) {
  m_v4l2OutputBuffer.cPlane[0] = new BYTE[m_iDecodedWidth * m_iDecodedHeight];
  m_v4l2OutputBuffer.cPlane[1] = new BYTE[m_iDecodedWidth * (m_iDecodedHeight >> 1)];
}
