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

#include "system.h"
#include "Application.h"
#include "threads/Atomics.h"

#include "RKCodec.h"
#include "DynamicDll.h"
#include "utils/log.h"
#include "utils/StringUtils.h"


#include "cores/dvdplayer/DVDClock.h"
#include "cores/VideoRenderers/RenderFlags.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "guilib/GraphicContext.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"

#include "android/jni/Surface.h"
#include "android/jni/SurfaceTexture.h"
#include "android/activity/AndroidFeatures.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#ifndef RK_KODI_DEBUG
#define RK_KODI_DEBUG 
#endif

int64_t RKAdjustLatency = 0;

class DllLibRKCodecInterface
{
public:
  virtual ~DllLibRKCodecInterface(){}

  virtual int       kodi_init(void *info) = 0;

  virtual int       kodi_open() = 0;

  virtual int       kodi_write(void *data, unsigned int isize, double pts, double dts) = 0;

  virtual bool      kodi_close() = 0;

  virtual int       kodi_flush() = 0;

  virtual int       kodi_reset() = 0;

  virtual int       kodi_pause() = 0;

  virtual int       kodi_resume() = 0;

  virtual int       kodi_set_rect(int x1, int y1, int width, int height) = 0;
  
  virtual int64_t   kodi_get_video_pts() = 0;

  virtual int64_t   kodi_set_video_pts(int64_t pts) = 0;

  virtual int       kodi_set_drop_state(bool b) = 0;

  virtual int       kodi_set_3d_mode(int mode, bool bMVC3d = false) = 0;

  virtual int       kodi_set_23976_match(bool b) = 0;

  virtual int64_t   kodi_get_adjust_latency() = 0;

  virtual int       kodi_get_vpu_level() = 0;
  
};

class DllLibRKCodec : public DllDynamic, DllLibRKCodecInterface
{
  // librkcodec is static linked into librkffplayer.so
  DECLARE_DLL_WRAPPER(DllLibRKCodec, "librkffplayer.so");

  DEFINE_METHOD1(int,     kodi_init, (void* p1));
  DEFINE_METHOD0(int,     kodi_open);
  DEFINE_METHOD4(int,     kodi_write, (void *p1, unsigned int p2, double p3, double p4));
  DEFINE_METHOD0(bool,    kodi_close);
  DEFINE_METHOD0(int,     kodi_flush);
  DEFINE_METHOD0(int,     kodi_reset);
  DEFINE_METHOD0(int,     kodi_pause);
  DEFINE_METHOD0(int,     kodi_resume);
  DEFINE_METHOD4(int,     kodi_set_rect, (int p1, int p2, int p3, int p4));
  DEFINE_METHOD0(INT64,   kodi_get_video_pts);
  DEFINE_METHOD1(INT64,   kodi_set_video_pts, (INT64 p1));
  DEFINE_METHOD0(int,     kodi_get_vpu_level);
  DEFINE_METHOD1(int,     kodi_set_drop_state, (bool p1));
  DEFINE_METHOD2(int,     kodi_set_3d_mode, (int p1, bool p2));
  DEFINE_METHOD1(int,     kodi_set_23976_match, (bool p1));
  DEFINE_METHOD0(INT64,   kodi_get_adjust_latency);

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(kodi_init)
    RESOLVE_METHOD(kodi_open)
    RESOLVE_METHOD(kodi_write)
    RESOLVE_METHOD(kodi_reset)
    RESOLVE_METHOD(kodi_close)
    RESOLVE_METHOD(kodi_flush)
    RESOLVE_METHOD(kodi_pause)
    RESOLVE_METHOD(kodi_resume)
    RESOLVE_METHOD(kodi_set_rect)
    RESOLVE_METHOD(kodi_get_video_pts)
    RESOLVE_METHOD(kodi_set_video_pts)
    RESOLVE_METHOD(kodi_get_vpu_level)
    RESOLVE_METHOD(kodi_set_drop_state)
    RESOLVE_METHOD(kodi_set_3d_mode)
    RESOLVE_METHOD(kodi_set_23976_match)
    RESOLVE_METHOD(kodi_get_adjust_latency)
  END_METHOD_RESOLVE()
  
};


CRKCodec::CRKCodec() : CThread("CRKCodec")
{

  #ifdef RK_KODI_DEBUG
  CLog::Log(LOGDEBUG, "CRKCodec::CRKCodec()!");
  #endif
  m_opened      = false;
  m_speed       = DVD_PLAYSPEED_NORMAL;
  m_1st_pts     = 0;
  m_cur_pts     = 0;
  m_last_pts    = 0;
  m_iAdjuestLatency = 0;
  m_1st_adjust  = 0;
  m_cur_adjust  = 0;
  m_last_adjust = 0;
  RKAdjustLatency = 0;
  m_bDrop       = false;
  m_bMVC3D      = false;
  m_i3DMode     = RK_3DMODE_NONE;
  m_user_stereo_mode = RENDER_STEREO_MODE_OFF;
  m_kodi_stereo_mode = RENDER_STEREO_MODE_OFF;
  m_display_rect.x1 = m_display_rect.x2 = m_display_rect.y1 = m_display_rect.y2 = 0;
  m_dll         = new DllLibRKCodec;
    
  if (!m_dll->Load())
  {
    CLog::Log(LOGDEBUG, "CRKCodec::m_dll load fail!");
  }

}

CRKCodec::~CRKCodec()
{
  StopThread();
  delete m_dll, m_dll = NULL;
}

bool CRKCodec::OpenDecoder(CDVDStreamInfo &hints)
{
  CLog::Log(LOGDEBUG,"RKCodec Open filename:%s",hints.filename.c_str());
  m_hints          = hints;
  m_info.codec_id  = hints.codec;
  m_info.height    = hints.height;
  m_info.width     = hints.width;
  m_info.extradata = hints.extradata;
  m_info.extrasize = hints.extrasize;
  m_info.mvc3d     = false;
  int err = m_dll->kodi_init(&m_info);
  if (m_bMVC3D)
  {
    CLog::Log(LOGDEBUG,"CRKCodec::MVC Detected!");
    m_user_stereo_mode = RENDER_STEREO_MODE_SPLIT_VERTICAL;
    SetVideo3dMode(RK_3DMODE_SIDE_BY_SIDE);

    // in 4.4
    //g_graphicsContext.SetStereoMode(m_user_stereo_mode);
  }
  err = m_dll->kodi_open();
  if (!err)
  {
    CheckVideoRate(hints);
    Create();
    m_opened = true;
    m_display_rect = CRect(0, 0, CDisplaySettings::GetInstance().GetCurrentResolutionInfo().iWidth, CDisplaySettings::GetInstance().GetCurrentResolutionInfo().iHeight);
    CLog::Log(LOGDEBUG,"CRKCodec::kodi_opencodec() width = %d , height = %d, sWidth = %d, sHeight = %d, bFullStreen = %d", 
    CDisplaySettings::GetInstance().GetCurrentResolutionInfo().iWidth,
    CDisplaySettings::GetInstance().GetCurrentResolutionInfo().iHeight,
    CDisplaySettings::GetInstance().GetCurrentResolutionInfo().iScreenWidth,
    CDisplaySettings::GetInstance().GetCurrentResolutionInfo().iScreenHeight,
    CDisplaySettings::GetInstance().GetCurrentResolutionInfo().bFullScreen);
    CLog::Log(LOGDEBUG,"CRKCodec::kodi_opencodec() OpenDecoder Success!");
    g_renderManager.RegisterRenderUpdateCallBack((const void*)this, RenderUpdateCallBack);
    return true;
  }
  return false;
}


void CRKCodec::CheckVideoRate(CDVDStreamInfo &hints)
{
  // handle video rate
  if (hints.rfpsrate > 0 && hints.rfpsscale != 0)
  {
    // check ffmpeg r_frame_rate 1st
    video_rate = 0.5 + (float)UNIT_FREQ * hints.rfpsscale / hints.rfpsrate;
  }
  else if (hints.fpsrate > 0 && hints.fpsscale != 0)
  {
    // then ffmpeg avg_frame_rate next
    video_rate = 0.5 + (float)UNIT_FREQ * hints.fpsscale / hints.fpsrate;
  }
  else
  {
    // default 24fps
    video_rate = 0.5 + (float)UNIT_FREQ * 1001 / 25000;
  }

  CLog::Log(LOGDEBUG, " CRKCodec::video rate %f",video_rate);
}

void CRKCodec::CloseDecoder(void)
{
  CLog::Log(LOGDEBUG, "CRKCodec::CloseDecoder()!");
  StopThread();
  m_dll->kodi_close();
  g_graphicsContext.SetStereoMode(RENDER_STEREO_MODE_OFF);
}

void CRKCodec::Reset()
{
  CLog::Log(LOGDEBUG, "CRKCodec::Reset()!");
  if(!m_opened)
    return;
  m_dll->kodi_reset();
  m_1st_pts = 0;
  m_cur_pts = 0;
  m_last_pts = 0;
  m_1st_adjust = 0;
  m_cur_adjust = 0;
}

void CRKCodec::Flush()
{
  if(!m_opened)
    return;
  m_dll->kodi_flush();
  m_1st_pts = 0;
  m_cur_pts = 0;
  m_last_pts = 0;
}

int CRKCodec::Decode(uint8_t *pData, size_t iSize, double dts, double pts)
{

  int rtn = VC_BUFFER;
  if(!m_opened)
    return VC_BUFFER;
  
  g_renderManager.RegisterRenderUpdateCallBack((const void*)this, RenderUpdateCallBack);
  
  if (m_1st_pts <= 100)
  {
    m_1st_pts = (int64_t)pts;
  }
  int res = m_dll->kodi_write(pData, iSize, pts, dts);
  switch(res)
  {
    case RK_DECODE_STATE_BUFFER:
      rtn = VC_BUFFER;
      break;
    case RK_DECODE_STATE_PICTURE:
      if (m_speed != DVD_PLAYSPEED_PAUSE)
        rtn = VC_PICTURE;
      else
        rtn |= VC_PICTURE;
      break;
    case RK_DECODE_STATE_BUFFER_PICTURE:
      rtn |= VC_PICTURE;
      break;
    case RK_DECODE_STATE_ERROR:
      rtn = VC_ERROR;
      break;
    default:
      rtn = VC_ERROR;
      break;
  }
  if (((int64_t)pts) >100)
    m_last_pts = pts;
  return rtn;
}


bool CRKCodec::GetPicture(DVDVideoPicture *pDvdVideoPicture)
{
  if(!m_opened)
    return false;
  pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->format = RENDER_FMT_BYPASS;
  pDvdVideoPicture->iDuration = (double)(video_rate * DVD_TIME_BASE) / UNIT_FREQ;
  pDvdVideoPicture->dts = DVD_NOPTS_VALUE;
  pDvdVideoPicture->pts = DVD_NOPTS_VALUE;
  if (m_speed == DVD_PLAYSPEED_NORMAL)
  {
    pDvdVideoPicture->pts = GetPlayerPtsSeconds() * (double)DVD_TIME_BASE;
    pDvdVideoPicture->pts += 1 * pDvdVideoPicture->iDuration;
  }
  else
  {
    if (m_cur_pts <= 100)
      pDvdVideoPicture->pts = (double)m_1st_pts / PTS_FREQ * DVD_TIME_BASE;
    else
      pDvdVideoPicture->pts = (double)m_cur_pts / PTS_FREQ * DVD_TIME_BASE;
  }
  
  return true;
}


double CRKCodec::GetPlayerPtsSeconds()
{
  double clock_pts = 0.0;
  CDVDClock *playerclock = CDVDClock::GetMasterClock();
  if (playerclock)
    clock_pts = playerclock->GetClock() / DVD_TIME_BASE;

  return clock_pts;
}

int64_t  CRKCodec::GetVideoPts()
{
  return m_dll->kodi_get_video_pts();
}

int64_t  CRKCodec::SetVideoPts(int64_t  pts)
{
  return m_dll->kodi_set_video_pts(pts);
}

void CRKCodec::SetSpeed(int speed)
{
  CLog::Log(LOGDEBUG,"RKTEST RKCODEC SetSpeed %d",speed);
  m_speed = speed;
  if (!m_opened)
    return;
  
  switch(speed)
  {
    case DVD_PLAYSPEED_PAUSE:
      m_dll->kodi_pause();
      break;
    case DVD_PLAYSPEED_NORMAL:
      m_dll->kodi_resume();
      break;
    default:
      m_dll->kodi_resume();
      break;
  }
  
  return;
}

void CRKCodec::SetDropState(bool b)
{
  if (m_bDrop != b)
  {
    //int ret = m_dll->kodi_set_drop_state(b);
    m_bDrop = b;
  }
  return;
}

int CRKCodec::GetDataSize()
{
  if(!m_opened)
    return 0;
  return 0;
}

double CRKCodec::GetTimeSize()
{
  if(!m_opened)
    return 0;
  
  if (m_cur_pts <= 100)
    m_timesize = (double)(m_last_pts - m_1st_pts) / DVD_TIME_BASE;
  else
    m_timesize = (double)(m_last_pts - m_cur_pts) / DVD_TIME_BASE;

  double timesize = m_timesize;
  if (timesize < 0.0)
    timesize = 0.0;
  else if (timesize > 7.0)
    timesize = 7.0;

  return timesize;
}

void CRKCodec::Process()
{
  #ifdef RK_KODI_DEBUG
  CLog::Log(LOGDEBUG, "CRKCodec::Process()!");
  #endif
  
  SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);
  while(!m_bStop)
  {
    int64_t video_pts = 0;
    if (m_1st_pts > 0 || m_1st_pts == DVD_NOPTS_VALUE)
    {
      video_pts = GetVideoPts();
      if (m_cur_pts != video_pts)
      {
        double app_pts = GetPlayerPtsSeconds();
        m_cur_pts = video_pts;
        
        m_iAdjuestLatency = m_dll->kodi_get_adjust_latency();
        RKAdjustLatency = m_iAdjuestLatency;
        m_cur_adjust = m_iAdjuestLatency;
        if ((m_cur_adjust - m_last_adjust) > DVD_MSEC_TO_TIME(10)|| (m_cur_adjust - m_last_adjust) < -DVD_MSEC_TO_TIME(10))
          m_1st_adjust = m_cur_adjust;
        m_last_adjust = m_cur_adjust;
        
        double offset  = g_renderManager.GetDisplayLatency() - CMediaSettings::GetInstance().GetCurrentVideoSettings().m_AudioDelay;
        app_pts += offset;
        double error = app_pts - (double)(video_pts - (m_cur_adjust - m_1st_adjust))/ DVD_TIME_BASE;
        double abs_error = fabs(error);
        if (abs_error > 0.125)
        {
          if (abs_error > 0.15)
          {
            SetVideoPts((int64_t)(app_pts * DVD_TIME_BASE));
          }
          else
          {
            SetVideoPts((int64_t)(video_pts + error / 2 *DVD_TIME_BASE));
          }
        }
      }
    }
SLEEP:
    Sleep(100);
  }
  SetPriority(THREAD_PRIORITY_NORMAL);
  #ifdef RK_KODI_DEBUG
  CLog::Log(LOGDEBUG, "CRKCodec::Process Stopped!");
  #endif  
}

void CRKCodec::SetVideo3dMode(const int mode)
{
  if (m_i3DMode != mode /*&& m_bMVC3D*/)
  {
    m_i3DMode = mode;
    m_dll->kodi_set_3d_mode(m_i3DMode, m_bMVC3D);
  }
}

void CRKCodec::Set23976Match(bool b)
{
  CLog::Log(LOGDEBUG,"Set23976Match %d",b);
  m_dll->kodi_set_23976_match(b);
}

void CRKCodec::RenderUpdateCallBack(const void *ctx, const CRect &SrcRect, const CRect &DestRect)
{
  ((CRKCodec*)ctx)->UpdateRenderRect(SrcRect,DestRect);
}


void CRKCodec::UpdateRenderRect(const CRect &SrcRect, const CRect &DestRect)
{
  
  RENDER_STEREO_MODE stereo_mode = g_graphicsContext.GetStereoMode();
  if (stereo_mode != RENDER_STEREO_MODE_OFF)
  {
    g_graphicsContext.SetStereoMode(RENDER_STEREO_MODE_OFF);
    if (m_user_stereo_mode != stereo_mode)
    {
      switch(stereo_mode)
      {
        case RENDER_STEREO_MODE_SPLIT_HORIZONTAL:
          SetVideo3dMode(RK_3DMODE_TOP_BOTTOM);
          break;
        case RENDER_STEREO_MODE_SPLIT_VERTICAL:
          SetVideo3dMode(RK_3DMODE_SIDE_BY_SIDE);
          break;
        case RENDER_STEREO_MODE_MONO:
          SetVideo3dMode(RK_3DMODE_NONE);
          break;
        default:
          //SetVideo3dMode(RK_3DMODE_NONE);
          break;
      }
      CLog::Log(LOGDEBUG,"UpdateRenderRect %d ==> %d",m_user_stereo_mode,stereo_mode);
      m_user_stereo_mode = stereo_mode;
    }
  }
  
  if (m_display_rect != DestRect)
  {  
    CLog::Log(LOGDEBUG,"CRKCodec::RenderUpdateCallBack");
    m_display_rect = DestRect;
    if (stereo_mode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL)
    {
      m_display_rect.y2 *= 2.0;
    }
    else if (stereo_mode == RENDER_STEREO_MODE_SPLIT_VERTICAL)
    {
      m_display_rect.x2 *= 2.0;
    }
    m_dll->kodi_set_rect(m_display_rect.x1, m_display_rect.y1, (int)m_display_rect.Width(), (int)m_display_rect.Height());
    m_display_rect = DestRect;
  }

  // old in 4.4
  /*
    RENDER_STEREO_MODE stereo_mode = g_graphicsContext.GetStereoMode();
	if (m_user_stereo_mode != stereo_mode)
	{
		m_user_stereo_mode = stereo_mode;
		switch(m_user_stereo_mode)
		{
		case RENDER_STEREO_MODE_SPLIT_HORIZONTAL:
			SetVideo3dMode(RK_3DMODE_TOP_BOTTOM);
			break;
		case RENDER_STEREO_MODE_SPLIT_VERTICAL:
			SetVideo3dMode(RK_3DMODE_SIDE_BY_SIDE);
			break;
		default:
			SetVideo3dMode(RK_3DMODE_NONE);
		}
	}
	
	if (m_display_rect != DestRect)
	{	
		CLog::Log(LOGDEBUG,"CRKCodec::RenderUpdateCallBack");
		m_display_rect = DestRect;
		if (m_user_stereo_mode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL)
		{
			m_display_rect.y2 *= 2.0;
		}
		else if (m_user_stereo_mode == RENDER_STEREO_MODE_SPLIT_VERTICAL)
		{
			m_display_rect.x2 *= 2.0;
		}
		m_dll->kodi_set_rect(m_display_rect.x1, m_display_rect.y1, (int)m_display_rect.Width(), (int)m_display_rect.Height());
		m_display_rect = DestRect;
	}*/
}


