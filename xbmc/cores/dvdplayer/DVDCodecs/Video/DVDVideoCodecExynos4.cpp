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
#include "DVDVideoCodecExynos4.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDStreamInfo.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDCodecUtils.h"

#define MAJOR_VERSION 12

#include "settings/Settings.h"
#if MAJOR_VERSION < 13
	#include "settings/GUISettings.h"
#else
	#include "settings/DisplaySettings.h"
	#include "settings/AdvancedSettings.h"
#endif
#include "utils/fastmemcpy.h"

#include <linux/LinuxV4l2.h>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>
#include <dirent.h>

#define USE_FIMC 1 //doesn't work now, do not undefine!

#ifdef CLASSNAME
#undef CLASSNAME
#endif
#define CLASSNAME "CDVDVideoCodecExynos4"

CDVDVideoCodecExynos4::CDVDVideoCodecExynos4() : CDVDVideoCodec() {
  m_v4l2MFCOutputBuffers = NULL;
  m_v4l2MFCCaptureBuffers = NULL;
  m_v4l2FIMCCaptureBuffers = NULL;

  m_MFCOutputBuffersCount = 0;
  m_MFCCaptureBuffersCount = 0;
  m_FIMCOutputBuffersCount = 0;
  m_FIMCCaptureBuffersCount = 0;

  m_iMFCCapturePlane1Size = -1;
  m_iMFCCapturePlane2Size = -1;
  m_iFIMCCapturePlane1Size = -1;
  m_iFIMCCapturePlane2Size = -1;
  m_iFIMCCapturePlane3Size = -1;

  m_iDecodedWidth = 0;
  m_iDecodedHeight = 0;
  m_iConvertedWidth = 0;
  m_iConvertedHeight = 0;
  m_iDecoderHandle = -1;
  m_iConverterHandle = -1;
  m_bVideoConvert = false;
  m_bDropPictures = false;
  
  m_bFIMCStartConverter = true;
  
  m_iFIMCdequeuedBufferNumber = -1;
  
  memzero(m_videoBuffer);
}

CDVDVideoCodecExynos4::~CDVDVideoCodecExynos4() {
  Dispose();
}

bool CDVDVideoCodecExynos4::OpenDevices() {
  DIR *dir;
  struct dirent *ent;

  if ((dir = opendir ("/sys/class/video4linux/")) != NULL) {
    while ((ent = readdir (dir)) != NULL) {
      if (strncmp(ent->d_name, "video", 5) == 0) {
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
        if (fgets(drivername, 32, fp) != NULL) {
          p = strchr(drivername, '\n');
          if (p != NULL)
            *p = '\0';
        } else {
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

        sprintf(devname, "/dev/%s", ++p);

        if (m_iDecoderHandle < 0 && strncmp(drivername, "s5p-mfc-dec", 11) == 0) {
          struct v4l2_capability cap;
          int fd = open(devname, O_RDWR | O_NONBLOCK, 0);
          if (fd > 0) {
            memzero(cap);
            ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
            if (ret == 0)
              if ((cap.capabilities & V4L2_CAP_VIDEO_M2M_MPLANE ||
                ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) && (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT_MPLANE))) &&
                (cap.capabilities & V4L2_CAP_STREAMING)) {
                m_iDecoderHandle = fd;
                CLog::Log(LOGDEBUG, "%s::%s - Found %s %s", CLASSNAME, __func__, drivername, devname);
              }
          }
          if (m_iDecoderHandle < 0)
            close(fd);
        }
        if (m_iConverterHandle < 0 && strstr(drivername, "fimc") != NULL && strstr(drivername, "m2m") != NULL) {
          struct v4l2_capability cap;
          int fd = open(devname, O_RDWR, 0);
          if (fd > 0) {
            memzero(cap);
            ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
            if (ret == 0)
              if ((cap.capabilities & V4L2_CAP_VIDEO_M2M_MPLANE ||
                ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) && (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT_MPLANE))) &&
                (cap.capabilities & V4L2_CAP_STREAMING)) {
                m_iConverterHandle = fd;
                CLog::Log(LOGDEBUG, "%s::%s - Found %s %s", CLASSNAME, __func__, drivername, devname);
              }
          }
          if (m_iConverterHandle < 0)
            close(fd);
        }
        if (m_iDecoderHandle >= 0 && m_iConverterHandle >= 0)
          return true;
      }
    }
    closedir (dir);
  }
  return false;
}

bool CDVDVideoCodecExynos4::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) {
  struct v4l2_format fmt;
  struct v4l2_control ctrl;
  struct v4l2_crop crop;
  int ret = 0;

  if (hints.software)
    return false;

  Dispose();

  if (!OpenDevices()) {
    CLog::Log(LOGDEBUG, "%s::%s - Needed devices not found", CLASSNAME, __func__);
    return false;
  }

  m_bVideoConvert = m_converter.Open(hints.codec, (uint8_t *)hints.extradata, hints.extrasize, true);

  unsigned int extraSize = 0;
  uint8_t *extraData = NULL;

  if(m_bVideoConvert) {
    if(m_converter.GetExtraData() != NULL && m_converter.GetExtraSize() > 0) {
      extraSize = m_converter.GetExtraSize();
      extraData = m_converter.GetExtraData();
    }
  } else {
    if(hints.extrasize > 0 && hints.extradata != NULL) {
      extraSize = hints.extrasize;
      extraData = (uint8_t*)hints.extradata;
    }
  }
  
  // Setup mfc output queue (OUTPUT - name of the queue where TO encoded frames are streamed, CAPTURE - name of the queue where FROM decoded frames are taken)
  // Set mfc output format
  memzero(fmt);
  switch(hints.codec)
  {
/*
    case CODEC_TYPE_VC1_RCV:
      return V4L2_PIX_FMT_VC1_ANNEX_L;
*/
    case CODEC_ID_VC1:
      fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_VC1_ANNEX_G;
      m_name = "mfc-vc1";
      break;
    case CODEC_ID_MPEG1VIDEO:
      fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_MPEG1;
      m_name = "mfc-mpeg1";
      break;
    case CODEC_ID_MPEG2VIDEO:
      fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_MPEG2;
      m_name = "mfc-mpeg2";
      break;
    case CODEC_ID_MPEG4:
      fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_MPEG4;
      m_name = "mfc-mpeg4";
      break;
    case CODEC_ID_H263:
      fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H263;
      m_name = "mfc-h263";
      break;
    case CODEC_ID_H264:
      fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
      m_name = "mfc-h264";
      break;
    default:
      return false;
      break;
  }
  fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
  fmt.fmt.pix_mp.plane_fmt[0].sizeimage = STREAM_BUFFER_SIZE;
  fmt.fmt.pix_mp.num_planes = V4L2_NUM_MAX_PLANES;
  ret = ioctl(m_iDecoderHandle, VIDIOC_S_FMT, &fmt);
  if (ret != 0) {
    CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT S_FMT failed", CLASSNAME, __func__);
    return false;
  }
/* Debug call, unneded for production
  // Get mfc output format
  memzero(fmt);
  fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
  ret = ioctl(m_iDecoderHandle, VIDIOC_G_FMT, &fmt);
  if (ret != 0) {
    CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT G_FMT failed", CLASSNAME, __func__);
    return false;
  }
  msg("\e[1;32mMFC OUTPUT\e[0m Setup MFC decoding buffer size=%u (requested=%u)", CLASSNAME, __func__, fmt.fmt.pix_mp.plane_fmt[0].sizeimage, STREAM_BUFFER_SIZE);
*/  
  // Request mfc output buffers
  m_MFCOutputBuffersCount = CLinuxV4l2::RequestBuffer(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_MMAP, MFC_OUTPUT_BUFFERS_CNT);
  if (m_MFCOutputBuffersCount == V4L2_ERROR) {
    CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT REQBUFS failed", CLASSNAME, __func__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s::%s - MFC OUTPUT REQBUFS Number of MFC buffers is %d (requested %d)", CLASSNAME, __func__, m_MFCOutputBuffersCount, MFC_OUTPUT_BUFFERS_CNT);

  // Memory Map mfc output buffers
  m_v4l2MFCOutputBuffers = (V4L2Buffer *)calloc(m_MFCOutputBuffersCount, sizeof(V4L2Buffer));
  if(!m_v4l2MFCOutputBuffers) {
    CLog::Log(LOGERROR, "%s::%s - MFC cannot allocate OUTPUT buffers in memory", CLASSNAME, __func__);
    return false;
  }
  if(!CLinuxV4l2::MmapBuffers(m_iDecoderHandle, m_MFCOutputBuffersCount, m_v4l2MFCOutputBuffers, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_MMAP, false)) {
    CLog::Log(LOGERROR, "%s::%s - MFC Cannot mmap OUTPUT buffers", CLASSNAME, __func__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s::%s - MFC OUTPUT Succesfully mmapped %d buffers", CLASSNAME, __func__, m_MFCOutputBuffersCount);

  // Prepare header
  m_v4l2MFCOutputBuffers[0].iBytesUsed[0] = extraSize;
  fast_memcpy((uint8_t *)m_v4l2MFCOutputBuffers[0].cPlane[0], extraData, extraSize);

  // Queue header to mfc output queue
  ret = CLinuxV4l2::QueueBuffer(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_MMAP, m_v4l2MFCOutputBuffers[0].iNumPlanes, 0, &m_v4l2MFCOutputBuffers[0]);
  if (ret == V4L2_ERROR) {
    CLog::Log(LOGERROR, "%s::%s - MFC Error queuing header", CLASSNAME, __func__);
    return false;
  }
  m_v4l2MFCOutputBuffers[ret].bQueue = true;
  CLog::Log(LOGDEBUG, "%s::%s - MFC OUTPUT <- %d header of size %d", CLASSNAME, __func__, ret, extraSize);

  // STREAMON on mfc OUTPUT
  if (!CLinuxV4l2::StreamOn(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, VIDIOC_STREAMON)) {
    CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT Failed to Stream ON", CLASSNAME, __func__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s::%s - MFC OUTPUT Stream ON", CLASSNAME, __func__);

  // Setup mfc capture queue
  // Get mfc capture picture format
  memzero(fmt);
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  ret = ioctl(m_iDecoderHandle, VIDIOC_G_FMT, &fmt);
  if (ret) {
    CLog::Log(LOGERROR, "%s::%s - MFC CAPTURE G_FMT Failed", CLASSNAME, __func__);
    return false;
  }
  m_iMFCCapturePlane1Size = fmt.fmt.pix_mp.plane_fmt[0].sizeimage;
  m_iMFCCapturePlane2Size = fmt.fmt.pix_mp.plane_fmt[1].sizeimage;
  CLog::Log(LOGDEBUG, "%s::%s - MFC CAPTURE G_FMT: fmt (%dx%d), plane[0]=%d plane[1]=%d", CLASSNAME, __func__, fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height, m_iMFCCapturePlane1Size, m_iMFCCapturePlane2Size);

#ifdef USE_FIMC
  // Setup FIMC OUTPUT fmt with data from MFC CAPTURE received on previous step
  fmt.type                                = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
  fmt.fmt.pix_mp.field                    = V4L2_FIELD_ANY;
  fmt.fmt.pix_mp.pixelformat              = V4L2_PIX_FMT_NV12MT;
  fmt.fmt.pix_mp.num_planes               = V4L2_NUM_MAX_PLANES;
  ret = ioctl(m_iConverterHandle, VIDIOC_S_FMT, &fmt);
  if (ret != 0) {
    CLog::Log(LOGERROR, "%s::%s - FIMC OUTPUT S_FMT Failed", CLASSNAME, __func__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s::%s - FIMC OUTPUT S_FMT (%dx%d)", CLASSNAME, __func__, fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height);
#endif

  // Get mfc needed number of buffers
  memzero(ctrl);
  ctrl.id = V4L2_CID_MIN_BUFFERS_FOR_CAPTURE;
  ret = ioctl(m_iDecoderHandle, VIDIOC_G_CTRL, &ctrl);
  if (ret) {
    CLog::Log(LOGERROR, "%s::%s - MFC CAPTURE Failed to get the number of buffers required", CLASSNAME, __func__);
    return false;
  }
  m_MFCCaptureBuffersCount = ctrl.value + MFC_CAPTURE_EXTRA_BUFFER_CNT;

  // Get mfc capture crop
  memzero(crop);
  crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  ret = ioctl(m_iDecoderHandle, VIDIOC_G_CROP, &crop);
  if (ret) {
    CLog::Log(LOGERROR, "%s::%s - MFC CAPTURE G_CROP Failed to get crop information", CLASSNAME, __func__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s::%s - MFC CAPTURE G_CROP (%dx%d)", CLASSNAME, __func__, crop.c.width, crop.c.height);
  m_iDecodedWidth = crop.c.width;
  m_iDecodedHeight = crop.c.height;

#ifdef USE_FIMC
  // Setup FIMC OUTPUT crop with data from MFC CAPTURE received on previous step
  crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
  if (ioctl(m_iConverterHandle, VIDIOC_S_CROP, &crop)) {
    CLog::Log(LOGERROR, "%s::%s - FIMC OUTPUT S_CROP Failed to set crop information", CLASSNAME, __func__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s::%s - FIMC OUTPUT S_CROP (%dx%d)", CLASSNAME, __func__, crop.c.width, crop.c.height);

  // Calculate FIMC final picture size be scaled to fit screen
#if MAJOR_VERSION < 13
  RESOLUTION_INFO& res_info = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()];
#else
  RESOLUTION_INFO res_info =  CDisplaySettings::Get().GetResolutionInfo(g_graphicsContext.GetVideoResolution());
#endif
  double ratio = std::min((double)res_info.iScreenWidth / (double)m_iDecodedWidth, (double)res_info.iScreenHeight / (double)m_iDecodedHeight);
  int width = (int)((double)m_iDecodedWidth * ratio);
  int height = (int)((double)m_iDecodedHeight * ratio);
  if (width%2)
    width--;
  if (height%2)
    height--;
#else
  // No scaling, resulting picture will be the same size as source
  int width = m_iDecodedWidth;
  int height = m_iDecodedHeight;
#endif

  // Request mfc capture buffers
  m_MFCCaptureBuffersCount = CLinuxV4l2::RequestBuffer(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP, m_MFCCaptureBuffersCount);
  if (m_MFCCaptureBuffersCount == V4L2_ERROR) {
    CLog::Log(LOGERROR, "%s::%s - MFC CAPTURE REQBUFS failed", CLASSNAME, __func__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s::%s - MFC CAPTURE REQBUFS Number of buffers is %d (extra %d)", CLASSNAME, __func__, m_MFCCaptureBuffersCount, MFC_CAPTURE_EXTRA_BUFFER_CNT);

  // Allocate, Memory Map and queue mfc capture buffers
  m_v4l2MFCCaptureBuffers = (V4L2Buffer *)calloc(m_MFCCaptureBuffersCount, sizeof(V4L2Buffer));
  if(!m_v4l2MFCCaptureBuffers) {
    CLog::Log(LOGERROR, "%s::%s - MFC CAPTURE Cannot allocate memory for buffers", CLASSNAME, __func__);
    return false;
  }
  if(!CLinuxV4l2::MmapBuffers(m_iDecoderHandle, m_MFCCaptureBuffersCount, m_v4l2MFCCaptureBuffers, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP, true)) {
    CLog::Log(LOGERROR, "%s::%s - MFC CAPTURE Cannot mmap memory for buffers", CLASSNAME, __func__);
    return false;
  }
  for (int n = 0; n < m_MFCCaptureBuffersCount; n++) {
    m_v4l2MFCCaptureBuffers[n].iBytesUsed[0] = m_iMFCCapturePlane1Size;
    m_v4l2MFCCaptureBuffers[n].iBytesUsed[1] = m_iMFCCapturePlane2Size;
    m_v4l2MFCCaptureBuffers[n].bQueue = true;
  }
  CLog::Log(LOGDEBUG, "%s::%s - MFC CAPTURE Succesfully allocated, mmapped and queued %d buffers", CLASSNAME, __func__, m_MFCCaptureBuffersCount);

  // STREAMON on mfc CAPTURE
  if (!CLinuxV4l2::StreamOn(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, VIDIOC_STREAMON)) {
	CLog::Log(LOGERROR, "%s::%s - MFC CAPTURE Failed to Stream ON", CLASSNAME, __func__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s::%s - MFC CAPTURE Stream ON", CLASSNAME, __func__);

#ifdef USE_FIMC
  // Request fimc capture buffers
  ret = CLinuxV4l2::RequestBuffer(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_USERPTR, m_MFCCaptureBuffersCount);
  if (ret == V4L2_ERROR) {
    CLog::Log(LOGERROR, "%s::%s - FIMC OUTPUT REQBUFS failed", CLASSNAME, __func__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s::%s - FIMC OUTPUT REQBUFS Number of buffers is %d", CLASSNAME, __func__, ret);
  
  // Setup fimc capture
  memzero(fmt);
  fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420M;
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  fmt.fmt.pix_mp.width = width;
  fmt.fmt.pix_mp.height = height;
  fmt.fmt.pix_mp.num_planes = V4L2_NUM_MAX_PLANES;
  fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
  ret = ioctl(m_iConverterHandle, VIDIOC_S_FMT, &fmt);
  if (ret != 0) {
    CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE S_FMT Failed", CLASSNAME, __func__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s::%s - FIMC CAPTURE S_FMT %dx%d", CLASSNAME, __func__, fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height);
  m_iConvertedWidth = fmt.fmt.pix_mp.width;
  m_iConvertedHeight = fmt.fmt.pix_mp.height;
#else
  m_iConvertedWidth = width;
  m_iConvertedHeight = height;
#endif

#ifdef USE_FIMC
  // Setup FIMC CAPTURE crop
  memzero(crop);
  crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  crop.c.left = 0;
  crop.c.top = 0;
  crop.c.width = m_iConvertedWidth;
  crop.c.height = m_iConvertedHeight;
  if (ioctl(m_iConverterHandle, VIDIOC_S_CROP, &crop)) {
    CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE S_CROP Failed", CLASSNAME, __func__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s::%s - FIMC CAPTURE S_CROP (%dx%d)", CLASSNAME, __func__, crop.c.width, crop.c.height);

  // Get FIMC produced picture details to adjust output buffer parameters with these values
  memzero(fmt);
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  ret = ioctl(m_iConverterHandle, VIDIOC_G_FMT, &fmt);
  if (ret) {
    CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE G_FMT Failed", CLASSNAME, __func__);
    return false;
  }
  m_iFIMCCapturePlane1Size = fmt.fmt.pix_mp.plane_fmt[0].sizeimage;
  m_iFIMCCapturePlane2Size = fmt.fmt.pix_mp.plane_fmt[1].sizeimage;
  m_iFIMCCapturePlane3Size = fmt.fmt.pix_mp.plane_fmt[2].sizeimage;

  // Request fimc capture buffers
  m_FIMCCaptureBuffersCount = CLinuxV4l2::RequestBuffer(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP, FIMC_CAPTURE_BUFFERS_CNT);
  if (m_FIMCCaptureBuffersCount == V4L2_ERROR) {
    CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE REQBUFS failed", CLASSNAME, __func__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s::%s - FIMC CAPTURE REQBUFS Number of buffers is %d", CLASSNAME, __func__, m_FIMCCaptureBuffersCount);
  CLog::Log(LOGDEBUG, "%s::%s - FIMC CAPTURE buffer parameters: plane[0]=%d plane[1]=%d plane[2]=%d", CLASSNAME, __func__, m_iFIMCCapturePlane1Size, m_iFIMCCapturePlane2Size, m_iFIMCCapturePlane3Size);

  // Allocate, Memory Map and queue fimc capture buffers
  m_v4l2FIMCCaptureBuffers = (V4L2Buffer *)calloc(m_FIMCCaptureBuffersCount, sizeof(V4L2Buffer));
  if(!m_v4l2FIMCCaptureBuffers) {
   CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE Cannot allocate memory for buffers", CLASSNAME, __func__);
   return false;
  }
  if(!CLinuxV4l2::MmapBuffers(m_iConverterHandle, m_FIMCCaptureBuffersCount, m_v4l2FIMCCaptureBuffers, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP, true)) {
    CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE Cannot mmap for capture buffers", CLASSNAME, __func__);
    return false;
  }
  for (int n = 0; n < m_FIMCCaptureBuffersCount; n++) {
    m_v4l2FIMCCaptureBuffers[n].iBytesUsed[0] = m_iFIMCCapturePlane1Size;
    m_v4l2FIMCCaptureBuffers[n].iBytesUsed[1] = m_iFIMCCapturePlane2Size;
    m_v4l2FIMCCaptureBuffers[n].iBytesUsed[2] = m_iFIMCCapturePlane3Size;
    m_v4l2FIMCCaptureBuffers[n].bQueue = true;
  }
  CLog::Log(LOGDEBUG, "%s::%s - FIMC CAPTURE Succesfully allocated, mmapped and queued %d buffers", CLASSNAME, __func__, m_FIMCCaptureBuffersCount);
#endif

  // Dequeue header from MFC, we don't need it anymore
  ret = CLinuxV4l2::DequeueBuffer(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_MMAP, V4L2_NUM_MAX_PLANES);
  if (ret < 0) {
    CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT error dequeue header, got number %d, errno %d", CLASSNAME, __func__, ret, errno);
    return false;
  }
  m_v4l2MFCOutputBuffers[ret].bQueue = false;
  CLog::Log(LOGDEBUG, "%s::%s - MFC OUTPUT -> %d header", CLASSNAME, __func__, ret);

  CLog::Log(LOGNOTICE, "%s::%s - MFC Setup succesfull, start streaming", CLASSNAME, __func__);
  printf("%s::%s - MFC Setup succesfull, start streaming\n", CLASSNAME, __func__);
  
  return true;
}

void CDVDVideoCodecExynos4::Dispose() {
  CLog::Log(LOGDEBUG, "%s::%s - Freeing memory allocated for buffers", CLASSNAME, __func__);
  if (m_v4l2MFCOutputBuffers)
    m_v4l2MFCOutputBuffers = CLinuxV4l2::FreeBuffers(m_MFCOutputBuffersCount, m_v4l2MFCOutputBuffers);
  if (m_v4l2MFCCaptureBuffers)
    m_v4l2MFCCaptureBuffers = CLinuxV4l2::FreeBuffers(m_MFCCaptureBuffersCount, m_v4l2MFCCaptureBuffers);
  if (m_v4l2FIMCCaptureBuffers)
    m_v4l2FIMCCaptureBuffers = CLinuxV4l2::FreeBuffers(m_FIMCCaptureBuffersCount, m_v4l2FIMCCaptureBuffers);
  CLog::Log(LOGDEBUG, "%s::%s - Closing devices", CLASSNAME, __func__);
  if (m_iDecoderHandle >= 0) {
    if (CLinuxV4l2::StreamOn(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, VIDIOC_STREAMOFF))
      CLog::Log(LOGDEBUG, "%s::%s - MFC OUTPUT Stream OFF", CLASSNAME, __func__);
    if (CLinuxV4l2::StreamOn(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, VIDIOC_STREAMOFF))
      CLog::Log(LOGDEBUG, "%s::%s - MFC CAPTURE Stream OFF", CLASSNAME, __func__);
    close(m_iDecoderHandle);
  }
  if (m_iConverterHandle >= 0) {
    if (CLinuxV4l2::StreamOn(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, VIDIOC_STREAMOFF))
      CLog::Log(LOGDEBUG, "%s::%s - FIMC OUTPUT Stream OFF", CLASSNAME, __func__);
    if (CLinuxV4l2::StreamOn(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, VIDIOC_STREAMOFF))
      CLog::Log(LOGDEBUG, "%s::%s - FIMC CAPTURE Stream OFF", CLASSNAME, __func__);
    close(m_iConverterHandle);
  }

  while(!m_pts.empty())
    m_pts.pop();
  while(!m_dts.empty())
    m_dts.pop();

  m_iDecodedWidth = 0;
  m_iDecodedHeight = 0;
  m_iConvertedWidth = 0;
  m_iConvertedHeight = 0;
  m_iDecoderHandle = -1;
  m_iConverterHandle = -1;
  m_bVideoConvert = false;
  m_bDropPictures = false;

  memzero(m_videoBuffer);
}

void CDVDVideoCodecExynos4::SetDropState(bool bDrop) {

  m_bDropPictures = bDrop;

}

int CDVDVideoCodecExynos4::Decode(BYTE* pData, int iSize, double dts, double pts) {
  int ret = -1;
  int index = 0;

//  unsigned int dtime = XbmcThreads::SystemClockMillis();

  if(pData) {
    int demuxer_bytes = iSize;
    uint8_t *demuxer_content = pData;

    //printf("%s::%s - Frame of size %d, pts = %f, dts = %f", demuxer_bytes, pts, dts);

	// Find buffer ready to be filled
    while (index < m_MFCOutputBuffersCount && m_v4l2MFCOutputBuffers[index].bQueue)
      index++;

    if (index >= m_MFCOutputBuffersCount) { //all input buffers are busy, dequeue needed
      ret = CLinuxV4l2::PollOutput(m_iDecoderHandle, 1000); // POLLIN - Capture, POLLOUT - Output
      if (ret == V4L2_ERROR) {
        CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT PollOutput Error", CLASSNAME, __func__);
        return VC_ERROR;
      } else if (ret == V4L2_READY) {
        index = CLinuxV4l2::DequeueBuffer(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_MMAP, V4L2_NUM_MAX_PLANES);
        if (index < 0) {
          CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT error dequeue output buffer, got number %d, errno %d", CLASSNAME, __func__, index, errno);
          return VC_ERROR;
        }
        //printf("\e[1;32mMFC OUTPUT\e[0m -> %d", index);
        m_v4l2MFCOutputBuffers[index].bQueue = false;
      } else if (ret == V4L2_BUSY) { // buffer is still busy
        CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT All buffers are queued and busy, no space for new frame to decode. Very broken situation.", CLASSNAME, __func__);
        /* FIXME This should be handled as abnormal situation that should be addressed, otherwise decoding will stuck here forever */
        return VC_FLUSHED;
      } else {
        CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT\e[0m PollOutput error %d, errno %d", CLASSNAME, __func__, ret, errno);
        return VC_ERROR;
      }
    }

    if(m_bVideoConvert) {
      m_converter.Convert(demuxer_content, demuxer_bytes);
      demuxer_bytes = m_converter.GetConvertSize();
      demuxer_content = m_converter.GetConvertBuffer();
    }

    if(demuxer_bytes < m_v4l2MFCOutputBuffers[index].iSize[0]) {
      m_pts.push(-pts);
      m_dts.push(-dts);

      fast_memcpy((uint8_t *)m_v4l2MFCOutputBuffers[index].cPlane[0], demuxer_content, demuxer_bytes);
      m_v4l2MFCOutputBuffers[index].iBytesUsed[0] = demuxer_bytes;

      ret = CLinuxV4l2::QueueBuffer(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_MMAP, m_v4l2MFCOutputBuffers[index].iNumPlanes, index, &m_v4l2MFCOutputBuffers[index]);
      if (ret == V4L2_ERROR) {
        CLog::Log(LOGERROR, "%s::%s - MFC OUTPUT Failed to queue buffer with index %d, errno %d", CLASSNAME, __func__, index, errno);
        return VC_ERROR;
      }
      m_v4l2MFCOutputBuffers[index].bQueue = true;
      //printf("\e[1;32mMFC OUTPUT\e[0m %d <-", ret);
    } else
      CLog::Log(LOGERROR, "%s::%s - Packet to big for streambuffer", CLASSNAME, __func__);
  }

  // Dequeue decoded frame
  index = CLinuxV4l2::DequeueBuffer(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP, V4L2_NUM_MAX_PLANES);
  if (index < 0) {
    if (errno == EAGAIN) // Buffer is still busy, queue more
      return VC_BUFFER;
    CLog::Log(LOGERROR, "%s::%s - MFC CAPTURE error dequeue output buffer, got number %d, errno %d", CLASSNAME, __func__, ret, errno);
    return VC_ERROR;
  }
  //printf("\e[1;32mMFC CAPTURE\e[0m -> %d", index);
  m_v4l2MFCCaptureBuffers[index].bQueue = false;

  if (m_bDropPictures) {
    m_videoBuffer.iFlags      |= DVP_FLAG_DROPPED;
    CLog::Log(LOGDEBUG, "%s::%s - Dropping frame with index %d", CLASSNAME, __func__, index);
  } else {
#ifdef USE_FIMC
    if (m_iFIMCdequeuedBufferNumber >= 0 && !m_v4l2FIMCCaptureBuffers[m_iFIMCdequeuedBufferNumber].bQueue) {
      ret = CLinuxV4l2::QueueBuffer(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP, m_v4l2FIMCCaptureBuffers[m_iFIMCdequeuedBufferNumber].iNumPlanes, m_iFIMCdequeuedBufferNumber, &m_v4l2FIMCCaptureBuffers[m_iFIMCdequeuedBufferNumber]);
      if (ret == V4L2_ERROR) {
        CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE Failed to queue buffer with index %d, errno = %d", CLASSNAME, __func__, m_iFIMCdequeuedBufferNumber, errno);
        return VC_ERROR;
      }
      //printf("\e[1;31mFIMC CAPTURE\e[0m %d <-", ret);
      m_v4l2FIMCCaptureBuffers[ret].bQueue = true;
      m_iFIMCdequeuedBufferNumber = -1;
    }

    ret = CLinuxV4l2::QueueBuffer(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_USERPTR, m_v4l2MFCCaptureBuffers[index].iNumPlanes, index, &m_v4l2MFCCaptureBuffers[index]);
    if (ret == V4L2_ERROR) {
      CLog::Log(LOGERROR, "%s::%s - FIMC OUTPUT Failed to queue buffer with index %d, errno %d", CLASSNAME, __func__, ret, errno);
      return VC_ERROR;
    }
    m_v4l2MFCCaptureBuffers[ret].bQueue = true;
    //printf("\e[1;31mFIMC OUTPUT\e[0m %d <-", ret);

    if (m_bFIMCStartConverter) {
      if (CLinuxV4l2::StreamOn(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, VIDIOC_STREAMON))
        CLog::Log(LOGDEBUG, "%s::%s - FIMC OUTPUT Stream ON", CLASSNAME, __func__);
      else
        CLog::Log(LOGERROR, "%s::%s - FIMC OUTPUT Failed to Stream ON", CLASSNAME, __func__);
      if (CLinuxV4l2::StreamOn(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, VIDIOC_STREAMON))
        CLog::Log(LOGDEBUG, "%s::%s - FIMC CAPTURE Stream ON", CLASSNAME, __func__);
      else
        CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE Failed to Stream ON", CLASSNAME, __func__);
      m_bFIMCStartConverter = false;
      return VC_BUFFER; //Queue one more frame for double buffering on FIMC
    }

    ret = CLinuxV4l2::PollOutput(m_iConverterHandle, 1000/25); // 25 fps
    if (ret == V4L2_ERROR) {
      CLog::Log(LOGERROR, "%s::%s - FIMC PollOutput Error", CLASSNAME, __func__);
      return VC_ERROR;
    } else if (ret == V4L2_READY) {
      // Dequeue frame from fimc output
      index = CLinuxV4l2::DequeueBuffer(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_USERPTR, V4L2_NUM_MAX_PLANES);
      if (index < 0) {
        CLog::Log(LOGERROR, "%s::%s - FIMC OUTPUT error dequeue output buffer, got number %d", CLASSNAME, __func__, index);
        return VC_ERROR;
      }
      //printf("\e[1;31mFIMC OUTPUT\e[0m -> %d", index);
      m_v4l2MFCCaptureBuffers[index].bQueue = false;
    } else if (ret == V4L2_BUSY) { // buffer is still busy
      CLog::Log(LOGERROR, "%s::%s - FIMC OUTPUT Buffer is still busy", CLASSNAME, __func__);
      return VC_ERROR;
    } else {
      CLog::Log(LOGERROR, "%s::%s - FIMC PollOutput error %d, errno %d", CLASSNAME, __func__, ret, errno);
      return VC_ERROR;
    }

    m_iFIMCdequeuedBufferNumber = CLinuxV4l2::DequeueBuffer(m_iConverterHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP, V4L2_NUM_MAX_PLANES);
    if (m_iFIMCdequeuedBufferNumber < 0) {
      if (errno == EAGAIN) // Dequeue buffer not ready, need more data on input. EAGAIN = 11
        return VC_BUFFER;
      CLog::Log(LOGERROR, "%s::%s - FIMC CAPTURE error dequeue output buffer, got number %d, errno %d", CLASSNAME, __func__, m_iFIMCdequeuedBufferNumber, errno);
      return VC_ERROR;
    }
    //printf("\e[1;31mFIMC CAPTURE\e[0m -> %d", m_iFIMCdequeuedBufferNumber);
    m_v4l2FIMCCaptureBuffers[m_iFIMCdequeuedBufferNumber].bQueue = false;
#endif

    m_videoBuffer.iFlags          = DVP_FLAG_ALLOCATED;
    m_videoBuffer.color_range     = 0;
    m_videoBuffer.color_matrix    = 4;

    m_videoBuffer.iDisplayWidth   = m_iConvertedWidth;
    m_videoBuffer.iDisplayHeight  = m_iConvertedHeight;
    m_videoBuffer.iWidth          = m_iConvertedWidth;
    m_videoBuffer.iHeight         = m_iConvertedHeight;

    m_videoBuffer.data[0]         = 0;
    m_videoBuffer.data[1]         = 0;
    m_videoBuffer.data[2]         = 0;
    m_videoBuffer.data[3]         = 0;
    
#ifdef USE_FIMC
    m_videoBuffer.format          = RENDER_FMT_YUV420P;
    m_videoBuffer.iLineSize[0]    = m_iConvertedWidth;
    m_videoBuffer.iLineSize[1]    = m_iConvertedWidth >> 1;
    m_videoBuffer.iLineSize[2]    = m_iConvertedWidth >> 1;
    m_videoBuffer.iLineSize[3]    = 0;
    m_videoBuffer.data[0]         = (BYTE*)m_v4l2FIMCCaptureBuffers[m_iFIMCdequeuedBufferNumber].cPlane[0];
    m_videoBuffer.data[1]         = (BYTE*)m_v4l2FIMCCaptureBuffers[m_iFIMCdequeuedBufferNumber].cPlane[1];
    m_videoBuffer.data[2]         = (BYTE*)m_v4l2FIMCCaptureBuffers[m_iFIMCdequeuedBufferNumber].cPlane[2];
#else
    m_videoBuffer.format          = RENDER_FMT_NV12MT;
    m_videoBuffer.iLineSize[0]    = m_iConvertedWidth;
    m_videoBuffer.iLineSize[1]    = m_iConvertedWidth;
    m_videoBuffer.iLineSize[2]    = 0;
    m_videoBuffer.iLineSize[3]    = 0;
    m_videoBuffer.data[0]         = (BYTE*)m_v4l2MFCCaptureBuffers[index].cPlane[0];
    m_videoBuffer.data[1]         = (BYTE*)m_v4l2MFCCaptureBuffers[index].cPlane[1];
#endif
  }

  // Pop pts/dts only when picture is finally ready to be showed up or skipped
  if(m_pts.size()) {
    m_videoBuffer.pts = -m_pts.top(); // MFC always return frames in order and assigning them their pts'es from the input
                                      // will lead to reshuffle. This will assign least pts in the queue to the frame dequeued.
    m_videoBuffer.dts = -m_dts.top();
    
    m_pts.pop();
    m_dts.pop();
  } else {
    CLog::Log(LOGERROR, "%s::%s - no pts value", CLASSNAME, __func__);
    m_videoBuffer.pts           = DVD_NOPTS_VALUE;
    m_videoBuffer.dts           = DVD_NOPTS_VALUE;
  }

  // Queue dequeued from FIMC OUPUT frame back to MFC CAPTURE
  if (&m_v4l2MFCCaptureBuffers[index] && !m_v4l2MFCCaptureBuffers[index].bQueue) {
    int ret = CLinuxV4l2::QueueBuffer(m_iDecoderHandle, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP, m_v4l2MFCCaptureBuffers[index].iNumPlanes, index, &m_v4l2MFCCaptureBuffers[index]);
    if (ret < 0) {
      CLog::Log(LOGERROR, "%s::%s - queue output buffer\n", CLASSNAME, __func__);
      m_videoBuffer.iFlags      |= DVP_FLAG_DROPPED;
      m_videoBuffer.iFlags      &= DVP_FLAG_ALLOCATED;
      return VC_ERROR;
    }
    m_v4l2MFCCaptureBuffers[index].bQueue = true;
    //printf("\e[1;32mMFC CAPTURE\e[0m <- %d", ret);
  }

//  msg("Decode time: %d", XbmcThreads::SystemClockMillis() - dtime);
  
  return VC_PICTURE; // Picture is finally ready to be processed further
}

void CDVDVideoCodecExynos4::Reset() {

}

bool CDVDVideoCodecExynos4::GetPicture(DVDVideoPicture* pDvdVideoPicture) {

  *pDvdVideoPicture = m_videoBuffer;

  return true;
}

bool CDVDVideoCodecExynos4::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
  return CDVDVideoCodec::ClearPicture(pDvdVideoPicture);
}
