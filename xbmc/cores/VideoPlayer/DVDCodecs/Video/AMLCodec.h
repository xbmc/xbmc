/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDVideoCodec.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/IPlayer.h"
#include "windowing/Resolution.h"
#include "rendering/RenderSystem.h"
#include "utils/Geometry.h"

#include <deque>
#include <atomic>

typedef struct am_private_t am_private_t;

class DllLibAmCodec;

class PosixFile;
typedef std::shared_ptr<PosixFile> PosixFilePtr;

class CProcessInfo;

class CAMLCodec
{
public:
  CAMLCodec(CProcessInfo &processInfo);
  virtual ~CAMLCodec();

  bool          OpenDecoder(CDVDStreamInfo &hints);
  void          CloseDecoder();
  void          Reset();

  bool          AddData(uint8_t *pData, size_t size, double dts, double pts);
  CDVDVideoCodec::VCReturn GetPicture(VideoPicture* pVideoPicture);

  void          SetSpeed(int speed);
  void          SetDrain(bool drain){m_drain = drain;};
  void          SetVideoRect(const CRect &SrcRect, const CRect &DestRect);
  void          SetVideoRate(int videoRate);
  int           GetOMXPts() const { return static_cast<int>(m_cur_pts); }
  uint32_t      GetBufferIndex() const { return m_bufferIndex; };
  static float  OMXPtsToSeconds(int omxpts);
  static int    OMXDurationToNs(int duration);
  int           GetAmlDuration() const;
  int           ReleaseFrame(const uint32_t index, bool bDrop = false);

  static int    PollFrame();
  static void   SetPollDevice(int device);

private:
  void          ShowMainVideo(const bool show);
  void          SetVideoZoom(const float zoom);
  void          SetVideoContrast(const int contrast);
  void          SetVideoBrightness(const int brightness);
  void          SetVideoSaturation(const int saturation);
  bool          OpenAmlVideo(const CDVDStreamInfo &hints);
  void          CloseAmlVideo();
  std::string   GetVfmMap(const std::string &name);
  void          SetVfmMap(const std::string &name, const std::string &map);
  int           DequeueBuffer();
  float         GetTimeSize();
  unsigned int  GetDecoderVideoRate();

  DllLibAmCodec   *m_dll;
  bool             m_opened;
  bool             m_ptsIs64us;
  bool             m_drain = false;
  am_private_t    *am_private;
  CDVDStreamInfo   m_hints;
  int              m_speed;
  int64_t          m_cur_pts;
  int64_t          m_last_pts;
  int64_t          m_ptsOverflow;
  uint32_t         m_bufferIndex;

  CRect            m_dst_rect;
  CRect            m_display_rect;

  int              m_view_mode = -1;
  RENDER_STEREO_MODE m_guiStereoMode = RENDER_STEREO_MODE_OFF;
  RENDER_STEREO_VIEW m_guiStereoView = RENDER_STEREO_VIEW_OFF;
  float            m_zoom = -1.0f;
  int              m_contrast = -1;
  int              m_brightness = -1;
  RESOLUTION       m_video_res = RES_INVALID;

  static const unsigned int STATE_PREFILLED  = 1;
  static const unsigned int STATE_HASPTS     = 2;

  unsigned int m_state;

  PosixFilePtr     m_amlVideoFile;
  std::string      m_defaultVfmMap;

  std::deque<uint32_t> m_frameSizes;
  std::uint32_t m_frameSizeSum;

  static std::atomic_flag  m_pollSync;
  static int m_pollDevice;
  CProcessInfo &m_processInfo;
};
