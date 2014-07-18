#pragma once

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

#include "DVDVideoCodec.h"
#include "DVDStreamInfo.h"
#include "utils/log.h"
#include "utils/BitstreamConverter.h"

#include "xbmc/linux/LinuxV4l2.h"

#define STREAM_BUFFER_SIZE              1048576 // compressed frame size. 1080p mpeg4 10Mb/s can be up to 786k in size, so this is to make sure frame fits into buffer
						// for unknown reason, possibly firmware bug, if set to other values, it corrupts adjacent value in the setup data structure for h264 streams
#define MFC_OUTPUT_BUFFERS_CNT          2       // 1 doesn't work at all
#define MFC_CAPTURE_EXTRA_BUFFER_CNT    16      // these are extra buffers, better keep their count as big as going to be simultaneous dequeued buffers number

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif

class MfcDecoder
{
public:
  MfcDecoder();
  virtual ~MfcDecoder();
  void Dispose();

  bool        OpenDevice();
  bool        HasNV12Support() { return m_hasNV12Support; }; // NV12 support should be starting with MFC6
  bool        SetupOutputFormat(CDVDStreamInfo &hints);
  const char* GetOutputName() { return m_name.c_str(); };
  bool        RequestBuffers();
  bool        SetupCaptureBuffers();
  int         GetCaptureBuffersCount() { return m_MFCCaptureBuffersCount; };
  V4L2Buffer* GetCaptureBuffer(int index);
  bool        SetCaptureFormat();
  bool        GetCaptureFormat(struct v4l2_format* fmt);
  bool        GetCaptureCrop(struct v4l2_crop* crop);
  bool        SetupOutputBuffers();
  int         GetOutputBuffersCount()  { return m_MFCOutputBuffersCount;  };
  bool        IsOutputBufferEmpty(int index);

  bool        QueueHeader(CDVDStreamInfo &hints);
  bool        SendBuffer(int index, uint8_t* demuxer_content, int demuxer_bytes, double pts);
  bool        DequeueOutputBuffer(int *result, double *timestamp);
  bool        QueueCaptureBuffer(int index);
  bool        DequeueDecodedFrame(int *result, double *timestamp);

private:
  int                 m_iDecoderHandle;
  bool                HasNV12MTSupport();
  bool                m_hasNV12Support;
  std::string         m_name;
  int                 m_MFCOutputBuffersCount;
  V4L2Buffer         *m_v4l2MFCOutputBuffers;
  int                 m_MFCCaptureBuffersCount;
  V4L2Buffer         *m_v4l2MFCCaptureBuffers;
  bool                m_bVideoConvert;
  CBitstreamConverter m_converter;
};
