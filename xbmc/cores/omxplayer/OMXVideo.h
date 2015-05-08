#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#if defined(HAVE_OMXLIB)

#include "OMXCore.h"
#include "DVDStreamInfo.h"

#include <IL/OMX_Video.h>

#include "OMXClock.h"

#include "guilib/Geometry.h"
#include "DVDDemuxers/DVDDemux.h"
#include "xbmc/settings/VideoSettings.h"
#include "threads/CriticalSection.h"
#include "xbmc/rendering/RenderSystem.h"
#include <string>

#define VIDEO_BUFFERS 60

#define CLASSNAME "COMXVideo"

typedef void (*ResolutionUpdateCallBackFn)(void *ctx, uint32_t width, uint32_t height, float framerate, float display_aspect);

class COMXVideo
{
public:
  COMXVideo();
  ~COMXVideo();

  // Required overrides
  bool SendDecoderConfig();
  bool Open(CDVDStreamInfo &hints, OMXClock *clock, EDEINTERLACEMODE deinterlace = VS_DEINTERLACEMODE_OFF, bool hdmi_clock_sync = false);
  bool PortSettingsChanged();
  void RegisterResolutionUpdateCallBack(void *ctx, ResolutionUpdateCallBackFn callback) { m_res_ctx = ctx; m_res_callback = callback; }
  void Close(void);
  unsigned int GetFreeSpace();
  unsigned int GetSize();
  int  Decode(uint8_t *pData, int iSize, double dts, double pts);
  void Reset(void);
  void SetDropState(bool bDrop);
  std::string GetDecoderName() { return m_video_codec_name; };
  void SetVideoRect(const CRect& SrcRect, const CRect& DestRect, RENDER_STEREO_MODE video_mode, RENDER_STEREO_MODE display_mode);
  int GetInputBufferSize();
  bool GetPlayerInfo(double &match, double &phase, double &pll);
  void SubmitEOS();
  bool IsEOS();
  bool SubmittedEOS() const { return m_submitted_eos; }
  bool BadState() { return m_omx_decoder.BadState(); };
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
  bool              m_setStartTime;

  uint8_t           *m_extradata;
  int               m_extrasize;

  std::string       m_video_codec_name;

  bool              m_deinterlace;
  EDEINTERLACEMODE  m_deinterlace_request;
  bool              m_hdmi_clock_sync;
  ResolutionUpdateCallBackFn m_res_callback;
  void              *m_res_ctx;
  bool              m_submitted_eos;
  bool              m_failed_eos;
  OMX_DISPLAYTRANSFORMTYPE m_transform;
  bool              m_settings_changed;
  static bool NaluFormatStartCodes(enum AVCodecID codec, uint8_t *in_extradata, int in_extrasize);
  CCriticalSection m_critSection;
};

#endif
