/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "OMXCore.h"
#include "DVDStreamInfo.h"

#include <IL/OMX_Video.h>

#include "OMXClock.h"

#include "utils/Geometry.h"
#include "DVDDemuxers/DVDDemux.h"
#include "xbmc/cores/VideoSettings.h"
#include "threads/CriticalSection.h"
#include "xbmc/rendering/RenderSystem.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include <string>

#define VIDEO_BUFFERS 60

#define CLASSNAME "COMXVideo"

typedef void (*ResolutionUpdateCallBackFn)(void *ctx, uint32_t width, uint32_t height, float framerate, float display_aspect);

struct ResolutionUpdateInfo {
  uint32_t width;
  uint32_t height;
  float framerate;
  float display_aspect;
  bool changed;
};

class COMXVideo
{
public:
  COMXVideo(CRenderManager& renderManager, CProcessInfo &processInfo);
  ~COMXVideo();

  // Required overrides
  bool SendDecoderConfig();
  bool Open(CDVDStreamInfo &hints, OMXClock *clock, bool hdmi_clock_sync = false);
  bool PortSettingsChanged(ResolutionUpdateInfo &resinfo);
  void RegisterResolutionUpdateCallBack(void *ctx, ResolutionUpdateCallBackFn callback) { m_res_ctx = ctx; m_res_callback = callback; }
  void Close(void);
  unsigned int GetFreeSpace();
  unsigned int GetSize();
  int  Decode(uint8_t *pData, int iSize, double dts, double pts, bool &settings_changed);
  void Reset(void);
  void SetDropState(bool bDrop);
  std::string GetDecoderName() { return m_video_codec_name; };
  void SetVideoRect(const CRect& SrcRect, const CRect& DestRect, RENDER_STEREO_MODE video_mode, RENDER_STEREO_MODE display_mode, bool stereo_invert);
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

  COMXCoreTunnel    m_omx_tunnel_decoder;
  COMXCoreTunnel    m_omx_tunnel_clock;
  COMXCoreTunnel    m_omx_tunnel_sched;
  COMXCoreTunnel    m_omx_tunnel_image_fx;
  bool              m_is_open;
  bool              m_setStartTime;

  uint8_t           *m_extradata;
  int               m_extrasize;

  std::string       m_video_codec_name;

  bool              m_deinterlace;
  bool              m_hdmi_clock_sync;
  ResolutionUpdateCallBackFn m_res_callback;
  void              *m_res_ctx;
  bool              m_submitted_eos;
  bool              m_failed_eos;
  OMX_DISPLAYTRANSFORMTYPE m_transform;
  bool              m_settings_changed;
  bool              m_isPi1;
  CRenderManager&   m_renderManager;
  CProcessInfo&     m_processInfo;
  static bool NaluFormatStartCodes(enum AVCodecID codec, uint8_t *in_extradata, int in_extrasize);
  CCriticalSection m_critSection;
};
