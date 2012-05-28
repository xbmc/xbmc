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
#include "cores/VideoRenderers/RenderFlags.h"
#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "video/VideoReferenceClock.h"
#include "utils/MathUtils.h"
#include "DVDPlayer.h"
#include "DVDPlayerVideo.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDCodecs/DVDCodecUtils.h"
#include "DVDCodecs/Video/DVDVideoPPFFmpeg.h"
#include "DVDCodecs/Video/DVDVideoCodecFFmpeg.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDOverlayRenderer.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/Overlay/DVDOverlayCodecCC.h"
#include "DVDCodecs/Overlay/DVDOverlaySSA.h"
#include "guilib/GraphicContext.h"
#include <sstream>
#include <iomanip>
#include <numeric>
#include <iterator>
#include "guilib/GraphicContext.h"
#include "utils/log.h"

using namespace std;
using namespace RenderManager;

class CPulldownCorrection
{
public:
  CPulldownCorrection()
  {
    m_duration = 0.0;
    m_accum    = 0;
    m_total    = 0;
    m_next     = m_pattern.end();
  }

  void init(double fps, int *begin, int *end)
  {
    std::copy(begin, end, std::back_inserter(m_pattern));
    m_duration = DVD_TIME_BASE / fps;
    m_accum    = 0;
    m_total    = std::accumulate(m_pattern.begin(), m_pattern.end(), 0);
    m_next     = m_pattern.begin();
  }

  double pts()
  {
    double input  = m_duration * std::distance(m_pattern.begin(), m_next);
    double output = m_duration * m_accum / m_total;
    return output - input;
  }

  double dur()
  {
    return m_duration * m_pattern.size() * *m_next / m_total;
  }

  void next()
  {
    m_accum += *m_next;
    if(++m_next == m_pattern.end())
    {
      m_next  = m_pattern.begin();
      m_accum = 0;
    }
  }

  bool enabled()
  {
    return !m_pattern.empty();
  }
private:
  double                     m_duration;
  int                        m_total;
  int                        m_accum;
  std::vector<int>           m_pattern;
  std::vector<int>::iterator m_next;
};


class CDVDMsgVideoCodecChange : public CDVDMsg
{
public:
  CDVDMsgVideoCodecChange(const CDVDStreamInfo &hints, CDVDVideoCodec* codec)
    : CDVDMsg(GENERAL_STREAMCHANGE)
    , m_codec(codec)
    , m_hints(hints)
  {}
 ~CDVDMsgVideoCodecChange()
  {
    delete m_codec;
  }
  CDVDVideoCodec* m_codec;
  CDVDStreamInfo  m_hints;
};


CDVDPlayerVideo::CDVDPlayerVideo( CDVDClock* pClock
                                , CDVDOverlayContainer* pOverlayContainer
                                , CDVDMessageQueue& parent)
: CThread("DVDPlayerVideo")
, m_messageQueue("video")
, m_messageParent(parent)
{
  m_pClock = pClock;
  m_pOverlayContainer = pOverlayContainer;
  m_pTempOverlayPicture = NULL;
  m_pVideoCodec = NULL;
  m_pOverlayCodecCC = NULL;
  m_speed = DVD_PLAYSPEED_NORMAL;

  m_bRenderSubs = false;
  m_stalled = false;
  m_started = false;
  m_iVideoDelay = 0;
  m_iSubtitleDelay = 0;
  m_FlipTimeStamp = 0.0;
  m_iLateFrames = 0;
  m_iDroppedRequest = 0;
  m_fForcedAspectRatio = 0;
  m_iNrOfPicturesNotToSkip = 0;
  m_messageQueue.SetMaxDataSize(40 * 1024 * 1024);
  m_messageQueue.SetMaxTimeSize(8.0);

  m_iCurrentPts = DVD_NOPTS_VALUE;
  m_iDroppedFrames = 0;
  m_fFrameRate = 25;
  m_bCalcFrameRate = false;
  m_fStableFrameRate = 0.0;
  m_iFrameRateCount = 0;
  m_bAllowDrop = false;
  m_iFrameRateErr = 0;
  m_iFrameRateLength = 0;
  m_bFpsInvalid = false;
  m_bAllowFullscreen = false;
  memset(&m_output, 0, sizeof(m_output));
}

CDVDPlayerVideo::~CDVDPlayerVideo()
{
  StopThread();
  g_VideoReferenceClock.StopThread();
}

double CDVDPlayerVideo::GetOutputDelay()
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

bool CDVDPlayerVideo::OpenStream( CDVDStreamInfo &hint )
{
  unsigned int surfaces = 0;
  std::vector<ERenderFormat> formats;
#ifdef HAS_VIDEO_PLAYBACK
  surfaces = g_renderManager.GetProcessorSize();
  formats  = g_renderManager.SupportedFormats();
#endif


  CLog::Log(LOGNOTICE, "Creating video codec with codec id: %i", hint.codec);
  CDVDVideoCodec* codec = CDVDFactoryCodec::CreateVideoCodec(hint, surfaces, formats);
  if(!codec)
  {
    CLog::Log(LOGERROR, "Unsupported video codec");
    return false;
  }

  if(CSettings::Get().GetBool("videoplayer.usedisplayasclock") && !g_VideoReferenceClock.IsRunning())
  {
    g_VideoReferenceClock.Create();
    //we have to wait for the clock to start otherwise alsa can cause trouble
    if (!g_VideoReferenceClock.WaitStarted(2000))
      CLog::Log(LOGDEBUG, "g_VideoReferenceClock didn't start in time");
  }

  if(m_messageQueue.IsInited())
    m_messageQueue.Put(new CDVDMsgVideoCodecChange(hint, codec), 0);
  else
  {
    OpenStream(hint, codec);
    CLog::Log(LOGNOTICE, "Creating video thread");
    m_messageQueue.Init();
    Create();
  }
  return true;
}

void CDVDPlayerVideo::OpenStream(CDVDStreamInfo &hint, CDVDVideoCodec* codec)
{
  //reported fps is usually not completely correct
  if (hint.fpsrate && hint.fpsscale)
    m_fFrameRate = DVD_TIME_BASE / CDVDCodecUtils::NormalizeFrameduration((double)DVD_TIME_BASE * hint.fpsscale / hint.fpsrate);
  else
    m_fFrameRate = 25;

  m_bFpsInvalid = (hint.fpsrate == 0 || hint.fpsscale == 0);

  m_bCalcFrameRate = CSettings::Get().GetBool("videoplayer.usedisplayasclock") ||
                     CSettings::Get().GetInt("videoplayer.adjustrefreshrate") != ADJUST_REFRESHRATE_OFF;
  ResetFrameRateCalc();

  m_iDroppedRequest = 0;
  m_iLateFrames = 0;

  if( m_fFrameRate > 100 || m_fFrameRate < 5 )
  {
    CLog::Log(LOGERROR, "CDVDPlayerVideo::OpenStream - Invalid framerate %d, using forced 25fps and just trust timestamps", (int)m_fFrameRate);
    m_fFrameRate = 25;
  }

  // use aspect in stream if available
  if(hint.forced_aspect)
    m_fForcedAspectRatio = hint.aspect;
  else
    m_fForcedAspectRatio = 0.0;

  if (m_pVideoCodec)
    delete m_pVideoCodec;

  m_pVideoCodec = codec;
  m_hints   = hint;
  m_stalled = m_messageQueue.GetPacketCount(CDVDMsg::DEMUXER_PACKET) == 0;
  m_started = false;
  m_codecname = m_pVideoCodec->GetName();
  m_packets.clear();
}

void CDVDPlayerVideo::CloseStream(bool bWaitForBuffers)
{
  // wait until buffers are empty
  if (bWaitForBuffers && m_speed > 0) m_messageQueue.WaitUntilEmpty();

  m_messageQueue.Abort();

  // wait for decode_video thread to end
  CLog::Log(LOGNOTICE, "waiting for video thread to exit");

  StopThread(); // will set this->m_bStop to true

  m_messageQueue.End();

  CLog::Log(LOGNOTICE, "deleting video codec");
  if (m_pVideoCodec)
  {
    m_pVideoCodec->Dispose();
    delete m_pVideoCodec;
    m_pVideoCodec = NULL;
  }

  if (m_pTempOverlayPicture)
  {
    CDVDCodecUtils::FreePicture(m_pTempOverlayPicture);
    m_pTempOverlayPicture = NULL;
  }

  //tell the clock we stopped playing video
  m_pClock->UpdateFramerate(0.0);
}

void CDVDPlayerVideo::OnStartup()
{
  m_iDroppedFrames = 0;

  m_crop.x1 = m_crop.x2 = 0.0f;
  m_crop.y1 = m_crop.y2 = 0.0f;

  m_iCurrentPts = DVD_NOPTS_VALUE;
  m_FlipTimeStamp = m_pClock->GetAbsoluteClock();

}

void CDVDPlayerVideo::Process()
{
  CLog::Log(LOGNOTICE, "running thread: video_thread");

  DVDVideoPicture picture;
  CPulldownCorrection pulldown;
  CDVDVideoPPFFmpeg mPostProcess("");
  CStdString sPostProcessType;
  bool bPostProcessDeint = false;

  memset(&picture, 0, sizeof(DVDVideoPicture));

  double pts = 0;
  double frametime = (double)DVD_TIME_BASE / m_fFrameRate;

  int iDropped = 0; //frames dropped in a row
  bool bRequestDrop = false;
  int iDropDirective;

  m_videoStats.Start();
  m_droppingStats.Reset();

  while (!m_bStop)
  {
    int iQueueTimeOut = (int)(m_stalled ? frametime / 4 : frametime * 10) / 1000;
    int iPriority = (m_speed == DVD_PLAYSPEED_PAUSE && m_started) ? 1 : 0;

    CDVDMsg* pMsg;
    MsgQueueReturnCode ret = m_messageQueue.Get(&pMsg, iQueueTimeOut, iPriority);

    if (MSGQ_IS_ERROR(ret))
    {
      CLog::Log(LOGERROR, "Got MSGQ_ABORT or MSGO_IS_ERROR return true");
      break;
    }
    else if (ret == MSGQ_TIMEOUT)
    {
      // if we only wanted priority messages, this isn't a stall
      if( iPriority )
        continue;

      //Okey, start rendering at stream fps now instead, we are likely in a stillframe
      if( !m_stalled )
      {
        if(m_started)
          CLog::Log(LOGINFO, "CDVDPlayerVideo - Stillframe detected, switching to forced %f fps", m_fFrameRate);
        m_stalled = true;
        pts+= frametime*4;
      }

      //Waiting timed out, output last picture
      if( picture.iFlags & DVP_FLAG_ALLOCATED )
      {
        //Remove interlaced flag before outputting
        //no need to output this as if it was interlaced
        picture.iFlags &= ~DVP_FLAG_INTERLACED;
        picture.iFlags |= DVP_FLAG_NOSKIP;
        OutputPicture(&picture, pts);
        pts+= frametime;
      }

      continue;
    }

    if (pMsg->IsType(CDVDMsg::GENERAL_SYNCHRONIZE))
    {
      if(((CDVDMsgGeneralSynchronize*)pMsg)->Wait(100, SYNCSOURCE_VIDEO))
      {
        CLog::Log(LOGDEBUG, "CDVDPlayerVideo - CDVDMsg::GENERAL_SYNCHRONIZE");

        /* we may be very much off correct pts here, but next picture may be a still*/
        /* make sure it isn't dropped */
        m_iNrOfPicturesNotToSkip = 5;
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

      double delay = m_FlipTimeStamp - m_pClock->GetAbsoluteClock();
      if( delay > frametime ) delay = frametime;
      else if( delay < 0 )    delay = 0;

      if(pMsgGeneralResync->m_clock)
      {
        CLog::Log(LOGDEBUG, "CDVDPlayerVideo - CDVDMsg::GENERAL_RESYNC(%f, 1)", pts);
        m_pClock->Discontinuity(pts - delay);
      }
      else
        CLog::Log(LOGDEBUG, "CDVDPlayerVideo - CDVDMsg::GENERAL_RESYNC(%f, 0)", pts);

      pMsgGeneralResync->Release();
      continue;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_DELAY))
    {
      if (m_speed != DVD_PLAYSPEED_PAUSE)
      {
        double timeout = static_cast<CDVDMsgDouble*>(pMsg)->m_value;

        CLog::Log(LOGDEBUG, "CDVDPlayerVideo - CDVDMsg::GENERAL_DELAY(%f)", timeout);

        timeout *= (double)DVD_PLAYSPEED_NORMAL / abs(m_speed);
        timeout += CDVDClock::GetAbsoluteClock();

        while(!m_bStop && CDVDClock::GetAbsoluteClock() < timeout)
          Sleep(1);
      }
    }
    else if (pMsg->IsType(CDVDMsg::VIDEO_SET_ASPECT))
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerVideo - CDVDMsg::VIDEO_SET_ASPECT");
      m_fForcedAspectRatio = *((CDVDMsgDouble*)pMsg);
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESET))
    {
      if(m_pVideoCodec)
        m_pVideoCodec->Reset();
      picture.iFlags &= ~DVP_FLAG_ALLOCATED;
      m_packets.clear();
      m_started = false;
      m_droppingStats.Reset();
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH)) // private message sent by (CDVDPlayerVideo::Flush())
    {
      if(m_pVideoCodec)
        m_pVideoCodec->Reset();
      picture.iFlags &= ~DVP_FLAG_ALLOCATED;
      m_packets.clear();

      m_pullupCorrection.Flush();
      //we need to recalculate the framerate
      //TODO: this needs to be set on a streamchange instead
      ResetFrameRateCalc();
      m_droppingStats.Reset();

      m_stalled = true;
      m_started = false;
    }
    else if (pMsg->IsType(CDVDMsg::VIDEO_NOSKIP))
    {
      // libmpeg2 is also returning incomplete frames after a dvd cell change
      // so the first few pictures are not the correct ones to display in some cases
      // just display those together with the correct one.
      // (setting it to 2 will skip some menu stills, 5 is working ok for me).
      m_iNrOfPicturesNotToSkip = 5;
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      m_speed = static_cast<CDVDMsgInt*>(pMsg)->m_value;
      if(m_speed == DVD_PLAYSPEED_PAUSE)
        m_iNrOfPicturesNotToSkip = 0;

      if (m_pVideoCodec)
        m_pVideoCodec->SetSpeed(m_speed);
      m_droppingStats.Reset();
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_STARTED))
    {
      if(m_started)
        m_messageParent.Put(new CDVDMsgInt(CDVDMsg::PLAYER_STARTED, DVDPLAYER_VIDEO));
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_DISPLAYTIME))
    {
      CDVDPlayer::SPlayerState& state = ((CDVDMsgType<CDVDPlayer::SPlayerState>*)pMsg)->m_value;

      if(state.time_src == CDVDPlayer::ETIMESOURCE_CLOCK)
        state.time      = DVD_TIME_TO_MSEC(m_pClock->GetClock(state.timestamp) + state.time_offset);
      else
        state.timestamp = CDVDClock::GetAbsoluteClock();
      state.player    = DVDPLAYER_VIDEO;
      m_messageParent.Put(pMsg->Acquire());
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_STREAMCHANGE))
    {
      CDVDMsgVideoCodecChange* msg(static_cast<CDVDMsgVideoCodecChange*>(pMsg));
      OpenStream(msg->m_hints, msg->m_codec);
      msg->m_codec = NULL;
      picture.iFlags &= ~DVP_FLAG_ALLOCATED;
    }

    if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      DemuxPacket* pPacket = ((CDVDMsgDemuxerPacket*)pMsg)->GetPacket();
      bool bPacketDrop     = ((CDVDMsgDemuxerPacket*)pMsg)->GetPacketDrop();

      if (m_stalled)
      {
        CLog::Log(LOGINFO, "CDVDPlayerVideo - Stillframe left, switching to normal playback");
        m_stalled = false;

        //don't allow the first frames after a still to be dropped
        //sometimes we get multiple stills (long duration frames) after each other
        //in normal mpegs
        m_iNrOfPicturesNotToSkip = 5;
      }
      else if( iDropped*frametime > DVD_MSEC_TO_TIME(100) && m_iNrOfPicturesNotToSkip == 0 )
      { // if we dropped too many pictures in a row, insert a forced picture
        m_iNrOfPicturesNotToSkip = 1;
      }

      bRequestDrop = false;
      iDropDirective = CalcDropRequirement(pts);
      if (iDropDirective & EOS_VERYLATE)
      {
        if (m_bAllowDrop)
        {
          m_pullupCorrection.Flush();
          bRequestDrop = true;
        }
      }
      int codecControl = 0;
      if (iDropDirective & EOS_BUFFER_LEVEL)
        codecControl |= DVP_FLAG_DRAIN;
      if (m_speed > DVD_PLAYSPEED_NORMAL)
        codecControl |= DVP_FLAG_NO_POSTPROC;
      m_pVideoCodec->SetCodecControl(codecControl);
      if (iDropDirective & EOS_DROPPED)
      {
        m_iDroppedFrames++;
        iDropped++;
      }

      if (m_messageQueue.GetDataSize() == 0
      ||  m_speed < 0)
      {
        bRequestDrop = false;
        m_iDroppedRequest = 0;
        m_iLateFrames     = 0;
      }

      // if player want's us to drop this packet, do so nomatter what
      if(bPacketDrop)
        bRequestDrop = true;

      // tell codec if next frame should be dropped
      // problem here, if one packet contains more than one frame
      // both frames will be dropped in that case instead of just the first
      // decoder still needs to provide an empty image structure, with correct flags
      m_pVideoCodec->SetDropState(bRequestDrop);

      // ask codec to do deinterlacing if possible
      EDEINTERLACEMODE mDeintMode = CMediaSettings::Get().GetCurrentVideoSettings().m_DeinterlaceMode;
      EINTERLACEMETHOD mInt       = g_renderManager.AutoInterlaceMethod(CMediaSettings::Get().GetCurrentVideoSettings().m_InterlaceMethod);

      unsigned int     mFilters = 0;

      if (mDeintMode != VS_DEINTERLACEMODE_OFF)
      {
        if (mInt == VS_INTERLACEMETHOD_DEINTERLACE)
          mFilters = CDVDVideoCodec::FILTER_DEINTERLACE_ANY;
        else if(mInt == VS_INTERLACEMETHOD_DEINTERLACE_HALF)
          mFilters = CDVDVideoCodec::FILTER_DEINTERLACE_ANY | CDVDVideoCodec::FILTER_DEINTERLACE_HALFED;

        if (mDeintMode == VS_DEINTERLACEMODE_AUTO && mFilters)
          mFilters |=  CDVDVideoCodec::FILTER_DEINTERLACE_FLAGGED;
      }

      if (!g_renderManager.Supports(RENDERFEATURE_ROTATION))
        mFilters |= CDVDVideoCodec::FILTER_ROTATE;

      mFilters = m_pVideoCodec->SetFilters(mFilters);

      int iDecoderState = m_pVideoCodec->Decode(pPacket->pData, pPacket->iSize, pPacket->dts, pPacket->pts);

      // buffer packets so we can recover should decoder flush for some reason
      if(m_pVideoCodec->GetConvergeCount() > 0)
      {
        m_packets.push_back(DVDMessageListItem(pMsg, 0));
        if(m_packets.size() > m_pVideoCodec->GetConvergeCount()
        || m_packets.size() * frametime > DVD_SEC_TO_TIME(10))
          m_packets.pop_front();
      }

      m_videoStats.AddSampleBytes(pPacket->iSize);

      // reset the request, the following while loop may break before
      // setting the flag to a new value
      bRequestDrop = false;

      // loop while no error
      while (!m_bStop)
      {

        // if decoder was flushed, we need to seek back again to resume rendering
        if (iDecoderState & VC_FLUSHED)
        {
          CLog::Log(LOGDEBUG, "CDVDPlayerVideo - video decoder was flushed");
          while(!m_packets.empty())
          {
            CDVDMsgDemuxerPacket* msg = (CDVDMsgDemuxerPacket*)m_packets.front().message->Acquire();
            m_packets.pop_front();

            // all packets except the last one should be dropped
            // if prio packets and current packet should be dropped, this is likely a new reset
            msg->m_drop = !m_packets.empty() || (iPriority > 0 && bPacketDrop);
            m_messageQueue.Put(msg, iPriority + 10);
          }

          m_pVideoCodec->Reset();
          m_packets.clear();
          picture.iFlags &= ~DVP_FLAG_ALLOCATED;
          g_renderManager.DiscardBuffer();
          break;
        }

        // if decoder had an error, tell it to reset to avoid more problems
        if (iDecoderState & VC_ERROR)
        {
          CLog::Log(LOGDEBUG, "CDVDPlayerVideo - video decoder returned error");
          break;
        }

        // check for a new picture
        if (iDecoderState & VC_PICTURE)
        {

          // try to retrieve the picture (should never fail!), unless there is a demuxer bug ofcours
          m_pVideoCodec->ClearPicture(&picture);
          if (m_pVideoCodec->GetPicture(&picture))
          {
            sPostProcessType.clear();

            if(picture.iDuration == 0.0)
              picture.iDuration = frametime;

            if(bPacketDrop)
              picture.iFlags |= DVP_FLAG_DROPPED;

            if (m_iNrOfPicturesNotToSkip > 0)
            {
              picture.iFlags |= DVP_FLAG_NOSKIP;
              m_iNrOfPicturesNotToSkip--;
            }

            // validate picture timing,
            // if both dts/pts invalid, use pts calulated from picture.iDuration
            // if pts invalid use dts, else use picture.pts as passed
            if (picture.dts == DVD_NOPTS_VALUE && picture.pts == DVD_NOPTS_VALUE)
              picture.pts = pts;
            else if (picture.pts == DVD_NOPTS_VALUE)
              picture.pts = picture.dts;

            /* use forced aspect if any */
            if( m_fForcedAspectRatio != 0.0f )
              picture.iDisplayWidth = (int) (picture.iDisplayHeight * m_fForcedAspectRatio);

            //Deinterlace if codec said format was interlaced or if we have selected we want to deinterlace
            //this video
            if ((mDeintMode == VS_DEINTERLACEMODE_AUTO && (picture.iFlags & DVP_FLAG_INTERLACED)) || mDeintMode == VS_DEINTERLACEMODE_FORCE)
            {
              if(mInt == VS_INTERLACEMETHOD_SW_BLEND)
              {
                if (!sPostProcessType.empty())
                  sPostProcessType += ",";
                sPostProcessType += g_advancedSettings.m_videoPPFFmpegDeint;
                bPostProcessDeint = true;
              }
            }

            if (CMediaSettings::Get().GetCurrentVideoSettings().m_PostProcess)
            {
              if (!sPostProcessType.empty())
                sPostProcessType += ",";
              // This is what mplayer uses for its "high-quality filter combination"
              sPostProcessType += g_advancedSettings.m_videoPPFFmpegPostProc;
            }

            if (!sPostProcessType.empty())
            {
              mPostProcess.SetType(sPostProcessType, bPostProcessDeint);
              if (mPostProcess.Process(&picture))
                mPostProcess.GetPicture(&picture);
            }

            /* if frame has a pts (usually originiating from demux packet), use that */
            if(picture.pts != DVD_NOPTS_VALUE)
            {
              if(pulldown.enabled())
                picture.pts += pulldown.pts();

              pts = picture.pts;
            }

            if(pulldown.enabled())
            {
              picture.iDuration = pulldown.dur();
              pulldown.next();
            }

            if (picture.iRepeatPicture)
              picture.iDuration *= picture.iRepeatPicture + 1;

            int iResult = OutputPicture(&picture, pts);

            if(m_started == false)
            {
              m_codecname = m_pVideoCodec->GetName();
              m_started = true;
              m_messageParent.Put(new CDVDMsgInt(CDVDMsg::PLAYER_STARTED, DVDPLAYER_VIDEO));
            }

            // guess next frame pts. iDuration is always valid
            if (m_speed != 0)
              pts += picture.iDuration * m_speed / abs(m_speed);

            if( iResult & EOS_ABORT )
            {
              //if we break here and we directly try to decode again wihout
              //flushing the video codec things break for some reason
              //i think the decoder (libmpeg2 atleast) still has a pointer
              //to the data, and when the packet is freed that will fail.
              iDecoderState = m_pVideoCodec->Decode(NULL, 0, DVD_NOPTS_VALUE, DVD_NOPTS_VALUE);
              break;
            }

            if( (iResult & EOS_DROPPED) && !bPacketDrop )
            {
              m_iDroppedFrames++;
              iDropped++;
            }
            else
              iDropped = 0;

            bRequestDrop = (iResult & EOS_VERYLATE) == EOS_VERYLATE;
          }
          else
          {
            CLog::Log(LOGWARNING, "Decoder Error getting videoPicture.");
            m_pVideoCodec->Reset();
          }
        }

        /*
        if (iDecoderState & VC_USERDATA)
        {
          // found some userdata while decoding a frame
          // could be closed captioning
          DVDVideoUserData videoUserData;
          if (m_pVideoCodec->GetUserData(&videoUserData))
          {
            ProcessVideoUserData(&videoUserData, pts);
          }
        }
        */

        // if the decoder needs more data, we just break this loop
        // and try to get more data from the videoQueue
        if (iDecoderState & VC_BUFFER)
          break;

        // the decoder didn't need more data, flush the remaning buffer
        iDecoderState = m_pVideoCodec->Decode(NULL, 0, DVD_NOPTS_VALUE, DVD_NOPTS_VALUE);
      }
    }

    // all data is used by the decoder, we can safely free it now
    pMsg->Release();
  }

  // we need to let decoder release any picture retained resources.
  m_pVideoCodec->ClearPicture(&picture);
}

void CDVDPlayerVideo::OnExit()
{
  if (m_pOverlayCodecCC)
  {
    m_pOverlayCodecCC->Dispose();
    m_pOverlayCodecCC = NULL;
  }

  CLog::Log(LOGNOTICE, "thread end: video_thread");
}

void CDVDPlayerVideo::ProcessVideoUserData(DVDVideoUserData* pVideoUserData, double pts)
{
  // check userdata type
  uint8_t* data = pVideoUserData->data;
  int size = pVideoUserData->size;

  if (size >= 2)
  {
    if (data[0] == 'C' && data[1] == 'C')
    {
      data += 2;
      size -= 2;

      // closed captioning
      if (!m_pOverlayCodecCC)
      {
        m_pOverlayCodecCC = new CDVDOverlayCodecCC();
        CDVDCodecOptions options;
        CDVDStreamInfo info;
        if (!m_pOverlayCodecCC->Open(info, options))
        {
          delete m_pOverlayCodecCC;
          m_pOverlayCodecCC = NULL;
        }
      }

      if (m_pOverlayCodecCC)
      {
        DemuxPacket packet;
        packet.pData = data;
        packet.iSize = size;
        packet.pts = DVD_NOPTS_VALUE;
        packet.dts = DVD_NOPTS_VALUE;
        m_pOverlayCodecCC->Decode(&packet);

        CDVDOverlay* overlay;
        while((overlay = m_pOverlayCodecCC->GetOverlay()) != NULL)
        {
          overlay->iPTSStartTime += pts;
          if(overlay->iPTSStopTime != 0.0)
            overlay->iPTSStopTime += pts;

          m_pOverlayContainer->Add(overlay);
          overlay->Release();
        }
      }
    }
  }
}

bool CDVDPlayerVideo::InitializedOutputDevice()
{
#ifdef HAS_VIDEO_PLAYBACK
  return g_renderManager.IsStarted();
#else
  return false;
#endif
}

void CDVDPlayerVideo::SetSpeed(int speed)
{
  if(m_messageQueue.IsInited())
    m_messageQueue.Put( new CDVDMsgInt(CDVDMsg::PLAYER_SETSPEED, speed), 1 );
  else
    m_speed = speed;
}

void CDVDPlayerVideo::StepFrame()
{
  m_iNrOfPicturesNotToSkip++;
}

void CDVDPlayerVideo::Flush()
{
  /* flush using message as this get's called from dvdplayer thread */
  /* and any demux packet that has been taken out of queue need to */
  /* be disposed of before we flush */
  m_messageQueue.Flush();
  m_messageQueue.Put(new CDVDMsg(CDVDMsg::GENERAL_FLUSH), 1);
}

int CDVDPlayerVideo::GetLevel()
{
  int level = m_messageQueue.GetLevel();

  // fast exit, if the message queue is full, we do not care about the codec queue.
  if (level == 100)
    return level;

  // Now for the harder choices, the message queue could be time or size based.
  // In order to return the proper summed level, we need to know which.
  if (m_messageQueue.IsDataBased())
  {
    int datasize = m_messageQueue.GetDataSize();
    if (m_pVideoCodec)
      datasize += m_pVideoCodec->GetDataSize();
    return min(100, (int)(100 * datasize / (m_messageQueue.GetMaxDataSize() * m_messageQueue.GetMaxTimeSize())));
  }
  else
  {
    double timesize = m_messageQueue.GetTimeSize();
    if (m_pVideoCodec)
      timesize += m_pVideoCodec->GetTimeSize();
    return min(100, MathUtils::round_int(100.0 * m_messageQueue.GetMaxTimeSize() * timesize));
  }

  return level;
}

#ifdef HAS_VIDEO_PLAYBACK
void CDVDPlayerVideo::ProcessOverlays(DVDVideoPicture* pSource, double pts)
{
  // remove any overlays that are out of time
  if (m_started)
    m_pOverlayContainer->CleanUp(pts - m_iSubtitleDelay);

  enum EOverlay
  { OVERLAY_AUTO // select mode auto
  , OVERLAY_GPU  // render osd using gpu
  , OVERLAY_BUF  // render osd on buffer
  } render = OVERLAY_AUTO;

  if(pSource->format == RENDER_FMT_YUV420P)
  {
    if(g_Windowing.GetRenderQuirks() & RENDER_QUIRKS_MAJORMEMLEAK_OVERLAYRENDERER)
    {
      // for now use cpu for ssa overlays as it currently allocates and
      // frees textures for each frame this causes a hugh memory leak
      // on some mesa intel drivers

      if(m_pOverlayContainer->ContainsOverlayType(DVDOVERLAY_TYPE_SPU)
      || m_pOverlayContainer->ContainsOverlayType(DVDOVERLAY_TYPE_IMAGE)
      || m_pOverlayContainer->ContainsOverlayType(DVDOVERLAY_TYPE_SSA) )
        render = OVERLAY_BUF;
    }

    if(render == OVERLAY_BUF)
    {
      // rendering spu overlay types directly on video memory costs a lot of processing power.
      // thus we allocate a temp picture, copy the original to it (needed because the same picture can be used more than once).
      // then do all the rendering on that temp picture and finaly copy it to video memory.
      // In almost all cases this is 5 or more times faster!.

      if(m_pTempOverlayPicture && ( m_pTempOverlayPicture->iWidth  != pSource->iWidth
                                 || m_pTempOverlayPicture->iHeight != pSource->iHeight))
      {
        CDVDCodecUtils::FreePicture(m_pTempOverlayPicture);
        m_pTempOverlayPicture = NULL;
      }

      if(!m_pTempOverlayPicture)
        m_pTempOverlayPicture = CDVDCodecUtils::AllocatePicture(pSource->iWidth, pSource->iHeight);
      if(!m_pTempOverlayPicture)
        return;

      CDVDCodecUtils::CopyPicture(m_pTempOverlayPicture, pSource);
      memcpy(pSource->data     , m_pTempOverlayPicture->data     , sizeof(pSource->data));
      memcpy(pSource->iLineSize, m_pTempOverlayPicture->iLineSize, sizeof(pSource->iLineSize));
    }
  }

  if(render == OVERLAY_AUTO)
    render = OVERLAY_GPU;

  VecOverlays overlays;

  {
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

      if (render == OVERLAY_GPU)
        g_renderManager.AddOverlay(*it, pts2);

      if (render == OVERLAY_BUF)
        CDVDOverlayRenderer::Render(pSource, *it, pts2);
    }
  }


}
#endif

static std::string GetRenderFormatName(ERenderFormat format)
{
  switch(format)
  {
    case RENDER_FMT_YUV420P:   return "YV12";
    case RENDER_FMT_YUV420P16: return "YV12P16";
    case RENDER_FMT_YUV420P10: return "YV12P10";
    case RENDER_FMT_NV12:      return "NV12";
    case RENDER_FMT_UYVY422:   return "UYVY";
    case RENDER_FMT_YUYV422:   return "YUY2";
    case RENDER_FMT_VDPAU:     return "VDPAU";
    case RENDER_FMT_VDPAU_420: return "VDPAU_420";
    case RENDER_FMT_DXVA:      return "DXVA";
    case RENDER_FMT_VAAPI:     return "VAAPI";
    case RENDER_FMT_OMXEGL:    return "OMXEGL";
    case RENDER_FMT_CVBREF:    return "BGRA";
    case RENDER_FMT_EGLIMG:    return "EGLIMG";
    case RENDER_FMT_BYPASS:    return "BYPASS";
    case RENDER_FMT_MEDIACODEC:return "MEDIACODEC";
    case RENDER_FMT_NONE:      return "NONE";
  }
  return "UNKNOWN";
}

std::string CDVDPlayerVideo::GetStereoMode()
{
  std::string  stereo_mode;

  switch(CMediaSettings::Get().GetCurrentVideoSettings().m_StereoMode)
  {
    case RENDER_STEREO_MODE_SPLIT_VERTICAL:   stereo_mode = "left_right"; break;
    case RENDER_STEREO_MODE_SPLIT_HORIZONTAL: stereo_mode = "top_bottom"; break;
    default:                                  stereo_mode = m_hints.stereo_mode; break;
  }

  if(CMediaSettings::Get().GetCurrentVideoSettings().m_StereoInvert)
    stereo_mode = GetStereoModeInvert(stereo_mode);
  return stereo_mode;
}

int CDVDPlayerVideo::OutputPicture(const DVDVideoPicture* src, double pts)
{
  /* picture buffer is not allowed to be modified in this call */
  DVDVideoPicture picture(*src);
  DVDVideoPicture* pPicture = &picture;

  /* grab stereo mode from image if available */
  if(src->stereo_mode[0])
    m_hints.stereo_mode = src->stereo_mode;

  /* figure out steremode expected based on user settings and hints */
  unsigned int stereo_flags = GetStereoModeFlags(GetStereoMode());

#ifdef HAS_VIDEO_PLAYBACK
  double config_framerate = m_bFpsInvalid ? 0.0 : m_fFrameRate;
  /* check so that our format or aspect has changed. if it has, reconfigure renderer */
  if (!g_renderManager.IsConfigured()
   || ( m_output.width           != pPicture->iWidth )
   || ( m_output.height          != pPicture->iHeight )
   || ( m_output.dwidth          != pPicture->iDisplayWidth )
   || ( m_output.dheight         != pPicture->iDisplayHeight )
   || ( m_output.framerate       != config_framerate )
   || ( m_output.color_format    != (unsigned int)pPicture->format )
   || ( m_output.extended_format != pPicture->extended_format )
   || ( m_output.color_matrix    != pPicture->color_matrix    && pPicture->color_matrix    != 0 ) // don't reconfigure on unspecified
   || ( m_output.chroma_position != pPicture->chroma_position && pPicture->chroma_position != 0 )
   || ( m_output.color_primaries != pPicture->color_primaries && pPicture->color_primaries != 0 )
   || ( m_output.color_transfer  != pPicture->color_transfer  && pPicture->color_transfer  != 0 )
   || ( m_output.color_range     != pPicture->color_range )
   || ( m_output.stereo_flags    != stereo_flags))
  {
    CLog::Log(LOGNOTICE, " fps: %f, pwidth: %i, pheight: %i, dwidth: %i, dheight: %i"
                       , config_framerate
                       , pPicture->iWidth
                       , pPicture->iHeight
                       , pPicture->iDisplayWidth
                       , pPicture->iDisplayHeight);

    unsigned flags = 0;
    if(pPicture->color_range == 1)
      flags |= CONF_FLAGS_YUV_FULLRANGE;

    flags |= GetFlagsChromaPosition(pPicture->chroma_position)
          |  GetFlagsColorMatrix(pPicture->color_matrix, pPicture->iWidth, pPicture->iHeight)
          |  GetFlagsColorPrimaries(pPicture->color_primaries)
          |  GetFlagsColorTransfer(pPicture->color_transfer);

    CStdString formatstr = GetRenderFormatName(pPicture->format);

    if(m_bAllowFullscreen)
    {
      flags |= CONF_FLAGS_FULLSCREEN;
      m_bAllowFullscreen = false; // only allow on first configure
    }

    flags |= stereo_flags;

    CLog::Log(LOGDEBUG,"%s - change configuration. %dx%d. framerate: %4.2f. format: %s",__FUNCTION__,pPicture->iWidth, pPicture->iHeight, config_framerate, formatstr.c_str());
    if(!g_renderManager.Configure(pPicture->iWidth
                                , pPicture->iHeight
                                , pPicture->iDisplayWidth
                                , pPicture->iDisplayHeight
                                , config_framerate
                                , flags
                                , pPicture->format
                                , pPicture->extended_format
                                , m_hints.orientation
                                , m_pVideoCodec->GetAllowedReferences()))
    {
      CLog::Log(LOGERROR, "%s - failed to configure renderer", __FUNCTION__);
      return EOS_ABORT;
    }

    m_output.width           = pPicture->iWidth;
    m_output.height          = pPicture->iHeight;
    m_output.dwidth          = pPicture->iDisplayWidth;
    m_output.dheight         = pPicture->iDisplayHeight;
    m_output.framerate       = config_framerate;
    m_output.color_format    = pPicture->format;
    m_output.extended_format = pPicture->extended_format;
    m_output.color_matrix    = pPicture->color_matrix;
    m_output.chroma_position = pPicture->chroma_position;
    m_output.color_primaries = pPicture->color_primaries;
    m_output.color_transfer  = pPicture->color_transfer;
    m_output.color_range     = pPicture->color_range;
    m_output.stereo_flags    = stereo_flags;
  }

  int    result  = 0;

  if (!g_renderManager.IsStarted()) {
    CLog::Log(LOGERROR, "%s - renderer not started", __FUNCTION__);
    return EOS_ABORT;
  }

  //correct any pattern in the timestamps
  if (m_output.color_format != RENDER_FMT_BYPASS)
  {
    m_pullupCorrection.Add(pts);
    pts += m_pullupCorrection.GetCorrection();
  }

  //try to calculate the framerate
  CalcFrameRate();

  // signal to clock what our framerate is, it may want to adjust it's
  // speed to better match with our video renderer's output speed
  double interval;
  int refreshrate = m_pClock->UpdateFramerate(m_fFrameRate, &interval);
  if (refreshrate > 0) //refreshrate of -1 means the videoreferenceclock is not running
  {//when using the videoreferenceclock, a frame is always presented half a vblank interval too late
    pts -= DVD_TIME_BASE * interval;
  }

  if (m_output.color_format != RENDER_FMT_BYPASS)
  {
    // Correct pts by user set delay and rendering delay
    pts += m_iVideoDelay - DVD_SEC_TO_TIME(g_renderManager.GetDisplayLatency());
  }

  // calculate the time we need to delay this picture before displaying
  double iSleepTime, iClockSleep, iFrameSleep, iPlayingClock, iCurrentClock, iFrameDuration;

  iPlayingClock = m_pClock->GetClock(iCurrentClock, false); // snapshot current clock
  iClockSleep = pts - iPlayingClock; //sleep calculated by pts to clock comparison
  iFrameSleep = m_FlipTimeStamp - iCurrentClock; // sleep calculated by duration of frame
  iFrameDuration = pPicture->iDuration;

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

  if( m_started == false )
    iSleepTime = 0.0;
  else if( m_stalled )
    iSleepTime = iFrameSleep;
  else
    iSleepTime = iClockSleep;

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

  if ((pPicture->iFlags & DVP_FLAG_DROPPED))
  {
    m_droppingStats.AddOutputDropGain(pts, 1/m_fFrameRate);
    CLog::Log(LOGDEBUG,"%s - dropped in output", __FUNCTION__);
    return result | EOS_DROPPED;
  }

  // set fieldsync if picture is interlaced
  EFIELDSYNC mDisplayField = FS_NONE;
  if( pPicture->iFlags & DVP_FLAG_INTERLACED )
  {
    if( pPicture->iFlags & DVP_FLAG_TOP_FIELD_FIRST )
      mDisplayField = FS_TOP;
    else
      mDisplayField = FS_BOT;
  }

  AutoCrop(pPicture);

  int buffer = g_renderManager.WaitForBuffer(m_bStop, std::max(DVD_TIME_TO_MSEC(iSleepTime) + 500, 1));
  if (buffer < 0)
    return EOS_DROPPED;

  ProcessOverlays(pPicture, pts);

  int index = g_renderManager.AddVideoPicture(*pPicture);

  // video device might not be done yet
  while (index < 0 && !CThread::m_bStop &&
         CDVDClock::GetAbsoluteClock(false) < iCurrentClock + iSleepTime + DVD_MSEC_TO_TIME(500) )
  {
    Sleep(1);
    index = g_renderManager.AddVideoPicture(*pPicture);
  }

  if (index < 0)
    return EOS_DROPPED;

  g_renderManager.FlipPage(CThread::m_bStop, (iCurrentClock + iSleepTime) / DVD_TIME_BASE, pts, -1, mDisplayField);

  return result;
#else
  // no video renderer, let's mark it as dropped
  return EOS_DROPPED;
#endif
}

void CDVDPlayerVideo::AutoCrop(DVDVideoPicture *pPicture)
{
  if ((pPicture->format == RENDER_FMT_YUV420P) ||
     (pPicture->format == RENDER_FMT_NV12) ||
     (pPicture->format == RENDER_FMT_YUYV422) ||
     (pPicture->format == RENDER_FMT_UYVY422))
  {
    RECT crop;

    if (CMediaSettings::Get().GetCurrentVideoSettings().m_Crop)
      AutoCrop(pPicture, crop);
    else
    { // reset to defaults
      crop.left   = 0;
      crop.right  = 0;
      crop.top    = 0;
      crop.bottom = 0;
    }

    m_crop.x1 += ((float)crop.left   - m_crop.x1) * 0.1;
    m_crop.x2 += ((float)crop.right  - m_crop.x2) * 0.1;
    m_crop.y1 += ((float)crop.top    - m_crop.y1) * 0.1;
    m_crop.y2 += ((float)crop.bottom - m_crop.y2) * 0.1;

    crop.left   = MathUtils::round_int(m_crop.x1);
    crop.right  = MathUtils::round_int(m_crop.x2);
    crop.top    = MathUtils::round_int(m_crop.y1);
    crop.bottom = MathUtils::round_int(m_crop.y2);

    //compare with hysteresis
# define HYST(n, o) ((n) > (o) || (n) + 1 < (o))
    if(HYST(CMediaSettings::Get().GetCurrentVideoSettings().m_CropLeft  , crop.left)
    || HYST(CMediaSettings::Get().GetCurrentVideoSettings().m_CropRight , crop.right)
    || HYST(CMediaSettings::Get().GetCurrentVideoSettings().m_CropTop   , crop.top)
    || HYST(CMediaSettings::Get().GetCurrentVideoSettings().m_CropBottom, crop.bottom))
    {
      CMediaSettings::Get().GetCurrentVideoSettings().m_CropLeft   = crop.left;
      CMediaSettings::Get().GetCurrentVideoSettings().m_CropRight  = crop.right;
      CMediaSettings::Get().GetCurrentVideoSettings().m_CropTop    = crop.top;
      CMediaSettings::Get().GetCurrentVideoSettings().m_CropBottom = crop.bottom;
      g_renderManager.SetViewMode(CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode);
    }
# undef HYST
  }
}

void CDVDPlayerVideo::AutoCrop(DVDVideoPicture *pPicture, RECT &crop)
{
  crop.left   = CMediaSettings::Get().GetCurrentVideoSettings().m_CropLeft;
  crop.right  = CMediaSettings::Get().GetCurrentVideoSettings().m_CropRight;
  crop.top    = CMediaSettings::Get().GetCurrentVideoSettings().m_CropTop;
  crop.bottom = CMediaSettings::Get().GetCurrentVideoSettings().m_CropBottom;

  int black  = 16; // what is black in the image
  int level  = 8;  // how high above this should we detect
  int multi  = 4;  // what multiple of last line should failing line be to accept
  uint8_t *s;
  int last, detect, black2;

  // top and bottom levels
  black2 = black * pPicture->iWidth;
  detect = level * pPicture->iWidth + black2;

  //YV12 and NV12 have planar Y plane
  //YUY2 and UYVY have Y packed with U and V
  int xspacing = 1;
  int xstart   = 0;
  if (pPicture->format == RENDER_FMT_YUYV422)
    xspacing = 2;
  else if (pPicture->format == RENDER_FMT_UYVY422)
  {
    xspacing = 2;
    xstart   = 1;
  }

  // Crop top
  s      = pPicture->data[0];
  last   = black2;
  for (unsigned int y = 0; y < pPicture->iHeight/2; y++)
  {
    int total = 0;
    for (unsigned int x = xstart; x < pPicture->iWidth * xspacing; x += xspacing)
      total += s[x];
    s += pPicture->iLineSize[0];

    if (total > detect)
    {
      if (total - black2 > (last - black2) * multi)
        crop.top = y;
      break;
    }
    last = total;
  }

  // Crop bottom
  s    = pPicture->data[0] + (pPicture->iHeight-1) * pPicture->iLineSize[0];
  last = black2;
  for (unsigned int y = (int)pPicture->iHeight; y > pPicture->iHeight/2; y--)
  {
    int total = 0;
    for (unsigned int x = xstart; x < pPicture->iWidth * xspacing; x += xspacing)
      total += s[x];
    s -= pPicture->iLineSize[0];

    if (total > detect)
    {
      if (total - black2 > (last - black2) * multi)
        crop.bottom = pPicture->iHeight - y;
      break;
    }
    last = total;
  }

  // left and right levels
  black2 = black * pPicture->iHeight;
  detect = level * pPicture->iHeight + black2;


  // Crop left
  s    = pPicture->data[0];
  last = black2;
  for (unsigned int x = xstart; x < pPicture->iWidth/2*xspacing; x += xspacing)
  {
    int total = 0;
    for (unsigned int y = 0; y < pPicture->iHeight; y++)
      total += s[y * pPicture->iLineSize[0]];
    s++;
    if (total > detect)
    {
      if (total - black2 > (last - black2) * multi)
        crop.left = x / xspacing;
      break;
    }
    last = total;
  }

  // Crop right
  s    = pPicture->data[0] + (pPicture->iWidth-1);
  last = black2;
  for (unsigned int x = (int)pPicture->iWidth*xspacing-1; x > pPicture->iWidth/2*xspacing; x -= xspacing)
  {
    int total = 0;
    for (unsigned int y = 0; y < pPicture->iHeight; y++)
      total += s[y * pPicture->iLineSize[0]];
    s--;

    if (total > detect)
    {
      if (total - black2 > (last - black2) * multi)
        crop.right = pPicture->iWidth - (x / xspacing);
      break;
    }
    last = total;
  }

  // We always crop equally on each side to get zoom
  // effect intead of moving the image. Aslong as the
  // max crop isn't much larger than the min crop
  // use that.
  int min, max;

  min = std::min(crop.left, crop.right);
  max = std::max(crop.left, crop.right);
  if(10 * (max - min) / pPicture->iWidth < 1)
    crop.left = crop.right = max;
  else
    crop.left = crop.right = min;

  min = std::min(crop.top, crop.bottom);
  max = std::max(crop.top, crop.bottom);
  if(10 * (max - min) / pPicture->iHeight < 1)
    crop.top = crop.bottom = max;
  else
    crop.top = crop.bottom = min;
}

std::string CDVDPlayerVideo::GetPlayerInfo()
{
  std::ostringstream s;
  s << "fr:"     << fixed << setprecision(3) << m_fFrameRate;
  s << ", vq:"   << setw(2) << min(99,GetLevel()) << "%";
  s << ", dc:"   << m_codecname;
  s << ", Mb/s:" << fixed << setprecision(2) << (double)GetVideoBitrate() / (1024.0*1024.0);
  s << ", drop:" << m_iDroppedFrames;
  s << ", skip:" << g_renderManager.GetSkippedFrames();

  int pc = m_pullupCorrection.GetPatternLength();
  if (pc > 0)
    s << ", pc:" << pc;
  else
    s << ", pc:none";

  return s.str();
}

int CDVDPlayerVideo::GetVideoBitrate()
{
  return (int)m_videoStats.GetBitrate();
}

void CDVDPlayerVideo::ResetFrameRateCalc()
{
  m_fStableFrameRate = 0.0;
  m_iFrameRateCount  = 0;
  m_iFrameRateLength = 1;
  m_iFrameRateErr    = 0;

  m_bAllowDrop       = (!m_bCalcFrameRate && CMediaSettings::Get().GetCurrentVideoSettings().m_ScalingMethod != VS_SCALINGMETHOD_AUTO) ||
                        g_advancedSettings.m_videoFpsDetect == 0;
}

#define MAXFRAMERATEDIFF   0.01
#define MAXFRAMESERR    1000

void CDVDPlayerVideo::CalcFrameRate()
{
  if (m_iFrameRateLength >= 128 || g_advancedSettings.m_videoFpsDetect == 0)
    return; //don't calculate the fps

  //only calculate the framerate if sync playback to display is on, adjust refreshrate is on,
  //or scaling method is set to auto
  if (!m_bCalcFrameRate && CMediaSettings::Get().GetCurrentVideoSettings().m_ScalingMethod != VS_SCALINGMETHOD_AUTO)
  {
    ResetFrameRateCalc();
    return;
  }

  if (!m_pullupCorrection.HasFullBuffer())
    return; //we can only calculate the frameduration if m_pullupCorrection has a full buffer

  //see if m_pullupCorrection was able to detect a pattern in the timestamps
  //and is able to calculate the correct frame duration from it
  double frameduration = m_pullupCorrection.GetFrameDuration();

  if (frameduration == DVD_NOPTS_VALUE ||
      (g_advancedSettings.m_videoFpsDetect == 1 && m_pullupCorrection.GetPatternLength() > 1))
  {
    //reset the stored framerates if no good framerate was detected
    m_fStableFrameRate = 0.0;
    m_iFrameRateCount = 0;
    m_iFrameRateErr++;

    if (m_iFrameRateErr == MAXFRAMESERR && m_iFrameRateLength == 1)
    {
      CLog::Log(LOGDEBUG,"%s counted %i frames without being able to calculate the framerate, giving up", __FUNCTION__, m_iFrameRateErr);
      m_bAllowDrop = true;
      m_iFrameRateLength = 128;
    }
    return;
  }

  double framerate = DVD_TIME_BASE / frameduration;

  //store the current calculated framerate if we don't have any yet
  if (m_iFrameRateCount == 0)
  {
    m_fStableFrameRate = framerate;
    m_iFrameRateCount++;
  }
  //check if the current detected framerate matches with the stored ones
  else if (fabs(m_fStableFrameRate / m_iFrameRateCount - framerate) <= MAXFRAMERATEDIFF)
  {
    m_fStableFrameRate += framerate; //store the calculated framerate
    m_iFrameRateCount++;

    //if we've measured m_iFrameRateLength seconds of framerates,
    if (m_iFrameRateCount >= MathUtils::round_int(framerate) * m_iFrameRateLength)
    {
      //store the calculated framerate if it differs too much from m_fFrameRate
      if (fabs(m_fFrameRate - (m_fStableFrameRate / m_iFrameRateCount)) > MAXFRAMERATEDIFF || m_bFpsInvalid)
      {
        CLog::Log(LOGDEBUG,"%s framerate was:%f calculated:%f", __FUNCTION__, m_fFrameRate, m_fStableFrameRate / m_iFrameRateCount);
        m_fFrameRate = m_fStableFrameRate / m_iFrameRateCount;
        m_bFpsInvalid = false;
      }

      //reset the stored framerates
      m_fStableFrameRate = 0.0;
      m_iFrameRateCount = 0;
      m_iFrameRateLength *= 2; //double the length we should measure framerates

      //we're allowed to drop frames because we calculated a good framerate
      m_bAllowDrop = true;
    }
  }
  else //the calculated framerate didn't match, reset the stored ones
  {
    m_fStableFrameRate = 0.0;
    m_iFrameRateCount = 0;
  }
}

int CDVDPlayerVideo::CalcDropRequirement(double pts)
{
  int result = 0;
  double iSleepTime;
  double iDecoderPts, iRenderPts;
  double iInterval;
  int    interlaced;
  double iGain;
  double iLateness;
  bool   bNewFrame;
  int    iSkippedDeint = 0;
  int    iBufferLevel;

  // get decoder stats
  if (!m_pVideoCodec->GetPts(iDecoderPts, iSkippedDeint, interlaced))
    iDecoderPts = pts;
  if (iDecoderPts == DVD_NOPTS_VALUE)
    iDecoderPts = pts;

  // get render stats
  g_renderManager.GetStats(iSleepTime, iRenderPts, iBufferLevel);

  if (iBufferLevel < 0)
    result |= EOS_BUFFER_LEVEL;
  else if (iBufferLevel < 2)
  {
    result |= EOS_BUFFER_LEVEL;
    CLog::Log(LOGDEBUG,"CDVDPlayerVideo::CalcDropRequirement - hurry: %d", iBufferLevel);
  }

  bNewFrame = iDecoderPts != m_droppingStats.m_lastDecoderPts;

  if (interlaced)
    iInterval = 2/m_fFrameRate*(double)DVD_TIME_BASE;
  else
    iInterval = 1/m_fFrameRate*(double)DVD_TIME_BASE;

  if (m_droppingStats.m_lastDecoderPts > 0
      && bNewFrame
      && m_bAllowDrop
      && m_droppingStats.m_dropRequests > 0)
  {
    iGain = (iDecoderPts - m_droppingStats.m_lastDecoderPts - iInterval)/(double)DVD_TIME_BASE;
    if (iSkippedDeint)
    {
      CDroppingStats::CGain gain;
      gain.gain = 1/m_fFrameRate;
      gain.pts = iDecoderPts;
      m_droppingStats.m_gain.push_back(gain);
      m_droppingStats.m_totalGain += gain.gain;
      result |= EOS_DROPPED;
      m_droppingStats.m_dropRequests = 0;
      CLog::Log(LOGDEBUG,"CDVDPlayerVideo::CalcDropRequirement - dropped de-interlacing cycle, Sleeptime: %f, Bufferlevel: %d", iSleepTime, iBufferLevel);
    }
    else if (iGain > 1/m_fFrameRate)
    {
      CDroppingStats::CGain gain;
      gain.gain = iGain;
      gain.pts = iDecoderPts;
      m_droppingStats.m_gain.push_back(gain);
      m_droppingStats.m_totalGain += iGain;
      result |= EOS_DROPPED;
      m_droppingStats.m_dropRequests = 0;
      CLog::Log(LOGDEBUG,"CDVDPlayerVideo::CalcDropRequirement - dropped in decoder, Sleeptime: %f, Bufferlevel: %d, Gain: %f", iSleepTime, iBufferLevel, iGain);
    }
  }
  m_droppingStats.m_lastDecoderPts = iDecoderPts;

  // subtract gains
  while (!m_droppingStats.m_gain.empty() &&
         iRenderPts >= m_droppingStats.m_gain.front().pts)
  {
    m_droppingStats.m_totalGain -= m_droppingStats.m_gain.front().gain;
    m_droppingStats.m_gain.pop_front();
  }

  // calculate lateness
  iLateness = iSleepTime + m_droppingStats.m_totalGain;
  if (iLateness < 0 && m_speed)
  {
    if (bNewFrame)
      m_droppingStats.m_lateFrames++;

    // if lateness is smaller than frametime, we observe this state
    // for 10 cycles
    if (m_droppingStats.m_lateFrames > 10 || iLateness < -2/m_fFrameRate)
    {
      // is frame allowed to skip
      if (m_iNrOfPicturesNotToSkip <= 0)
      {
        result |= EOS_VERYLATE;
        if (bNewFrame)
          m_droppingStats.m_dropRequests++;
      }
    }
  }
  else
  {
    m_droppingStats.m_dropRequests = 0;
    m_droppingStats.m_lateFrames = 0;
  }
  m_droppingStats.m_lastRenderPts = iRenderPts;
  return result;
}

void CDroppingStats::Reset()
{
  m_gain.clear();
  m_totalGain = 0;
  m_lastDecoderPts = 0;
  m_lastRenderPts = 0;
  m_lateFrames = 0;
  m_dropRequests = 0;
}

void CDroppingStats::AddOutputDropGain(double pts, double frametime)
{
  CDroppingStats::CGain gain;
  gain.gain = frametime;
  gain.pts = pts;
  m_gain.push_back(gain);
  m_totalGain += frametime;
}
