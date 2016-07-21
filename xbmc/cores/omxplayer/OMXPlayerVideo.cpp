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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#elif defined(TARGET_WINDOWS)
#include "system.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "OMXPlayerVideo.h"

#include "linux/XMemUtils.h"
#include "utils/BitstreamStats.h"

#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDCodecs/DVDCodecUtils.h"
#include "windowing/WindowingFactory.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/MediaSettings.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFormats.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "guilib/GraphicContext.h"

#include "linux/RBP.h"

using namespace RenderManager;

class COMXMsgVideoCodecChange : public CDVDMsg
{
public:
  COMXMsgVideoCodecChange(const CDVDStreamInfo &hints, COMXVideo *codec)
    : CDVDMsg(GENERAL_STREAMCHANGE)
    , m_codec(codec)
    , m_hints(hints)
  {}
 ~COMXMsgVideoCodecChange()
  {
    delete m_codec;
  }
  COMXVideo       *m_codec;
  CDVDStreamInfo  m_hints;
};

OMXPlayerVideo::OMXPlayerVideo(OMXClock *av_clock,
                               CDVDOverlayContainer* pOverlayContainer,
                               CDVDMessageQueue& parent, CRenderManager& renderManager, CProcessInfo &processInfo)
: CThread("OMXPlayerVideo")
, IDVDStreamPlayerVideo(processInfo)
, m_messageQueue("video")
, m_omxVideo(renderManager, processInfo)
, m_messageParent(parent)
, m_renderManager(renderManager)
{
  m_av_clock              = av_clock;
  m_pOverlayContainer     = pOverlayContainer;
  m_open                  = false;
  m_stream_id             = -1;
  m_fFrameRate            = 25.0f;
  m_hdmi_clock_sync       = false;
  m_speed                 = DVD_PLAYSPEED_NORMAL;
  m_stalled               = false;
  m_iSubtitleDelay        = 0;
  m_bRenderSubs           = false;
  m_flags                 = 0;
  m_bAllowFullscreen      = false;
  m_iCurrentPts           = DVD_NOPTS_VALUE;
  m_fForcedAspectRatio    = 0.0f;
  bool small_mem = g_RBP.GetArmMem() < 256;
  m_messageQueue.SetMaxDataSize((small_mem ? 10:40) * 1024 * 1024);
  m_messageQueue.SetMaxTimeSize(8.0);

  m_src_rect.SetRect(0, 0, 0, 0);
  m_dst_rect.SetRect(0, 0, 0, 0);
  m_video_stereo_mode = RENDER_STEREO_MODE_OFF;
  m_display_stereo_mode = RENDER_STEREO_MODE_OFF;
  m_StereoInvert = false;
  m_syncState = IDVDStreamPlayer::SYNC_STARTING;
  m_iCurrentPts = DVD_NOPTS_VALUE;
  m_nextOverlay = DVD_NOPTS_VALUE;
  m_flush = false;
}

OMXPlayerVideo::~OMXPlayerVideo()
{
  CloseStream(false);
}

bool OMXPlayerVideo::OpenStream(CDVDStreamInfo &hints)
{
  m_hints       = hints;
  m_hdmi_clock_sync = (CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE) != ADJUST_REFRESHRATE_OFF);
  m_syncState = IDVDStreamPlayer::SYNC_STARTING;
  m_flush       = false;
  m_stalled     = m_messageQueue.GetPacketCount(CDVDMsg::DEMUXER_PACKET) == 0;
  m_nextOverlay = DVD_NOPTS_VALUE;
  // force SetVideoRect to be called initially
  m_src_rect.SetRect(0, 0, 0, 0);
  m_dst_rect.SetRect(0, 0, 0, 0);
  m_video_stereo_mode = RENDER_STEREO_MODE_OFF;
  m_display_stereo_mode = RENDER_STEREO_MODE_OFF;
  m_StereoInvert = false;

  if (!m_DllBcmHost.Load())
    return false;

  if(!OpenDecoder())
  {
    return false;
  }

  if(m_messageQueue.IsInited())
    m_messageQueue.Put(new COMXMsgVideoCodecChange(hints, NULL), 0);
  else
  {
    if(!OpenStream(hints, NULL))
      return false;
    CLog::Log(LOGNOTICE, "Creating video thread");
    m_messageQueue.Init();
    Create();
  }

  m_open        = true;
  m_iCurrentPts = DVD_NOPTS_VALUE;
  m_nextOverlay = DVD_NOPTS_VALUE;

  return true;
}

bool OMXPlayerVideo::OpenStream(CDVDStreamInfo &hints, COMXVideo *codec)
{
  return true;
}

void OMXPlayerVideo::CloseStream(bool bWaitForBuffers)
{
  // wait until buffers are empty
  if (bWaitForBuffers && m_speed > 0) m_messageQueue.WaitUntilEmpty();

  m_messageQueue.Abort();

  if(IsRunning())
  {
    m_bAbortOutput = true;
    StopThread();
  }

  m_messageQueue.End();

  m_open          = false;
  m_stream_id     = -1;
  m_speed         = DVD_PLAYSPEED_NORMAL;

  m_omxVideo.Close();

  if(m_DllBcmHost.IsLoaded())
    m_DllBcmHost.Unload();
}

void OMXPlayerVideo::OnStartup()
{
}

void OMXPlayerVideo::OnExit()
{
  CLog::Log(LOGNOTICE, "thread end: video_thread");
}

double OMXPlayerVideo::NextOverlay(double pts)
{
  double delta_start, delta_stop, min_delta = DVD_NOPTS_VALUE;

  CSingleLock lock(*m_pOverlayContainer);
  VecOverlays* pVecOverlays = m_pOverlayContainer->GetOverlays();
  VecOverlaysIter it = pVecOverlays->begin();

  //Find the minimum time before a subtitle is added or removed
  while (it != pVecOverlays->end())
  {
    CDVDOverlay* pOverlay = *it++;
    if(!pOverlay->bForced && !m_bRenderSubs)
      continue;

    double pts2 = pOverlay->bForced ? pts : pts - m_iSubtitleDelay;

    delta_start = pOverlay->iPTSStartTime - pts2;
    delta_stop = pOverlay->iPTSStopTime - pts2;

    // when currently on screen, we periodically update to allow (limited rate) ASS animation
    if (delta_start <= 0.0 && delta_stop > 0.0 && (min_delta == DVD_NOPTS_VALUE || DVD_MSEC_TO_TIME(100) < min_delta))
      min_delta = DVD_MSEC_TO_TIME(100);

    else if (delta_start > 0.0 && (min_delta == DVD_NOPTS_VALUE || delta_start < min_delta))
      min_delta = delta_start;

    else if (delta_stop > 0.0 && (min_delta == DVD_NOPTS_VALUE || delta_stop < min_delta))
      min_delta = delta_stop;
  }
  return min_delta == DVD_NOPTS_VALUE ? pts+DVD_MSEC_TO_TIME(500) : pts+std::max(min_delta, DVD_MSEC_TO_TIME(100));
}


void OMXPlayerVideo::ProcessOverlays(double pts)
{
  // remove any overlays that are out of time
  if (m_syncState == IDVDStreamPlayer::SYNC_INSYNC)
    m_pOverlayContainer->CleanUp(pts - m_iSubtitleDelay);

  VecOverlays overlays;

  CSingleLock lock(*m_pOverlayContainer);

  VecOverlays* pVecOverlays = m_pOverlayContainer->GetOverlays();
  VecOverlaysIter it = pVecOverlays->begin();

  //Check all overlays and render those that should be rendered, based on time and forced
  //Both forced and subs should check timing
  while (it != pVecOverlays->end())
  {
    CDVDOverlay* pOverlay = *it++;
    if(!pOverlay->bForced && !m_bRenderSubs)
      continue;

    double pts2 = pOverlay->bForced ? pts : pts - m_iSubtitleDelay;

    if((pOverlay->iPTSStartTime <= pts2 && (pOverlay->iPTSStopTime > pts2 || pOverlay->iPTSStopTime == 0LL)))
    {
      if(pOverlay->IsOverlayType(DVDOVERLAY_TYPE_GROUP))
        overlays.insert(overlays.end(), static_cast<CDVDOverlayGroup*>(pOverlay)->m_overlays.begin()
                                      , static_cast<CDVDOverlayGroup*>(pOverlay)->m_overlays.end());
      else
        overlays.push_back(pOverlay);
    }
  }

  for(it = overlays.begin(); it != overlays.end(); ++it)
  {
    double pts2 = (*it)->bForced ? pts : pts - m_iSubtitleDelay;
    m_renderManager.AddOverlay(*it, pts2);
  }
}

std::string OMXPlayerVideo::GetStereoMode()
{
  std::string  stereo_mode;

  switch(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_StereoMode)
  {
    case RENDER_STEREO_MODE_SPLIT_VERTICAL:   stereo_mode = "left_right"; break;
    case RENDER_STEREO_MODE_SPLIT_HORIZONTAL: stereo_mode = "top_bottom"; break;
    default:                                  stereo_mode = m_hints.stereo_mode; break;
  }

  if(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_StereoInvert)
    stereo_mode = GetStereoModeInvert(stereo_mode);
  return stereo_mode;
}

void OMXPlayerVideo::Output(double pts, bool bDropPacket)
{
  if (!m_renderManager.IsConfigured()) {
    CLog::Log(LOGINFO, "%s - renderer not configured", __FUNCTION__);
    return;
  }

  if (CThread::m_bStop)
    return;

  CRect SrcRect, DestRect, viewRect;
  m_renderManager.GetVideoRect(SrcRect, DestRect, viewRect);
  SetVideoRect(SrcRect, DestRect);

  // we aim to submit subtitles 100ms early
  const double preroll = DVD_MSEC_TO_TIME(100);
  double media_pts = m_av_clock->OMXMediaTime();

  if (m_nextOverlay != DVD_NOPTS_VALUE && media_pts != 0.0 && media_pts + preroll <= m_nextOverlay)
    return;

  m_bAbortOutput = false;
  int buffer = m_renderManager.WaitForBuffer(m_bAbortOutput);
  if (buffer < 0)
    return;

  double subtitle_pts = m_nextOverlay;
  double time = subtitle_pts != DVD_NOPTS_VALUE ? subtitle_pts - media_pts : 0.0;

  m_nextOverlay = NextOverlay(media_pts + preroll);

  ProcessOverlays(media_pts + preroll);

  time += m_av_clock->GetAbsoluteClock();
  m_renderManager.FlipPage(m_bAbortOutput, time/DVD_TIME_BASE);
}

void OMXPlayerVideo::Process()
{
  double frametime = (double)DVD_TIME_BASE / m_fFrameRate;
  bool bRequestDrop = false;
  bool settings_changed = false;

  m_videoStats.Start();

  while(!m_bStop)
  {
    int iQueueTimeOut = (int)(m_stalled ? frametime / 4 : frametime * 10) / 1000;
    int iPriority = (m_speed == DVD_PLAYSPEED_PAUSE && m_syncState == IDVDStreamPlayer::SYNC_INSYNC) ? 1 : 0;

    if (m_syncState == IDVDStreamPlayer::SYNC_WAITSYNC)
      iPriority = 1;

    CDVDMsg* pMsg;
    MsgQueueReturnCode ret = m_messageQueue.Get(&pMsg, iQueueTimeOut, iPriority);

    if (MSGQ_IS_ERROR(ret) || ret == MSGQ_ABORT)
    {
      CLog::Log(LOGERROR, "OMXPlayerVideo: Got MSGQ_IS_ERROR(%d) Aborting", (int)ret);
      break;
    }
    else if (ret == MSGQ_TIMEOUT)
    {
      continue;
    }

    if (pMsg->IsType(CDVDMsg::GENERAL_SYNCHRONIZE))
    {
      if(((CDVDMsgGeneralSynchronize*)pMsg)->Wait(100, SYNCSOURCE_VIDEO))
      {
        CLog::Log(LOGDEBUG, "COMXPlayerVideo - CDVDMsg::GENERAL_SYNCHRONIZE");

      }
      else
        m_messageQueue.Put(pMsg->Acquire(), 1); /* push back as prio message, to process other prio messages */
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESYNC))
    {
      double pts = static_cast<CDVDMsgDouble*>(pMsg)->m_value;

      m_nextOverlay = DVD_NOPTS_VALUE;
      m_iCurrentPts = DVD_NOPTS_VALUE;
      m_syncState = IDVDStreamPlayer::SYNC_INSYNC;

      CLog::Log(LOGDEBUG, "CVideoPlayerVideo - CDVDMsg::GENERAL_RESYNC(%f)", pts);
    }
    else if (pMsg->IsType(CDVDMsg::VIDEO_SET_ASPECT))
    {
      m_fForcedAspectRatio = *((CDVDMsgDouble*)pMsg);
      CLog::Log(LOGDEBUG, "COMXPlayerVideo - CDVDMsg::VIDEO_SET_ASPECT %.2f", m_fForcedAspectRatio);
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESET))
    {
      CLog::Log(LOGDEBUG, "COMXPlayerVideo - CDVDMsg::GENERAL_RESET");
      m_syncState = IDVDStreamPlayer::SYNC_STARTING;
      m_nextOverlay = DVD_NOPTS_VALUE;
      m_iCurrentPts = DVD_NOPTS_VALUE;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH)) // private message sent by (COMXPlayerVideo::Flush())
    {
      bool sync = static_cast<CDVDMsgBool*>(pMsg)->m_value;
      CLog::Log(LOGDEBUG, "COMXPlayerVideo - CDVDMsg::GENERAL_FLUSH(%d)", sync);
      m_stalled = true;
      m_syncState = IDVDStreamPlayer::SYNC_STARTING;
      m_nextOverlay = DVD_NOPTS_VALUE;
      m_iCurrentPts = DVD_NOPTS_VALUE;
      m_omxVideo.Reset();
      m_flush = false;
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      if (m_speed != static_cast<CDVDMsgInt*>(pMsg)->m_value)
      {
        m_speed = static_cast<CDVDMsgInt*>(pMsg)->m_value;
        CLog::Log(LOGDEBUG, "COMXPlayerVideo - CDVDMsg::PLAYER_SETSPEED %d", m_speed);
      }
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_STREAMCHANGE))
    {
      COMXMsgVideoCodecChange* msg(static_cast<COMXMsgVideoCodecChange*>(pMsg));
      OpenStream(msg->m_hints, msg->m_codec);
      msg->m_codec = NULL;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_EOF))
    {
      CLog::Log(LOGDEBUG, "COMXPlayerVideo - CDVDMsg::GENERAL_EOF");
      SubmitEOS();
    }
    else if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      DemuxPacket* pPacket = ((CDVDMsgDemuxerPacket*)pMsg)->GetPacket();
      bool bPacketDrop     = ((CDVDMsgDemuxerPacket*)pMsg)->GetPacketDrop();

      #ifdef _DEBUG
      CLog::Log(LOGINFO, "Video: dts:%.0f pts:%.0f size:%d (s:%d f:%d d:%d l:%d) s:%d %d/%d late:%d\n", pPacket->dts, pPacket->pts, 
          (int)pPacket->iSize, m_syncState, m_flush, bPacketDrop, m_stalled, m_speed, 0, 0, 0);
      #endif
      if (m_messageQueue.GetDataSize() == 0
      ||  m_speed < 0)
      {
        bRequestDrop = false;
      }

      // if player want's us to drop this packet, do so nomatter what
      if(bPacketDrop)
        bRequestDrop = true;

      m_omxVideo.SetDropState(bRequestDrop);

      while (!m_bStop)
      {
        // discard if flushing as clocks may be stopped and we'll never submit it
        if (m_flush)
           break;

        if((int)m_omxVideo.GetFreeSpace() < pPacket->iSize)
        {
          Sleep(10);
          continue;
        }
  
        if (m_stalled)
        {
          if(m_syncState == IDVDStreamPlayer::SYNC_INSYNC)
            CLog::Log(LOGINFO, "COMXPlayerVideo - Stillframe left, switching to normal playback");
          m_stalled = false;
        }

        double dts = pPacket->dts;
        double pts = pPacket->pts;
        double iVideoDelay = m_renderManager.GetDelay() * (DVD_TIME_BASE / 1000.0);

        if (dts != DVD_NOPTS_VALUE)
          dts += iVideoDelay;

        if (pts != DVD_NOPTS_VALUE)
          pts += iVideoDelay;

        m_omxVideo.Decode(pPacket->pData, pPacket->iSize, dts, m_hints.ptsinvalid ? DVD_NOPTS_VALUE : pts, settings_changed);

        if (pts == DVD_NOPTS_VALUE)
          pts = dts;

        Output(pts, bRequestDrop);
        if(pts != DVD_NOPTS_VALUE)
          m_iCurrentPts = pts;

        if (m_syncState == IDVDStreamPlayer::SYNC_STARTING && !bRequestDrop && settings_changed)
        {
          m_processInfo.SetVideoDecoderName(m_omxVideo.GetDecoderName(), true);
          m_syncState = IDVDStreamPlayer::SYNC_WAITSYNC;
          SStartMsg msg;
          msg.player = VideoPlayer_VIDEO;
          msg.cachetime = DVD_MSEC_TO_TIME(50); //! @todo implement
          msg.cachetotal = DVD_MSEC_TO_TIME(100); //! @todo implement
          msg.timestamp = pts;
          m_messageParent.Put(new CDVDMsgType<SStartMsg>(CDVDMsg::PLAYER_STARTED, msg));
        }

        break;
      }

      bRequestDrop = false;

      m_videoStats.AddSampleBytes(pPacket->iSize);
    }
    pMsg->Release();

  }
}

bool OMXPlayerVideo::StepFrame()
{
  if (!m_av_clock)
    return false;
  m_av_clock->OMXStep();
  return true;
}

void OMXPlayerVideo::Flush(bool sync)
{
  m_flush = true;
  m_messageQueue.Flush();
  m_messageQueue.Flush(CDVDMsg::GENERAL_EOF);
  m_messageQueue.Put(new CDVDMsgBool(CDVDMsg::GENERAL_FLUSH, sync), 1);
  m_bAbortOutput = true;
}

bool OMXPlayerVideo::OpenDecoder()
{
  if(!m_av_clock)
    return false;

  m_processInfo.ResetVideoCodecInfo();

  if (m_hints.fpsrate && m_hints.fpsscale)
    m_fFrameRate = DVD_TIME_BASE / CDVDCodecUtils::NormalizeFrameduration((double)DVD_TIME_BASE * m_hints.fpsscale / m_hints.fpsrate);
  else
    m_fFrameRate = 25;

  if( m_fFrameRate > 100 || m_fFrameRate < 5 )
  {
    CLog::Log(LOGINFO, "OMXPlayerVideo::OpenDecoder : Invalid framerate %d, using forced 25fps and just trust timestamps\n", (int)m_fFrameRate);
    m_fFrameRate = 25;
  }
  m_processInfo.SetVideoFps(m_fFrameRate);

  // use aspect in stream if available
  if (m_hints.forced_aspect)
    m_fForcedAspectRatio = m_hints.aspect;
  else
    m_fForcedAspectRatio = 0.0;

  bool bVideoDecoderOpen = m_omxVideo.Open(m_hints, m_av_clock, m_hdmi_clock_sync);
  m_omxVideo.RegisterResolutionUpdateCallBack((void *)this, ResolutionUpdateCallBack);

  if(!bVideoDecoderOpen)
  {
    CLog::Log(LOGERROR, "OMXPlayerVideo : Error open video output");
    m_omxVideo.Close();
  }
  else
  {
    CLog::Log(LOGINFO, "OMXPlayerVideo::OpenDecoder : Video codec %s width %d height %d profile %d fps %f\n",
        m_omxVideo.GetDecoderName().c_str() , m_hints.width, m_hints.height, m_hints.profile, m_fFrameRate);

    m_processInfo.SetVideoDecoderName(m_omxVideo.GetDecoderName(), true);
  }

  return bVideoDecoderOpen;
}

int  OMXPlayerVideo::GetDecoderBufferSize()
{
  return m_omxVideo.GetInputBufferSize();
}

int  OMXPlayerVideo::GetDecoderFreeSpace()
{
  return m_omxVideo.GetFreeSpace();
}

void OMXPlayerVideo::SubmitEOS()
{
  m_omxVideo.SubmitEOS();
}

bool OMXPlayerVideo::IsEOS()
{
  return m_omxVideo.IsEOS();
}

void OMXPlayerVideo::SetSpeed(int speed)
{
  if(m_messageQueue.IsInited())
    m_messageQueue.Put( new CDVDMsgInt(CDVDMsg::PLAYER_SETSPEED, speed), 1 );
  else
    m_speed = speed;
}

std::string OMXPlayerVideo::GetPlayerInfo()
{
  double match = 0.0f, phase = 0.0f, pll = 0.0f;
  std::ostringstream s;
  s << "fr:"     << std::fixed << std::setprecision(3) << m_fFrameRate;
  s << ", vq:"   << std::setw(2) << std::min(99,GetLevel()) << "%";
  s << ", dc:"   << m_codecname;
  s << ", Mb/s:" << std::fixed << std::setprecision(2) << (double)GetVideoBitrate() / (1024.0*1024.0);
  if (m_omxVideo.GetPlayerInfo(match, phase, pll))
  {
     s << ", match:" << std::fixed << std::setprecision(2) << match;
     s << ", phase:" << std::fixed << std::setprecision(2) << phase;
     s << ", pll:"   << std::fixed << std::setprecision(5) << pll;
  }
  return s.str();
}

int OMXPlayerVideo::GetVideoBitrate()
{
  return (int)m_videoStats.GetBitrate();
}

double OMXPlayerVideo::GetOutputDelay()
{
  double time = m_messageQueue.GetPacketCount(CDVDMsg::DEMUXER_PACKET);
  if( m_fFrameRate )
    time = (time * DVD_TIME_BASE) / m_fFrameRate;
  else
    time = 0.0;

  if( m_speed != 0 )
    time = time * DVD_PLAYSPEED_NORMAL / abs(m_speed);

  return time;
}

int OMXPlayerVideo::GetFreeSpace()
{
  return m_omxVideo.GetFreeSpace();
}

void OMXPlayerVideo::SetVideoRect(const CRect &InSrcRect, const CRect &InDestRect)
{
  CRect SrcRect = InSrcRect, DestRect = InDestRect;
  unsigned flags = GetStereoModeFlags(GetStereoMode());
  RENDER_STEREO_MODE video_stereo_mode = (flags & CONF_FLAGS_STEREO_MODE_SBS) ? RENDER_STEREO_MODE_SPLIT_VERTICAL :
                                         (flags & CONF_FLAGS_STEREO_MODE_TAB) ? RENDER_STEREO_MODE_SPLIT_HORIZONTAL : RENDER_STEREO_MODE_OFF;
  bool stereo_invert                   = (flags & CONF_FLAGS_STEREO_CADANCE_RIGHT_LEFT) ? true : false;
  RENDER_STEREO_MODE display_stereo_mode = g_graphicsContext.GetStereoMode();

  // ignore video stereo mode when 3D display mode is disabled
  if (display_stereo_mode == RENDER_STEREO_MODE_OFF)
    video_stereo_mode = RENDER_STEREO_MODE_OFF;

  // fix up transposed video
  if (m_hints.orientation == 90 || m_hints.orientation == 270)
  {
    float newWidth, newHeight;
    float aspectRatio = GetAspectRatio();
    // clamp width if too wide
    if (DestRect.Height() > DestRect.Width())
    {
      newWidth = DestRect.Width(); // clamp to the width of the old dest rect
      newHeight = newWidth * aspectRatio;
    }
    else // else clamp to height
    {
      newHeight = DestRect.Height(); // clamp to the height of the old dest rect
      newWidth = newHeight / aspectRatio;
    }

    // calculate the center point of the view and offsets
    float centerX = DestRect.x1 + DestRect.Width() * 0.5f;
    float centerY = DestRect.y1 + DestRect.Height() * 0.5f;
    float diffX = newWidth * 0.5f;
    float diffY = newHeight * 0.5f;

    DestRect.x1 = centerX - diffX;
    DestRect.x2 = centerX + diffX;
    DestRect.y1 = centerY - diffY;
    DestRect.y2 = centerY + diffY;
  }

  // check if destination rect or video view mode has changed
  if (!(m_dst_rect != DestRect) && !(m_src_rect != SrcRect) && m_video_stereo_mode == video_stereo_mode && m_display_stereo_mode == display_stereo_mode && m_StereoInvert == stereo_invert)
    return;

  CLog::Log(LOGDEBUG, "OMXPlayerVideo::%s %d,%d,%d,%d -> %d,%d,%d,%d (%d,%d,%d,%d,%s)", __func__,
      (int)SrcRect.x1, (int)SrcRect.y1, (int)SrcRect.x2, (int)SrcRect.y2,
      (int)DestRect.x1, (int)DestRect.y1, (int)DestRect.x2, (int)DestRect.y2,
      video_stereo_mode, display_stereo_mode, CMediaSettings::GetInstance().GetCurrentVideoSettings().m_StereoInvert, g_graphicsContext.GetStereoView(), OMXPlayerVideo::GetStereoMode().c_str());

  m_src_rect = SrcRect;
  m_dst_rect = DestRect;
  m_video_stereo_mode = video_stereo_mode;
  m_display_stereo_mode = display_stereo_mode;
  m_StereoInvert = stereo_invert;

  // might need to scale up m_dst_rect to display size as video decodes
  // to separate video plane that is at display size.
  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  CRect gui(0, 0, CDisplaySettings::GetInstance().GetResolutionInfo(res).iWidth, CDisplaySettings::GetInstance().GetResolutionInfo(res).iHeight);
  CRect display(0, 0, CDisplaySettings::GetInstance().GetResolutionInfo(res).iScreenWidth, CDisplaySettings::GetInstance().GetResolutionInfo(res).iScreenHeight);

  if (display_stereo_mode == RENDER_STEREO_MODE_SPLIT_VERTICAL)
  {
    float width = DestRect.x2 - DestRect.x1;
    DestRect.x1 *= 2.0f;
    DestRect.x2 = DestRect.x1 + 2.0f * width;
  }
  else if (display_stereo_mode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL)
  {
    float height = DestRect.y2 - DestRect.y1;
    DestRect.y1 *= 2.0f;
    DestRect.y2 = DestRect.y1 + 2.0f * height;
  }

  if (gui != display)
  {
    float xscale = display.Width()  / gui.Width();
    float yscale = display.Height() / gui.Height();
    DestRect.x1 *= xscale;
    DestRect.x2 *= xscale;
    DestRect.y1 *= yscale;
    DestRect.y2 *= yscale;
  }
  m_omxVideo.SetVideoRect(SrcRect, DestRect, m_video_stereo_mode, m_display_stereo_mode, m_StereoInvert);
}

void OMXPlayerVideo::ResolutionUpdateCallBack(uint32_t width, uint32_t height, float framerate, float display_aspect)
{
  RESOLUTION res  = g_graphicsContext.GetVideoResolution();
  uint32_t video_width   = CDisplaySettings::GetInstance().GetResolutionInfo(res).iScreenWidth;
  uint32_t video_height  = CDisplaySettings::GetInstance().GetResolutionInfo(res).iScreenHeight;

  ERenderFormat format = RENDER_FMT_BYPASS;

  /* figure out steremode expected based on user settings and hints */
  unsigned flags = GetStereoModeFlags(GetStereoMode());

  if(m_bAllowFullscreen)
  {
    flags |= CONF_FLAGS_FULLSCREEN;
    m_bAllowFullscreen = false; // only allow on first configure
  }

  m_processInfo.SetVideoDimensions(width, height);
  m_processInfo.SetVideoDAR(display_aspect);

  unsigned int iDisplayWidth  = width;
  unsigned int iDisplayHeight = height;

  /* use forced aspect if any */
  if( m_fForcedAspectRatio != 0.0f )
    iDisplayWidth = (int) (iDisplayHeight * m_fForcedAspectRatio);
  else if( display_aspect != 0.0f )
    iDisplayWidth = (int) (iDisplayHeight * display_aspect);

  m_fFrameRate = DVD_TIME_BASE / CDVDCodecUtils::NormalizeFrameduration((double)DVD_TIME_BASE / framerate);
  m_processInfo.SetVideoFps(m_fFrameRate);

  CLog::Log(LOGDEBUG,"%s - change configuration. video:%dx%d. framerate: %4.2f. %dx%d format: BYPASS",
      __FUNCTION__, video_width, video_height, m_fFrameRate, iDisplayWidth, iDisplayHeight);

  DVDVideoPicture picture;
  memset(&picture, 0, sizeof(DVDVideoPicture));

  picture.iWidth = width;
  picture.iHeight = height;
  picture.iDisplayWidth = iDisplayWidth;
  picture.iDisplayHeight = iDisplayHeight;
  picture.format = format;

  if(!m_renderManager.Configure(picture, m_fFrameRate, flags, m_hints.orientation, 3))
  {
    CLog::Log(LOGERROR, "%s - failed to configure renderer", __FUNCTION__);
    return;
  }

  m_src_rect.SetRect(0, 0, 0, 0);
  m_dst_rect.SetRect(0, 0, 0, 0);
}

void OMXPlayerVideo::ResolutionUpdateCallBack(void *ctx, uint32_t width, uint32_t height, float framerate, float display_aspect)
{
  OMXPlayerVideo *player = static_cast<OMXPlayerVideo*>(ctx);
  player->ResolutionUpdateCallBack(width, height, framerate, display_aspect);
}

