/* TODO / BUGS
* - when playing audio only, xbmc renders the screen itself, but we don't get 50 / 60 fps. player bug or general xbmc bug? (XBMC bug, happens in music view only)
* - fix spu / subtitle stuff, using default colors for it now
* - fixing memleaks, I know there are a lot of them atm :-(
* - create a seperate subtitle class for handling subtitles (we should support seperate vob / srt files too)
*   and cleanup al SPUInfo realted code in the DVDPlayer class (we should convert it to YUV anyway)
* -
* - keep track of criticalsections that dll's initialize
* - keep track of file handles that dll's use
* - when stopping a movie, the latest packet that comes from the demuxer is not freed
* - fix aspect ratio's (currently we get the wrong information from libmpeg2)
*/

#include "../../stdafx.h"
#include "DVDPlayer.h"
#include "DVDPlayerDLL.h"

#include "DVDInputStreams\DVDInputStream.h"
#include "DVDInputStreams\DVDFactoryInputStream.h"
#include "DVDInputStreams\DVDInputStreamNavigator.h"

#include "DVDDemuxers\DVDDemux.h"
#include "DVDDemuxers\DVDDemuxUtils.h"
#include "DVDDemuxers\DVDFactoryDemuxer.h"

#include "DVDCodecs\DVDVideoCodec.h"

#include "DVDCodecs\DVDFactoryCodec.h"
#include "DVDCodecs\DVDCodecUtils.h"

#include "..\..\util.h"
#include "../../utils/GUIInfoManager.h"

#define RINT(x) ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)))

static int step = 0;

CDVDPlayer::CDVDPlayer(IPlayerCallback& callback)
    : IPlayer(callback), CThread(), m_dvdPlayerVideo(&m_dvdspus, &m_clock), m_dvdPlayerAudio(&m_clock)
{
  m_pDLLavformat = NULL;
  m_pDLLavcodec = NULL;

  m_pDemuxer = NULL;
  m_pInputStream = NULL;
  m_pCurrentDemuxStreamVideo = NULL;
  m_pCurrentDemuxStreamAudio = NULL;

  m_hReadyEvent = CreateEvent(NULL, true, false, "DVDReadyEvent");

  InitializeCriticalSection(&m_critStreamSection);
  memset(&m_dvd, 0, sizeof(DVDInfo));
  m_dvd.iCurrentCell = -1;
  m_dvd.iNAVPackStart = -1;
  m_dvd.iNAVPackFinish = -1;
  m_dvd.iFlagSentStart = 0;
  m_bReadData = false;
  m_filename[0] = '\0';
  m_bAbortRequest = false;
  m_iCurrentVideoStream = 0;
  m_iCurrentAudioStream = 0;
  m_iCurrentPhysicalAudioStream = -1;
  m_bRenderSubtitle = false;
}

CDVDPlayer::~CDVDPlayer()
{
  CloseFile();

  CloseHandle(m_hReadyEvent);
  DeleteCriticalSection(&m_critStreamSection);
}

bool CDVDPlayer::Load()
{
  if (!m_pDLLavcodec)
  {
    CLog::Log(LOGNOTICE, "CDVDPlayer::Load() Loading avcodec.dll");
    // load the dll with memory tracking enabled !!!
    // this is really needed for avcodec.dll because it doesn't free all memory after closing the codec
    // there is a function av_free_static() for it, but even that doesn't do it perfectly
    m_pDLLavcodec = new DllLoader("Q:\\system\\players\\dvdplayer\\avcodec.dll", true);
    if ( !m_pDLLavcodec->Parse() )
    {
      CLog::Log(LOGERROR, "CDVDPlayer::Load() parse avcodec.dll failed");
      delete m_pDLLavcodec;
      m_pDLLavcodec = NULL;
      return false;
    }
    CLog::Log(LOGNOTICE, "CDVDPlayer::Load() resolving imports for avcodec.dll");
    if ( !m_pDLLavcodec->ResolveImports() )
    {
      CLog::Log(LOGERROR, "CDVDPlayer::Load() resolving imports for avcodec.dll failed");
    }
    CLog::Log(LOGNOTICE, "CDVDPlayer::Load() resolving exports for avcodec.dll");
    if (!dvdplayer_load_dll_avcodec(*m_pDLLavcodec))
    {
      CLog::Log(LOGERROR, "CDVDPlayer::Load() resolving exports for avcodec.dll failed");
      Unload();
      return false;
    }
  }

  if (!m_pDLLavformat)
  {
    CLog::Log(LOGNOTICE, "CDVDPlayer::Load() Loading avformat.dll");
    m_pDLLavformat = new DllLoader("Q:\\system\\players\\dvdplayer\\avformat.dll", true);
    if (!m_pDLLavformat->Parse())
    {
      CLog::Log(LOGERROR, "CDVDPlayer::Load() parse avformat.dll failed");
      delete m_pDLLavformat;
      m_pDLLavformat = NULL;
      return false;
    }
    CLog::Log(LOGNOTICE, "CDVDPlayer::Load() resolving imports for avformat.dll");
    if (!m_pDLLavformat->ResolveImports() )
    {
      CLog::Log(LOGERROR, "CDVDPlayer::Load() resolving imports for avformat.dll failed");
    }
    CLog::Log(LOGNOTICE, "CDVDPlayer::Load() resolving exports for avformat.dll");
    if (!dvdplayer_load_dll_avformat(*m_pDLLavformat))
    {
      CLog::Log(LOGERROR, "CDVDPlayer::Load() resolving exports for avformat.dll failed");
      Unload();
      return false;
    }
  }

  return true;
}

void CDVDPlayer::Unload()
{
  if (m_pDLLavformat)
  {
    CLog::Log(LOGNOTICE, "CDVDPlayer::Unload() Unloading avformat.dll");
    delete m_pDLLavformat;
    m_pDLLavformat = NULL;
  }

  if (m_pDLLavcodec)
  {
    CLog::Log(LOGNOTICE, "CDVDPlayer::Unload() Unloading avcodec.dll");
    delete m_pDLLavcodec;
    m_pDLLavcodec = NULL;
  }

  CLog::Log(LOGNOTICE, "CDVDPlayer::Unload() Done unloading dll's");
}

bool CDVDPlayer::OpenFile(const CFileItem& file, __int64 iStartTime)
{
  CStdString strFile = file.m_strPath;
  CLog::Log(LOGNOTICE, "DVDPlayer: Opening: %s", strFile.c_str());

  // if playing a file close it first
  // this has to be changed so we won't have to close it.
  CloseFile();

  m_bAbortRequest = false;
  step = 0;
  m_iSpeed = 1;

  if (!Load()) return false;

  m_dvd.state = DVDSTATE_NORMAL;
  m_dvd.iSelectedSPUStream = -1;
  m_dvd.iSelectedAudioStream = -1;

  if (strFile.Find("dvd://") >= 0) strcpy(m_filename, "\\Device\\Cdrom0");
  else strcpy(m_filename, strFile.c_str());

  ResetEvent(m_hReadyEvent);
  Create();
  WaitForSingleObject(m_hReadyEvent, INFINITE);

  // if we are playing a media file with pictures, we should wait for the video output device to be initialized
  // if we don't wait, the fullscreen window will init with a picture that is 0 pixels width and high
  if (m_iCurrentVideoStream >= 0 ||
      m_pInputStream && m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD)
  {
    while (!m_bAbortRequest && !m_dvdPlayerVideo.InitializedOutputDevice()) Sleep(1);
  }

  // m_bPlaying could be set to false in the meantime, which indicates an error
  return !m_bStop;
}

bool CDVDPlayer::CloseFile()
{
  CLog::Log(LOGNOTICE, "DVDPlayer: closefile called");
  m_bAbortRequest = true;

  CLog::Log(LOGNOTICE, "DVDPlayer: waiting for threads to exit");
  StopThread();
  CLog::Log(LOGNOTICE, "DVDPlayer: finished waiting");

  return true;
}

bool CDVDPlayer::IsPlaying() const
{
  return !m_bStop;
}

void CDVDPlayer::OnStartup()
{
  m_iCurrentVideoStream = -1;
  m_iCurrentAudioStream = -1;

  m_messenger.Clear();
}

void CDVDPlayer::Process()
{
  int video_index = -1;
  int audio_index = -1;

  CLog::Log(LOGNOTICE, "Creating InputStream");
  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(this, m_filename);
  if (!m_pInputStream || !m_pInputStream->Open(m_filename))
  {
    CLog::Log(LOGERROR, "InputStream: Error opening, %s", m_filename);
    // inputstream will be destroyed in OnExit()
    return ;
  }
  if (m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD) CLog::Log(LOGNOTICE, "DVDPlayer: playing a dvd with menu's");

  CLog::Log(LOGNOTICE, "Creating Demuxer");
  m_pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(m_pInputStream);
  if (!m_pDemuxer || !m_pDemuxer->Open(m_pInputStream))
  {
    CLog::Log(LOGERROR, "Demuxer: Error opening, demuxer");
    // inputstream and the demuxer will be destroyed in OnExit()
    return ;
  }

  // find first audio / video streams
  for (int i = 0; i < m_pDemuxer->GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(i);
    if (pStream->type == STREAM_AUDIO && audio_index < 0) audio_index = i;
    else if (pStream->type == STREAM_VIDEO && video_index < 0) video_index = i;
  }

  // open the streams
  if (audio_index >= 0) OpenDefaultAudioStream();
  if (video_index >= 0) OpenVideoStream(video_index);

  // if we use libdvdnav, streaminfo is not avaiable, we will find this later on
  if (m_iCurrentVideoStream < 0 && m_iCurrentAudioStream < 0 && m_pInputStream->m_streamType != DVDSTREAM_TYPE_DVD)
  {
    CLog::Log(LOGERROR, "%s: could not open codecs\n", m_filename);
    return ;
  }

  // we are done initializing now, set the readyevent
  SetEvent(m_hReadyEvent);

  m_callback.OnPlayBackStarted();

  int iErrorCounter = 0;
  for (;;)
  {
    // if the queues are full, no need to read more
    while (!m_bAbortRequest && (m_dvdPlayerAudio.m_packetQueue.IsFull() || m_dvdPlayerVideo.m_packetQueue.IsFull()))
    {
      Sleep(10);
    }

    if (m_bAbortRequest) break;

    // handle messages send to this thread, like seek or demuxer reset requests
    HandleMessages();

    // read a data frame from stream.
    CDVDDemux::DemuxPacket* pPacket = m_pDemuxer->Read();
    // m_bFirstHandleMessages could be true, in which case we should handle some messages first
    if (m_bFirstHandleMessages && !pPacket)
    {
      m_bFirstHandleMessages = false;
      continue;
    }

    m_bReadData = true; // used for stutterfree dvd menu navigation
    static int oldstate = DVDSTATE_STILL; // test variable that should be removed sometimes , cause read error's shouldn't happen

    if (m_dvd.state == DVDSTATE_STILL)
    {
      // only send a still state to the video decoder once.
      if (!m_dvd.bDisplayedStill)
      {
        // notify the video decoder we recieved a still frame by sending an empty packet
        // bug!, we need todo this cause av_read_frame doesn't seem to send all data needed for the picture
        // maybe this can be fixed by writing the av_read_frame function ourself?
        if (!pPacket) pPacket = CDVDDemuxUtils::AllocateDemuxPacket();
        // m_pVideoCodec->Flush();
        m_dvdPlayerVideo.m_packetQueue.Put(pPacket, (void*)DVDSTATE_STILL);
        m_dvd.bDisplayedStill = true;
        oldstate = DVDSTATE_STILL;
      }
      else if (pPacket) CDVDDemuxUtils::FreeDemuxPacket(pPacket);
      continue;
    }
    else if (!pPacket)
    {
      CLog::Log(LOGERROR, "Error reading data form demuxer");
      
      if(++iErrorCounter<100) //Allow 100 errors in a row before giving up
        continue;
      else
        break;
    }

    oldstate = DVDSTATE_NORMAL;
    iErrorCounter=0;

    CDemuxStream *pStream = m_pDemuxer->GetStream(pPacket->iStreamId);

    int iFileStreamId = m_pDemuxer->GetStream(pPacket->iStreamId)->iId;
    if (iFileStreamId >= m_dvd.iSelectedSPUStream &&
        iFileStreamId <= m_dvd.iSelectedSPUStream)
    {
      SPUInfo* pSPUInfo = m_dvdspus.AddData(pPacket->pData, pPacket->iSize, iFileStreamId, pPacket->pts);

      if (pSPUInfo)
      {
        CLog::DebugLog("av_read_frame: Got complete SPU packet");
        DVDOverlayPicture* pOverlayPicture = new DVDOverlayPicture;
        /// XXX /////////
        pOverlayPicture->bForced = pSPUInfo->bForced;
        pOverlayPicture->height = pSPUInfo->height;
        pOverlayPicture->iPTSStartTime = pSPUInfo->iPTSStartTime;
        pOverlayPicture->iPTSStopTime = pSPUInfo->iPTSStopTime;
        pOverlayPicture->iSPUSize = pSPUInfo->iSPUSize;
        pOverlayPicture->pBFData = pSPUInfo->pBFData;
        pOverlayPicture->pTFData = pSPUInfo->pTFData;
        pOverlayPicture->width = pSPUInfo->width;
        pOverlayPicture->x = pSPUInfo->x;
        pOverlayPicture->y = pSPUInfo->y;

        memcpy(pOverlayPicture->color, pSPUInfo->color, sizeof(pSPUInfo->color));
        memcpy(pOverlayPicture->alpha, pSPUInfo->alpha, sizeof(pSPUInfo->alpha));
        pOverlayPicture->pData = new BYTE[70000];
        memcpy(pOverlayPicture->pData, pSPUInfo->result, sizeof(pSPUInfo->result));
        /// XXX /////////

        m_dvdPlayerVideo.m_overlay.Add(pOverlayPicture);

        // JM hack (commented out m_dvd.state == DVDSTATE_STILL)
        // It's no hack, the timeing of the highlight will be handled by the SPU package
        if (pSPUInfo->bForced /*&& m_dvd.state == DVDSTATE_STILL*/)
        {
          // recieved new menu overlay (button), update the screen when displaying a still
          OnDVDNavResult(NULL, DVDNAV_HIGHLIGHT); // hack
        }
        // end JM hack

        delete pSPUInfo;
      }
      // free it, content is already copied into a special structure
      CDVDDemuxUtils::FreeDemuxPacket(pPacket);
    }
    else
    {
      int iNrOfStreams = m_pDemuxer->GetNrOfStreams();
      if (m_iCurrentAudioStream >= iNrOfStreams) CloseAudioStream(false);
      if (m_iCurrentVideoStream >= iNrOfStreams) CloseVideoStream(false);

      LockStreams();

      CDemuxStream* pStream = m_pDemuxer->GetStream(pPacket->iStreamId);
      if (pPacket->iStreamId == m_iCurrentAudioStream ||
          (m_iCurrentAudioStream < 0 && pStream->type == STREAM_AUDIO))
      {
        // if we have no audio stream yet, openup the audio device now
        if (m_iCurrentAudioStream < 0) OpenDefaultAudioStream();
        else
        {
          CDemuxStreamAudio* pStreamAudio = (CDemuxStreamAudio*)pStream;

          // if audio information changed close the audio device first
          if (m_dvdPlayerAudio.m_codec != pStreamAudio->codec ||
              m_dvdPlayerAudio.m_iSourceChannels != pStreamAudio->iChannels) CloseAudioStream(false);
        }

        if (m_iCurrentAudioStream >= 0) 
          if (!(m_dvd.iFlagSentStart & 1) && pPacket->pts > m_dvd.iNAVPackStart && pPacket->pts < m_dvd.iNAVPackFinish)
          {
            m_dvdPlayerAudio.m_packetQueue.Put(pPacket, (void*)DVDSTATE_RESYNC);
            m_dvd.iFlagSentStart |= 1;
          }
          else
            m_dvdPlayerAudio.m_packetQueue.Put(pPacket);

        else CDVDDemuxUtils::FreeDemuxPacket(pPacket);
      }
      else if (pPacket->iStreamId == m_iCurrentVideoStream ||
               (m_iCurrentVideoStream < 0 && pStream->type == STREAM_VIDEO))
      {
        // if we have no video stream yet, openup the video device now
        if (m_iCurrentVideoStream < 0) OpenVideoStream(pPacket->iStreamId);

        if (m_iCurrentVideoStream >= 0) 
          if (!(m_dvd.iFlagSentStart & 2) && pPacket->pts > m_dvd.iNAVPackStart && pPacket->pts < m_dvd.iNAVPackFinish)
          {
            //For some reason this isn't always found.. not sure why exactly. what should be used?? pts or dts???
            m_dvdPlayerVideo.m_packetQueue.Put(pPacket, (void*)DVDSTATE_RESYNC);
            m_dvd.iFlagSentStart |= 2;
          }
          else
            m_dvdPlayerVideo.m_packetQueue.Put(pPacket);

        else CDVDDemuxUtils::FreeDemuxPacket(pPacket);
      }
      else CDVDDemuxUtils::FreeDemuxPacket(pPacket); // free it since we won't do anything with it

      UnlockStreams();
    }
  }
}

void CDVDPlayer::OnExit()
{
  CLog::Log(LOGNOTICE, "CDVDPlayer::OnExit()");

  // close each stream
  if (!m_bAbortRequest) CLog::Log(LOGNOTICE, "DVDPlayer: eof, waiting for queues to empty");
  if (m_iCurrentAudioStream >= 0)
  {
    CLog::Log(LOGNOTICE, "DVDPlayer: closing audio stream");
    CloseAudioStream(!m_bAbortRequest);
  }
  if (m_iCurrentVideoStream >= 0)
  {
    CLog::Log(LOGNOTICE, "DVDPlayer: closing video stream");
    CloseVideoStream(!m_bAbortRequest);
  }

  // destroy the demuxer
  if (m_pDemuxer)
  {
    CLog::Log(LOGNOTICE, "CDVDPlayer::OnExit() deleting demuxer");
    m_pDemuxer->Dispose();
    delete m_pDemuxer;
  }
  m_pDemuxer = NULL;

  // destroy the inputstream
  if (m_pInputStream)
  {
    CLog::Log(LOGNOTICE, "CDVDPlayer::OnExit() deleting input stream");
    delete m_pInputStream;
  }
  m_pInputStream = NULL;

  // unload our dll's
  Unload();

  // if we didn't stop playing, advance to the next item in xbmc's playlist
  if (!m_bAbortRequest) m_callback.OnPlayBackEnded();

  m_pCurrentDemuxStreamVideo = NULL;
  m_pCurrentDemuxStreamAudio = NULL;

  // set event to inform openfile something went wrong in case openfile is still waiting for this event
  SetEvent(m_hReadyEvent);
}

void CDVDPlayer::HandleMessages()
{
  DVDPlayerMessage* pMessage = m_messenger.Recieve();

  while (pMessage)
  {
    switch (pMessage->iMessage)
    {
    case DVDMESSAGE_SEEK:
      {
        if (m_pInputStream && m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD) 
        {
          //We can't seek in a menu
          if (!IsInMenu())
          {
            // need to get the seek based on file positition working in CDVDInputStreamNavigator
            // so that demuxers can control the stream (seeking in this case)
            // for now use time based seeking
            if (((CDVDInputStreamNavigator*)m_pInputStream)->Seek(pMessage->iValue))
            {
              FlushBuffers();
              // can't do this this way, because our streampointers (m_pCurrentDemuxStreamAudio and Video)
              // will get invalid
            
              /// m_pDemuxer->Reset();
            
              // let the code below handle the reset
              /// m_messenger.ResetDemuxer();
              
              // just as easy to leave it away
              // libmpeg2 can handle streams that are incorrect, so a bit of incorrect data is no problem here
              // better would be to flush all data in the demuxer, but not to close reset it!
            }
            else CLog::Log(LOGWARNING, "error while seeking");
          }
          else CLog::Log(LOGWARNING, "can't seek in a menu");
        }
        else
        {
          if (m_pDemuxer->Seek(pMessage->iValue)) 
          {
            FlushBuffers();
          }
          else CLog::Log(LOGWARNING, "error while seeking");
        }
        break;
      }
    case DVDMESSAGE_CHAPTER_NEXT:
    case DVDMESSAGE_CHAPTER_PREV:
      {
        if (m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD)
        {
          CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;

          if (pMessage->iMessage == DVDMESSAGE_CHAPTER_NEXT) pStream->OnNext();
          if (pMessage->iMessage == DVDMESSAGE_CHAPTER_PREV) pStream->OnPrevious();

          //m_bReadData = false;
          m_dvd.iDVDStillTime = 0;
          m_dvd.state = DVDSTATE_NORMAL;
          m_dvd.iDVDStillStartTime = 0;
          // sleep until the dvd player is reading some data with read_frame
          // while(!m_bReadData) Sleep(1);
          // m_iCommands |= DVDCOMMAND_FLUSH;
          FlushBuffers();
        }
        break;
      }
    case DVDMESSAGE_RESET_DEMUXER:
      {
        // after the reset our m_pCurrentStreams will be invalid, so these should
        // be closed anyway (we should wait for the buffers though)
        CloseAudioStream(true);
        CloseVideoStream(true);

        // we need to reset the demuxer, probably because the streams have changed
        m_pDemuxer->Reset();

        // and read again
        // pPacket = m_pDemuxer->Read();
        break;
      }

    }

    delete pMessage;
    pMessage = m_messenger.Recieve();
  }
}

void CDVDPlayer::Pause()
{
  if (m_iSpeed == 0) m_iSpeed = 1;
  else
  {
    m_iSpeed = 0;
  }

  if (m_iSpeed == 1)
  {
    //m_clock.Discontinuity(CLOCK_DISC_NORMAL, m_dvdPlayerAudio.GetClock());
    m_dvdPlayerAudio.Resume();
    m_dvdPlayerVideo.Resume();
  }
  else
  {
    m_dvdPlayerAudio.Pause(); // XXX, this won't work for ffwd or ffrw
    m_dvdPlayerVideo.Pause();
  }

  /* pause or resume the video *
  cur_stream->paused = !cur_stream->paused;
  if (cur_stream->paused)
  {
    cur_stream->video_current_pts = get_video_clock();
  }
  step = 0;*/
}

bool CDVDPlayer::IsPaused() const
{
  return (m_iSpeed == 0);
}

bool CDVDPlayer::HasVideo()
{
  if (m_pInputStream)
  {
    if (m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD || m_iCurrentVideoStream >= 0) return true;
  }
  return false;
}

bool CDVDPlayer::HasAudio()
{
  return (m_iCurrentAudioStream >= 0);
  return false;
}

void CDVDPlayer::SwitchToNextLanguage()
{}

void CDVDPlayer::ToggleSubtitles()
{
  m_bRenderSubtitle = !m_bRenderSubtitle;
}

void CDVDPlayer::SubtitleOffset(bool bPlus)
{}

void CDVDPlayer::Seek(bool bPlus, bool bLargeStep)
{
  float percent = (bLargeStep ? 10.0f : 2.0f);
  
  if (bPlus) percent += GetPercentage();
  else percent = GetPercentage() - percent;

  if (percent >= 0 && percent <= 100)
  {  
    // should be modified to seektime
    SeekPercentage(percent);
  }
}

void CDVDPlayer::ToggleFrameDrop()
{}

int CDVDPlayer::GetVolume()
{
  return 100;
}

void CDVDPlayer::SetVolume(long nVolume)
{}

void CDVDPlayer::SetContrast(bool bPlus)
{}

void CDVDPlayer::SetBrightness(bool bPlus)
{}

void CDVDPlayer::SetHue(bool bPlus)
{}

void CDVDPlayer::SetSaturation(bool bPlus)
{}

void CDVDPlayer::GetAudioInfo(CStdString& strAudioInfo)
{
  if (!m_bStop && m_pCurrentDemuxStreamAudio)
  {
    string strDemuxerInfo;
    m_pCurrentDemuxStreamAudio->GetStreamInfo(strDemuxerInfo);

    int bsize = m_dvdPlayerAudio.m_packetQueue.GetSize();
    if (bsize > 0) bsize = (int)(((double)m_dvdPlayerAudio.m_packetQueue.GetSize() / m_dvdPlayerAudio.m_packetQueue.GetMaxSize()) * 100);
    if (bsize > 99) bsize = 99;

    strAudioInfo.Format("%s, aq size: %i", strDemuxerInfo.c_str(), bsize);
  }
}

void CDVDPlayer::GetVideoInfo(CStdString& strVideoInfo)
{
  if (!m_bStop && m_pCurrentDemuxStreamVideo)
  {
    string strDemuxerInfo;
    m_pCurrentDemuxStreamVideo->GetStreamInfo(strDemuxerInfo);

    int bsize = m_dvdPlayerVideo.m_packetQueue.GetSize();
    if (bsize > 0) bsize = (int)(((double)m_dvdPlayerVideo.m_packetQueue.GetSize() / m_dvdPlayerVideo.m_packetQueue.GetMaxSize()) * 100);
    if (bsize > 99) bsize = 99;

    strVideoInfo.Format("%s, vq size: %i", strDemuxerInfo.c_str(), bsize);
  }
}

void CDVDPlayer::GetGeneralInfo(CStdString& strGeneralInfo)
{
  static char ginftemp[256];
  if (!m_bStop)
  {
    double dDelay = (double)m_dvdPlayerVideo.GetDelay() / DVD_TIME_BASE;
    double dDiff = (double)m_dvdPlayerVideo.GetDiff() / DVD_TIME_BASE;
    sprintf(ginftemp, "DVD Player ad:%6.3f, diff:%6.3f", dDelay, dDiff);
    strGeneralInfo = ginftemp;
  }
}

void CDVDPlayer::Update(bool bPauseDrawing)
{
  m_dvdPlayerVideo.Update(bPauseDrawing);
}

void CDVDPlayer::GetVideoRect(RECT& SrcRect, RECT& DestRect)
{
  m_dvdPlayerVideo.GetVideoRect(SrcRect, DestRect);
}

void CDVDPlayer::GetVideoAspectRatio(float& fAR)
{
  fAR = m_dvdPlayerVideo.GetAspectRatio();
}

void CDVDPlayer::RegisterAudioCallback(IAudioCallback* pCallback)
{
  m_dvdPlayerAudio.RegisterAudioCallback(pCallback);
}

void CDVDPlayer::UnRegisterAudioCallback()
{
  m_dvdPlayerAudio.UnRegisterAudioCallback();
}

void CDVDPlayer::AudioOffset(bool bPlus)
{}

void CDVDPlayer::SwitchToNextAudioLanguage()
{}

void CDVDPlayer::SeekPercentage(float iPercent)
{
  int iTotalMsec = GetTotalTime();
  int iTime = (int)(iTotalMsec * iPercent / 100);
  
  SeekTime(iTime);
}

float CDVDPlayer::GetPercentage()
{
  int iTotalTime = GetTotalTime();
  int iCurrentTime = (int)GetTimeInMsec();
  
  float fPercent = iCurrentTime * 100 / (float)iTotalTime;

  return fPercent;
}

//This is how much audio is delayed to video, we count the oposite in the dvdplayer
void CDVDPlayer::SetAVDelay(float fValue)
{
  m_dvdPlayerVideo.SetDelay( - (__int64)(fValue * DVD_TIME_BASE) ) ;
}

float CDVDPlayer::GetAVDelay()
{
  return - m_dvdPlayerVideo.GetDelay() / (float)DVD_TIME_BASE;
}

void CDVDPlayer::SetSubTitleDelay(float fValue)
{}

float CDVDPlayer::GetSubTitleDelay()
{
  return 0.0;
}

int CDVDPlayer::GetSubtitleCount()
{
  if (m_pInputStream && m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD)
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    return pStream->GetSubTitleStreamCount();
  }
  return 0;
}

int CDVDPlayer::GetSubtitle()
{
  if (m_dvd.iSelectedSPUStream < 0x20) return -1;
  return (m_dvd.iSelectedSPUStream - 0x20);
  return -1;
}

void CDVDPlayer::GetSubtitleName(int iStream, CStdString &strStreamName)
{
  if (m_pInputStream && m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD)
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    strStreamName = pStream->GetSubtitleStreamLanguage(iStream);
  }
  else strStreamName = "Unknown";
}

void CDVDPlayer::SetSubtitle(int iStream)
{
  m_dvd.iSelectedSPUStream = 0x20 + iStream;
  if (m_pInputStream && m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD)
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    pStream->SetActiveSubtitleStream(iStream);
  }
}

bool CDVDPlayer::GetSubtitleVisible()
{
  if (!m_bRenderSubtitle) return false;

  int id = m_dvd.iSelectedSPUStream - 0x20;
  if (id >= 0 && id <= 0x3f) return true;

  return false;
}

void CDVDPlayer::SetSubtitleVisible(bool bVisible)
{
  m_bRenderSubtitle = bVisible;
  m_dvdPlayerVideo.m_bRenderSubs = bVisible;
}

int CDVDPlayer::GetAudioStreamCount()
{
  if (m_pInputStream && m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD)
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    return pStream->GetAudioStreamCount();
  }
  return 0;
}

int CDVDPlayer::GetAudioStream()
{ /*
    if (m_pInputStream && m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD)
    {
      CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
      return pStream->GetActiveAudioStream();
    }
    return 0;*/
  int iAudioIndex = 0;

  for (int stream_index = 0; stream_index < m_pDemuxer->GetNrOfStreams(); stream_index++)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(stream_index);

    if (pStream == m_pCurrentDemuxStreamAudio) return iAudioIndex;
    if (pStream->type == STREAM_AUDIO) iAudioIndex++;
  }

  return 0;
}

void CDVDPlayer::GetAudioStreamName(int iStream, CStdString& strStreamName)
{
  if (m_pInputStream && m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD)
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    strStreamName = pStream->GetAudioStreamLanguage(iStream);
  }
  else strStreamName = "Unknown";
}

void CDVDPlayer::SetAudioStream(int iStream)
{
  int audio_index = -1;
  int old_index = 0;

  old_index = m_iCurrentAudioStream;
  for (int stream_index = 0; stream_index < m_pDemuxer->GetNrOfStreams(); stream_index++)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(stream_index);

    if (pStream->type == STREAM_AUDIO) audio_index++;
    if (iStream == audio_index)
    {
      m_iCurrentPhysicalAudioStream = pStream->iPhysicalId;

      if(m_pInputStream && m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD )
      {
        CDVDInputStreamNavigator* pInput = (CDVDInputStreamNavigator*)m_pInputStream;
        pInput->SetActiveAudioStream(m_iCurrentPhysicalAudioStream);
      }
    
      //Just close here, it will be opened automatically
      LockStreams();
      CloseAudioStream(false);
      UnlockStreams();
      break;
    }
  }
}

void CDVDPlayer::SeekTime(int iTime)
{
  // get timing and seeking from libdvdnav for dvd's
  if (m_pDemuxer)
  {
    m_messenger.Seek(iTime);
  }
  g_infoManager.m_bPerformingSeek = false;
}

// return the time in seconds, should be changed in xbmc to msec
__int64 CDVDPlayer::GetTime()
{
  return (GetTimeInMsec() / 1000);
}

int CDVDPlayer::GetTimeInMsec()
{
  // get timing and seeking from libdvdnav for dvd's
  if (m_pInputStream && m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD)
  {
    return ((CDVDInputStreamNavigator*)m_pInputStream)->GetTime(); // we should take our buffers into account
  }

  return (int)(m_clock.GetClock() / (DVD_TIME_BASE / 1000));
}

// return length in msec
int CDVDPlayer::GetTotalTime()
{
  // get timing and seeking from libdvdnav for dvd's
  if (m_pInputStream && m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD)
  {
    return ((CDVDInputStreamNavigator*)m_pInputStream)->GetTotalTime(); // we should take our buffers into account
  }

  if (m_pDemuxer) return m_pDemuxer->GetStreamLenght();
  return 0;
}

void CDVDPlayer::ToFFRW(int iSpeed)
{
  if (iSpeed != 1) m_dvdPlayerAudio.Pause();
  else m_dvdPlayerAudio.Resume();
  m_iSpeed = iSpeed;
}

void CDVDPlayer::ShowOSD(bool bOnoff)
{}

void CDVDPlayer::DoAudioWork()
{
  m_dvdPlayerAudio.DoWork();
}

bool CDVDPlayer::GetSubtitleExtension(CStdString &strSubtitleExtension)
{
  return false;
}

void CDVDPlayer::UpdateSubtitlePosition()
{}

void CDVDPlayer::RenderSubtitles()
{}

/*
 * Opens the First valid available audio stream
 */
bool CDVDPlayer::OpenDefaultAudioStream()
{
  for (int i = 0; i < m_pDemuxer->GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(i);
    if (pStream->type == STREAM_AUDIO)
    {
      if(m_iCurrentPhysicalAudioStream < 0 || pStream->iPhysicalId == m_iCurrentPhysicalAudioStream )
      {
        CDemuxStreamAudio* pStreamAudio = (CDemuxStreamAudio*)pStream;
        {
          if (OpenAudioStream(i)) return true;
        }
      }
    }
  }
  return false;
}

bool CDVDPlayer::OpenAudioStream(int iStream)
{
  CLog::Log(LOGNOTICE, "Opening audio stream: %i", iStream);

  if (iStream < 0 || iStream >= m_pDemuxer->GetNrOfStreams()) return false;
  if (m_pDemuxer->GetStream(iStream)->type != STREAM_AUDIO)
  {
    CLog::Log(LOGERROR, "Streamtype is not STREAM_AUDIO");
    return false;
  }
  CDemuxStreamAudio* pStreamAudio = (CDemuxStreamAudio*)m_pDemuxer->GetStream(iStream);

  m_bDrawedFrame = false;

  if (m_dvdPlayerAudio.OpenStream(pStreamAudio->codec, pStreamAudio->iChannels, pStreamAudio->iSampleRate))
  {
    m_dvdPlayerAudio.SetPriority(THREAD_PRIORITY_HIGHEST + 4);

    m_iCurrentAudioStream = iStream;
    m_pCurrentDemuxStreamAudio = pStreamAudio;
    return true;
  }

  return false;
}

bool CDVDPlayer::OpenVideoStream(int iStream)
{
  CLog::Log(LOGNOTICE, "Opening video stream: %i", iStream);

  if (iStream < 0 || iStream >= m_pDemuxer->GetNrOfStreams()) return false;
  if (m_pDemuxer->GetStream(iStream)->type != STREAM_VIDEO)
  {
    CLog::Log(LOGERROR, "Streamtype is not STREAM_VIDEO");
    return false;
  }
  CDemuxStreamVideo* pStreamVideo = (CDemuxStreamVideo*)m_pDemuxer->GetStream(iStream);

  if (m_dvdPlayerVideo.OpenStream(pStreamVideo->codec, pStreamVideo->iWidth, pStreamVideo->iHeight, pStreamVideo))
  {
    m_dvdPlayerVideo.SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);

    m_iCurrentVideoStream = iStream;
    m_pCurrentDemuxStreamVideo = pStreamVideo;
    return true;
  }

  return false;
}

bool CDVDPlayer::CloseAudioStream(bool bWaitForBuffers)
{
  CLog::Log(LOGNOTICE, "Closing audio stream");

  if (!m_pCurrentDemuxStreamAudio || m_iCurrentAudioStream < 0) return false; // can't close stream if the stream does not exist

  // if (m_pDemuxer->GetStream(m_iCurrentAudioStream)->type != STREAM_AUDIO) return false;

  m_dvdPlayerAudio.CloseStream(bWaitForBuffers);

  m_pCurrentDemuxStreamAudio = NULL;
  m_iCurrentAudioStream = -1;

  return true;
}

bool CDVDPlayer::CloseVideoStream(bool bWaitForBuffers) // bWaitForBuffers currently not used
{
  CLog::Log(LOGNOTICE, "Closing video stream");

  if (!m_pCurrentDemuxStreamVideo || m_iCurrentVideoStream < 0) return false; // can't close stream if the stream does not exist

  // if (m_pDemuxer->GetStream(m_iCurrentVideoStream)->type != STREAM_VIDEO) return false;

  m_dvdPlayerVideo.CloseStream(bWaitForBuffers);

  m_pCurrentDemuxStreamVideo = NULL;
  m_iCurrentVideoStream = -1;

  return true;
}

void CDVDPlayer::FlushBuffers()
{
  m_dvdPlayerAudio.Flush();
  m_dvdPlayerVideo.Flush();
  m_dvd.iFlagSentStart = 0; //We will have a discontinuity here
}

// since we call ffmpeg functions to decode, this is being called in the same thread as ::Process() is
int CDVDPlayer::OnDVDNavResult(void* pData, int iMessage)
{
  // cast the inputstream to CDVDInputStreamNavigator because this method is (should :) ) only be called
  // by libdvvdnav
  CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;

  switch (iMessage)
  {

  case DVDNAV_STILL_FRAME:
    {
      // CLog::Log(LOGDEBUG, "DVDNAV_STILL_FRAME");

      dvdnav_still_event_t *still_event = (dvdnav_still_event_t *)pData;
      // should wait the specified time here while we let the player running
      // after that call dvdnav_still_skip(m_dvdnav);

      if (m_dvd.state == DVDSTATE_STILL)
      {
        if (m_dvd.iDVDStillTime < 0xff)
        {
          if (((GetTickCount() - m_dvd.iDVDStillStartTime) / 1000) >= ((DWORD)m_dvd.iDVDStillTime))
          {
            m_dvd.iDVDStillTime = 0;
            m_dvd.state = DVDSTATE_NORMAL;
            m_dvd.iDVDStillStartTime = 0;
            pStream->SkipStill();

            return NAVRESULT_SKIPPED_STILL;
          }
          else Sleep(200);
        }
        else Sleep(10);
        return NAVRESULT_STILL_NOT_SKIPPED;
      }
      else
      {
        // else notify the player we have recieved a still frame
        CLog::Log(LOGDEBUG, "recieved still frame, waiting %i sec", still_event->length);

        m_dvd.iDVDStillTime = still_event->length;
        m_dvd.iDVDStillStartTime = GetTickCount();
        m_dvd.bDisplayedStill = false;
        m_dvd.state = DVDSTATE_STILL;
      }
    }
    break;
  case DVDNAV_SPU_CLUT_CHANGE:
    {
      CLog::Log(LOGDEBUG, "DVDNAV_SPU_CLUT_CHANGE");
      for (int i = 0; i < 16; i++)
      {
        BYTE* color = m_dvdspus.m_clut[i];
        BYTE* t = (BYTE*) & (((unsigned __int32*)pData)[i]);

        color[0] = t[2]; // Y
        color[1] = t[1]; // Cr
        color[2] = t[0]; // Cb
      }

      m_dvdspus.m_bHasClut = true;
    }
    break;
  case DVDNAV_SPU_STREAM_CHANGE:
    {
      CLog::Log(LOGDEBUG, "DVDNAV_SPU_STREAM_CHANGE");

      //Make sure we clear old overlay here, or else old forced items are left.
      m_dvdPlayerVideo.m_overlay.Clear();

      // update subtitle always in menu, or when no subtitle is selected in the movie
      if (m_dvd.iSelectedSPUStream == -1 || pStream->IsInMenu())
      {
        int iStream = pStream->GetActiveSubtitleStream();
        if (iStream >= 0) m_dvd.iSelectedSPUStream = 0x20 + iStream;
        else m_dvd.iSelectedSPUStream = -1;
      }
    }
    break;
  case DVDNAV_AUDIO_STREAM_CHANGE:
    {
      CLog::Log(LOGDEBUG, "DVDNAV_AUDIO_STREAM_CHANGE");
      
      //This should be the correct way i think, however we don't have any streams right now
      //since the demuxer hasn't started so it doesn't change. not sure how to do this.
      dvdnav_audio_stream_change_event_t* event = (dvdnav_audio_stream_change_event_t*)pData;
      
      //Tell system what audiostream should be opened by default
      if (m_iCurrentPhysicalAudioStream != event->physical)
      {
        m_iCurrentPhysicalAudioStream = event->physical;
        LockStreams();
        CloseAudioStream(false); //Close current, it will be reopened automatically
        UnlockStreams();
      }
    }
    break;
  case DVDNAV_HIGHLIGHT:
    {
      dvdnav_highlight_event_t* pInfo = (dvdnav_highlight_event_t*)pData;
      int iButton = pStream->GetCurrentButton();
      CLog::Log(LOGDEBUG, "DVDNAV_HIGHLIGHT: Highlight button %d\n", iButton);

      // set menu spu color and alpha data if there is a valid menu overlay
      DVDOverlayPicture* pOverlayPicture = m_dvdPlayerVideo.m_overlay.Get();

      if (pOverlayPicture)
      {
        //It's the last packet recieved that is of interest currently
        while( pOverlayPicture->pNext ) pOverlayPicture = pOverlayPicture->pNext;

        pStream->GetButtonInfo(pOverlayPicture, &m_dvdspus);
      }

      if (pStream->GetHighLightArea(&m_dvdPlayerVideo.m_overlay.button_i_x_start, &m_dvdPlayerVideo.m_overlay.button_i_x_end,
                                    &m_dvdPlayerVideo.m_overlay.button_i_y_start, &m_dvdPlayerVideo.m_overlay.button_i_y_end, iButton))
      {
        // need to update the display, so display the last known picture here
        // only needed if we are in a still, and when we have something else pts need to be
        // adjusted before we are allowed to update menupicture (first highlight)
        if (pStream->IsInMenu() && m_dvd.state == DVDSTATE_STILL)
        {
          m_dvdPlayerVideo.UpdateMenuPicture();
        }
      }
    }
    break;
  case DVDNAV_VTS_CHANGE:
    {
      dvdnav_vts_change_event_t* vts_change_event = (dvdnav_vts_change_event_t*)pData;
      CLog::Log(LOGDEBUG, "DVDNAV_VTS_CHANGE");

      // reset the demuxer, this also imples closing the video and the audio system
      // this is a bit tricky cause it's the demuxer that's is making this call in the end
      // so we send a message to indicate the main loop that the demuxer needs a reset
      // this also means the libdvdnav may not return any data packets after this command
      m_messenger.ResetDemuxer();
      m_bFirstHandleMessages = true;
    }
    break;
  case DVDNAV_CELL_CHANGE:
    {
      dvdnav_cell_change_event_t* cell_change_event = (dvdnav_cell_change_event_t*)pData;
      CLog::Log(LOGDEBUG, "DVDNAV_CELL_CHANGE");
      //if (cell_change_event->pgN != m_dvd.iCurrentCell)
      {
        m_dvd.iCurrentCell = cell_change_event->pgN;
        // m_iCommands |= DVDCOMMAND_SYNC;
      }
      // not implemented
    }
    break;
  case DVDNAV_NAV_PACKET:
    {
        pci_t* pci = (pci_t*)pData;

        if( pci->pci_gi.vobu_s_ptm*10 != m_dvd.iNAVPackFinish)
        {
          m_dvd.iFlagSentStart=0;
          //Only set this when we have the first non continous packet
          m_dvd.iNAVPackStart = pci->pci_gi.vobu_s_ptm*10;
        }
        m_dvd.iNAVPackFinish = pci->pci_gi.vobu_e_ptm*10;      
    }
    break;
  case DVDNAV_HOP_CHANNEL:
    {
      // This event is issued whenever a non-seamless operation has been executed.
      // Applications with fifos should drop the fifos content to speed up responsiveness.
      CLog::Log(LOGDEBUG, "DVDNAV_HOP_CHANNEL");
      //m_iCommands |= DVDCOMMAND_FLUSH;
      FlushBuffers();
    }
  case DVDNAV_STOP:
    {
      CLog::Log(LOGDEBUG, "DVDNAV_STOP");
      // not implemented
    }
    break;

    // types currently not used won't do anything
  case DVDNAV_BLOCK_OK:
  case DVDNAV_NOP:
  default:
  {}
    break;
  }
  return NAVRESULT_OK;
}

bool CDVDPlayer::OnAction(const CAction &action)
{
  if (!m_pInputStream) return false;

  if (m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD)
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;

    switch (action.wID)
    {
    case ACTION_PREV_ITEM:  // SKIP-:
      {
        CLog::DebugLog(" - pushed prev");
        m_messenger.ChapterPrevious();
        return true;
      }
      break;
    case ACTION_NEXT_ITEM:  // SKIP+:
      {
        CLog::DebugLog(" - pushed next");
        m_messenger.ChapterNext();
        return true;
      }
      break;
    case ACTION_ASPECT_RATIO:   // start button 
      {
        CLog::DebugLog(" - go to menu");
        pStream->OnMenu();
        m_dvd.state = DVDSTATE_NORMAL;
        return true;
      }
      break;
    }

    if (pStream->IsInMenu())
    {
      // if current button is out of range, default to 1
      if (pStream->GetCurrentButton() > pStream->GetTotalButtons())
      {
        // pStream->SelectButton(1); // select default button
      }

      switch (action.wID)
      {
      case  ACTION_SHOW_OSD:  // MENU
        {
          CLog::DebugLog(" - menu select");
          pStream->OnMenu();
          m_dvd.state = DVDSTATE_NORMAL;
        }
        break;
      case ACTION_PREVIOUS_MENU:
        {
          CLog::DebugLog(" - menu back");
          pStream->OnBack();
          m_dvd.state = DVDSTATE_NORMAL;
        }
        break;
      case ACTION_MOVE_LEFT:
        {
          CLog::DebugLog(" - move left");
          pStream->OnLeft();
        }
        break;
      case ACTION_MOVE_RIGHT:
        {
          CLog::DebugLog(" - move right");
          pStream->OnRight();
        }
        break;
      case ACTION_MOVE_UP:
        {
          CLog::DebugLog(" - move up");
          pStream->OnUp();
        }
        break;
      case ACTION_MOVE_DOWN:
        {
          CLog::DebugLog(" - move down");
          pStream->OnDown();
        }
        break;
      case ACTION_SELECT_ITEM:
        {
          CLog::DebugLog(" - button select");

          // todo, when in menu show the button pushed overlay

          m_dvd.iSelectedSPUStream = -1;

          pStream->ActivateButton();

          m_bReadData = false;

          // sleep until the dvd player is reading some data with read_frame
          // in case of a still we should wait until the still is drawed.
          //m_iCommands |= DVDCOMMAND_FLUSH;
          FlushBuffers(); // this will also flush the overlay's (subtitles and menus)
          m_dvd.state = DVDSTATE_NORMAL;
          while (!m_bReadData) Sleep(1);

        }
        break;
      default:
        return false;
        break;
      }
      return true; // message is handled
    }
  }

  // return false to inform the caller we didn't handle the message
  return false;
}

bool CDVDPlayer::IsInMenu() const
{
  if (m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD)
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    return pStream->IsInMenu();
  }
  return false;
}

void CDVDPlayer::LockStreams()
{
  EnterCriticalSection(&m_critStreamSection);
}

void CDVDPlayer::UnlockStreams()
{
  LeaveCriticalSection(&m_critStreamSection);
}

