
#include "stdafx.h"
#include "DVDPlayer.h"
#include "DVDPlayerVideo.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDCodecs/DVDCodecUtils.h"
#include "DVDCodecs/Video/DVDVideoPPFFmpeg.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "../../Util.h"
#include "DVDOverlayRenderer.h"
#include "DVDPerformanceCounter.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/Overlay/DVDOverlayCodecCC.h"
#include <sstream>

CDVDPlayerVideo::CDVDPlayerVideo(CDVDClock* pClock, CDVDOverlayContainer* pOverlayContainer) 
: CThread()
{
  m_pClock = pClock;
  m_pOverlayContainer = pOverlayContainer;
  m_pTempOverlayPicture = NULL;
  m_pVideoCodec = NULL;
  m_pOverlayCodecCC = NULL;
  m_speed = DVD_PLAYSPEED_NORMAL;
  
  m_bRenderSubs = false;
  m_DetectedStill = false;
  m_iVideoDelay = 0;
  m_iSubtitleDelay = 0;
  m_fForcedAspectRatio = 0;
  m_iNrOfPicturesNotToSkip = 0;
  InitializeCriticalSection(&m_critCodecSection);
  m_messageQueue.SetMaxDataSize(20 * 256 * 1024); 
  g_dvdPerformanceCounter.EnableVideoQueue(&m_messageQueue);
  
  m_iCurrentPts = DVD_NOPTS_VALUE;
  m_iDroppedFrames = 0;
  m_bDropFrames = true;
  m_fFrameRate = 25;
  m_bAllowFullscreen = false;
}

CDVDPlayerVideo::~CDVDPlayerVideo()
{
  StopThread();
  g_dvdPerformanceCounter.DisableVideoQueue();
  DeleteCriticalSection(&m_critCodecSection);
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

  if (hint.fpsrate && hint.fpsscale)
  {
    m_fFrameRate = (float)hint.fpsrate / hint.fpsscale;
    m_autosync = 10;
  }
  else
  {
    m_fFrameRate = 25;
    m_autosync = 1; // avoid using frame time as we don't know it accurate
  }

  if( m_fFrameRate > 100 || m_fFrameRate < 5 )
  {
    CLog::Log(LOGERROR, "CDVDPlayerVideo::OpenStream - Invalid framerate %d, using forced 25fps and just trust timestamps", (int)m_fFrameRate);
    m_fFrameRate = 25;
    m_autosync = 1; // avoid using frame time as we don't know it accurate
  }

  // use aspect in stream if available
  m_fForcedAspectRatio = hint.aspect;

  // should alway's be NULL!!!!, it will probably crash anyway when deleting m_pVideoCodec here.
  if (m_pVideoCodec)
  {
    CLog::Log(LOGFATAL, "CDVDPlayerVideo::OpenStream() m_pVideoCodec != NULL");
    return false;
  }

  CLog::Log(LOGNOTICE, "Creating video codec with codec id: %i", hint.codec);
  m_pVideoCodec = CDVDFactoryCodec::CreateVideoCodec( hint );

  if( !m_pVideoCodec )
  {
    CLog::Log(LOGERROR, "Unsupported video codec");
    return false;
  }

  m_messageQueue.Init();

  CLog::Log(LOGNOTICE, "Creating video thread");
  Create();

  return true;
}

void CDVDPlayerVideo::CloseStream(bool bWaitForBuffers)
{
  m_messageQueue.Abort();

  // wait for decode_video thread to end
  CLog::Log(LOGNOTICE, "waiting for video thread to exit");

  StopThread(); // will set this->m_bStop to true  

  m_messageQueue.End();
  m_pOverlayContainer->Clear();

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
}

void CDVDPlayerVideo::OnStartup()
{
  CThread::SetName("CDVDPlayerVideo");
  m_iDroppedFrames = 0;
  
  m_iCurrentPts = DVD_NOPTS_VALUE;
  m_FlipTimeStamp = m_pClock->GetAbsoluteClock();
  m_DetectedStill = false;
  
  memset(&m_output, 0, sizeof(m_output));
#ifdef HAS_VIDEO_PLAYBACK
  g_renderManager.PreInit();
#endif
  g_dvdPerformanceCounter.EnableVideoDecodePerformance(ThreadHandle());
}

void CDVDPlayerVideo::Process()
{
  CLog::Log(LOGNOTICE, "running thread: video_thread");

  DVDVideoPicture picture;
  CDVDVideoPPFFmpeg mDeinterlace(CDVDVideoPPFFmpeg::ED_DEINT_FFMPEG);

  memset(&picture, 0, sizeof(DVDVideoPicture));
  
  double pts = 0;
  double frametime = (double)DVD_TIME_BASE / m_fFrameRate;

  int iDropped = 0; //frames dropped in a row
  bool bRequestDrop = false;  

  m_videoStats.Start();

  while (!m_bStop)
  {
    while (m_speed == DVD_PLAYSPEED_PAUSE && !m_messageQueue.RecievedAbortRequest() && m_iNrOfPicturesNotToSkip==0) Sleep(5);

    int iQueueTimeOut = (int)(m_DetectedStill ? frametime / 4 : frametime * 4) / 1000;
    
    CDVDMsg* pMsg;
    MsgQueueReturnCode ret = m_messageQueue.Get(&pMsg, iQueueTimeOut);
   
    if (MSGQ_IS_ERROR(ret) || ret == MSGQ_ABORT) 
    {
      CLog::Log(LOGERROR, "Got MSGQ_ABORT or MSGO_IS_ERROR return true");
      break;
    }
    else if (ret == MSGQ_TIMEOUT)
    {
      //Okey, start rendering at stream fps now instead, we are likely in a stillframe
      if( !m_DetectedStill )
      {
        CLog::Log(LOGINFO, "CDVDPlayerVideo - Stillframe detected, switching to forced %f fps", m_fFrameRate);
        m_DetectedStill = true;
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
      CLog::Log(LOGDEBUG, "CDVDPlayerVideo - CDVDMsg::GENERAL_SYNCHRONIZE");

      CDVDMsgGeneralSynchronize* pMsgGeneralSynchronize = (CDVDMsgGeneralSynchronize*)pMsg;
      pMsgGeneralSynchronize->Wait(&m_bStop, SYNCSOURCE_VIDEO);

      pMsgGeneralSynchronize->Release();

      /* we may be very much off correct pts here, but next picture may be a still*/
      /* make sure it isn't dropped */
      m_iNrOfPicturesNotToSkip = 5;
      continue;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESYNC))
    {      
      CDVDMsgGeneralResync* pMsgGeneralResync = (CDVDMsgGeneralResync*)pMsg;

      if(pMsgGeneralResync->m_timestamp != DVD_NOPTS_VALUE)
        pts = pMsgGeneralResync->m_timestamp;
            
      if(pMsgGeneralResync->m_clock)
      {
        double delay = m_FlipTimeStamp - m_pClock->GetAbsoluteClock();
        
        if( delay > frametime ) delay = frametime;
        else if( delay < 0 ) delay = 0;

        m_pClock->Discontinuity(CLOCK_DISC_NORMAL, pts, delay);
        CLog::Log(LOGDEBUG, "CDVDPlayerVideo:: Resync - clock:%f, delay:%f", pts, delay);
      }

      pMsgGeneralResync->Release();
      continue;
    }
    else if (pMsg->IsType(CDVDMsg::VIDEO_SET_ASPECT))
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerVideo - CDVDMsg::VIDEO_SET_ASPECT");
      m_fForcedAspectRatio = ((CDVDMsgVideoSetAspect*)pMsg)->GetAspect();
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH)) // private message sent by (CDVDPlayerVideo::Flush())
    {
      EnterCriticalSection(&m_critCodecSection);
      if(m_pVideoCodec)
        m_pVideoCodec->Reset();
      LeaveCriticalSection(&m_critCodecSection);
    }
    else if (pMsg->IsType(CDVDMsg::VIDEO_NOSKIP))
    {
      // libmpeg2 is also returning incomplete frames after a dvd cell change
      // so the first few pictures are not the correct ones to display in some cases
      // just display those together with the correct one.
      // (setting it to 2 will skip some menu stills, 5 is working ok for me).
      m_iNrOfPicturesNotToSkip = 5;
    }
    
    if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      CDVDMsgDemuxerPacket* pMsgDemuxerPacket = (CDVDMsgDemuxerPacket*)pMsg;
      CDVDDemux::DemuxPacket* pPacket = pMsgDemuxerPacket->GetPacket();

      if (m_DetectedStill)
      {
        CLog::Log(LOGINFO, "CDVDPlayerVideo - Stillframe left, switching to normal playback");      
        m_DetectedStill = false;

        //don't allow the first frames after a still to be dropped
        //sometimes we get multiple stills (long duration frames) after each other
        //in normal mpegs
        m_iNrOfPicturesNotToSkip = 5;
      }
      else if( iDropped*frametime > DVD_MSEC_TO_TIME(100) )
      { // if we dropped too many pictures in a row, insert a forced picture
        m_iNrOfPicturesNotToSkip++;
      }
      else if (m_speed < 0)
      { // playing backward, don't drop any pictures
        m_iNrOfPicturesNotToSkip = 5;
      }

      // if we have pictures that can't be skipped, never request drop
      if( m_iNrOfPicturesNotToSkip > 0 || !m_bDropFrames )
        bRequestDrop = false;

#ifdef PROFILE
      bRequestDrop = false;
#endif
      
      EnterCriticalSection(&m_critCodecSection);
      // tell codec if next frame should be dropped
      // problem here, if one packet contains more than one frame
      // both frames will be dropped in that case instead of just the first
      // decoder still needs to provide an empty image structure, with correct flags
      m_pVideoCodec->SetDropState(bRequestDrop);

      int iDecoderState = m_pVideoCodec->Decode(pPacket->pData, pPacket->iSize, pPacket->pts);
      m_videoStats.AddSampleBytes(pPacket->iSize);
      // assume decoder dropped a picture if it didn't give us any
      // picture from a demux packet, this should be reasonable
      // for libavformat as a demuxer as it normally packetizes
      // pictures when they come from demuxer
      if(bRequestDrop && iDecoderState & VC_BUFFER)
      {
        m_iDroppedFrames++;
        iDropped++;
      }

      // loop while no error
      while (!(iDecoderState & VC_ERROR))
      {
        // check for a new picture
        if (iDecoderState & VC_PICTURE)
        {

          // try to retrieve the picture (should never fail!), unless there is a demuxer bug ofcours
          memset(&picture, 0, sizeof(DVDVideoPicture));
          if (m_pVideoCodec->GetPicture(&picture))
          {
            picture.iGroupId = pPacket->iGroupId;
            
            if (picture.iDuration == 0)
              // should not use frametime here. cause framerate can change while playing video
              picture.iDuration = frametime;

            if (m_iNrOfPicturesNotToSkip > 0)
            {
              picture.iFlags |= DVP_FLAG_NOSKIP;
              m_iNrOfPicturesNotToSkip--;
            }
            
            //Deinterlace if codec said format was interlaced or if we have selected we want to deinterlace
            //this video
            EINTERLACEMETHOD mInt = g_stSettings.m_currentVideoSettings.m_InterlaceMethod;
            if( mInt == VS_INTERLACEMETHOD_DEINTERLACE )
            {
              mDeinterlace.Process(&picture);
              mDeinterlace.GetPicture(&picture);
            }
            else if( mInt == VS_INTERLACEMETHOD_RENDER_WEAVE || mInt == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED )
            { 
              /* if we are syncing frames, dvdplayer will be forced to play at a given framerate */
              /* unless we directly sync to the correct pts, we won't get a/v sync as video can never catch up */
              picture.iFlags |= DVP_FLAG_NOAUTOSYNC;
            }

            /* try to figure out a pts for this frame */
            if(picture.pts != DVD_NOPTS_VALUE)
              pts = picture.pts;
            else if(pPacket->dts != DVD_NOPTS_VALUE)
              pts = pPacket->dts;
            
            /* use forced aspect if any */
            if( m_fForcedAspectRatio != 0.0f )
              picture.iDisplayWidth = (int) (picture.iDisplayHeight * m_fForcedAspectRatio);

            int iResult;
            do 
            {
              try 
              {
                iResult = OutputPicture(&picture, pts);
              }
              catch (...)
              {
                CLog::Log(LOGERROR, "%s - Exception caught when outputing picture", __FUNCTION__);
                iResult = EOS_ABORT;
              }

              if (iResult == EOS_ABORT) break;

              // guess next frame pts. iDuration is always valid
              pts += picture.iDuration * m_speed / abs(m_speed);
            }
            while (picture.iRepeatPicture-- > 0);
            
            if( iResult & EOS_ABORT )
            {
              //if we break here and we directly try to decode again wihout 
              //flushing the video codec things break for some reason
              //i think the decoder (libmpeg2 atleast) still has a pointer
              //to the data, and when the packet is freed that will fail.
              iDecoderState = m_pVideoCodec->Decode(NULL, 0, DVD_NOPTS_VALUE);
              break;
            }
            
            if( iResult & EOS_DROPPED )
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
        iDecoderState = m_pVideoCodec->Decode(NULL, 0, DVD_NOPTS_VALUE);
      }

      // if decoder had an error, tell it to reset to avoid more problems
      if( iDecoderState & VC_ERROR ) 
        m_pVideoCodec->Reset();

      LeaveCriticalSection(&m_critCodecSection);
    }    
    
    // all data is used by the decoder, we can safely free it now
    pMsg->Release();
  }
}

void CDVDPlayerVideo::OnExit()
{
  g_dvdPerformanceCounter.DisableVideoDecodePerformance();
  
  CLog::Log(LOGNOTICE, "uninitting video device");
#ifdef HAS_VIDEO_PLAYBACK
  g_renderManager.UnInit();
#endif

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
  BYTE* data = pVideoUserData->data;
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
        int iState = 0;
        while (!(iState & OC_ERROR) && !(iState & OC_BUFFER))
        {
          iState = m_pOverlayCodecCC->Decode(data, size, pts);
          data = NULL;
          size = 0;
          
          if (iState & OC_OVERLAY)
          {
            CDVDOverlay* pOverlay = m_pOverlayCodecCC->GetOverlay();
            m_pOverlayContainer->Add(pOverlay);
          }
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
  m_speed = speed;
  if(m_speed == DVD_PLAYSPEED_PAUSE)
    m_iNrOfPicturesNotToSkip = 0;
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
  m_messageQueue.Put(new CDVDMsgGeneralFlush());  
}

void CDVDPlayerVideo::ProcessOverlays(DVDVideoPicture* pSource, YV12Image* pDest, double pts)
{
  // remove any overlays that are out of time
  m_pOverlayContainer->CleanUp(min(pts, pts - m_iSubtitleDelay));

  // rendering spu overlay types directly on video memory costs a lot of processing power.
  // thus we allocate a temp picture, copy the original to it (needed because the same picture can be used more than once).
  // then do all the rendering on that temp picture and finaly copy it to video memory.
  // In almost all cases this is 5 or more times faster!.
  bool bHasSpecialOverlay = m_pOverlayContainer->ContainsOverlayType(DVDOVERLAY_TYPE_SPU) 
                         || m_pOverlayContainer->ContainsOverlayType(DVDOVERLAY_TYPE_IMAGE);
  
  if (bHasSpecialOverlay)
  {
    if (m_pTempOverlayPicture && (m_pTempOverlayPicture->iWidth != pSource->iWidth || m_pTempOverlayPicture->iHeight != pSource->iHeight))
    {
      CDVDCodecUtils::FreePicture(m_pTempOverlayPicture);
      m_pTempOverlayPicture = NULL;
    }
    
    if (!m_pTempOverlayPicture) m_pTempOverlayPicture = CDVDCodecUtils::AllocatePicture(pSource->iWidth, pSource->iHeight);
  }

  if (bHasSpecialOverlay && m_pTempOverlayPicture) CDVDCodecUtils::CopyPicture(m_pTempOverlayPicture, pSource);
  else CDVDCodecUtils::CopyPictureToOverlay(pDest, pSource);
  
  m_pOverlayContainer->Lock();

  VecOverlays* pVecOverlays = m_pOverlayContainer->GetOverlays();
  VecOverlaysIter it = pVecOverlays->begin();
  
  //Check all overlays and render those that should be rendered, based on time and forced
  //Both forced and subs should check timeing, pts == 0 in the stillframe case
  while (it != pVecOverlays->end())
  {
    CDVDOverlay* pOverlay = *it++;
    if(!pOverlay->bForced && !m_bRenderSubs)
      continue;

    if(pOverlay->iGroupId != pSource->iGroupId)
      continue;

    double pts2 = pOverlay->bForced ? pts : pts - m_iSubtitleDelay;

    if(pOverlay->iPTSStartTime <= pts2 && (pOverlay->iPTSStopTime >= pts2 || pOverlay->iPTSStopTime == 0LL) || pts == 0)
    {
      if (bHasSpecialOverlay && m_pTempOverlayPicture) 
        CDVDOverlayRenderer::Render(m_pTempOverlayPicture, pOverlay);
      else 
        CDVDOverlayRenderer::Render(pDest, pOverlay);
    }
  }
  
  m_pOverlayContainer->Unlock();
  
  if (bHasSpecialOverlay && m_pTempOverlayPicture)
  {
    CDVDCodecUtils::CopyPictureToOverlay(pDest, m_pTempOverlayPicture);
  }
}

int CDVDPlayerVideo::OutputPicture(DVDVideoPicture* pPicture, double pts)
{
  /* check so that our format or aspect has changed. if it has, reconfigure renderer */
  if (m_output.width != pPicture->iWidth
   || m_output.height != pPicture->iHeight
   || m_output.dwidth != pPicture->iDisplayWidth
   || m_output.dheight != pPicture->iDisplayHeight
   || m_output.framerate != m_fFrameRate
   || ( m_output.color_matrix != pPicture->color_matrix && pPicture->color_matrix != 0 ) // don't reconfigure on unspecified
   || m_output.color_range != pPicture->color_range)
  {
    CLog::Log(LOGNOTICE, " fps: %f, pwidth: %i, pheight: %i, dwidth: %i, dheight: %i",
      m_fFrameRate, pPicture->iWidth, pPicture->iHeight, pPicture->iDisplayWidth, pPicture->iDisplayHeight);
    unsigned flags = 0;
    if(pPicture->color_range == 1)
      flags |= CONF_FLAGS_YUV_FULLRANGE;

    switch(pPicture->color_matrix)
    {
      case 7: // SMPTE 240M (1987)
        flags |= CONF_FLAGS_YUVCOEF_240M; 
        break;
      case 6: // SMPTE 170M
      case 5: // ITU-R BT.470-2
      case 4: // FCC
        flags |= CONF_FLAGS_YUVCOEF_BT601;
        break;
      case 3: // RESERVED
      case 2: // UNSPECIFIED
      case 1: // ITU-R Rec.709 (1990) -- BT.709
      default: 
        flags |= CONF_FLAGS_YUVCOEF_BT709;
    }

    if(m_bAllowFullscreen)
    {
      flags |= CONF_FLAGS_FULLSCREEN;
      m_bAllowFullscreen = false; // only allow on first configure
    }

#ifdef HAS_VIDEO_PLAYBACK
    CLog::Log(LOGDEBUG,"%s- change configuration. %dx%d. framerate: %4.2f",__FUNCTION__,pPicture->iWidth, pPicture->iHeight,m_fFrameRate);
    if(!g_renderManager.Configure(pPicture->iWidth, pPicture->iHeight, pPicture->iDisplayWidth, pPicture->iDisplayHeight, m_fFrameRate, flags))
    {
      CLog::Log(LOGERROR, "%s - failed to configure renderer", __FUNCTION__);
      return EOS_ABORT;
    }
#endif

    m_output.width = pPicture->iWidth;
    m_output.height = pPicture->iHeight;
    m_output.dwidth = pPicture->iDisplayWidth;
    m_output.dheight = pPicture->iDisplayHeight;
    m_output.framerate = m_fFrameRate;
    m_output.color_matrix = pPicture->color_matrix;
    m_output.color_range = pPicture->color_range;
  }
  
#ifdef HAS_VIDEO_PLAYBACK
  if (!g_renderManager.IsStarted()) {
    CLog::Log(LOGERROR, "%s - renderer not started", __FUNCTION__);
    return EOS_ABORT;
  }
#endif

  int result = 0;

  //User set delay
  pts += m_iVideoDelay;

  // calculate the time we need to delay this picture before displaying
  double iSleepTime, iClockSleep, iFrameSleep, iCurrentClock, iFrameDuration;
  
  iCurrentClock = m_pClock->GetAbsoluteClock(); // snapshot current clock  
  iClockSleep = pts - m_pClock->GetClock();  //sleep calculated by pts to clock comparison
  iFrameSleep = m_FlipTimeStamp - iCurrentClock; // sleep calculated by duration of frame
  iFrameDuration = pPicture->iDuration;

  // correct sleep times based on speed
  if(m_speed)
  {
    iClockSleep = iClockSleep * DVD_PLAYSPEED_NORMAL / m_speed;
    iFrameSleep = iFrameSleep * DVD_PLAYSPEED_NORMAL / abs(m_speed);
    iFrameDuration = iFrameDuration * DVD_PLAYSPEED_NORMAL / abs(m_speed);
  }

  // dropping to a very low framerate is not correct (it should not happen at all)
  iClockSleep = min(iClockSleep, DVD_MSEC_TO_TIME(500));
  iFrameSleep = min(iFrameSleep, DVD_MSEC_TO_TIME(500));

  if( m_DetectedStill )
  { // when we render a still, we can't sync to clock anyway
    iSleepTime = iFrameSleep;
  }
  else
  {
    // try to decide on how to sync framerate
    if( pPicture->iFlags & DVP_FLAG_NOAUTOSYNC )
      iSleepTime = iClockSleep;
    else
      iSleepTime = iFrameSleep + (iClockSleep - iFrameSleep) / m_autosync;
  }

#ifdef PROFILE /* during profiling, try to play as fast as possible */
  iSleepTime = 0;
#endif

  // present the current pts of this frame to user, and include the actual
  // presentation delay, to allow him to adjust for it
  if( !m_DetectedStill )
    m_iCurrentPts = pts - max(0.0, iSleepTime);

  // timestamp when we think next picture should be displayed based on current duration
  m_FlipTimeStamp  = iCurrentClock;
  m_FlipTimeStamp += max(0.0, iSleepTime);
  m_FlipTimeStamp += iFrameDuration;

  // ask decoder to drop frames next round, as we are very late
  if( iClockSleep < -DVD_MSEC_TO_TIME(100) )
    result |= EOS_VERYLATE;

  if( m_speed < 0 )
  {
    if( iClockSleep < -DVD_MSEC_TO_TIME(200) 
    && !(pPicture->iFlags & DVP_FLAG_NOSKIP) )
      return result | EOS_DROPPED;  
  }

  if( (pPicture->iFlags & DVP_FLAG_DROPPED) ) 
    return result | EOS_DROPPED;

#ifdef HAS_VIDEO_PLAYBACK

  float maxfps = g_renderManager.GetMaximumFPS();
  if( m_fFrameRate * abs(m_speed) / DVD_PLAYSPEED_NORMAL >  maxfps )
  {
    // calculate frame dropping pattern to render at this speed
    // we do that by deciding if this or next frame is closest
    // to the flip timestamp
    double current   = fabs(m_dropbase -  m_droptime);
    double next      = fabs(m_dropbase - (m_droptime + iFrameDuration));
    double frametime = (double)DVD_TIME_BASE / maxfps;

    m_droptime += iFrameDuration;
#ifndef PROFILE
    if( next < current && !(pPicture->iFlags & DVP_FLAG_NOSKIP) )
      return result | EOS_DROPPED;
#endif

    while(m_dropbase < m_droptime)             m_dropbase += frametime;
    while(m_dropbase - frametime > m_droptime) m_dropbase -= frametime;
  } 
  else
  {
    m_droptime = 0.0f;
    m_dropbase = 0.0f;
  }

  // set fieldsync if picture is interlaced
  EFIELDSYNC mDisplayField = FS_NONE;
  if( pPicture->iFlags & DVP_FLAG_INTERLACED )
  {
    if( pPicture->iFlags & DVP_FLAG_TOP_FIELD_FIRST )
      mDisplayField = FS_ODD;
    else
      mDisplayField = FS_EVEN;
  }

  // copy picture to overlay
  YV12Image image;

  int index = g_renderManager.GetImage(&image);
  if (index < 0) 
    return EOS_DROPPED;

  ProcessOverlays(pPicture, &image, pts);
  
  // tell the renderer that we've finished with the image (so it can do any
  // post processing before FlipPage() is called.)
  g_renderManager.ReleaseImage(index);

  // present this image after the given delay, but correct for copy time
  double delay = iCurrentClock + iSleepTime - m_pClock->GetAbsoluteClock();

  if(delay<0)
    g_renderManager.FlipPage( 0, -1, mDisplayField);
  else
    g_renderManager.FlipPage( (DWORD)(delay * 1000 / DVD_TIME_BASE), -1, mDisplayField);

#else
  // no video renderer, let's mark it as dropped
  result|= EOS_DROPPED;
#endif

  return result;

}

void CDVDPlayerVideo::UpdateMenuPicture()
{
  if (m_pVideoCodec)
  {
    DVDVideoPicture picture;
    EnterCriticalSection(&m_critCodecSection);
    if (m_pVideoCodec->GetPicture(&picture))
    {
      picture.iFlags |= DVP_FLAG_NOSKIP;
      OutputPicture(&picture, 0);
    }
    LeaveCriticalSection(&m_critCodecSection);
  }
}

std::string CDVDPlayerVideo::GetPlayerInfo()
{
  std::ostringstream s;
  s << "vq size:" << min(100,100 * m_messageQueue.GetDataSize() / m_messageQueue.GetMaxDataSize()) << "%";
  s << ", ";
  s << "cpu: " << (int)(100 * CThread::GetRelativeUsage()) << "%, ";
  s << "bitrate: " << (GetVideoBitrate() / 8) / 1024 << " KBytes/s";
  return s.str();
}

int CDVDPlayerVideo::GetVideoBitrate()
{
  return (int)m_videoStats.GetBitrate();
}

