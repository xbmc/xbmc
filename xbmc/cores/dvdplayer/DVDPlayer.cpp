/* TODO / BUGS
* - create a seperate subtitle class for handling subtitles (we should support seperate vob / srt files too)
*   and cleanup al SPUInfo realted code in the DVDPlayer class
* -
* - keep track of criticalsections that dll's initialize (DLL Loader)
* - when stopping a movie, the latest packet that comes from the demuxer is not freed???
*/

#include "../../stdafx.h"
#include "DVDPlayer.h"

#include "DVDInputStreams\DVDInputStream.h"
#include "DVDInputStreams\DVDFactoryInputStream.h"
#include "DVDInputStreams\DVDInputStreamNavigator.h"

#include "DVDDemuxers\DVDDemux.h"
#include "DVDDemuxers\DVDDemuxUtils.h"
#include "DVDDemuxers\DVDFactoryDemuxer.h"

#include "..\..\util.h"
#include "../../utils/GUIInfoManager.h"
#include "DVDPerformanceCounter.h"

CDVDPlayer::CDVDPlayer(IPlayerCallback& callback)
    : IPlayer(callback),
      CThread(),
      m_dvdPlayerVideo(&m_clock, &m_overlayContainer),
      m_dvdPlayerAudio(&m_clock),
      m_dvdPlayerSubtitle(&m_overlayContainer)
{
  m_pDemuxer = NULL;
  m_pInputStream = NULL;
  
  m_hReadyEvent = CreateEvent(NULL, true, false, NULL);

  InitializeCriticalSection(&m_critStreamSection);
  
  memset(&m_dvd, 0, sizeof(DVDInfo));
  m_dvd.iCurrentCell = -1;
  m_dvd.iNAVPackStart = -1;
  m_dvd.iNAVPackFinish = -1;
  m_dvd.iFlagSentStart = 0;
  m_dvd.iSelectedAudioStream = -1;
  m_dvd.iSelectedSPUStream = -1;

 
  m_filename[0] = '\0';
  m_bAbortRequest = false;  
  m_iCurrentStreamVideo = -1;
  m_iCurrentStreamAudio = -1;
  m_iCurrentStreamSubtitle = -1;
  m_bRenderSubtitle = false;
  m_bDontSkipNextFrame = false;
}

CDVDPlayer::~CDVDPlayer()
{
  CloseFile();

  CloseHandle(m_hReadyEvent);
  DeleteCriticalSection(&m_critStreamSection);
}

bool CDVDPlayer::OpenFile(const CFileItem& file, __int64 iStartTime)
{
  CStdString strFile = file.m_strPath;
  
  CLog::Log(LOGNOTICE, "DVDPlayer: Opening: %s", strFile.c_str());

  // if playing a file close it first
  // this has to be changed so we won't have to close it.
  CloseFile();

  m_bAbortRequest = false;
  m_iSpeed = 1;

  m_dvd.state = DVDSTATE_NORMAL;
  m_dvd.iSelectedSPUStream = -1;
  m_dvd.iSelectedAudioStream = -1;

  if (strFile.Find("dvd://") >= 0 ||
      strFile.CompareNoCase("d:\\video_ts\\video_ts.ifo") == 0 ||
      strFile.CompareNoCase("iso9660://video_ts/video_ts.ifo") == 0)
  {
    strcpy(m_filename, "\\Device\\Cdrom0");
  }
  else strcpy(m_filename, strFile.c_str());

  ResetEvent(m_hReadyEvent);
  Create();
  WaitForSingleObject(m_hReadyEvent, INFINITE);

  SetSubtitleVisible(g_stSettings.m_currentVideoSettings.m_SubtitleOn);

  // if we are playing a media file with pictures, we should wait for the video output device to be initialized
  // if we don't wait, the fullscreen window will init with a picture that is 0 pixels width and high
  bool bProcessThreadIsAlive = true;
  if (m_iCurrentStreamVideo >= 0 ||
      m_pInputStream && (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) || m_pInputStream->HasExtension("vob")))
  {
    while (bProcessThreadIsAlive && !m_bAbortRequest && !m_dvdPlayerVideo.InitializedOutputDevice())
    {
      bProcessThreadIsAlive = !WaitForThreadExit(0);
      Sleep(1);
    }
  }
  
  // m_bPlaying could be set to false in the meantime, which indicates an error
  return (bProcessThreadIsAlive && !m_bStop);
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
  CThread::SetName("CDVDPlayer");
  m_iCurrentStreamVideo = -1;
  m_iCurrentStreamAudio = -1;

  m_messenger.Clear();
  
  g_dvdPerformanceCounter.EnableMainPerformance(ThreadHandle());
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
    return;
  }
  
  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CLog::Log(LOGNOTICE, "DVDPlayer: playing a dvd with menu's");
  }
  
  CLog::Log(LOGNOTICE, "Creating Demuxer");
  m_pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(m_pInputStream);
  if (!m_pDemuxer || !m_pDemuxer->Open(m_pInputStream))
  {
    CLog::Log(LOGERROR, "Demuxer: Error opening, demuxer");
    // inputstream and the demuxer will be destroyed in OnExit()
    return;
  }

  // find first audio / video streams
  for (int i = 0; i < m_pDemuxer->GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(i);
    if (pStream->type == STREAM_AUDIO && audio_index < 0) audio_index = i;
    else if (pStream->type == STREAM_VIDEO && video_index < 0) video_index = i;
  }

  //m_dvdPlayerSubtitle.Init();
  //m_dvdPlayerSubtitle.FindSubtitles(m_filename);
  
  // open the streams
  if (audio_index >= 0) OpenDefaultAudioStream();
  if (video_index >= 0) OpenVideoStream(video_index);

  // if we use libdvdnav, streaminfo is not avaiable, we will find this later on
  if (m_iCurrentStreamVideo < 0 && m_iCurrentStreamAudio < 0 &&
      m_pInputStream->IsStreamType(DVDSTREAM_TYPE_FILE) &&
      !m_pInputStream->HasExtension("vob"))
  {
    CLog::Log(LOGERROR, "%s: could not open codecs\n", m_filename);
    return;
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

    
    if( m_iSpeed > 1 || m_iSpeed < 0)
    {
      bool bMenu = IsInMenu();

      // don't allow rewind in menu
      if( bMenu && m_iSpeed < 0 ) ToFFRW(1);

      if( m_iCurrentStreamVideo >= 0 
        && m_dvdPlayerVideo.GetCurrentPts() != DVD_NOPTS_VALUE 
        && !bMenu )
      { 

        // check how much off clock video is when ff/rw:ing
        // a problem here is that seeking isn't very accurate
        // and since the clock will be resynced after seek
        // we might actually not really be playing at the wanted 
        // speed. we'd need to have some way to not resync the clock 
        // after a seek to remember timing. still need to handle 
        // discontinuities somehow
        // when seeking, give the player a headstart of 1 second to make sure the time it takes 
        // to seek doesn't make a difference. 
        __int64 iError = m_clock.GetClock() - m_dvdPlayerVideo.GetCurrentPts();

        if( m_iSpeed > 0 && iError > DVD_SEC_TO_TIME(1*m_iSpeed) )
        {
          CLog::Log(LOGDEBUG, "CDVDPlayer::Process - FF Seeking to catch up");        
          SeekTime( GetTime() + 1000*m_iSpeed);
        }
        else if( m_iSpeed < 0 && iError < DVD_SEC_TO_TIME(1*m_iSpeed) )
        {        
          SeekTime( GetTime() + 1000*m_iSpeed);
        }      
        m_bDontSkipNextFrame = true;
      }
    }

      
    // handle messages send to this thread, like seek or demuxer reset requests
    HandleMessages();
    m_bReadAgain = false;
  
    // read a data frame from stream.
    CDVDDemux::DemuxPacket* pPacket = m_pDemuxer->Read();

    // in a read action, the dvd navigator can do certain actions that require
    // us to read again
    if (m_bReadAgain)
    {
      if (pPacket)
      {
        CDVDDemuxUtils::FreeDemuxPacket(pPacket);
      }
      continue;
    }

    if (!pPacket)
    {            
      if (m_dvd.state == DVDSTATE_STILL) continue;
      else if( m_pInputStream->IsEOF() ) break;
      else 
      { 
        // keep on trying until user wants us to stop.
        iErrorCounter++;
        CLog::Log(LOGERROR, "Error reading data from demuxer");

        // maybe reseting the demuxer at this point would be a good idea, should it have failed in some way.        
        if( iErrorCounter > 1000 )
        {
          m_pDemuxer->Reset();
          iErrorCounter = 0;
        }
        
        continue;
      }
    }

    iErrorCounter = 0;

    // process subtitles
    if (pPacket->dts != DVD_NOPTS_VALUE)
    {
      //m_dvdPlayerSubtitle.Process(pPacket->dts);
    }
        

    CDemuxStream *pStream = m_pDemuxer->GetStream(pPacket->iStreamId);    

    
    if (m_pInputStream && pStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    {      
      // Stream selection for DVD's this 
      // should probably come as messages in the packet instead      
  
      int iPhysicalId = pStream->iPhysicalId;

      if( pStream->type == STREAM_SUBTITLE
       && iPhysicalId == m_dvd.iSelectedSPUStream       
       && pStream->iId != m_iCurrentStreamSubtitle )
      {
        // dvd subtitle stream changed
        m_iCurrentStreamSubtitle = pStream->iId;
      }
      if( pStream->type == STREAM_AUDIO
       && iPhysicalId == m_dvd.iSelectedAudioStream       
       && pStream->iId != m_iCurrentStreamAudio )
      {
        // dvd audio stream changed
        CloseAudioStream( true ); // this should wait here in the normal case, however if user changes from osd one should wait
        if( OpenAudioStream( pStream->iId ) )
        {
          if( m_iCurrentStreamVideo >= 0 )
          { // up until now we wheren't playing audio, but we did play video
            // this will change what is used to sync the dvdclock. 
            // since the new audio data doesn't have to have any relation
            // to the current video data in the packet que, we have to 
            // wait for it to empty
                      
            // this happens if a new cell has audio data, but previous didn't
            // and both have video data
            
            m_dvdPlayerVideo.m_packetQueue.WaitUntilEmpty();
          }
        }
      }
    }

    // TODO: Move default opening of first available stream to here
    //       If a dvd doesn't select any subtitle/audio stream to begin
    //       with, first available stream will be opened as it is now
    //       even thou the dvd as selected that none should be opened
    //       yet. 

    int iNrOfStreams = m_pDemuxer->GetNrOfStreams();
    if (m_iCurrentStreamAudio >= iNrOfStreams) CloseAudioStream(false);
    if (m_iCurrentStreamVideo >= iNrOfStreams) CloseVideoStream(false);
    if (m_iCurrentStreamSubtitle >= iNrOfStreams) m_iCurrentStreamSubtitle = -1;

    LockStreams();
    {
      if (pPacket->iStreamId == m_iCurrentStreamAudio ||
          (m_iCurrentStreamAudio < 0 && pStream->type == STREAM_AUDIO))
      {
        ProcessAudioData(pStream, pPacket);
      }
      else if (pPacket->iStreamId == m_iCurrentStreamVideo ||
              (m_iCurrentStreamVideo < 0 && pStream->type == STREAM_VIDEO))
      {
        ProcessVideoData(pStream, pPacket);
      }
      else if (pPacket->iStreamId == m_iCurrentStreamSubtitle ||
              (m_iCurrentStreamSubtitle < 0 && pStream->type == STREAM_SUBTITLE))
      {
        ProcessSubData(pStream, pPacket);
      }

      else CDVDDemuxUtils::FreeDemuxPacket(pPacket); // free it since we won't do anything with it
    }
    UnlockStreams();
  }
}

void CDVDPlayer::ProcessAudioData(CDemuxStream* pStream, CDVDDemux::DemuxPacket* pPacket)
{
  int iPacketMessage = 0;
  
  // if we have no audio stream yet, openup the audio device now
  if (m_iCurrentStreamAudio < 0) OpenAudioStream( pPacket->iStreamId );
  else
  {
    CDemuxStreamAudio* pStreamAudio = (CDemuxStreamAudio*)pStream;

    // if audio information changed close the audio device first
    if (m_dvdPlayerAudio.m_codec != pStreamAudio->codec ||
      m_dvdPlayerAudio.m_iSourceChannels != pStreamAudio->iChannels) 
    {
      CloseAudioStream(true);
      OpenAudioStream( pPacket->iStreamId );
    }
  }

  //If this is the first packet after a discontinuity, send it as a resync
  if( !(m_dvd.iFlagSentStart & 1) ) 
  {
    m_dvd.iFlagSentStart |= 1;
    iPacketMessage |= DVDPACKET_MESSAGE_RESYNC;
  }

  if (m_iCurrentStreamAudio >= 0)
    m_dvdPlayerAudio.m_packetQueue.Put(pPacket, (void*)iPacketMessage);
  else 
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

void CDVDPlayer::ProcessVideoData(CDemuxStream* pStream, CDVDDemux::DemuxPacket* pPacket)
{
  int iPacketMessage = 0;
  
  // if we have no video stream yet, openup the video device now
  if (m_iCurrentStreamVideo < 0) OpenVideoStream(pPacket->iStreamId);

  if (m_bDontSkipNextFrame)
  {
    iPacketMessage |= DVDPACKET_MESSAGE_NOSKIP;
    m_bDontSkipNextFrame = false;
  }
  
  //If this is the first packet after a discontinuity, send it as a resync
  if( !(m_dvd.iFlagSentStart & 1) ) 
  {
    m_dvd.iFlagSentStart |= 1;
    iPacketMessage |= DVDPACKET_MESSAGE_RESYNC;
  }

  if (m_iCurrentStreamVideo >= 0)
    m_dvdPlayerVideo.m_packetQueue.Put(pPacket, (void*)iPacketMessage);
  else 
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

void CDVDPlayer::ProcessSubData(CDemuxStream* pStream, CDVDDemux::DemuxPacket* pPacket)
{  
  // if no subtitle stream is selected, select this one
  if( m_iCurrentStreamSubtitle < 0 ) 
    m_iCurrentStreamSubtitle = pStream->iId;

  if (pStream->codec == 0x17000) //CODEC_ID_DVD_SUBTITLE)
  {
    CSPUInfo* pSPUInfo = m_dvdspus.AddData(pPacket->pData, pPacket->iSize, pPacket->pts);

    if (pSPUInfo)
    {
      CLog::Log(LOGDEBUG, "CDVDPlayer::ProcessSubData: Got complete SPU packet");

      m_overlayContainer.Add(pSPUInfo);

      if (pSPUInfo->bForced)
      {
        // recieved new menu overlay (button), retrieve cropping information
        UpdateOverlayInfo(LIBDVDNAV_BUTTON_NORMAL);
      }
    }
  }
  
  // free it, content is already copied into a special structure
  CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}
  
void CDVDPlayer::OnExit()
{
  g_dvdPerformanceCounter.DisableMainPerformance();
  
  CLog::Log(LOGNOTICE, "CDVDPlayer::OnExit()");
  
  // close each stream
  if (!m_bAbortRequest) CLog::Log(LOGNOTICE, "DVDPlayer: eof, waiting for queues to empty");
  if (m_iCurrentStreamAudio >= 0)
  {
    CLog::Log(LOGNOTICE, "DVDPlayer: closing audio stream");
    CloseAudioStream(!m_bAbortRequest);
  }
  if (m_iCurrentStreamVideo >= 0)
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

  // close subtitle stuff
  CLog::Log(LOGNOTICE, "CDVDPlayer::OnExit() deiniting subtitle handler");
  m_dvdPlayerSubtitle.DeInit();
  
  // if we didn't stop playing, advance to the next item in xbmc's playlist
  if (!m_bAbortRequest) m_callback.OnPlayBackEnded();

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
        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD)) 
        {
          // We can't seek in a menu
          if (!IsInMenu())
          {
            // need to get the seek based on file positition working in CDVDInputStreamNavigator
            // so that demuxers can control the stream (seeking in this case)
            // for now use time based seeking
            CLog::Log(LOGDEBUG, "CDVDInputStreamNavigator seek to: %d", pMessage->iValue);
            if (((CDVDInputStreamNavigator*)m_pInputStream)->Seek(pMessage->iValue))
            {
              CLog::Log(LOGDEBUG, "CDVDInputStreamNavigator seek to: %d, succes", pMessage->iValue);
              FlushBuffers();
            }
            else CLog::Log(LOGWARNING, "error while seeking");
          }
          else CLog::Log(LOGWARNING, "can't seek in a menu");
        }
        else
        {
          CLog::Log(LOGDEBUG, "demuxer seek to: %d", pMessage->iValue);
          if (m_pDemuxer->Seek(pMessage->iValue)) 
          {
            CLog::Log(LOGDEBUG, "demuxer seek to: %d, succes", pMessage->iValue);
            FlushBuffers();
          }
          else CLog::Log(LOGWARNING, "error while seeking");
        }
        
        // set flag to indicate we have finished a seeking request
        g_infoManager.m_performingSeek = false;
      }
      break;
    case DVDMESSAGE_RESET_DEMUXER:
      {
        // after the reset our m_pCurrentStreams will be invalid, so these should
        // be closed anyway (we should wait for the buffers though)
        CloseAudioStream(true);
        CloseVideoStream(true);

        // we need to reset the demuxer, probably because the streams have changed
        m_pDemuxer->Reset();
      }
      break;
    case DVDMESSAGE_SET_AUDIO:
      {        
        CDemuxStream* pStream = m_pDemuxer->GetStreamFromAudioId(pMessage->iValue);
        if( pStream )
        {
          LockStreams();
          
          CloseAudioStream(false);
          OpenAudioStream(pStream->iId);

          UnlockStreams();
        }
      }
      break;
    case DVDMESSAGE_SET_SUBTITLE:
      {
        CDemuxStream* pStream = m_pDemuxer->GetStreamFromSubtitleId(pMessage->iValue);
        if( pStream )
        {          
          m_iCurrentStreamSubtitle = pStream->iId;
        }
      }
      break;
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
    m_clock.Resume();
    m_dvdPlayerVideo.Resume();
    m_dvdPlayerAudio.Resume();
  }
  else
  {
    m_dvdPlayerAudio.Pause(); // XXX, this won't work for ffwd or ffrw
    m_dvdPlayerVideo.Pause();
    m_clock.Pause();
  }
}

bool CDVDPlayer::IsPaused() const
{
  return (m_iSpeed == 0);
}

bool CDVDPlayer::HasVideo()
{
  if (m_pInputStream)
  {
    if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) || m_iCurrentStreamVideo >= 0) return true;
  }
  return false;
}

bool CDVDPlayer::HasAudio()
{
  return (m_iCurrentStreamAudio >= 0);
}

void CDVDPlayer::SwitchToNextLanguage()
{}

void CDVDPlayer::ToggleSubtitles()
{
  SetSubtitleVisible(!GetSubtitleVisible());
  //m_bRenderSubtitle = !m_bRenderSubtitle;
}

void CDVDPlayer::SubtitleOffset(bool bPlus)
{
}

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
{
}

void CDVDPlayer::GetAudioInfo(CStdString& strAudioInfo)
{
  if (!m_bStop && m_iCurrentStreamAudio >= 0)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(m_iCurrentStreamAudio);
    if (pStream && pStream->type == STREAM_AUDIO)
    {
      string strDemuxerInfo;
      ((CDemuxStreamAudio*)pStream)->GetStreamInfo(strDemuxerInfo);

      int bsize = m_dvdPlayerAudio.m_packetQueue.GetSize();
      if (bsize > 0) bsize = (int)(((double)m_dvdPlayerAudio.m_packetQueue.GetSize() / m_dvdPlayerAudio.m_packetQueue.GetMaxSize()) * 100);
      if (bsize > 99) bsize = 99;

      strAudioInfo.Format("%s, aq size: %i", strDemuxerInfo.c_str(), bsize);
    }
  }
}

void CDVDPlayer::GetVideoInfo(CStdString& strVideoInfo)
{
  if (!m_bStop && m_iCurrentStreamVideo >= 0)
  {
    string strDemuxerInfo;
    CDemuxStream* pStream = m_pDemuxer->GetStream(m_iCurrentStreamVideo);
    if (pStream && pStream->type == STREAM_VIDEO)
    {
      ((CDemuxStreamVideo*)pStream)->GetStreamInfo(strDemuxerInfo);

      int bsize = m_dvdPlayerVideo.m_packetQueue.GetSize();
      if (bsize > 0) bsize = (int)(((double)m_dvdPlayerVideo.m_packetQueue.GetSize() / m_dvdPlayerVideo.m_packetQueue.GetMaxSize()) * 100);
      if (bsize > 99) bsize = 99;

      strVideoInfo.Format("%s, vq size: %i", strDemuxerInfo.c_str(), bsize);
    }
  }
}

void CDVDPlayer::GetGeneralInfo(CStdString& strGeneralInfo)
{
  if (!m_bStop)
  {
    double dDelay = (double)m_dvdPlayerVideo.GetDelay() / DVD_TIME_BASE;
    double dDiff = (double)m_dvdPlayerVideo.GetDiff() / DVD_TIME_BASE;
    int iFramesDropped = m_dvdPlayerVideo.GetNrOfDroppedFrames();
    
    strGeneralInfo.Format("DVD Player ad:%6.3f, diff:%6.3f, dropped:%d", dDelay, dDiff, iFramesDropped);
  }
}

void CDVDPlayer::AudioOffset(bool bPlus)
{
}

void CDVDPlayer::SwitchToNextAudioLanguage()
{
}

void CDVDPlayer::SeekPercentage(float iPercent)
{

  __int64 iTotalMsec = GetTotalTimeInMsec();
  __int64 iTime = (__int64)(iTotalMsec * iPercent / 100);

  SeekTime(iTime);
}

float CDVDPlayer::GetPercentage()
{
  __int64 iTotalTime = GetTotalTimeInMsec();

  if (iTotalTime != 0)
  {
    return GetTime() * 100 / (float)iTotalTime;
  }

  return 0.0f;
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
{
}

float CDVDPlayer::GetSubTitleDelay()
{
  return 0.0;
}

// priority: 1: libdvdnav, 2: external subtitles, 3: muxed subtitles
int CDVDPlayer::GetSubtitleCount()
{
  // for dvd's we get the info from libdvdnav
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    return pStream->GetSubTitleStreamCount();
  }
  
  // for external subtitle files
  // if (m_subtitle.vecSubtitleFiles.size() > 0) return m_subtitle.vecSubtitleFiles.size();
  
  // last one is the demuxer
  if (m_pDemuxer) return m_pDemuxer->GetNrOfSubtitleStreams();

  return 0;
}

int CDVDPlayer::GetSubtitle()
{
  // libdvdnav
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {    
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    return pStream->GetActiveSubtitleStream();
  }
  
  // external subtitles   
  else 
  {
    int iSubtitleStreams = m_pDemuxer->GetNrOfSubtitleStreams();    
    for (int i = 0; i < iSubtitleStreams; i++)
    {
      CDemuxStream* pStream = m_pDemuxer->GetStreamFromSubtitleId(i);
      if (pStream && pStream->iId == m_iCurrentStreamSubtitle) return i;
    }
  }
  return -1;
}

void CDVDPlayer::GetSubtitleName(int iStream, CStdString &strStreamName)
{
  strStreamName.Format("%d. ", iStream);
  CDemuxStream* pStream = m_pDemuxer->GetStreamFromSubtitleId(iStream);

  if( !pStream )
  {
    CLog::Log(LOGWARNING, "CDVDPlayer::GetSubtitleName: Invalid stream: id %i", iStream);
    strStreamName += " (Invalid)";
    return;
  }

  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    strStreamName += pStream->GetSubtitleStreamLanguage(iStream);
  }
  else 
  {
    CStdString strName;
    pStream->GetStreamName(strName);    
    strStreamName += strName;
  }
}

void CDVDPlayer::SetSubtitle(int iStream)
{
  // for dvd's we set it using the navigater, it will then send a change subtitle to the dvdplayer
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    pStream->SetActiveSubtitleStream(iStream);
  }
  else if (m_pDemuxer)
  {
    m_messenger.SetSubtitle(iStream);
  }
}

bool CDVDPlayer::GetSubtitleVisible()
{
  //if (!m_bRenderSubtitle) return false;

  //int id = m_dvd.iSelectedSPUStream - 0x20;
  //if (id >= 0 && id <= 0x3f) return true;

  //return false;
  
  return m_bRenderSubtitle;
}

void CDVDPlayer::SetSubtitleVisible(bool bVisible)
{
  g_stSettings.m_currentVideoSettings.m_SubtitleOn = bVisible;
  m_bRenderSubtitle = bVisible;
  
  // maybe it is better to let m_dvdPlayerVideo use a callback to get this value
  m_dvdPlayerVideo.EnableSubtitle(bVisible);

  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    pStream->EnableSubtitleStream(bVisible);
  }
}

int CDVDPlayer::GetAudioStreamCount()
{
  // for dvd's we get the information from libdvdnav
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    return pStream->GetAudioStreamCount();
  }
  else if (m_pDemuxer)
  {
    return m_pDemuxer->GetNrOfAudioStreams();
  }
  return 0;
}

int CDVDPlayer::GetAudioStream()
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    return pStream->GetActiveAudioStream();
  }
  else
  {
    int iAudioStreams = m_pDemuxer->GetNrOfAudioStreams();    
    for (int i = 0; i < iAudioStreams; i++)
    {
      CDemuxStream* pStream = m_pDemuxer->GetStreamFromAudioId(i);
      if (pStream && pStream->iId == m_iCurrentStreamAudio) return i;
    }
  }
  return -1;
}

void CDVDPlayer::GetAudioStreamName(int iStream, CStdString& strStreamName)
{
  strStreamName.Format("%d. ", iStream);  
  CDemuxStreamAudio* pStream = m_pDemuxer->GetStreamFromAudioId(iStream);

  if( !pStream )
  {
    CLog::Log(LOGWARNING, "CDVDPlayer::GetAudioStreamName: Invalid stream: id %i", iStream);
    strStreamName += " (Invalid)";
    return;
  }

  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    strStreamName += pStream->GetAudioStreamLanguage(iStream);    
  }
  else 
  {
    CStdString strName;
    pStream->GetStreamName(strName);    
    strStreamName += strName;
  }
  
  std::string strType;
  pStream->GetStreamType(strType);
  if( strType.length() > 0 )
  {
    strStreamName += " - ";
    strStreamName += strType;
  }
}

void CDVDPlayer::SetAudioStream(int iStream)
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    //This will send an event that audiostream was changed
    CDVDInputStreamNavigator* pInput = (CDVDInputStreamNavigator*)m_pInputStream;
    pInput->SetActiveAudioStream(iStream);
  }
  else
  {
    m_messenger.SetAudio(iStream);
  }
}

void CDVDPlayer::SeekTime(__int64 iTime)
{
  if (m_pDemuxer) m_messenger.Seek((int)iTime);
}

// return the time in milliseconds
__int64 CDVDPlayer::GetTime()
{
  // get timing and seeking from libdvdnav for dvd's
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    return ((CDVDInputStreamNavigator*)m_pInputStream)->GetTime(); // we should take our buffers into account
  }

  int iMsecs = (int)(m_clock.GetClock() / (DVD_TIME_BASE / 1000));
  //if (m_pDemuxer)
  //{
  //  int iMsecsStart = m_pDemuxer->GetStreamStart();
  //  if (iMsecs > iMsecsStart) iMsecs -=iMsecsStart;
  //}
    
  return iMsecs;
}

// return length in msec
__int64 CDVDPlayer::GetTotalTimeInMsec()
{
  // get timing and seeking from libdvdnav for dvd's
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    return ((CDVDInputStreamNavigator*)m_pInputStream)->GetTotalTime(); // we should take our buffers into account
  }

  if (m_pDemuxer) return m_pDemuxer->GetStreamLenght();
  return 0;

}

// return length in seconds.. this should be changed to return in milleseconds throughout xbmc
int CDVDPlayer::GetTotalTime()
{
  return (int)(GetTotalTimeInMsec() / 1000);
}

void CDVDPlayer::ToFFRW(int iSpeed)
{
  // can't rewind in menu as seeking isn't possible
  // forward is fine
  if( iSpeed < 0 && IsInMenu() ) return;

  // current way this is done.
  // audioplayer, stops outputing audio to audiorendere, but still tries to 
  // sleep an correct amount for each packet
  // videoplayer just plays faster after the clock speed has been increased
  // 1. disable audio
  // 2. skip frames and adjust their pts or the clock
  m_iSpeed = iSpeed;
  m_dvdPlayerAudio.SetSpeed(iSpeed);
  m_dvdPlayerVideo.SetSpeed(iSpeed);
  m_clock.SetSpeed(iSpeed);
}

void CDVDPlayer::ShowOSD(bool bOnoff)
{}

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
      if (m_dvd.iSelectedAudioStream < 0 || pStream->iPhysicalId == m_dvd.iSelectedAudioStream)
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
  
  if( !pStreamAudio || pStreamAudio->disabled )
    return false;

  if (m_dvdPlayerAudio.OpenStream(pStreamAudio))
  {
    m_dvdPlayerAudio.SetPriority(THREAD_PRIORITY_HIGHEST + 4);

    m_iCurrentStreamAudio = iStream;
    return true;
  }
  
  // mark stream as disabled, to disallaw further attempts
  CLog::Log(LOGWARNING, "CDVDPlayer::OpenAudioStream - Unsupported stream %d. Stream disabled.", iStream);
  pStreamAudio->disabled = true;

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

  if( !pStreamVideo || pStreamVideo->disabled )
    return false;

  if (m_dvdPlayerVideo.OpenStream(pStreamVideo))
  {
    m_dvdPlayerVideo.SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);

    m_iCurrentStreamVideo = iStream;
    return true;
  }

  // mark stream as disabled, to disallaw further attempts
  CLog::Log(LOGWARNING, "CDVDPlayer::OpenVideoStream - Unsupported stream %d. Stream disabled.", iStream);
  pStreamVideo->disabled = true;

  return false;
}

bool CDVDPlayer::CloseAudioStream(bool bWaitForBuffers)
{
  CLog::Log(LOGNOTICE, "Closing audio stream");
  
  LockStreams();
  // cannot close the stream if it does not exist
  if (m_iCurrentStreamAudio >= 0)
  {
    m_dvdPlayerAudio.CloseStream(bWaitForBuffers);

    m_iCurrentStreamAudio = -1;
  }
  UnlockStreams();
  
  return true;
}

bool CDVDPlayer::CloseVideoStream(bool bWaitForBuffers) // bWaitForBuffers currently not used
{
  CLog::Log(LOGNOTICE, "Closing video stream");

  if (m_iCurrentStreamVideo < 0) return false; // can't close stream if the stream does not exist

  m_dvdPlayerVideo.CloseStream(bWaitForBuffers);

  m_iCurrentStreamVideo = -1;

  return true;
}

void CDVDPlayer::FlushBuffers()
{
  //m_pDemuxer->Flush();
  m_dvdPlayerAudio.Flush();
  m_dvdPlayerVideo.Flush();
  //m_dvdPlayerSubtitle.Flush();
  
  // clear subtitle and menu overlays
  m_overlayContainer.Clear(); 
  
  m_dvd.iFlagSentStart = 0; //We will have a discontinuity here
  
  //m_bReadAgain = true; // XXX
  // this makes sure a new packet is read
}

// since we call ffmpeg functions to decode, this is being called in the same thread as ::Process() is
int CDVDPlayer::OnDVDNavResult(void* pData, int iMessage)
{
  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;

    switch (iMessage)
    {
    case DVDNAV_STILL_FRAME:
      {
        //CLog::Log(LOGDEBUG, "DVDNAV_STILL_FRAME");

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

        dvdnav_spu_stream_change_event_t* event = (dvdnav_spu_stream_change_event_t*)pData;

        int iStream = event->physical_wide;
        if (!(iStream & 0x80) && !IsInMenu())
        {
          // subtitles are enabled in the dvd, do so in the osd if we are in the main movie
          g_stSettings.m_currentVideoSettings.m_SubtitleOn = true;
          m_bRenderSubtitle = true;
          m_dvdPlayerVideo.EnableSubtitle(true);
        }
        else
        {
          // not in main movie, or subtitles are disabled
          g_stSettings.m_currentVideoSettings.m_SubtitleOn = false;
          m_bRenderSubtitle = false;
          m_dvdPlayerVideo.EnableSubtitle(false);
        }

        if (iStream >= 0) m_dvd.iSelectedSPUStream = (iStream & ~0x80);
        else m_dvd.iSelectedSPUStream = -1;
      }
      break;
    case DVDNAV_AUDIO_STREAM_CHANGE:
      {
        CLog::Log(LOGDEBUG, "DVDNAV_AUDIO_STREAM_CHANGE");
        
        // This should be the correct way i think, however we don't have any streams right now
        // since the demuxer hasn't started so it doesn't change. not sure how to do this.
        dvdnav_audio_stream_change_event_t* event = (dvdnav_audio_stream_change_event_t*)pData;
        
        // Tell system what audiostream should be opened by default
        if (m_dvd.iSelectedAudioStream != event->physical)
        {
          m_dvd.iSelectedAudioStream = event->physical;
          
          // Close current audio stream, it will be reopened automatically
          CloseAudioStream(false); 
        }
      }
      break;
    case DVDNAV_HIGHLIGHT:
      {
        dvdnav_highlight_event_t* pInfo = (dvdnav_highlight_event_t*)pData;
        int iButton = pStream->GetCurrentButton();
        CLog::Log(LOGDEBUG, "DVDNAV_HIGHLIGHT: Highlight button %d\n", iButton);

        UpdateOverlayInfo(LIBDVDNAV_BUTTON_NORMAL);
      }
      break;
    case DVDNAV_VTS_CHANGE:
      {
        dvdnav_vts_change_event_t* vts_change_event = (dvdnav_vts_change_event_t*)pData;
        CLog::Log(LOGDEBUG, "DVDNAV_VTS_CHANGE");

        //Make sure we clear all the old overlays here, or else old forced items are left.
        m_overlayContainer.Clear();
        
        // reset the demuxer, this also imples closing the video and the audio system
        // this is a bit tricky cause it's the demuxer that's is making this call in the end
        // so we send a message to indicate the main loop that the demuxer needs a reset
        // this also means the libdvdnav may not return any data packets after this command
        m_messenger.ResetDemuxer();
        m_bReadAgain = true;

        //Force an aspect ratio that is set in the dvdheaders if available
        //techinally wrong place to do, should really be done when next video packet 
        //is decoded.. but won't cause too many problems
        m_dvdPlayerVideo.SetAspectRatio(pStream->GetVideoAspectRatio());      
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
        
        m_dvd.state = DVDSTATE_NORMAL;

        m_bDontSkipNextFrame = true;
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

        FlushBuffers();
        //m_bReadAgain = true;
      }
      break;
    case DVDNAV_STOP:
      {
        CLog::Log(LOGDEBUG, "DVDNAV_STOP");
        m_dvd.state = DVDSTATE_NORMAL;
      }
      break;
    default:
    {}
      break;
    }
  }
  return NAVRESULT_OK;
}

bool CDVDPlayer::OnAction(const CAction &action)
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;

    switch (action.wID)
    {
    case ACTION_PREV_ITEM:  // SKIP-:
      {
        CLog::Log(LOGDEBUG, " - pushed prev");
        pStream->OnPrevious();
        return true;
      }
      break;
    case ACTION_NEXT_ITEM:  // SKIP+:
      {
        CLog::Log(LOGDEBUG, " - pushed next");
        pStream->OnNext();
        return true;
      }
      break;
    case ACTION_SHOW_VIDEOMENU:   // start button 
      {
        CLog::Log(LOGDEBUG, " - go to menu");
        pStream->OnMenu();
        return true;
      }
      break;
    }

    if (pStream->IsInMenu())
    {
      switch (action.wID)
      {
      case ACTION_PREVIOUS_MENU:
        {
          CLog::Log(LOGDEBUG, " - menu back");
          pStream->OnBack();
        }
        break;
      case ACTION_MOVE_LEFT:
        {
          CLog::Log(LOGDEBUG, " - move left");
          pStream->OnLeft();
        }
        break;
      case ACTION_MOVE_RIGHT:
        {
          CLog::Log(LOGDEBUG, " - move right");
          pStream->OnRight();
        }
        break;
      case ACTION_MOVE_UP:
        {
          CLog::Log(LOGDEBUG, " - move up");
          pStream->OnUp();
        }
        break;
      case ACTION_MOVE_DOWN:
        {
          CLog::Log(LOGDEBUG, " - move down");
          pStream->OnDown();
        }
        break;
      case ACTION_SELECT_ITEM:
        {
          CLog::Log(LOGDEBUG, " - button select");

          // show button pushed overlay
          UpdateOverlayInfo(LIBDVDNAV_BUTTON_CLICKED);
        
          m_dvd.iSelectedSPUStream = -1;

          pStream->ActivateButton();
        }
        break;
      case REMOTE_0:
      case REMOTE_1:
      case REMOTE_2:
      case REMOTE_3:
      case REMOTE_4:
      case REMOTE_5:
      case REMOTE_6:
      case REMOTE_7:
      case REMOTE_8:
      case REMOTE_9:
        {
          // Offset from key codes back to button number
          int button = action.wID - REMOTE_0;
          CLog::Log(LOGDEBUG, " - button pressed %d", button);
          pStream->SelectButton(button);
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
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    return pStream->IsInMenu();
  }
  return false;
}

/*
 * iAction should be LIBDVDNAV_BUTTON_NORMAL or LIBDVDNAV_BUTTON_CLICKED
 */
void CDVDPlayer::UpdateOverlayInfo(int iAction)
{
  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;

    VecOverlays* pVecOverlays = m_overlayContainer.GetOverlays();
    
    //It's the last packet recieved that is of interest currently
    if (pVecOverlays->size() > 0)
    {
      CDVDOverlay* pOverlay = pVecOverlays->back();

      if (pOverlay->IsOverlayType(DVDOVERLAY_TYPE_SPU))
      {
        CDVDOverlaySpu* pOverlaySpu = (CDVDOverlaySpu*)pOverlay;
      
        // make sure its a forced (menu) overlay
        if (pOverlaySpu->bForced)
        {
          // set menu spu color and alpha data if there is a valid menu overlay
          pStream->GetCurrentButtonInfo(pOverlaySpu, &m_dvdspus, iAction);
        }
      }
      else
      {
        CLog::Log(LOGERROR, "CDVDPlayer::UpdateOverlayInfo : last overlay container is not of a dvd SPU type, unable to update overlay info");
      }
    }
  }
}

bool CDVDPlayer::HasMenu()
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    return true;
  else
    return false;
}

//IChapterProvider* CDVDPlayer::GetChapterProvider()
//{
//  if (m_pInputStream && m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD)
//  {
//    m_chapterReader.SetFile(m_filename);
//    m_chapterReader.SetCurrentTitle(1);
//    return (&m_chapterReader);
//  }
//  return NULL;
//}

bool CDVDPlayer::GetCurrentSubtitle(CStdStringW& strSubtitle)
{
  __int64 pts = m_dvdPlayerVideo.GetCurrentPts();
  bool bGotSubtitle = m_dvdPlayerSubtitle.GetCurrentSubtitle(strSubtitle, pts);
  return bGotSubtitle;
}
