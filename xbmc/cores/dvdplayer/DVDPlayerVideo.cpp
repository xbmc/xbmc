
#include "../../stdafx.h"
#include "DVDPlayer.h"
#include "DVDPlayerVideo.h"
#include "DVDCodecs\DVDFactoryCodec.h"
#include "DVDCodecs\DVDCodecUtils.h"
#include "..\..\util.h"

DWORD video_refresh_thread(void *arg);

#define EMULATE_INTTYPES
#include "ffmpeg\avcodec.h"

#define ABS(a) ((a) >= 0 ? (a) : (-(a)))


// for displaying video in the small window
static CDVDVideo* pdvdVideo = NULL;
void xbox_dvdplayer_render_update()
{
  if (pdvdVideo) pdvdVideo->Update(false);
}

CDVDPlayerVideo::CDVDPlayerVideo(CDVDDemuxSPU* spu, CDVDClock* pClock) : CThread()
{
  m_pDVDSpu = spu;
  m_pClock = pClock;
  m_pVideoCodec = NULL;
  m_bInitializedOutputDevice = false;
  m_pOverlayPicture = NULL;
  m_iSpeed = 1;
  m_bRenderSubs = false;
  m_fFPS = 25;
  m_iVideoDelay = 0;

  InitializeCriticalSection(&m_critCodecSection);
  m_packetQueue.SetMaxSize(5 * 256 * 1024); // 1310720

  pdvdVideo = &m_dvdVideo;

  // create sections and event for thread sync (should be destroyed at video stop)
  InitializeCriticalSection(&m_critSection);
  m_hEvent = CreateEvent(NULL, false, false, "dvd picture queue event");

}

CDVDPlayerVideo::~CDVDPlayerVideo()
{
  // close the stream, and don't wait for the audio to be finished
  // CloseStream(true);
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
  m_overlay.Clear();

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
}

void CDVDPlayerVideo::Process()
{
  CLog::Log(LOGNOTICE, "running thread: video_thread");

  HANDLE hVideoRefreshThread;
  CDVDDemux::DemuxPacket* pPacket;
  DVDVideoPicture picture;
  memset(&picture, 0, sizeof(DVDVideoPicture));
  __int64 pts=0;
  int dvdstate;

  m_bRunningVideo = true;

  hVideoRefreshThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)video_refresh_thread, this, 0, NULL);
  SetThreadPriority(hVideoRefreshThread, THREAD_PRIORITY_TIME_CRITICAL);

  while (!m_bStop)
  {
    while (m_iSpeed == 0 && !m_packetQueue.RecievedAbortRequest()) Sleep(5);

    if (m_packetQueue.Get(&pPacket, 1, (void**)&dvdstate) < 0) break;

    if (dvdstate == DVDSTATE_RESYNC)
    {
      //Discontinuity found..
      //Audio side normally handles discontinuities so don't do anything here.
    }

    EnterCriticalSection(&m_critCodecSection);
    int iDecoderState = m_pVideoCodec->Decode(pPacket->pData, pPacket->iSize);


    // loop while no error
    while (!(iDecoderState & VC_ERROR))
    {
      // check for a new picture
      if (iDecoderState & VC_PICTURE)
      {

        // try to retrieve the picture (should never fail!), unless there is a demuxer bug ofcours
        if (m_pVideoCodec->GetPicture(&picture))
        { /*
                    if (dvdstate == DVDSTATE_STILL)
                    {
                      // copy still to buffer for later use
                      if (pDVDPlayer->m_dvd.pStillPicture)
                      {
                        OutputDebugString("ERROR !!!, picture already allocated, memoryleak!");
                      }
                      pDVDPlayer->m_dvd.pStillPicture = CDVDCodecUtils::AllocatePicture(picture.iWidth, picture.iHeight);
                      CDVDCodecUtils::CopyPicture(pDVDPlayer->m_dvd.pStillPicture, &picture);
                    }
          */
          if( picture.iFrameType == 1 || picture.iFrameType == 0) //Only use pts when we have an I frame
            pts = pPacket->dts == DVD_NOPTS_VALUE ? 0 : pPacket->dts;

          int iOutputState=0;
          do 
          {
            if( iOutputState = OutputPicture(&picture, pts) ) break;
            if( picture.iDuration ) pts+=picture.iDuration;
          } while (picture.iRepeatPicture-- > 0);
          if( iOutputState < 0 ) break;

        }
        else
        {
          CLog::Log(LOGWARNING, "Decoder Error getting videoPicture.");
          m_pVideoCodec->Reset();
        }
      }

      // if the decoder needs more data, we just break this loop
      // and try to get more data from the videoQueue
      // in case of a DVD_STILLPICTURE there is never more data, just break so we can keep the picture for later
      if (iDecoderState & VC_BUFFER || dvdstate == DVDSTATE_STILL) break;

      // the decoder didn't need more data, flush the remaning buffer
      iDecoderState = m_pVideoCodec->Decode(NULL, NULL);
    }
    LeaveCriticalSection(&m_critCodecSection);
    // all data is used by the decoder, we can safely free it now
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
  }

  CLog::Log(LOGNOTICE, "wating for video_refresh_thread");
  m_bRunningVideo = false;
  WaitForSingleObject(hVideoRefreshThread, INFINITE);

  CLog::Log(LOGNOTICE, "thread end: video_thread");

  CLog::Log(LOGNOTICE, "uninitting video device");
}

void CDVDPlayerVideo::OnExit()
{
  if (m_pOverlayPicture)
  {
    CLog::Log(LOGNOTICE, "CDVDPlayer::OnExit() freeing overlay picture");
    CDVDCodecUtils::FreePicture(m_pOverlayPicture);
  }
  m_pOverlayPicture = NULL;

  m_dvdVideo.UnInit();
  m_bInitializedOutputDevice = false;

  CLog::Log(LOGNOTICE, "thread end: audio_thread");
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
  EnterCriticalSection(&m_critCodecSection);
  m_packetQueue.Flush();
  m_overlay.Clear(); // not thread safe !!!!
  if (m_pVideoCodec)
  {
    m_pVideoCodec->Reset();
  }
  LeaveCriticalSection(&m_critCodecSection);
}


int CDVDPlayerVideo::OutputPicture(DVDVideoPicture* pPicture, __int64 pts1)
{
  DVDVideoPicture *vp;

  // compute the exact PTS for the picture if it is omitted in the stream
  __int64 pts;
  {
    __int64 frame_delay;
    pts = pts1;

    /* update video clock for next frame */
    frame_delay = ((__int64)m_pDemuxStreamVideo->iFpsScale * DVD_TIME_BASE) / m_pDemuxStreamVideo->iFpsRate;

    /* for MPEG2, the frame can be repeated, so we update the clock accordingly */
    if (pPicture->iRepeatPicture) frame_delay += pPicture->iRepeatPicture * (frame_delay / 2);

    /* update video clock with pts, if present */
    // if (pts != 0) is->video_clock = pts + frame_delay;
    // else is->video_clock += frame_delay;
  }
  if (pts == DVD_NOPTS_VALUE)
  {
#ifdef _DEBUG
    // pts should always be valid here for the picture
    while (1) CLog::DebugLog("CDVDPlayerVideo::OutputPicture, invalid pts value");
#endif

  }
  // end of compute

  // wait  until we have space to put a new picture
  EnterCriticalSection(&m_critSection);
  while (pictq_size >= VIDEO_PICTURE_QUEUE_SIZE && !m_packetQueue.RecievedAbortRequest())
  {
    LeaveCriticalSection(&m_critSection);
    WaitForSingleObject(m_hEvent, INFINITE);
    EnterCriticalSection(&m_critSection);
  }
  LeaveCriticalSection(&m_critSection);

  if (m_packetQueue.RecievedAbortRequest()) return -1;

  vp = &pictq[pictq_windex];

  if( m_pDemuxStreamVideo->iFpsRate != 0 && m_pDemuxStreamVideo->iFpsScale != 0)
    m_fFPS = ((float)m_pDemuxStreamVideo->iFpsRate / m_pDemuxStreamVideo->iFpsScale);
  else
  {
    CLog::Log(LOGERROR, "Demuxer reported invalid framerate: %d or fpsscale: %d", m_pDemuxStreamVideo->iFpsRate, m_pDemuxStreamVideo->iFpsScale);
    m_fFPS = 25;
  }

  if (!m_bInitializedOutputDevice)
  {
    CLog::Log(LOGNOTICE, "Initializing video device");

    m_dvdVideo.Init();

    CLog::Log(LOGNOTICE, "  fps: %f, pwidth: %i, pheight: %i, dwidth: %i, dheight: %i",
              m_fFPS, pPicture->iWidth, pPicture->iHeight, pPicture->iDisplayWidth, pPicture->iDisplayHeight);

    m_dvdVideo.Config(m_fFPS, pPicture->iWidth, pPicture->iHeight, pPicture->iDisplayWidth, pPicture->iDisplayHeight);
    m_bInitializedOutputDevice = true;
  }

  if (m_bInitializedOutputDevice)
  {

    // create overlay picture if not already done
    if (!m_pOverlayPicture)
    {
      m_pOverlayPicture = CDVDCodecUtils::AllocatePicture(pPicture->iWidth, pPicture->iHeight);
    }

    // copy picture to overlay
    YUVOverlay overlay = m_dvdVideo.LockYUVOverlay();
    CDVDCodecUtils::CopyPictureToOverlay(&overlay, pPicture);

    // remove any overlays that are out of time
    m_overlay.CleanUp(pts);

    DVDOverlayPicture* pOverlayPicture = m_overlay.Get();

    //Check all overlays and render those that should be rendered, based on time and forced
    //Both forced and subs should check timeing, pts == 0 in the stillframe case
    while (pOverlayPicture)
    {
      if ((pOverlayPicture->bForced || m_bRenderSubs)
          && ((pOverlayPicture->iPTSStartTime <= pts && pOverlayPicture->iPTSStopTime >= pts) || pts == 0))
      {
        // display subtitle, if bForced is true, it's a menu overlay and we should crop it
        m_overlay.RenderYUV(&overlay, pOverlayPicture, pOverlayPicture->bForced);
      }

      pOverlayPicture = pOverlayPicture->pNext;
    }

    m_dvdVideo.UnlockYUVOverlay();


    vp->pts = pts;

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
  return 0;
}

void CDVDPlayerVideo::Update(bool bPauseDrawing)
{
  m_dvdVideo.Update(bPauseDrawing);
}

void CDVDPlayerVideo::UpdateMenuPicture()
{
  if (m_pVideoCodec)
  {
    DVDVideoPicture picture;
    EnterCriticalSection(&m_critCodecSection);
    if (m_pVideoCodec->GetPicture(&picture))
    {
      OutputPicture(&picture, 0);
    }
    LeaveCriticalSection(&m_critCodecSection);
  }
}

void CDVDPlayerVideo::GetVideoRect(RECT& SrcRect, RECT& DestRect)
{
  m_dvdVideo.GetVideoRect(SrcRect, DestRect);
}

float CDVDPlayerVideo::GetAspectRatio()
{
  return m_dvdVideo.GetAspectRatio();
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

/* called to display each frame */
DWORD video_refresh_thread(void *arg)
{
  CDVDPlayerVideo* pDVDPlayerVideo = (CDVDPlayerVideo*)arg;
  DVDVideoPicture *vp;

  CLog::Log(LOGNOTICE, "running thread: video_refresh_thread");
  int iSleepTime = 0; //Needed to remember old sleeptime when in discontinuities
  CDVDClock frameclock;
  __int64 iTimeStamp = frameclock.GetClock();
  
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
      
      if( pDVDPlayerVideo->m_pClock->HadDiscontinuity(1*DVD_TIME_BASE) ) //Playback at normal fps untill 1 sec after discontinuity
        iSleepTime = (int)(DVD_TIME_BASE / pDVDPlayerVideo->m_fFPS) - (int)(frameclock.GetClock() - iTimeStamp);
      else
        iSleepTime = (int)((vp->pts + pDVDPlayerVideo->m_iVideoDelay - pDVDPlayerVideo->m_pClock->GetClock()) & 0xFFFFFFFF);
  
      
      if (iSleepTime > 500000) iSleepTime = 500000; // drop to a minimum of 2 frames/sec

      // we could drop some frames here too if iSleepTime < 0, but I don't think it will be any
      // use at this stage currently (drawing pictures isn't taking the most processing power)

      // sleep
      if (iSleepTime > 0) usleep(iSleepTime);

      // display picture
      // we expect the video device to be initialized here
      __int64 iTimeStamp = frameclock.GetClock();
      pDVDPlayerVideo->m_dvdVideo.FlipPage();

      // update queue size and signal for next picture
      if (++pDVDPlayerVideo->pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE) pDVDPlayerVideo->pictq_rindex = 0;

      EnterCriticalSection(&pDVDPlayerVideo->m_critSection);
      pDVDPlayerVideo->pictq_size--;
      SetEvent(pDVDPlayerVideo->m_hEvent);
      LeaveCriticalSection(&pDVDPlayerVideo->m_critSection);

    }
  }
  CLog::Log(LOGNOTICE, "thread end: video_refresh_thread");

  return 0;
}
