#pragma once

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

#include "DVDStreamInfo.h"

#include "xbmc/linux/LinuxV4l2.h"

#define FIMC_CAPTURE_BUFFERS_CNT      3 // 2 begins to be slow.

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

class FimcConverter
{
public:
  FimcConverter();
  ~FimcConverter();
  void Dispose();

  bool OpenDevice();
  bool RequestBuffers(int buffers);
  bool SetOutputFormat(struct v4l2_format* fmt);
  bool SetOutputCrop(struct v4l2_crop* crop);
  bool SetCaptureFormat(struct v4l2_format* fmt);
  bool SetCaptureCrop(struct v4l2_crop* crop);
  bool SetupCaptureBuffers();

  bool StartConverter();
  bool QueueCaptureBuffer();
  bool QueueOutputBuffer(int index, V4L2Buffer* v4l2MFCCaptureBuffer, int *status);
  bool DequeueOutputBuffer(int *status);
  bool DequeueCaptureBuffer(int *status);
  V4L2Buffer* GetCurrentCaptureBuffer() { return &m_v4l2FIMCCaptureBuffers[m_iFIMCdequeuedBufferNumber]; };
private:
  int m_iConverterHandle;

  int m_FIMCCaptureBuffersCount;
  V4L2Buffer *m_v4l2FIMCCaptureBuffers;

  int m_iFIMCCapturePlane1Size;
  int m_iFIMCCapturePlane2Size;
  int m_iFIMCCapturePlane3Size;

  int m_iFIMCdequeuedBufferNumber;
  bool m_bFIMCStartConverter;
};

