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
#include "threads/Thread.h"
#include <deque>

typedef struct am_private_t am_private_t;

class DllLibAmCodec;

class PosixFile;
typedef std::shared_ptr<PosixFile> PosixFilePtr;

class CAMLCodec : public CThread
{
public:
  CAMLCodec();
  virtual ~CAMLCodec();

  bool          OpenDecoder(CDVDStreamInfo &hints);
  void          CloseDecoder();
  void          Reset();

  int           Decode(uint8_t *pData, size_t size, double dts, double pts);

  bool          GetPicture(DVDVideoPicture* pDvdVideoPicture);
  void          SetSpeed(int speed);
  int           GetDataSize();
  double        GetTimeSize();
  void          SetVideoRect(const CRect &SrcRect, const CRect &DestRect);
  int64_t       GetCurPts() const { return m_cur_pts; }
  int       	GetOMXPts() const { return static_cast<int>(m_cur_pts - m_start_pts); }

protected:
  virtual void  Process();

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
  int           DequeueBuffer(int64_t &pts);

  DllLibAmCodec   *m_dll;
  bool             m_opened;
  am_private_t    *am_private;
  CDVDStreamInfo   m_hints;
  volatile int     m_speed;
  volatile int64_t m_1st_pts;
  volatile int64_t m_cur_pts;
  volatile double  m_timesize;
  volatile int64_t m_vbufsize;
  int64_t          m_start_dts;
  int64_t          m_start_pts;
  CEvent           m_ready_event;

  CRect            m_dst_rect;
  CRect            m_display_rect;

  int              m_view_mode;
  RENDER_STEREO_MODE m_stereo_mode;
  RENDER_STEREO_VIEW m_stereo_view;
  float            m_zoom;
  int              m_contrast;
  int              m_brightness;

  PosixFilePtr     m_amlVideoFile;
  std::string      m_defaultVfmMap;
  std::deque<int64_t>  m_ptsQueue;
  CCriticalSection m_ptsQueueMutex;
};
