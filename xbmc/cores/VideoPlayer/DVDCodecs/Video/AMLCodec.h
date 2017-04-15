#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "DVDVideoCodec.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/IPlayer.h"
#include "guilib/Geometry.h"
#include "rendering/RenderSystem.h"

#include <deque>
#include <atomic>

typedef struct am_private_t am_private_t;

class DllLibAmCodec;

class PosixFile;
typedef std::shared_ptr<PosixFile> PosixFilePtr;

class CAMLCodec
{
public:
  CAMLCodec();
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
  int64_t       GetCurPts() const { return m_cur_pts + m_start_adj; }
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
  void          SetVideo3dMode(const int mode3d);
  std::string   GetStereoMode();
  bool          OpenAmlVideo(const CDVDStreamInfo &hints);
  void          CloseAmlVideo();
  std::string   GetVfmMap(const std::string &name);
  void          SetVfmMap(const std::string &name, const std::string &map);
  int           DequeueBuffer();
  float         GetTimeSize();

  DllLibAmCodec   *m_dll;
  bool             m_opened;
  bool             m_ptsIs64us;
  bool             m_drain;
  am_private_t    *am_private;
  CDVDStreamInfo   m_hints;
  int              m_speed;
  int64_t          m_cur_pts;
  uint32_t         m_max_frame_size;
  int64_t          m_start_adj;
  int64_t          m_last_pts;
  uint32_t         m_bufferIndex;

  CRect            m_dst_rect;
  CRect            m_display_rect;

  int              m_view_mode;
  RENDER_STEREO_MODE m_stereo_mode;
  RENDER_STEREO_VIEW m_stereo_view;
  float            m_zoom;
  int              m_contrast;
  int              m_brightness;
  RESOLUTION       m_video_res;

  static const unsigned int STATE_PREFILLED  = 1;
  static const unsigned int STATE_HASPTS     = 2;

  unsigned int m_state;

  PosixFilePtr     m_amlVideoFile;
  std::string      m_defaultVfmMap;

  std::deque<uint32_t> m_frameSizes;
  std::uint32_t m_frameSizeSum;

  static std::atomic_flag  m_pollSync;
  static int m_pollDevice;
};
