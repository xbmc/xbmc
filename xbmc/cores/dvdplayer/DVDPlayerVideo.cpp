
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

DWORD video_refresh_thread(void *arg);

CDVDPlayerVideo::CDVDPlayerVideo(CDVDDemuxSPU* spu, CDVDClock* pClock, CDVDOverlayContainer* pOverlayContainer) : CThread()
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
  
  // create sections and event for thread sync (should be destroyed at video stop)
  InitializeCriticalSection(&m_critSection);
  m_hEvent = CreateEvent(NULL, false, false, NULL);
}

CDVDPlayerVideo::~CDVDPlayerVideo()
{
  g_dvdPerformanceCounter.DisableVideoQueue();
  DeleteCriticalSection(&m_critCodecSection);

  DeleteCriticalSection(&m_critSection);
  CloseHandle(m_hEvent);
}

bool CDVDPlayerVideo::OpenStream(CodecID codecID, int iWidth, int iHeight, CDemuxStreamVideo* pDemuxStreamVideo)
{
  m_pDemuxStreamVideo = pDemuxStreamVideo;

  // should alway's be NULL!!!!, it will probably crash anyway when deleting m_pVideoCodec here.
  if (m_pVideoCodec)
  {
    CLog::Log(LOGFATAL, "CDVDPlayerVideo::OpenStream() m_pVideoCodec != NULL");
    return false;
  }

  CLog::Log(LOGNOTICE, "Creating video codec with codec id: %i", codecID);
  m_pVideoCodec = CDVDFactoryCodec::CreateVideoCodec(codecID);

  if( !m_pVideoCodec )
  {
    CLog::Log(LOGERROR, "Unsupported video codec");
    return false;
  }

  if (!m_pVideoCodec->Open(codecID, iWidth, iHeight))
  {
    m_pVideoCodec->Dispose();
    delete m_pVideoCodec;
    m_pVideoCodec = NULL;
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

  /* note: we also signal this mutex to make sure we deblock the video thread in all cases */
  EnterCriticalSection(&m_critSection);
  SetEvent(m_hEvent);
  LeaveCriticalSection(&m_critSection);

  // wait for decode_video thread to end
  CLog::Log(LOGNOTICE, "waiting for video thread to exit");

  StopThread(); // will set this->m_bStop to true
  this->WaitForThreadExit(INFINITE);

  m_packetQueue.End();
  m_pOverlayContainer->Clear();

  CLog::Log(LOGNOTICE, "deleting video codec");
  if (m_pVideoCodec)
  {
    m_pVideoCodec->Dispose();
    delete m_pVideoCodec;
    m_pVideoCodec = NULL;
  }

  m_pDemuxStreamVideo = NULL;
}

void CDVDPlayerVideo::OnStartup()
{
  pictq_size = 0;
  pictq_rindex = 0;
  pictq_windex = 0;
  
  m_iDroppedFrames = 0;
  
  m_iCurrentPts = DVD_NOPTS_VALUE;
  
  g_dvdPerformanceCounter.EnableVideoDecodePerformance(ThreadHandle());
}

void CDVDPlayerVideo::Process()
{
  CLog::Log(LOGNOTICE, "running thread: video_thread");

  HANDLE hVideoRefreshThread;
  CDVDDemux::DemuxPacket* pPacket;
  DVDVideoPicture picture;
  CDVDVideoPPFFmpeg mDeinterlace(CDVDVideoPPFFmpeg::ED_DEINT_FFMPEG);

  memset(&picture, 0, sizeof(DVDVideoPicture));
  
  int dvdstate;
  __int64 pts = 0;
  m_bRunningVideo = true;

  unsigned int iFrameTime = (((__int64)(DVD_TIME_BASE/1000) * m_pDemuxStreamVideo->iFpsScale) / m_pDemuxStreamVideo->iFpsRate);
  if( iFrameTime == 0 ) iFrameTime = 40; //25 fps as a safety

  bool bDetectedStill = false;

  hVideoRefreshThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)video_refresh_thread, this, 0, NULL);
  SetThreadPriority(hVideoRefreshThread, THREAD_PRIORITY_TIME_CRITICAL);

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
            {
              if (m_pDemuxStreamVideo->iFpsRate && m_pDemuxStreamVideo->iFpsScale)
              {
                picture.iDuration = (unsigned int)(((__int64)DVD_TIME_BASE * m_pDemuxStreamVideo->iFpsScale) / m_pDemuxStreamVideo->iFpsRate);
              }
              else
              {
                picture.iDuration = DVD_TIME_BASE / 25;
              }
            }

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

            bool bResult;
            do 
            {
              bResult = OutputPicture(&picture, pts);
              if (!bResult) break;
              
              // guess next frame pts. iDuration is always valid
              pts += picture.iDuration;
            }
            while (picture.iRepeatPicture-- > 0);
            
            if( !bResult) break;
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
    }
    
    LeaveCriticalSection(&m_critCodecSection);
    
    // all data is used by the decoder, we can safely free it now
    if (pPacket) CDVDDemuxUtils::FreeDemuxPacket(pPacket);
  }

  CLog::Log(LOGNOTICE, "wating for video_refresh_thread");
  m_bRunningVideo = false;
  WaitForSingleObject(hVideoRefreshThread, INFINITE);

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
  //EnterCriticalSection(&m_critCodecSection);
  
  //int prio = GetThreadPriority(ThreadHandle());
  //SetPriority(THREAD_PRIORITY_LOW);
 
  m_packetQueue.Flush();
  //m_packetQueue.Put(NULL, (void*)DVDPACKET_MESSAGE_FLUSH);
  
  m_iCurrentPts = DVD_NOPTS_VALUE;
  //SetPriority(prio);
  
  //LeaveCriticalSection(&m_critCodecSection);
}


bool CDVDPlayerVideo::OutputPicture(DVDVideoPicture* pPicture, __int64 pts1)
{
  DVDVideoPicture *vp;
  
  __int64 pts = pts1;

  // wait until we have space to put a new picture
  EnterCriticalSection(&m_critSection);
  while (pictq_size >= VIDEO_PICTURE_QUEUE_SIZE && !m_packetQueue.RecievedAbortRequest())
  {
    LeaveCriticalSection(&m_critSection);
    WaitForSingleObject(m_hEvent, INFINITE);
    EnterCriticalSection(&m_critSection);
  }
  LeaveCriticalSection(&m_critSection);

  if (m_packetQueue.RecievedAbortRequest()) return false;

  vp = &pictq[pictq_windex];

  if (!m_bInitializedOutputDevice)
  {
    float fFPS;
    
    CLog::Log(LOGNOTICE, "Initializing video device");

    if( m_pDemuxStreamVideo->iFpsRate != 0 && m_pDemuxStreamVideo->iFpsScale != 0)
    {
      fFPS = ((float)m_pDemuxStreamVideo->iFpsRate / m_pDemuxStreamVideo->iFpsScale);
    }
    else
    {
      CLog::Log(LOGERROR, "Demuxer reported invalid framerate: %d or fpsscale: %d", m_pDemuxStreamVideo->iFpsRate, m_pDemuxStreamVideo->iFpsScale);
      fFPS = 25.0;
    }
  
    g_renderManager.PreInit();

    CLog::Log(LOGNOTICE, "  fps: %f, pwidth: %i, pheight: %i, dwidth: %i, dheight: %i",
              fFPS, pPicture->iWidth, pPicture->iHeight, pPicture->iDisplayWidth, pPicture->iDisplayHeight);

    g_renderManager.Configure(pPicture->iWidth, pPicture->iHeight, pPicture->iDisplayWidth, pPicture->iDisplayHeight, fFPS);
    m_bInitializedOutputDevice = true;
  }

  if (m_bInitializedOutputDevice)
  {
    // copy picture to overlay
    YV12Image image;
    if (g_renderManager.GetImage(&image))
    {
      CDVDCodecUtils::CopyPictureToOverlay(&image, pPicture);

      // remove any overlays that are out of time
      m_pOverlayContainer->CleanUp(pts);

      m_pOverlayContainer->Lock();
      
      CDVDOverlay* pOverlay;
      VecOverlays* pVecOverlays = m_pOverlayContainer->GetOverlays();
      VecOverlaysIter it = pVecOverlays->begin();

      //Check all overlays and render those that should be rendered, based on time and forced
      //Both forced and subs should check timeing, pts == 0 in the stillframe case
      while (it != pVecOverlays->end())
      {
        pOverlay = *it;
        if ((pOverlay->bForced || m_bRenderSubs)
            && ((pOverlay->iPTSStartTime <= pts && pOverlay->iPTSStopTime >= pts) || pts == 0))
        {
          // display subtitle, if bForced is true, it's a menu overlay and we should crop it
          CDVDOverlayRenderer::Render(&image, pOverlay);
        }
        it++;
      }
      m_pOverlayContainer->Unlock();
      
      // tell the renderer that we've finished with the image (so it can do any
      // post processing before FlipPage() is called.)
      g_renderManager.ReleaseImage();
    }

    //Hmm why not just assigne the entire picure??
    //the vp isn't used at all is it?? overlay is what's rendered all the time
    vp->pts = pts;
    vp->iDuration = pPicture->iDuration;
    vp->iFrameType = pPicture->iFrameType;
    vp->iFlags = pPicture->iFlags;

    /* now we can update the picture count */
    if (++pictq_windex == VIDEO_PICTURE_QUEUE_SIZE) pictq_windex = 0;

    EnterCriticalSection(&m_critSection);
    pictq_size++;
    LeaveCriticalSection(&m_critSection);

    if (m_iSpeed < 0 && m_iSpeed > 1)
    {
      // ffwd or rw
      //is->seek_req = 1;
      //is->seek_pos = (unsigned __int64)(pts * AV_TIME_BASE) + (m_iSpeed * 400000);
    }

  }
  return true;
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

/* called to display each frame */
DWORD video_refresh_thread(void *arg)
{
  CDVDPlayerVideo* pDVDPlayerVideo = (CDVDPlayerVideo*)arg;
  DVDVideoPicture *vp;

  // this one is needed for subtitles sync in DVDPlayer::GetCurrentSubtitle
  pDVDPlayerVideo->m_iCurrentPts = DVD_NOPTS_VALUE;
  
  CLog::Log(LOGNOTICE, "running thread: video_refresh_thread");
  int iSleepTime = 0;
  CDVDClock frameclock;
  __int64 iTimeStamp = frameclock.GetClock();
  unsigned int iFrameCount=30;
  unsigned int iFrameTimeError=300 * DVD_TIME_BASE / 1000;

  unsigned int iDroppedInARow=0;
  int diffPts = 0;
  while (pDVDPlayerVideo->m_bRunningVideo)
  {
    if (pDVDPlayerVideo->pictq_size == 0 || pDVDPlayerVideo->m_iSpeed == 0)
    {
      // if no picture, need to wait
      usleep(1000);
    }
    else
    {
      // dequeue the picture
      vp = &pDVDPlayerVideo->pictq[pDVDPlayerVideo->pictq_rindex];

      if( vp->iFlags & DVP_FLAG_INTERLACED )
      {
        if( vp->iFlags & DVP_FLAG_TOP_FIELD_FIRST )
          g_renderManager.SetFieldSync(FS_ODD);
        else
          g_renderManager.SetFieldSync(FS_EVEN);
      }
      else
        g_renderManager.SetFieldSync( FS_NONE );
      
      //Prepare image for display, will speed up rendering as it frees resources for decode earlier
      g_renderManager.PrepareDisplay();


      bool bDiscontinuity = pDVDPlayerVideo->m_pClock->HadDiscontinuity(DVD_TIME_BASE / 10);
      if (bDiscontinuity)
      {
        //Playback at normal fps until after discontinuity
        iSleepTime = vp->iDuration - (int)(frameclock.GetClock() - iTimeStamp);
      }
      else
      {
        iSleepTime = (int)((vp->pts - pDVDPlayerVideo->m_pClock->GetClock()) & 0xFFFFFFFF);
      
        //User set delay
        iSleepTime += (int)pDVDPlayerVideo->m_iVideoDelay;
      }
      
      //Adjust for flippage delay
      //iSleepTime -= (iFrameTimeError / iFrameCount);
      {
        int pts = (int)(vp->pts & 0xFFFFFFFF);
        int pts_diff = pts - diffPts;
        int clock = (int)(pDVDPlayerVideo->m_pClock->GetClock() & 0xFFFFFFFF);
        
        diffPts = pts;
        
        // CLog::DebugLog("pts:%d, pts diff:%d, clock:%d, sleep:%d, frame error:%d, frame count: %d", pts, pts_diff, clock, iSleepTime, iFrameTimeError, iFrameCount);
      }
      // dropping to a very low framerate is not correct (it should not happen at all)
      // clock and audio could be adjusted
      if (iSleepTime > (DVD_TIME_BASE / 2)) iSleepTime = (DVD_TIME_BASE / 2); // drop to a minimum of 2 frames/sec

      // we could drop some frames here too if iSleepTime < 0, but I don't think it will be any
      // use at this stage currently (drawing pictures isn't taking the most processing power)

      // sleep
      if (iSleepTime > 0) usleep(iSleepTime);

      // display picture
      // we expect the video device to be initialized here
      // skip this flip should we be later than a full frame
      iTimeStamp = frameclock.GetClock();
      
      // current pts is always equal to the pts of a picture, at least it is for external subtitles
      pDVDPlayerVideo->m_iCurrentPts = vp->pts;
      
      // menu pictures should never be skipped!
      if ((vp->iFlags & DVP_FLAG_NOSKIP) || iSleepTime > -(int)vp->iDuration*2 || iDroppedInARow > 15) 
      {
        g_renderManager.FlipPage(); 
      
        //Adjust using the delay in flippage
        __int64 iError = (frameclock.GetClock() - iTimeStamp);
        if( iError < vp->iDuration*2 )
        { //Only count when we actually have something that seem valid
          iFrameTimeError += (unsigned int)iError;
          iFrameCount++;
        }
        
        if( iFrameCount >= 240 )
        { //Keep around 120-240 too give new frames more effect
          iFrameCount /= 2;
          iFrameTimeError /= 2;
        }

        //Reset number of frames dropped in a row
        iDroppedInARow=0;
      }
      else
      {
        pDVDPlayerVideo->m_iDroppedFrames++;
        iDroppedInARow++;
      }

      // update queue size and signal for next picture
      if (++pDVDPlayerVideo->pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE) pDVDPlayerVideo->pictq_rindex = 0;

      EnterCriticalSection(&pDVDPlayerVideo->m_critSection);
      pDVDPlayerVideo->pictq_size--;
      SetEvent(pDVDPlayerVideo->m_hEvent);
      LeaveCriticalSection(&pDVDPlayerVideo->m_critSection);
    }
  }
  CLog::Log(LOGNOTICE, "thread end: video_refresh_thread");

  pDVDPlayerVideo->m_iCurrentPts = DVD_NOPTS_VALUE;
  
  return 0;
}
