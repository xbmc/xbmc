
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

CDVDPlayerVideo::CDVDPlayerVideo(CDVDDemuxSPU* spu, CDVDClock* pClock, CDVDOverlayContainer* pOverlayContainer) 
: CThread()
, m_PresentThread( pClock )
{
  m_pDVDSpu = spu;
  m_pClock = pClock;
  m_pOverlayContainer = pOverlayContainer;
  
  m_pVideoCodec = NULL;
  m_bInitializedOutputDevice = false;
  
  m_iSpeed = 1;
  m_bRenderSubs = false;
  m_iVideoDelay = 0;
  m_fForcedAspectRatio = 0;
  m_iNrOfPicturesNotToSkip = 0;
  InitializeCriticalSection(&m_critCodecSection);
  m_packetQueue.SetMaxSize(5 * 256 * 1024); // 1310720
  g_dvdPerformanceCounter.EnableVideoQueue(&m_packetQueue);
  
  m_iCurrentPts = DVD_NOPTS_VALUE;
  
  m_iDroppedFrames = 0;
  m_fFrameRate = 25;
}

CDVDPlayerVideo::~CDVDPlayerVideo()
{
  g_dvdPerformanceCounter.DisableVideoQueue();
  DeleteCriticalSection(&m_critCodecSection);
}

bool CDVDPlayerVideo::OpenStream(CDemuxStreamVideo* pDemuxStreamVideo)
{  

  if (pDemuxStreamVideo->iFpsRate && pDemuxStreamVideo->iFpsScale)
    m_fFrameRate = (float)pDemuxStreamVideo->iFpsRate / pDemuxStreamVideo->iFpsScale;
  else
    m_fFrameRate = 25;


  // should alway's be NULL!!!!, it will probably crash anyway when deleting m_pVideoCodec here.
  if (m_pVideoCodec)
  {
    CLog::Log(LOGFATAL, "CDVDPlayerVideo::OpenStream() m_pVideoCodec != NULL");
    return false;
  }

  CLog::Log(LOGNOTICE, "Creating video codec with codec id: %i", pDemuxStreamVideo->codec);
  m_pVideoCodec = CDVDFactoryCodec::CreateVideoCodec( pDemuxStreamVideo );

  if( !m_pVideoCodec )
  {
    CLog::Log(LOGERROR, "Unsupported video codec");
    return false;
  }

  m_packetQueue.Init();

  CLog::Log(LOGNOTICE, "Creating video thread");
  Create();

  return true;
}

void CDVDPlayerVideo::CloseStream(bool bWaitForBuffers)
{
  m_packetQueue.Abort();

  // wait for decode_video thread to end
  CLog::Log(LOGNOTICE, "waiting for video thread to exit");

  StopThread(); // will set this->m_bStop to true  

  m_packetQueue.End();
  m_pOverlayContainer->Clear();

  CLog::Log(LOGNOTICE, "deleting video codec");
  if (m_pVideoCodec)
  {
    m_pVideoCodec->Dispose();
    delete m_pVideoCodec;
    m_pVideoCodec = NULL;
  }  
}

void CDVDPlayerVideo::OnStartup()
{
  CThread::SetName("CDVDPlayerVideo");
  m_iDroppedFrames = 0;
  
  m_iCurrentPts = DVD_NOPTS_VALUE;
  
  g_dvdPerformanceCounter.EnableVideoDecodePerformance(ThreadHandle());
}

void CDVDPlayerVideo::Process()
{
  CLog::Log(LOGNOTICE, "running thread: video_thread");

  CDVDDemux::DemuxPacket* pPacket;
  DVDVideoPicture picture;
  CDVDVideoPPFFmpeg mDeinterlace(CDVDVideoPPFFmpeg::ED_DEINT_FFMPEG);

  memset(&picture, 0, sizeof(DVDVideoPicture));
  
  int dvdstate;
  __int64 pts = 0;

  unsigned int iFrameTime = (unsigned int)(DVD_TIME_BASE / m_fFrameRate) ;  

  bool bDetectedStill = false;

  int iDropped = 0; //frames dropped in a row
  bool bRequestDrop = false;  

  while (!m_bStop)
  {
    while (m_iSpeed == 0 && !m_packetQueue.RecievedAbortRequest()) Sleep(5);


    int ret = m_packetQueue.Get(&pPacket, bDetectedStill ? iFrameTime : iFrameTime * 4, (void**)&dvdstate);
    if( ret < 0 ) break;
    else if( ret == 0 )
    {
      //Waiting timed out, output last picture
      if( picture.iFlags & DVP_FLAG_ALLOCATED )
      {
        //Remove interlaced flag before outputting
        //no need to output this as if it was interlaced
        picture.iFlags &= ~DVP_FLAG_INTERLACED;
        picture.iFlags |= DVP_FLAG_NOSKIP;
        OutputPicture(&picture, 0);
      }

      //Okey, start rendering at stream fps now instead, we are likely in a stillframe
      if( !bDetectedStill )
      {
        CLog::Log(LOGINFO, "CDVDPlayerVideo - Stillframe detected, switching to forced %d fps", ( 1000 / iFrameTime ));
        bDetectedStill = true;
      }
      continue;
    }

    if (bDetectedStill)
    {
      CLog::Log(LOGINFO, "CDVDPlayerVideo - Stillframe left, switching to normal playback");      
      bDetectedStill = false;
    }

    EnterCriticalSection(&m_critCodecSection);

    if (dvdstate & DVDPACKET_MESSAGE_RESYNC)
    {
      //DVDPlayer asked us to sync playback clock
      m_pClock->Discontinuity(CLOCK_DISC_NORMAL, pPacket->dts);      
    }
    if (dvdstate & DVDPACKET_MESSAGE_NOSKIP)
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

    if( m_iSpeed < 0 )
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
    
    if (dvdstate & DVDPACKET_MESSAGE_FLUSH) // private mesasage sent by (CDVDPlayerVideo::Flush())
    {
      if (m_pVideoCodec)
      {
        m_pVideoCodec->Reset();
      }
    }
    else
    {
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
              iResult = OutputPicture(&picture, pts);

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

        // the decoder didn't need more data, flush the remaning buffer
        iDecoderState = m_pVideoCodec->Decode(NULL, NULL);
      }

      // if decoder had an error, tell it to reset to avoid more problems
      if( iDecoderState & VC_ERROR ) m_pVideoCodec->Reset();
    }
    
    LeaveCriticalSection(&m_critCodecSection);
    
    // all data is used by the decoder, we can safely free it now
    if (pPacket) CDVDDemuxUtils::FreeDemuxPacket(pPacket);
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

void CDVDPlayerVideo::Pause()
{
  m_iSpeed = 0;
}

bool CDVDPlayerVideo::InitializedOutputDevice()
{
  return m_bInitializedOutputDevice;
}

void CDVDPlayerVideo::Resume()
{
  m_iSpeed = 1;
}

void CDVDPlayerVideo::Flush()
{ 
  m_packetQueue.Flush();
  //m_packetQueue.Put(NULL, (void*)DVDPACKET_MESSAGE_FLUSH);

  m_iCurrentPts = DVD_NOPTS_VALUE;
}


CDVDPlayerVideo::EOUTPUTSTATUS CDVDPlayerVideo::OutputPicture(DVDVideoPicture* pPicture, __int64 pts)
{
  if (m_packetQueue.RecievedAbortRequest()) return EOS_ABORT;

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

      // remove any overlays that are out of time
      m_pOverlayContainer->CleanUp(pts);

      m_pOverlayContainer->Lock();
      
      // rendering spu overlay types directly on video memory costs a lot of processing power.
      // thus we allocate a temp picture, copy the original to it (needed because the same picture can be used more than once).
      // then do all the rendering on that temp picture and finaly copy it to video memory.
      // In almost all cases this is 5 or more times faster!.
      DVDVideoPicture* pTempPicture = NULL;
      if (m_pOverlayContainer->ContainsOverlayType(DVDOVERLAY_TYPE_SPU))
      {
        pTempPicture = CDVDCodecUtils::AllocatePicture(pPicture->iWidth, pPicture->iHeight);
        CDVDCodecUtils::CopyPicture(pTempPicture, pPicture);
      }
      else
      {
        CDVDCodecUtils::CopyPictureToOverlay(&image, pPicture);
      }
      
      CDVDOverlay* pOverlay;
      VecOverlays* pVecOverlays = m_pOverlayContainer->GetOverlays();
      VecOverlaysIter it = pVecOverlays->begin();
      
      //Check all overlays and render those that should be rendered, based on time and forced
      //Both forced and subs should check timeing, pts == 0 in the stillframe case
      while (it != pVecOverlays->end())
      {
        pOverlay = *it;
        if ((pOverlay->bForced || m_bRenderSubs) &&
            ((pOverlay->iPTSStartTime <= pts && (pOverlay->iPTSStopTime >= pts || pOverlay->iPTSStopTime == 0LL)) || pts == 0))
        {
          if (pTempPicture) CDVDOverlayRenderer::Render(pTempPicture, pOverlay);
          else CDVDOverlayRenderer::Render(&image, pOverlay);
        }
        it++;
      }
      
      if (pTempPicture)
      {
        CDVDCodecUtils::CopyPictureToOverlay(&image, pTempPicture);
        CDVDCodecUtils::FreePicture(pTempPicture);
        pTempPicture = NULL;
      }
      
      m_pOverlayContainer->Unlock();
      
      // tell the renderer that we've finished with the image (so it can do any
      // post processing before FlipPage() is called.)
      g_renderManager.ReleaseImage();

    }

    // calculate the time we need to delay this picture before displaying
    __int64 iSleepTime;

    if( m_pClock->HadDiscontinuity( DVD_MSEC_TO_TIME(1) ) )
    {
      //Playback at normal fps until after discontinuity
      iSleepTime = pPicture->iDuration;
      iSleepTime -= m_pClock->GetAbsoluteClock() - m_iFlipTimeStamp;
    }
    else if( m_iSpeed < 0 )
    {
      // don't sleep when going backwords, just push frames out
      iSleepTime = 0;
    }
    else
    {
      iSleepTime = pts - m_pClock->GetClock();

      //User set delay
      iSleepTime += m_iVideoDelay;

      // take any delay caused by waiting for vsync into consideration
      iSleepTime -= DVD_MSEC_TO_TIME( g_renderManager.GetAsyncFlipTime() );
    }

    // timestamp to find some sort of average frametime
    m_iFlipTimeStamp = m_pClock->GetAbsoluteClock();

    // dropping to a very low framerate is not correct (it should not happen at all)
    // clock and audio could be adjusted
    if (iSleepTime > DVD_MSEC_TO_TIME(500) ) iSleepTime = DVD_MSEC_TO_TIME(500); // drop to a minimum of 2 frames/sec

    // current pts is always equal to the pts of a picture, at least it is for external subtitles
    m_iCurrentPts = pts;

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
        }

      }

      iSleepTime = 0;
    }

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

__int64 CDVDPlayerVideo::GetDiff()
{
  return 0LL;
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
          continue;
        }
        
        if( mTime > 0 )
          usleep( (int)( mTime ) );        

        break;
      }

      //Time to display
      g_renderManager.FlipPage();
    }
  }

  CLog::Log(LOGDEBUG, "CPresentThread - Stopping()");
}


