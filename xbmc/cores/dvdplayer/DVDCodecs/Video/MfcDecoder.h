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
#include "utils/BitstreamConverter.h"
#include <vector>
#include <queue>

#include "xbmc/linux/LinuxV4l2.h"

#define STREAM_BUFFER_SIZE            1572864	//compressed frame size. 1080p mpeg4 10Mb/s can be un to 786k in size, so this is to make sure frame fits into buffer
#define MFC_OUTPUT_BUFFERS_CNT        2		//1 doesn't work at all
#define MFC_CAPTURE_EXTRA_BUFFER_CNT  3		//these are extra buffers, better keep their count as big as going to be simultaneous dequeued buffers number

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
  ~MfcDecoder();
  void Dispose();

  bool OpenDevice();
  //const std::vector<struct v4l2_fmtdesc>& GetFormats() { return formats; };
  bool HasNV12Support() { return hasNV12Support; };
  bool SetupOutputFormat(CDVDStreamInfo &hints);
  const char* GetOutputName() { return m_name.c_str(); };
  bool RequestBuffers();
  int  GetCaptureBuffersCount() { return m_MFCCaptureBuffersCount; };
  int  GetOutputBuffersCount()  { return m_MFCOutputBuffersCount;  };
  bool SetupOutputBuffers();
  bool QueueHeader(CDVDStreamInfo &hints);
  bool GetCaptureFormat(struct v4l2_format* fmt);
  bool GetCaptureCrop(struct v4l2_crop* crop);
  bool SetupCaptureBuffers();
  bool DequeueHeader();

  bool IsOutputBufferEmpty(int index) { return !m_v4l2MFCOutputBuffers[index].bQueue; };
  void SetCaptureBufferEmpty(int index) { m_v4l2MFCCaptureBuffers[index].bQueue = false; };
  void SetCaptureBufferBusy(int index)  { m_v4l2MFCCaptureBuffers[index].bQueue = true;  };
  V4L2Buffer* GetCaptureBuffer(int index) { return &(m_v4l2MFCCaptureBuffers[index]); };
  bool DequeueOutputBuffer(int *result);
  bool SendBuffer(int index, uint8_t* demuxer_content, int demuxer_bytes);
  bool DequeueDecodedFrame(int *result);
  bool QueueOutputBuffer(int index);

  void AddDecodedCaptureBuffer(int index) { m_MFCDecodedCaptureBuffers.push(index); };
  bool GetFirstDecodedCaptureBuffer(int *index);
  void RemoveFirstDecodedCaptureBuffer() { m_MFCDecodedCaptureBuffers.pop(); };
  bool SetCaptureFormat();

  void SetOutputBufferPlanes(int m_iDecodedWidth, int m_iDecodedHeight);

private:
  std::string m_name;
  int m_iDecoderHandle;
  bool hasNV12Support;
  bool hasNV12MTSupport;
  //std::vector<struct v4l2_fmtdesc> formats;

  int m_MFCOutputBuffersCount;
  V4L2Buffer *m_v4l2MFCOutputBuffers;

  int m_MFCCaptureBuffersCount;
  V4L2Buffer *m_v4l2MFCCaptureBuffers;

  int m_iMFCCapturePlane1Size;
  int m_iMFCCapturePlane2Size;

  bool m_bVideoConvert;
  CBitstreamConverter m_converter;

  std::queue<int> m_MFCDecodedCaptureBuffers;

  V4L2Buffer m_v4l2OutputBuffer;

};

