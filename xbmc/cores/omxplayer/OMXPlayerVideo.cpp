/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#elif defined(_WIN32)
#include "system.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "OMXPlayerVideo.h"

#include "linux/XMemUtils.h"
#include "utils/BitstreamStats.h"

#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDCodecs/DVDCodecUtils.h"
#include "windowing/WindowingFactory.h"
#include "DVDOverlayRenderer.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "cores/VideoRenderers/RenderFormats.h"
#include "cores/VideoRenderers/RenderFlags.h"

#include "OMXPlayer.h"

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
                               CDVDMessageQueue& parent)
: CThread("COMXPlayerVideo")
, m_messageQueue("video")
, m_messageParent(parent)
{
  m_av_clock              = av_clock;
  m_pOverlayContainer     = pOverlayContainer;
  m_pTempOverlayPicture   = NULL;
  m_open                  = false;
  m_stream_id             = -1;
  m_fFrameRate            = 25.0f;
  m_hdmi_clock_sync       = false;
  m_speed                 = DVD_PLAYSPEED_NORMAL;
  m_stalled               = false;
  m_codecname             = "";
  m_iSubtitleDelay        = 0;
  m_FlipTimeStamp         = 0.0;
  m_bRenderSubs           = false;
  m_flags                 = 0;
  m_bAllowFullscreen      = false;
  m_iCurrentPts           = DVD_NOPTS_VALUE;
  m_iVideoDelay           = 0;
  m_droptime              = 0.0;
  m_dropbase              = 0.0;
  m_autosync              = 1;
  m_fForcedAspectRatio    = 0.0f;
  m_messageQueue.SetMaxDataSize(10 * 1024 * 1024);
  m_messageQueue.SetMaxTimeSize(8.0);

  m_dst_rect.SetRect(0, 0, 0, 0);
}

OMXPlayerVideo::~OMXPlayerVideo()
{
  CloseStream(false);
}

bool OMXPlayerVideo::OpenStream(CDVDStreamInfo &hints)
{
  /*
  if(IsRunning())
    CloseStream(false);
  */

  m_hints       = hints;
  m_Deinterlace = ( g_settings.m_currentVideoSettings.m_DeinterlaceMode == VS_DEINTERLACEMODE_OFF ) ? false : true;
  m_hdmi_clock_sync = (g_guiSettings.GetInt("videoplayer.adjustrefreshrate") != ADJUST_REFRESHRATE_OFF);
  m_started     = false;
  m_flush       = false;
  m_stalled     = m_messageQueue.GetPacketCount(CDVDMsg::DEMUXER_PACKET) == 0;
  m_autosync    = 1;
  m_iSleepEndTime = DVD_NOPTS_VALUE;
  // force SetVideoRect to be called initially
  m_dst_rect.SetRect(0, 0, 0, 0);

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

  /*
  if(!OpenStream(hints, NULL))
    return false;

  CLog::Log(LOGNOTICE, "Creating video thread");
  m_messageQueue.Init();
  Create();
  */

  m_open        = true;

  return true;
}

bool OMXPlayerVideo::OpenStream(CDVDStreamInfo &hints, COMXVideo *codec)
{
  return true;
}

bool OMXPlayerVideo::CloseStream(bool bWaitForBuffers)
{
  // wait until buffers are empty
  if (bWaitForBuffers && m_speed > 0) m_messageQueue.WaitUntilEmpty();

  m_messageQueue.Abort();

  if(IsRunning())
    StopThread();

  m_messageQueue.End();

  m_open          = false;
  m_stream_id     = -1;
  m_speed         = DVD_PLAYSPEED_NORMAL;
  m_started       = false;

  if (m_pTempOverlayPicture)
  {
    CDVDCodecUtils::FreePicture(m_pTempOverlayPicture);
    m_pTempOverlayPicture = NULL;
  }

  m_av_clock->Lock();
  m_av_clock->OMXStop(false);
  m_omxVideo.Close();
  m_av_clock->HasVideo(false);
  m_av_clock->OMXReset(false);
  m_av_clock->UnLock();

  if(m_DllBcmHost.IsLoaded())
    m_DllBcmHost.Unload();

  return true;
}

void OMXPlayerVideo::OnStartup()
{
  m_iCurrentPts = DVD_NOPTS_VALUE;
  m_FlipTimeStamp = m_av_clock->GetAbsoluteClock();
}

void OMXPlayerVideo::OnExit()
{
  CLog::Log(LOGNOTICE, "thread end: video_thread");
}

void OMXPlayerVideo::ProcessOverlays(int iGroupId, double pts)
{
  // remove any overlays that are out of time
  if (m_started)
    m_pOverlayContainer->CleanUp(pts - m_iSubtitleDelay);

  VecOverlays overlays;

  CSingleLock lock(*m_pOverlayContainer);

  VecOverlays* pVecOverlays = m_pOverlayContainer->GetOverlays();
  VecOverlaysIter it = pVecOverlays->begin();

  //Check all overlays and render those that should be rendered, based on time and forced
  //Both forced and subs should check timeing, pts == 0 in the stillframe case
  while (it != pVecOverlays->end())
  {
    CDVDOverlay* pOverlay = *it++;
    if(!pOverlay->bForced && !m_bRenderSubs)
      continue;

    if(pOverlay->iGroupId != iGroupId)
      continue;

    double pts2 = pOverlay->bForced ? pts : pts - m_iSubtitleDelay;

    if((pOverlay->iPTSStartTime <= pts2 && (pOverlay->iPTSStopTime > pts2 || pOverlay->iPTSStopTime == 0LL)) || pts == 0)
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
    g_renderManager.AddOverlay(*it, pts2);
  }
}

void OMXPlayerVideo::Output(int iGroupId, double pts, bool bDropPacket)
{
  if (!g_renderManager.IsStarted()) {
    CLog::Log(LOGERROR, "%s - renderer not started", __FUNCTION__);
    return;
  }

  // calculate the time we need to delay this picture before displaying
  double iSleepTime, iClockSleep, iFrameSleep, iPlayingClock, iCurrentClock, iFrameDuration;

  iPlayingClock = m_av_clock->GetClock(iCurrentClock, false); // snapshot current clock
  iClockSleep = pts - iPlayingClock; //sleep calculated by pts to clock comparison
  iFrameSleep = m_FlipTimeStamp - iCurrentClock; // sleep calculated by duration of frame
  iFrameDuration = (double)DVD_TIME_BASE / m_fFrameRate; //pPacket->duration;

  // correct sleep times based on speed
  if(m_speed)
  {
    iClockSleep = iClockSleep * DVD_PLAYSPEED_NORMAL / m_speed;
    iFrameSleep = iFrameSleep * DVD_PLAYSPEED_NORMAL / abs(m_speed);
    iFrameDuration = iFrameDuration * DVD_PLAYSPEED_NORMAL / abs(m_speed);
  }
  else
  {
    iClockSleep = 0;
    iFrameSleep = 0;
  }

  // dropping to a very low framerate is not correct (it should not happen at all)
  iClockSleep = min(iClockSleep, DVD_MSEC_TO_TIME(500));
  iFrameSleep = min(iFrameSleep, DVD_MSEC_TO_TIME(500));

  if( m_stalled )
    iSleepTime = iFrameSleep;
  else
    iSleepTime = iFrameSleep + (iClockSleep - iFrameSleep) / m_autosync;

  // present the current pts of this frame to user, and include the actual
  // presentation delay, to allow him to adjust for it
  if( m_stalled )
    m_iCurrentPts = DVD_NOPTS_VALUE;
  else
    m_iCurrentPts = pts - max(0.0, iSleepTime);

  // timestamp when we think next picture should be displayed based on current duration
  m_FlipTimeStamp  = iCurrentClock;
  m_FlipTimeStamp += max(0.0, iSleepTime);
  m_FlipTimeStamp += iFrameDuration;

  if( m_speed < 0 )
  {
    if( iClockSleep < -DVD_MSEC_TO_TIME(200))
      return;
  }

  if(bDropPacket)
    return;

#if 0
  if( m_speed != DVD_PLAYSPEED_NORMAL)
  {
    // calculate frame dropping pattern to render at this speed
    // we do that by deciding if this or next frame is closest
    // to the flip timestamp
    double current   = fabs(m_dropbase -  m_droptime);
    double next      = fabs(m_dropbase - (m_droptime + iFrameDuration));
    double frametime = (double)DVD_TIME_BASE / m_fFrameRate;

    m_droptime += iFrameDuration;
#ifndef PROFILE
    if( next < current /*&& !(pPicture->iFlags & DVP_FLAG_NOSKIP) */)
      return /*result | EOS_DROPPED*/;
#endif

    while(!m_bStop && m_dropbase < m_droptime)             m_dropbase += frametime;
    while(!m_bStop && m_dropbase - frametime > m_droptime) m_dropbase -= frametime;
  }
  else
  {
    m_droptime = 0.0f;
    m_dropbase = 0.0f;
  }
#else
  m_droptime = 0.0f;
  m_dropbase = 0.0f;
#endif

  // DVDPlayer sleeps until m_iSleepEndTime here before calling FlipPage.
  // Video playback in asynchronous in OMXPlayer, so we don't want to do that here, as it prevents the video fifo from being kept full.
  // So, we keep track of when FlipPage would have been called on DVDPlayer and return early if it is not time.
  // m_iSleepEndTime == DVD_NOPTS_VALUE means we are not waiting to call FlipPage, otherwise it is the time we want to call FlipPage
  if (m_iSleepEndTime == DVD_NOPTS_VALUE) {
    m_iSleepEndTime = iCurrentClock + iSleepTime;
  }

  if (!CThread::m_bStop && m_av_clock->GetAbsoluteClock(false) < m_iSleepEndTime + DVD_MSEC_TO_TIME(500))
    return;

  double pts_media = m_av_clock->OMXMediaTime(false);
  ProcessOverlays(iGroupId, pts_media);

  g_renderManager.FlipPage(CThread::m_bStop, m_iSleepEndTime / DVD_TIME_BASE, -1, FS_NONE);

  m_iSleepEndTime = DVD_NOPTS_VALUE;

  //m_av_clock->WaitAbsoluteClock((iCurrentClock + iSleepTime));
}

void OMXPlayerVideo::Process()
{
  double pts = 0;
  double frametime = (double)DVD_TIME_BASE / m_fFrameRate;
  bool bRequestDrop = false;

  m_videoStats.Start();

  while(!m_bStop)
  {
    CDVDMsg* pMsg;
    int iQueueTimeOut = (int)(m_stalled ? frametime / 4 : frametime * 10) / 1000;
    int iPriority = (m_speed == DVD_PLAYSPEED_PAUSE && m_started) ? 1 : 0;
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

      pMsg->Release();

      continue;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESYNC))
    {
      CDVDMsgGeneralResync* pMsgGeneralResync = (CDVDMsgGeneralResync*)pMsg;

      if(pMsgGeneralResync->m_timestamp != DVD_NOPTS_VALUE)
        pts = pMsgGeneralResync->m_timestamp;

      double delay = m_FlipTimeStamp - m_av_clock->GetAbsoluteClock();
      if( delay > frametime ) delay = frametime;
      else if( delay < 0 )    delay = 0;

      if(pMsgGeneralResync->m_clock)
      {
        CLog::Log(LOGDEBUG, "COMXPlayerVideo - CDVDMsg::GENERAL_RESYNC(%f, 1)", pts);
        m_av_clock->Discontinuity(pts - delay);
        //m_av_clock->OMXUpdateClock(pts - delay);
      }
      else
        CLog::Log(LOGDEBUG, "COMXPlayerVideo - CDVDMsg::GENERAL_RESYNC(%f, 0)", pts);

      pMsgGeneralResync->Release();
      continue;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_DELAY))
    {
      if (m_speed != DVD_PLAYSPEED_PAUSE)
      {
        double timeout = static_cast<CDVDMsgDouble*>(pMsg)->m_value;

        CLog::Log(LOGDEBUG, "COMXPlayerVideo - CDVDMsg::GENERAL_DELAY(%f)", timeout);

        timeout *= (double)DVD_PLAYSPEED_NORMAL / abs(m_speed);
        timeout += m_av_clock->GetAbsoluteClock();

        while(!m_bStop && m_av_clock->GetAbsoluteClock() < timeout)
          Sleep(1);
      }
    }
    else if (pMsg->IsType(CDVDMsg::VIDEO_SET_ASPECT))
    {
      m_fForcedAspectRatio = *((CDVDMsgDouble*)pMsg);
      CLog::Log(LOGDEBUG, "COMXPlayerVideo - CDVDMsg::VIDEO_SET_ASPECT %.2f", m_fForcedAspectRatio);
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESET))
    {
      CLog::Log(LOGDEBUG, "COMXPlayerVideo - CDVDMsg::GENERAL_RESET");
      m_av_clock->Lock();
      m_av_clock->OMXStop(false);
      m_omxVideo.Reset();
      m_av_clock->OMXReset(false);
      m_av_clock->UnLock();
      m_started = false;
      m_iSleepEndTime = DVD_NOPTS_VALUE;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH)) // private message sent by (COMXPlayerVideo::Flush())
    {
      CLog::Log(LOGDEBUG, "COMXPlayerVideo - CDVDMsg::GENERAL_FLUSH");
      m_stalled = true;
      m_started = false;
      m_iSleepEndTime = DVD_NOPTS_VALUE;
      m_av_clock->Lock();
      m_av_clock->OMXStop(false);
      m_omxVideo.Reset();
      m_av_clock->OMXReset(false);
      m_av_clock->UnLock();
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
    else if (pMsg->IsType(CDVDMsg::PLAYER_STARTED))
    {
      CLog::Log(LOGDEBUG, "COMXPlayerVideo - CDVDMsg::PLAYER_STARTED %d", m_started);
      if(m_started)
        m_messageParent.Put(new CDVDMsgInt(CDVDMsg::PLAYER_STARTED, DVDPLAYER_VIDEO));
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
          (int)pPacket->iSize, m_started, m_flush, bPacketDrop, m_stalled, m_speed, 0, 0, m_av_clock->OMXLateCount(1));
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
          CLog::Log(LOGINFO, "COMXPlayerVideo - Stillframe left, switching to normal playback");
          m_stalled = false;
        }

        double output_pts = 0;
        // validate picture timing,
        // if both dts/pts invalid, use pts calulated from picture.iDuration
        // if pts invalid use dts, else use picture.pts as passed
        if (pPacket->dts == DVD_NOPTS_VALUE && pPacket->pts == DVD_NOPTS_VALUE)
          output_pts = pts;
        else if (pPacket->pts == DVD_NOPTS_VALUE)
          output_pts = pts;
        else
          output_pts = pPacket->pts;

        if(pPacket->pts != DVD_NOPTS_VALUE)
          pPacket->pts += m_iVideoDelay;

        if(pPacket->dts != DVD_NOPTS_VALUE)
          pPacket->dts += m_iVideoDelay;

        if(pPacket->duration == 0)
          pPacket->duration = frametime;

        if(output_pts != DVD_NOPTS_VALUE)
          pts = output_pts;

        m_omxVideo.Decode(pPacket->pData, pPacket->iSize, pPacket->dts, pPacket->pts);

        Output(pPacket->iGroupId, output_pts, bRequestDrop);

        if(m_started == false)
        {
          m_codecname = m_omxVideo.GetDecoderName();
          m_started = true;
          m_messageParent.Put(new CDVDMsgInt(CDVDMsg::PLAYER_STARTED, DVDPLAYER_VIDEO));
        }

        // guess next frame pts. iDuration is always valid
        if (m_speed != 0)
          pts += pPacket->duration * m_speed / abs(m_speed);

        break;
      }

      bRequestDrop = false;

      m_videoStats.AddSampleBytes(pPacket->iSize);
    }
    pMsg->Release();

  }
}

void OMXPlayerVideo::Flush()
{
  m_flush = true;
  m_messageQueue.Flush();
  m_messageQueue.Put(new CDVDMsg(CDVDMsg::GENERAL_FLUSH), 1);
}

bool OMXPlayerVideo::OpenDecoder()
{
  if(!m_av_clock)
    return false;

  if (m_hints.fpsrate && m_hints.fpsscale)
    m_fFrameRate = DVD_TIME_BASE / OMXClock::NormalizeFrameduration((double)DVD_TIME_BASE * m_hints.fpsscale / m_hints.fpsrate);
  else
    m_fFrameRate = 25;

  if( m_fFrameRate > 100 || m_fFrameRate < 5 )
  {
    CLog::Log(LOGINFO, "OMXPlayerVideo::OpenDecoder : Invalid framerate %d, using forced 25fps and just trust timestamps\n", (int)m_fFrameRate);
    m_fFrameRate = 25;
  }
  // use aspect in stream always
  m_fForcedAspectRatio = m_hints.aspect;


  m_av_clock->Lock();
  m_av_clock->OMXStop(false);

  bool bVideoDecoderOpen = m_omxVideo.Open(m_hints, m_av_clock, m_Deinterlace, m_hdmi_clock_sync);
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

    m_codecname = m_omxVideo.GetDecoderName();

    // if we are closer to ntsc version of framerate, let gpu know
    int   iFrameRate  = (int)(m_fFrameRate + 0.5f);
    bool  bNtscFreq  = fabs(m_fFrameRate * 1001.0f / 1000.0f - iFrameRate) < fabs(m_fFrameRate - iFrameRate);
    char  response[80], command[80];
    sprintf(command, "hdmi_ntsc_freqs %d", bNtscFreq);
    CLog::Log(LOGINFO, "OMXPlayerVideo::OpenDecoder fps: %f %s\n", m_fFrameRate, command);
    m_DllBcmHost.vc_gencmd(response, sizeof response, command);

    if(m_av_clock)
      m_av_clock->SetRefreshRate(m_fFrameRate);
  }

  m_av_clock->OMXStateExecute(false);
  m_av_clock->HasVideo(bVideoDecoderOpen);
  m_av_clock->OMXReset(false);
  m_av_clock->UnLock();

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

bool OMXPlayerVideo::SubmittedEOS()
{
  return m_omxVideo.SubmittedEOS();
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
  std::ostringstream s;
  s << "fr:"     << fixed << setprecision(3) << m_fFrameRate;
  s << ", vq:"   << setw(2) << min(99,GetLevel()) << "%";
  s << ", dc:"   << m_codecname;
  s << ", Mb/s:" << fixed << setprecision(2) << (double)GetVideoBitrate() / (1024.0*1024.0);

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

void OMXPlayerVideo::SetVideoRect(const CRect &SrcRect, const CRect &DestRect)
{
  // check if destination rect or video view mode has changed
  if ((m_dst_rect != DestRect) || (m_view_mode != g_settings.m_currentVideoSettings.m_ViewMode))
  {
    m_dst_rect  = DestRect;
    m_view_mode = g_settings.m_currentVideoSettings.m_ViewMode;
  }
  else
  {
    return;
  }

  // might need to scale up m_dst_rect to display size as video decodes
  // to separate video plane that is at display size.
  CRect gui, display, dst_rect;
  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  gui.SetRect(0, 0, g_settings.m_ResInfo[res].iWidth, g_settings.m_ResInfo[res].iHeight);
  display.SetRect(0, 0, g_settings.m_ResInfo[res].iScreenWidth, g_settings.m_ResInfo[res].iScreenHeight);
  
  dst_rect = m_dst_rect;
  if (gui != display)
  {
    float xscale = display.Width()  / gui.Width();
    float yscale = display.Height() / gui.Height();
    // video is displayed in absolute coordinates (bypassing half width or height GUI mode)
    if (m_flags & CONF_FLAGS_FORMAT_SBS) xscale *= 2.0f;
    if (m_flags & CONF_FLAGS_FORMAT_TB)  yscale *= 2.0f;
    dst_rect.x1 *= xscale;
    dst_rect.x2 *= xscale;
    dst_rect.y1 *= yscale;
    dst_rect.y2 *= yscale;
  }
  m_omxVideo.SetVideoRect(SrcRect, dst_rect);
}

void OMXPlayerVideo::RenderUpdateCallBack(const void *ctx, const CRect &SrcRect, const CRect &DestRect)
{
  OMXPlayerVideo *player = (OMXPlayerVideo*)ctx;
  player->SetVideoRect(SrcRect, DestRect);
}

void OMXPlayerVideo::ResolutionUpdateCallBack(uint32_t width, uint32_t height)
{
  RESOLUTION res  = g_graphicsContext.GetVideoResolution();
  uint32_t video_width   = g_settings.m_ResInfo[res].iScreenWidth;
  uint32_t video_height  = g_settings.m_ResInfo[res].iScreenHeight;

  unsigned flags = 0;
  ERenderFormat format = RENDER_FMT_BYPASS;

  if(m_bAllowFullscreen)
  {
    flags |= CONF_FLAGS_FULLSCREEN;
    m_bAllowFullscreen = false; // only allow on first configure
  }

  if(m_flags & CONF_FLAGS_FORMAT_SBS)
  {
    if(g_Windowing.Support3D(video_width, video_height, D3DPRESENTFLAG_MODE3DSBS))
    {
      CLog::Log(LOGNOTICE, "3DSBS movie found");
      flags |= CONF_FLAGS_FORMAT_SBS;
    }
  }
  else if(m_flags & CONF_FLAGS_FORMAT_TB)
  {
    if(g_Windowing.Support3D(video_width, video_height, D3DPRESENTFLAG_MODE3DTB))
    {
      CLog::Log(LOGNOTICE, "3DTB movie found");
      flags |= CONF_FLAGS_FORMAT_TB;
    }
  }

  unsigned int iDisplayWidth  = width;
  unsigned int iDisplayHeight = height;

  /* use forced aspect if any */
  if( m_fForcedAspectRatio != 0.0f )
    iDisplayWidth = (int) (iDisplayHeight * m_fForcedAspectRatio);

  CLog::Log(LOGDEBUG,"%s - change configuration. video:%dx%d. framerate: %4.2f. %dx%d format: BYPASS",
      __FUNCTION__, video_width, video_height, m_fFrameRate, iDisplayWidth, iDisplayHeight);

  if(!g_renderManager.Configure(width, height,
        iDisplayWidth, iDisplayHeight, m_fFrameRate, flags, format, 0,
        m_hints.orientation))
  {
    CLog::Log(LOGERROR, "%s - failed to configure renderer", __FUNCTION__);
    return;
  }

  g_renderManager.RegisterRenderUpdateCallBack((const void*)this, RenderUpdateCallBack);
}

void OMXPlayerVideo::ResolutionUpdateCallBack(void *ctx, uint32_t width, uint32_t height)
{
  OMXPlayerVideo *player = static_cast<OMXPlayerVideo*>(ctx);
  player->ResolutionUpdateCallBack(width, height);
}

