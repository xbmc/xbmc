#pragma once
/*
 *      Copyright (C) 2010-2012 Team XBMC
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

#if defined(HAVE_OMXLIB)

#include "OMXCore.h"
#include "DVDStreamInfo.h"

#include <IL/OMX_Video.h>

#include "utils/BitstreamConverter.h"

#include "OMXClock.h"

#include "guilib/Geometry.h"
#include "DVDDemuxers/DVDDemux.h"
#include <string>

#define VIDEO_BUFFERS 60

#define CLASSNAME "COMXVideo"

class COMXVideo
{
public:
  COMXVideo();
  ~COMXVideo();

  // Required overrides
  bool SendDecoderConfig();
  bool Open(CDVDStreamInfo &hints, OMXClock *clock, bool deinterlace = false, bool hdmi_clock_sync = false);
  void Close(void);
  unsigned int GetFreeSpace();
  unsigned int GetSize();
  int  Decode(uint8_t *pData, int iSize, double dts, double pts);
  void Reset(void);
  void SetDropState(bool bDrop);
  bool Pause();
  bool Resume();
  std::string GetDecoderName() { return m_video_codec_name; };
  void SetVideoRect(const CRect& SrcRect, const CRect& DestRect);
  int GetInputBufferSize();
  void WaitCompletion();
protected:
  // Video format
  bool              m_drop_state;
  unsigned int      m_decoded_width;
  unsigned int      m_decoded_height;

  OMX_VIDEO_CODINGTYPE m_codingType;

  COMXCoreComponent m_omx_decoder;
  COMXCoreComponent m_omx_render;
  COMXCoreComponent m_omx_sched;
  COMXCoreComponent m_omx_image_fx;
  COMXCoreComponent *m_omx_clock;
  OMXClock           *m_av_clock;

  COMXCoreTunel     m_omx_tunnel_decoder;
  COMXCoreTunel     m_omx_tunnel_clock;
  COMXCoreTunel     m_omx_tunnel_sched;
  COMXCoreTunel     m_omx_tunnel_image_fx;
  bool              m_is_open;

  bool              m_Pause;

  uint8_t           *m_extradata;
  int               m_extrasize;

  CBitstreamConverter   *m_converter;
  bool              m_video_convert;
  std::string       m_video_codec_name;

  bool              m_deinterlace;
  bool              m_hdmi_clock_sync;
  bool              m_first_frame;

  bool NaluFormatStartCodes(enum CodecID codec, uint8_t *in_extradata, int in_extrasize);
};

#endif
