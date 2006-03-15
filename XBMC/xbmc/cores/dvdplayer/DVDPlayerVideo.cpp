
#include "../../stdafx.h"
#include "DVDPlayer.h"
#include "DVDPlayerVideo.h"
#include "DVDCodecs\DVDFactoryCodec.h"
#include "DVDCodecs\DVDCodecUtils.h"
#include "DVDCodecs\Video\DVDVideoPPFFmpeg.h"
#include "DVDDemuxers\DVDDemux.h"
#include "DVDDemuxers\DVDDemuxUtils.h"
#include "..\..\util.h"
#include "DVDOverlayRenderer.h"
#include "DVDPerformanceCounter.h"

#include "DVDCodecs\DVDCodecs.h"

static __forceinline __int64 abs(__int64 value)
{
  if( value < 0 )
    return -value;
  else
    return value;
}


CDVDPlayerVideo::CDVDPlayerVideo(CDVDClock* pClock, CDVDOverlayContainer* pOverlayContainer) 
: CThread()
, m_PresentThread( pClock )
{
  m_pClock = pClock;
  m_pOverlayContainer = pOverlayContainer;
  m_pTempOverlayPicture = NULL;
  m_pVideoCodec = NULL;
  m_bInitializedOutputDevice = false;
  
  SetSpeed(DVD_PLAYSPEED_NORMAL);
  m_bRenderSubs = false;
  m_iVideoDelay = 0;
  m_fForcedAspectRatio = 0;
  m_iNrOfPicturesNotToSkip = 0;
  InitializeCriticalSection(&m_critCodecSection);
  m_messageQueue.SetMaxDataSize(5 * 256 * 1024); // 1310720
  g_dvdPerformanceCounter.EnableVideoQueue(&m_messageQueue);
  
  m_iCurrentPts = DVD_NOPTS_VALUE;
  
  m_iDroppedFrames = 0;
  m_fFrameRate = 25;
}

CDVDPlayerVideo::~CDVDPlayerVideo()
{
  g_dvdPerformanceCounter.DisableVideoQueue();
  DeleteCriticalSection(&m_critCodecSection);
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

  if( m_fFrameRate > 100 )
  {
    CLog::Log(LOGERROR, "CDVDPlayerVideo::OpenStream - Invalid framerate %d, using forced 25fps", (int)m_fFrameRate);
    m_fFrameRate = 25;
    m_autosync = 1; // avoid using frame time as we don't know it accurate
  }



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
  m_iFlipTimeStamp = m_pClock->GetAbsoluteClock();
  m_DetectedStill = false;
  
  g_dvdPerformanceCounter.EnableVideoDecodePerformance(ThreadHandle());
}

void CDVDPlayerVideo::Process()
{
  CLog::Log(LOGNOTICE, "running thread: video_thread");

  DVDVideoPicture picture;
  CDVDVideoPPFFmpeg mDeinterlace(CDVDVideoPPFFmpeg::ED_DEINT_FFMPEG);

  memset(&picture, 0, sizeof(DVDVideoPicture));
  
  __int64 pts = 0;

  unsigned int iFrameTime = (unsigned int)(DVD_TIME_BASE / m_fFrameRate) ;  

  int iDropped = 0; //frames dropped in a row
  bool bRequestDrop = false;  

  while (!m_bStop)
  {
    while (m_speed == DVD_PLAYSPEED_PAUSE && !m_messageQueue.RecievedAbortRequest()) Sleep(5);

    int iQueueTimeOut = (m_DetectedStill ? iFrameTime / 4 : iFrameTime * 4) / 1000;
    
    CDVDMsg* pMsg;
    MsgQueueReturnCode ret = m_messageQueue.Get(&pMsg, iQueueTimeOut);
   
    if (MSGQ_IS_ERROR(ret) || ret == MSGQ_ABORT) break;
    else if (ret == MSGQ_TIMEOUT)
    {
      //Okey, start rendering at stream fps now instead, we are likely in a stillframe
      if( !m_DetectedStill )
      {
        CLog::Log(LOGINFO, "CDVDPlayerVideo - Stillframe detected, switching to forced %d fps", (DVD_TIME_BASE / iFrameTime));
        m_DetectedStill = true;
        pts = m_pClock->GetClock();
      }

      //Waiting timed out, output last picture
      if( picture.iFlags & DVP_FLAG_ALLOCATED )
      {
        //Remove interlaced flag before outputting
        //no need to output this as if it was interlaced
        picture.iFlags &= ~DVP_FLAG_INTERLACED;
        picture.iFlags |= DVP_FLAG_NOSKIP;
        OutputPicture(&picture, pts);
        pts+= iFrameTime;
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
    else if (pMsg->IsType(CDVDMsg::GENERAL_SET_CLOCK))
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerVideo::Process - Resync recieved.");
      
      CDVDMsgGeneralSetClock* pMsgGeneralSetClock = (CDVDMsgGeneralSetClock*)pMsg;

      //DVDPlayer asked us to sync playback clock
      if( pMsgGeneralSetClock->GetDts() != DVD_NOPTS_VALUE )
        pts = pMsgGeneralSetClock->GetDts();

      __int64 delay = m_iFlipTimeStamp - m_pClock->GetAbsoluteClock();
      
      if( delay > iFrameTime ) delay = iFrameTime;
      else if( delay < 0 ) delay = 0;

      m_pClock->Discontinuity(CLOCK_DISC_NORMAL, pts, delay);
      CLog::Log(LOGDEBUG, "CDVDPlayerVideo:: Resync - clock:%I64d, delay:%I64d", pts, delay);      

      pMsgGeneralSetClock->Release();
      continue;
    } 
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESYNC))
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerVideo - CDVDMsg::GENERAL_RESYNC"); 
      
      CDVDMsgGeneralResync* pMsgGeneralResync = (CDVDMsgGeneralResync*)pMsg;
      
      //DVDPlayer asked us to sync playback clock
      if( pMsgGeneralResync->GetDts() != DVD_NOPTS_VALUE )
        pts = pMsgGeneralResync->GetDts();

      pMsgGeneralResync->Release();
      continue;
    }

    if (m_DetectedStill)
    {
      CLog::Log(LOGINFO, "CDVDPlayerVideo - Stillframe left, switching to normal playback");      
      m_DetectedStill = false;

      //don't allow the first frames after a still to be dropped
      //sometimes we get multiple stills (long duration frames) after each other
      //in normal mpegs
      m_iNrOfPicturesNotToSkip = 5; 
    }

    EnterCriticalSection(&m_critCodecSection);

    if (pMsg->IsType(CDVDMsg::VIDEO_NOSKIP))
    {
      // libmpeg2 is also returning incomplete frames after a dvd cell change
      // so the first few pictures are not the correct ones to display in some cases
      // just display those together with the correct one.
      // (setting it to 2 will skip some menu stills, 5 is working ok for me).
      m_iNrOfPicturesNotToSkip = 5;
    }
    
    if( iDropped > 30 )
    { // if we dropped too many pictures in a row, insert a forced picure
      m_iNrOfPicturesNotToSkip++;
      iDropped = 0;
    }

    if (m_speed < 0)
    { // playing backward, don't drop any pictures
      m_iNrOfPicturesNotToSkip = 5;
    }

    // if we have pictures that can't be skipped, never request drop
    if( m_iNrOfPicturesNotToSkip > 0 ) bRequestDrop = false;

    // tell codec if next frame should be dropped
    // problem here, if one packet contains more than one frame
    // both frames will be dropped in that case instead of just the first
    // decoder still needs to provide an empty image structure, with correct flags
    m_pVideoCodec->SetDropState(bRequestDrop);
    
    if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH)) // private mesasage sent by (CDVDPlayerVideo::Flush())
    {
      if (m_pVideoCodec)
      {
        m_pVideoCodec->Reset();
      }
    }
    else if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      CDVDMsgDemuxerPacket* pMsgDemuxerPacket = (CDVDMsgDemuxerPacket*)pMsg;
      CDVDDemux::DemuxPacket* pPacket = pMsgDemuxerPacket->GetPacket();
      
      int iDecoderState = m_pVideoCodec->Decode(pPacket->pData, pPacket->iSize);
      
      // loop while no error
      while (!(iDecoderState & VC_ERROR))
      {
        // check for a new picture
        if (iDecoderState & VC_PICTURE)
        {

          // try to retrieve the picture (should never fail!), unless there is a demuxer bug ofcours
          fast_memset(&picture, 0, sizeof(DVDVideoPicture));
          if (m_pVideoCodec->GetPicture(&picture))
          {          
            if (picture.iDuration == 0)
              // should not use iFrameTime here. cause framerate can change while playing video
              picture.iDuration = iFrameTime;

            if (m_iNrOfPicturesNotToSkip > 0)
            {
              picture.iFlags |= DVP_FLAG_NOSKIP;
              m_iNrOfPicturesNotToSkip--;
            }
            
            //Deinterlace if codec said format was interlaced or if we have selected we want to deinterlace
            //this video
            EINTERLACEMETHOD mInt = g_stSettings.m_currentVideoSettings.GetInterlaceMethod();
            if( mInt == VS_INTERLACEMETHOD_DEINTERLACE || (mInt == VS_INTERLACEMETHOD_DEINTERLACE_AUTO && (picture.iFlags & DVP_FLAG_INTERLACED)) )
            {
              mDeinterlace.Process(&picture);
              mDeinterlace.GetPicture(&picture);
            }
            

            if ((picture.iFrameType == FRAME_TYPE_I || picture.iFrameType == FRAME_TYPE_UNDEF) &&
                pPacket->dts != DVD_NOPTS_VALUE) //Only use pts when we have an I frame, or unknown
            {
              pts = pPacket->dts;
            }
            
            //Check if dvd has forced an aspect ratio
            if( m_fForcedAspectRatio != 0.0f )
            {
              picture.iDisplayWidth = (int) (picture.iDisplayHeight * m_fForcedAspectRatio);
            }

            EOUTPUTSTATUS iResult;
            do 
            {
              try 
              {
                iResult = OutputPicture(&picture, pts);
              }
              catch (...)
              {
                CLog::Log(LOGERROR, __FUNCTION__" - Exception caught when outputing picture");
                iResult = EOS_ABORT;
              }

              if (iResult == EOS_ABORT) break;

              // guess next frame pts. iDuration is always valid
              pts += picture.iDuration;
            }
            while (picture.iRepeatPicture-- > 0);

            bRequestDrop = false;
            if( iResult == EOS_ABORT )
            {
              //if we break here and we directly try to decode again wihout 
              //flushing the video codec things break for some reason
              //i think the decoder (libmpeg2 atleast) still has a pointer
              //to the data, and when the packet is freed that will fail.
              iDecoderState = m_pVideoCodec->Decode(NULL, NULL);
              break;
            }
            else if( iResult == EOS_DROPPED )
            {
              m_iDroppedFrames++;
              iDropped++;
            }
            else if( iResult == EOS_DROPPED_VERYLATE )
            {
              m_iDroppedFrames++;
              iDropped++;
              bRequestDrop = true;
            }
          }
          else
          {
            CLog::Log(LOGWARNING, "Decoder Error getting videoPicture.");
            m_pVideoCodec->Reset();
          }
        }

        // if the decoder needs more data, we just break this loop
        // and try to get more data from the videoQueue
        if (iDecoderState & VC_BUFFER) break;
        try
        {
          // the decoder didn't need more data, flush the remaning buffer
          iDecoderState = m_pVideoCodec->Decode(NULL, NULL);
        }
        catch(...)
        {
          CLog::Log(LOGERROR, __FUNCTION__" - Exception caught when decoding data");
          iDecoderState = VC_ERROR;
        }
      }

      // if decoder had an error, tell it to reset to avoid more problems
      if( iDecoderState & VC_ERROR ) m_pVideoCodec->Reset();
    }
    
    LeaveCriticalSection(&m_critCodecSection);
    
    // all data is used by the decoder, we can safely free it now
    pMsg->Release();
  }

  CLog::Log(LOGNOTICE, "thread end: video_thread");

  CLog::Log(LOGNOTICE, "uninitting video device");
}

void CDVDPlayerVideo::OnExit()
{
  g_dvdPerformanceCounter.DisableVideoDecodePerformance();
  
  g_renderManager.UnInit();
  m_bInitializedOutputDevice = false;

  CLog::Log(LOGNOTICE, "thread end: video_thread");
}

bool CDVDPlayerVideo::InitializedOutputDevice()
{
  return m_bInitializedOutputDevice;
}

void CDVDPlayerVideo::SetSpeed(int speed)
{
  m_speed = speed;
}

void CDVDPlayerVideo::Flush()
{ 
  m_messageQueue.Flush();
  //m_packetQueue.Put(NULL, (void*)DVDPACKET_MESSAGE_FLUSH);

  m_iCurrentPts = DVD_NOPTS_VALUE;
}

void CDVDPlayerVideo::ProcessOverlays(DVDVideoPicture* pSource, YV12Image* pDest, __int64 pts)
{
  // remove any overlays that are out of time
  m_pOverlayContainer->CleanUp(pts);

  // rendering spu overlay types directly on video memory costs a lot of processing power.
  // thus we allocate a temp picture, copy the original to it (needed because the same picture can be used more than once).
  // then do all the rendering on that temp picture and finaly copy it to video memory.
  // In almost all cases this is 5 or more times faster!.
  bool bHasSpecialOverlay = m_pOverlayContainer->ContainsOverlayType(DVDOVERLAY_TYPE_SPU);
  
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
    CDVDOverlay* pOverlay = *it;
    if ((pOverlay->bForced || m_bRenderSubs) &&
        ((pOverlay->iPTSStartTime <= pts && (pOverlay->iPTSStopTime >= pts || pOverlay->iPTSStopTime == 0LL)) || pts == 0))
    {
      if (bHasSpecialOverlay && m_pTempOverlayPicture) CDVDOverlayRenderer::Render(m_pTempOverlayPicture, pOverlay);
      else CDVDOverlayRenderer::Render(pDest, pOverlay);
    }
    it++;
  }
  
  m_pOverlayContainer->Unlock();
  
  if (bHasSpecialOverlay && m_pTempOverlayPicture)
  {
    CDVDCodecUtils::CopyPictureToOverlay(pDest, m_pTempOverlayPicture);
  }
}

CDVDPlayerVideo::EOUTPUTSTATUS CDVDPlayerVideo::OutputPicture(DVDVideoPicture* pPicture, __int64 pts)
{
  if (m_messageQueue.RecievedAbortRequest()) return EOS_ABORT;

  if (!m_bInitializedOutputDevice)
  {

    CLog::Log(LOGNOTICE, "Initializing video device");

    g_renderManager.PreInit();

    CLog::Log(LOGNOTICE, " fps: %f, pwidth: %i, pheight: %i, dwidth: %i, dheight: %i",
      m_fFrameRate, pPicture->iWidth, pPicture->iHeight, pPicture->iDisplayWidth, pPicture->iDisplayHeight);

    g_renderManager.Configure(pPicture->iWidth, pPicture->iHeight, pPicture->iDisplayWidth, pPicture->iDisplayHeight, m_fFrameRate);
    m_bInitializedOutputDevice = true;
  }

  if (m_bInitializedOutputDevice)
  {    
    if( !(pPicture->iFlags & DVP_FLAG_DROPPED) )
    {
      // copy picture to overlay
      YV12Image image;
      if( !g_renderManager.GetImage(&image) ) return EOS_DROPPED;

      ProcessOverlays(pPicture, &image, pts);
      
      // tell the renderer that we've finished with the image (so it can do any
      // post processing before FlipPage() is called.)
      g_renderManager.ReleaseImage();

    }

    // calculate the time we need to delay this picture before displaying
    __int64 iSleepTime, iClockSleep, iFrameSleep;

    // sleep calculated by duration of frame
    iFrameSleep = m_iFlipTimeStamp - m_pClock->GetAbsoluteClock();
    // negative frame sleep possible after a stillframe, don't allow
    if( iFrameSleep < 0 ) iFrameSleep = 0;

    //sleep calculated by pts to clock comparison
    iClockSleep = pts - m_pClock->GetClock();
    //User set delay
    iClockSleep += m_iVideoDelay;

    if (m_speed < 0)
    {
      // don't sleep when going backwords, just push frames out
      iSleepTime = 0;
    }
    else
    {
      /* try to decide on how to sync framerate */
      /* during a discontinuity, we have have to rely on frame duration completly*/
      /* otherwise we adjust against the playback clock to some extent */

      /* decouple clock and video as we approach a discontinuity */
      /* decouple clock and video a while after a discontinuity */
      const __int64 distance = m_pClock->DistanceToDisc();

      if(  abs(distance) < pPicture->iDuration*3 )
        iSleepTime = iFrameSleep + (iClockSleep - iFrameSleep) * 0;
      else
        iSleepTime = iFrameSleep + (iClockSleep - iFrameSleep) / m_autosync;
    }

    // dropping to a very low framerate is not correct (it should not happen at all)
    // clock and audio could be adjusted
    if (iSleepTime > DVD_MSEC_TO_TIME(500) ) iSleepTime = DVD_MSEC_TO_TIME(500); // drop to a minimum of 2 frames/sec

    // current pts is always equal to the pts of a picture, at least it is for external subtitles
    m_iCurrentPts = pts - (iSleepTime > 0 ? iSleepTime : 0);

    /* adjust for speed */
    if( m_speed > DVD_PLAYSPEED_NORMAL )
      iSleepTime = iSleepTime * DVD_PLAYSPEED_NORMAL / m_speed;

    if( iSleepTime < 0 )
    { // we are late, try to figure out how late
      // this could be improved if we had some average time
      // for decoding. so we know if we need to drop frames
      // in decoder
      if( !(pPicture->iFlags & DVP_FLAG_NOSKIP) )
      {

        iSleepTime*= -1;
        if( iSleepTime > 4*pPicture->iDuration )
        { // two frames late, signal that we are late. this will drop frames in decoder, untill we have an ok frame
          pPicture->iFlags |= DVP_FLAG_DROPPED;
          return EOS_DROPPED_VERYLATE;
        }
        else if( iSleepTime > 2*pPicture->iDuration )
        { // one frame late, drop in renderer     
          pPicture->iFlags |= DVP_FLAG_DROPPED;
          return EOS_DROPPED;
        }

      }
      iSleepTime = 0;
    }

    // timestamp when we think next picture should be displayed based on current duration
    m_iFlipTimeStamp = m_pClock->GetAbsoluteClock() ;
    m_iFlipTimeStamp += iSleepTime ;
    m_iFlipTimeStamp += pPicture->iDuration;

    if( (pPicture->iFlags & DVP_FLAG_DROPPED) ) return EOS_DROPPED;

    // set fieldsync if picture is interlaced
    EFIELDSYNC mDisplayField = FS_NONE;
    if( pPicture->iFlags & DVP_FLAG_INTERLACED )
    {
      if( pPicture->iFlags & DVP_FLAG_TOP_FIELD_FIRST )
        mDisplayField = FS_ODD;
      else
        mDisplayField = FS_EVEN;
    }

    // account delay caused by waiting for vsync.     
    iSleepTime -= m_PresentThread.GetDelay();

    // present this image after the given delay
    m_PresentThread.Present( m_pClock->GetAbsoluteClock() + iSleepTime, mDisplayField );

  }
  return EOS_OK;
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

__int64 CDVDPlayerVideo::GetDelay()
{
  return m_iVideoDelay;
}

void CDVDPlayerVideo::SetDelay(__int64 delay)
{
  m_iVideoDelay = delay;
}

void CDVDPlayerVideo::SetAspectRatio(float fAspectRatio)
{
  m_fForcedAspectRatio = fAspectRatio;
}

void CDVDPlayerVideo::CPresentThread::Present( __int64 iTimeStamp, EFIELDSYNC m_OnField )
{
  CSingleLock lock(m_critSection);

  m_iTimestamp = iTimeStamp;

  // set the field we wish to display on
  g_renderManager.SetFieldSync(m_OnField);

  // prepare for display
  g_renderManager.PrepareDisplay();

  // start waiting thread
  m_eventFrame.Set();
}

void CDVDPlayerVideo::CPresentThread::Process()
{
  CLog::Log(LOGDEBUG, "CPresentThread - Starting()");

  /* guess delay to be about 1 frame at 50fps */
  m_iDelay = DVD_MSEC_TO_TIME(20);

  __int64 iFlipStamp;

  while( !CThread::m_bStop )
  {
    m_eventFrame.Wait();

    { CSingleLock lock(m_critSection);

      __int64 mTime;

      while(1)
      {
        if( CThread::m_bStop ) return;

        mTime = ( m_iTimestamp - m_pClock->GetAbsoluteClock() ) / (DVD_TIME_BASE / 1000000);

        if( mTime > DVD_MSEC_TO_TIME(500) )
        {          
          usleep( DVD_MSEC_TO_TIME(500) );
          
          CLog::Log(LOGERROR, "CPresentThread - Too long sleeptime %I64d", mTime);
          break; // break here since sometimes mTime is completly invalid, shouldn't need any longer than 500msec sleep anyway
          //continue;
        }
        
        if( mTime > 0 )
          usleep( (int)( mTime ) );        

        break;
      }

      iFlipStamp = m_pClock->GetAbsoluteClock();

      //Time to display
      g_renderManager.FlipPage();

      /* calculate m_iDelay. m_iDelay will converge towards the correct value */
      /* timeconstant of about 9 frames */
      m_iDelay = (m_iDelay + 9*(m_pClock->GetAbsoluteClock() - iFlipStamp))/10;
      if( m_iDelay < 0 ) m_iDelay = 0; // fix for buggy clock
    }
  }

  CLog::Log(LOGDEBUG, "CPresentThread - Stopping()");
}


