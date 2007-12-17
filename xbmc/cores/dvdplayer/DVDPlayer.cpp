
#include "stdafx.h"
#include "DVDPlayer.h"

#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDInputStreams/DVDInputStreamNavigator.h"

#include "DVDDemuxers/DVDDemux.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDDemuxers/DVDFactoryDemuxer.h"
#include "DVDDemuxers/DVDDemuxFFmpeg.h"

#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDFactoryCodec.h"

#include "../../Util.h"
#include "../../utils/GUIInfoManager.h"
#include "DVDPerformanceCounter.h"

#include "../../FileSystem/cdioSupport.h"

#include "../../Picture.h"
#include "../ffmpeg/DllSwScale.h"

#include "../../xbmc/utils/PerformanceSample.h"

CDVDPlayer::CDVDPlayer(IPlayerCallback& callback)
    : IPlayer(callback),
      CThread(),
      m_dvdPlayerVideo(&m_clock, &m_overlayContainer),
      m_dvdPlayerAudio(&m_clock),
      m_dvdPlayerSubtitle(&m_overlayContainer)
{
  m_pDemuxer = NULL;
  m_pInputStream = NULL;

  m_tmLastSeek = time(NULL);

  m_hReadyEvent = CreateEvent(NULL, true, false, NULL);

  InitializeCriticalSection(&m_critStreamSection);

  memset(&m_dvd, 0, sizeof(DVDInfo));
  m_dvd.iSelectedAudioStream = -1;
  m_dvd.iSelectedSPUStream = -1;


  m_bAbortRequest = false;

  m_CurrentAudio.Clear();
  m_CurrentVideo.Clear();
  m_CurrentSubtitle.Clear();
  
  m_playSpeed = DVD_PLAYSPEED_NORMAL;
  m_caching = false;
#ifdef DVDDEBUG_MESSAGE_TRACKER
  g_dvdMessageTracker.Init();
#endif
}

CDVDPlayer::~CDVDPlayer()
{
  CloseFile();

  CloseHandle(m_hReadyEvent);
  DeleteCriticalSection(&m_critStreamSection);
#ifdef DVDDEBUG_MESSAGE_TRACKER
  g_dvdMessageTracker.DeInit();
#endif
}

bool CDVDPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  try
  {
    CStdString strFile = file.m_strPath;

    CLog::Log(LOGNOTICE, "DVDPlayer: Opening: %s", strFile.c_str());

    // if playing a file close it first
    // this has to be changed so we won't have to close it.
    CloseFile();

    m_bAbortRequest = false;
    SetPlaySpeed(DVD_PLAYSPEED_NORMAL);

    m_dvd.state = DVDSTATE_NORMAL;
    m_dvd.iSelectedSPUStream = -1;
    m_dvd.iSelectedAudioStream = -1;

    // settings that should be set before opening the file
    SetAVDelay(g_stSettings.m_currentVideoSettings.m_AudioDelay);
    
    if (strFile.Find("dvd://") >= 0 ||
        strFile.CompareNoCase("d:\\video_ts\\video_ts.ifo") == 0 ||
        strFile.CompareNoCase("iso9660://video_ts/video_ts.ifo") == 0)
    {
#ifdef _LINUX
      m_filename = MEDIA_DETECT::CCdIoSupport::GetDeviceFileName();
#else
      m_filename = "\\Device\\Cdrom0";
#endif
    }
    else 
      m_filename = strFile;

    m_content = file.GetContentType();

    /* otherwise player will think we need to be restarted */
    g_stSettings.m_currentVideoSettings.m_SubtitleCached = true;

    /* allow renderer to switch to fullscreen if requested */
    m_dvdPlayerVideo.EnableFullscreen(options.fullscreen);

    ResetEvent(m_hReadyEvent);
    Create();
    WaitForSingleObject(m_hReadyEvent, INFINITE);

    // Playback might have been stopped due to some error
    if (m_bStop) return false;

    /* check if we got a full dvd state, then use that */
    if( options.state.size() > 0 && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
      SetPlayerState(options.state);
    else if( options.starttime > 0 )
      SeekTime( (__int64)(options.starttime * 1000) );
    
    // settings that can only be set after the inputstream, demuxer and or codecs are opened
    SetSubtitleVisible(g_stSettings.m_currentVideoSettings.m_SubtitleOn);
   
    return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown on open", __FUNCTION__);
    return false;
  }
}

bool CDVDPlayer::CloseFile()
{
  CLog::Log(LOGNOTICE, "CDVDPlayer::CloseFile()");

  // set the abort request so that other threads can finish up
  m_bAbortRequest = true;

  // unpause the player
  SetPlaySpeed(DVD_PLAYSPEED_NORMAL);

  // tell demuxer to abort
  if(m_pDemuxer)
    m_pDemuxer->Abort();

  CLog::Log(LOGNOTICE, "DVDPlayer: waiting for threads to exit");

  // wait for the main thread to finish up
  // since this main thread cleans up all other resources and threads
  // we are done after the StopThread call
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
  m_CurrentVideo.Clear();
  m_CurrentAudio.Clear();
  m_CurrentSubtitle.Clear();

  m_messenger.Init();

  g_dvdPerformanceCounter.EnableMainPerformance(ThreadHandle());
}

void CDVDPlayer::GetFileMetaData(const CStdString &strPath, CFileItem *pItem)
{
  if (!pItem)
    return;

  MEASURE_FUNCTION;
  CDVDInputStream *pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, strPath, "");
  if (!pInputStream)
  {
    CLog::Log(LOGERROR, "%s - Error creating stream for %s", __FUNCTION__, strPath.c_str());
    return ;
  }

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) || !pInputStream->Open(strPath.c_str(), ""))
  {
    CLog::Log(LOGERROR, "%s - invalid stream in %s", __FUNCTION__, strPath.c_str());
    delete pInputStream;
    return ;
  }

  CDVDDemuxFFmpeg *pDemuxer = new CDVDDemuxFFmpeg;

  try
  {
    if (!pDemuxer->Open(pInputStream))
    {
      CLog::Log(LOGERROR, "%s - Error opening demuxer", __FUNCTION__);
      delete pDemuxer;
      delete pInputStream;
      return ;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when opeing demuxer", __FUNCTION__);
    if (pDemuxer)
      delete pDemuxer;
    delete pInputStream;
    return ;
  }

  AVFormatContext *pContext = pDemuxer->m_pFormatContext; 
  if (pContext)
  {
    int nLenMsec = pDemuxer->GetStreamLenght();
    CStdString strDuration;
    int nHours = nLenMsec / 1000 / 60 / 60;
    int nMinutes = ((nLenMsec / 1000) - nHours * 3600) / 60;
    int nSec = (nLenMsec / 1000)  - nHours * 3600 - nMinutes * 60;
    strDuration.Format("%02d:%02d:%02d", nHours, nMinutes, nSec);
    pItem->SetProperty("duration-msec", nLenMsec);
    pItem->SetProperty("duration-str", strDuration);
    pItem->SetProperty("title", pContext->title);
    pItem->SetProperty("author", pContext->author);
    pItem->SetProperty("copyright", pContext->copyright);
    pItem->SetProperty("comment", pContext->comment);
    pItem->SetProperty("album", pContext->album);
    pItem->SetProperty("year", pContext->year);
    pItem->SetProperty("track", pContext->track);
    pItem->SetProperty("genre", pContext->genre);
  }

  delete pDemuxer;
  pInputStream->Close();
  delete pInputStream;
  
}

bool CDVDPlayer::ExtractThumb(const CStdString &strPath, const CStdString &strTarget)
{
  MEASURE_FUNCTION;
  int nTime = timeGetTime();
  CDVDInputStream *pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, strPath, "");
  if (!pInputStream)
  {
    CLog::Log(LOGERROR, "InputStream: Error creating stream for %s", strPath.c_str());
    return false;
  }

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CLog::Log(LOGERROR, "InputStream: dvd streams not supported for thumb extraction, file: %s", strPath.c_str());
    delete pInputStream;
    return false;
  }

  if (!pInputStream->Open(strPath.c_str(), ""))
  {
    CLog::Log(LOGERROR, "InputStream: Error opening, %s", strPath.c_str());
    if (pInputStream)
      delete pInputStream;
    return false;
  }

  CDVDDemux *pDemuxer = NULL;

  try
  {
    pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(pInputStream);
    if(!pDemuxer)
    {
      delete pInputStream;
      CLog::Log(LOGERROR, "%s - Error creating demuxer", __FUNCTION__);
      return false;
    }

    if (!pDemuxer->Open(pInputStream))
    {
      CLog::Log(LOGERROR, "%s - Error opening demuxer", __FUNCTION__);
      delete pDemuxer;
      delete pInputStream;
      return false;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when opeing demuxer", __FUNCTION__);
    if (pDemuxer)
      delete pDemuxer;
    delete pInputStream;
    return false;
  }

  CDemuxStream* pStream = NULL;
  int nVideoStream = -1;
  for (int i = 0; i < pDemuxer->GetNrOfStreams(); i++)
  {
    pStream = pDemuxer->GetStream(i);
    if (pStream && pStream->type == STREAM_VIDEO)
    {
      nVideoStream = i;
      break;
    }
  }

  bool bOk = false;
  if (nVideoStream != -1)
  {
    CDVDStreamInfo hint(*pStream, true);
    CDVDVideoCodec *pVideoCodec = CDVDFactoryCodec::CreateVideoCodec( hint );
    if (pVideoCodec)
    {
      int nSeekTo = 300*1000; // in millis
      int nTotalLen = pDemuxer->GetStreamLenght();
      if (nSeekTo > nTotalLen / 2)
        nSeekTo = nTotalLen / 2;

      CLog::Log(LOGDEBUG,"%s - seeking to pos %dms (total: %dms) in %s", __FUNCTION__, nSeekTo, nTotalLen, strPath.c_str());
      if (pDemuxer->Seek(nSeekTo, true))
      {
        CDVDDemux::DemuxPacket* pPacket = NULL;
  
        bool bHasFrame = false;
        while (!bHasFrame)
        {
          do
          {
            pPacket = pDemuxer->Read();
          }   while (pPacket && pPacket->iStreamId != nVideoStream);

          if (pPacket)
          {
            int iDecoderState = pVideoCodec->Decode(pPacket->pData, pPacket->iSize, pPacket->pts);
            if (iDecoderState & VC_PICTURE)
            {
              bHasFrame = true;
              DVDVideoPicture picture;
              memset(&picture, 0, sizeof(DVDVideoPicture));
              if (pVideoCodec->GetPicture(&picture))
              {
                int nWidth = g_advancedSettings.m_thumbSize;
                double aspect = (double)picture.iWidth / (double)picture.iHeight;
                int nHeight = (int)((double)g_advancedSettings.m_thumbSize / aspect);
 
                DllSwScale dllSwScale;
                dllSwScale.Load();

                BYTE *pOutBuf = (BYTE*)new int[nWidth * nHeight * 4];
                struct SwsContext *context = dllSwScale.sws_getContext(picture.iWidth, picture.iHeight, 
                      PIX_FMT_YUV420P, nWidth, nHeight, PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL);
                uint8_t *src[] = { picture.data[0], picture.data[1], picture.data[2] };
                int     srcStride[] = { picture.iLineSize[0], picture.iLineSize[1], picture.iLineSize[2] };
                uint8_t *dst[] = { pOutBuf, 0, 0 };
                int     dstStride[] = { nWidth*4, 0, 0 };

                if (context)
                {
                  dllSwScale.sws_scale(context, src, srcStride, 0, picture.iHeight, dst, dstStride);  
                  dllSwScale.sws_freeContext(context);

                  CPicture out;
                  out.CreateThumbnailFromSurface(pOutBuf, nWidth, nHeight, nWidth * 4, strTarget);
                  bOk = true; 
                }

                dllSwScale.Unload();                
                delete [] pOutBuf;
              }
              else 
              {
                CLog::Log(LOGDEBUG,"%s - coudln't get picture from decoder in  %s", __FUNCTION__, strPath.c_str());
              }
            }
          }
          else 
          {
            CLog::Log(LOGDEBUG,"%s - decode failed in %s", __FUNCTION__, strPath.c_str());
            break;
          }
 
        }
      }
      delete pVideoCodec;
    }
  }

  delete pInputStream;

  int nTotalTime = timeGetTime() - nTime;
  CLog::Log(LOGDEBUG,"%s - measured %d ms to extract thumb from file <%s> ", __FUNCTION__, nTotalTime, strPath.c_str());
  return bOk;
}

void CDVDPlayer::Process()
{
  int video_index = -1;
  int audio_index = -1;

  CLog::Log(LOGNOTICE, "Creating InputStream");
  
  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(this, m_filename, m_content);
  if (!m_pInputStream || !m_pInputStream->Open(m_filename.c_str(), m_content))
  {
    CLog::Log(LOGERROR, "InputStream: Error opening, %s", m_filename.c_str());
    // inputstream will be destroyed in OnExit()
    return;
  }

  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CLog::Log(LOGNOTICE, "DVDPlayer: playing a dvd with menu's");
  }

  CLog::Log(LOGNOTICE, "Creating Demuxer");
  
  try
  {
    m_pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(m_pInputStream);
    if(!m_pDemuxer)
    {
      CLog::Log(LOGERROR, "%s - Error creating demuxer", __FUNCTION__);
      return;
    }

    if (!m_pDemuxer->Open(m_pInputStream))
    {
      CLog::Log(LOGERROR, "%s - Error opening demuxer", __FUNCTION__);
      return;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when opeing demuxer", __FUNCTION__);
    return;
  }

  // find any available external subtitles for non dvd files
  if( !m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) )
    CDVDFactorySubtitle::GetSubtitles(m_vecSubtitleFiles, m_filename);

  // find first audio / video streams
  for (int i = 0; i < m_pDemuxer->GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(i);
    if (pStream)
    {
      if (pStream->type == STREAM_AUDIO && audio_index < 0) audio_index = i;
      else if (pStream->type == STREAM_VIDEO && video_index < 0) video_index = i;
    }
  }

  // open the streams
  if (audio_index >= 0) OpenAudioStream(audio_index);
  if (video_index >= 0) OpenVideoStream(video_index);

  // open any external subtitle found
  if(m_vecSubtitleFiles.size() > 0)
  {
    CDVDStreamInfo hint;
    if(m_dvdPlayerSubtitle.OpenStream(hint, m_vecSubtitleFiles[0]))
      m_CurrentSubtitle.id = 0x100;
  }

  // we are done initializing now, set the readyevent
  SetEvent(m_hReadyEvent);

  m_callback.OnPlayBackStarted();

  while (!m_bAbortRequest)
  {
    // if the queues are full, no need to read more
    while (!m_bAbortRequest && (!m_dvdPlayerAudio.AcceptsData() || !m_dvdPlayerVideo.AcceptsData()))
    {
      HandleMessages();
      Sleep(10);

      if (m_caching)
      {
        // check here if we should stop caching
        // TODO - we could continue to wait, if filesystem can cache further

        m_clock.SetSpeed(m_playSpeed);
        m_dvdPlayerAudio.SetSpeed(m_playSpeed);
        m_dvdPlayerVideo.SetSpeed(m_playSpeed);
        m_caching = false;
      }
    }

    if (m_bAbortRequest)
      break;
    
    if(GetPlaySpeed() != DVD_PLAYSPEED_NORMAL && GetPlaySpeed() != DVD_PLAYSPEED_PAUSE)
    {
      if (IsInMenu())
      {
        // this can't be done in menu
        SetPlaySpeed(DVD_PLAYSPEED_NORMAL);

      }
      else if (m_CurrentVideo.id >= 0 
            &&  m_CurrentVideo.inited == true
            &&  m_dvdPlayerVideo.GetCurrentPts() != m_lastpts)
      {
        m_lastpts = m_dvdPlayerVideo.GetCurrentPts();
        // check how much off clock video is when ff/rw:ing
        // a problem here is that seeking isn't very accurate
        // and since the clock will be resynced after seek
        // we might actually not really be playing at the wanted
        // speed. we'd need to have some way to not resync the clock
        // after a seek to remember timing. still need to handle
        // discontinuities somehow

        // when seeking, give the player a headstart to make sure 
        // the time it takes to seek doesn't make a difference.
        double iError;
        iError = m_clock.GetClock() - m_lastpts;
        iError = iError * GetPlaySpeed() / abs(GetPlaySpeed());

        if(iError > DVD_MSEC_TO_TIME(1000))
        {
          CLog::Log(LOGDEBUG, "CDVDPlayer::Process - Seeking to catch up");
          __int64 iTime = (__int64)(GetTime() + 500.0 * GetPlaySpeed() / DVD_PLAYSPEED_NORMAL);
          m_messenger.Put(new CDVDMsgPlayerSeek(iTime, GetPlaySpeed() < 0));
        }
      }
    }

    // handle messages send to this thread, like seek or demuxer reset requests
    HandleMessages();

    // read a data frame from stream.
    CDVDDemux::DemuxPacket* pPacket = m_pDemuxer->Read();

    // check if we are too slow and need to recache
    if(!m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    {
      if (m_dvdPlayerAudio.IsStalled() && m_CurrentAudio.inited && m_CurrentAudio.id >= 0
      ||  m_dvdPlayerVideo.IsStalled() && m_CurrentVideo.inited && m_CurrentVideo.id >= 0)
      {
        if(!m_caching)
        {
          m_clock.SetSpeed(DVD_PLAYSPEED_PAUSE);
          m_dvdPlayerAudio.SetSpeed(DVD_PLAYSPEED_PAUSE);
          m_dvdPlayerVideo.SetSpeed(DVD_PLAYSPEED_PAUSE);
          m_caching = true;
        }
      }
    }

    if (!pPacket)
    {
      // when paused, demuxer could be be returning empty
      if (m_playSpeed == DVD_PLAYSPEED_PAUSE)
        continue;

      if (m_pInputStream->IsEOF()) break;
 
      if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
      {
        CDVDInputStreamNavigator* pStream = static_cast<CDVDInputStreamNavigator*>(m_pInputStream);

        // stream is holding back data untill demuxer has flushed
        if(pStream->IsHeld())
          pStream->SkipHold();

        // stills will be skipped
        if(m_dvd.state == DVDSTATE_STILL)
        {
          if (m_dvd.iDVDStillTime > 0)
          {
            if (GetTickCount() >= (m_dvd.iDVDStillStartTime + m_dvd.iDVDStillTime))
            {
              m_dvd.iDVDStillTime = 0;
              m_dvd.iDVDStillStartTime = 0;
              m_dvd.state = DVDSTATE_NORMAL;
              pStream->SkipStill();
              continue;
            }
          }
          Sleep(100);
        }

        // we don't consider dvd's ended untill navigator tells us so
        continue;          
      }

      // any demuxer supporting non blocking reads, should return empty packates
      CLog::Log(LOGINFO, "%s - EOF reading from demuxer", __FUNCTION__);
      break;
    }

    if(m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    {
      CDVDInputStreamNavigator *pInput = static_cast<CDVDInputStreamNavigator*>(m_pInputStream);

      if (pPacket->dts != DVD_NOPTS_VALUE)
        pPacket->dts -= pInput->GetTimeStampCorrection();
      if (pPacket->pts != DVD_NOPTS_VALUE)
        pPacket->pts -= pInput->GetTimeStampCorrection();
    }

    CDemuxStream *pStream = m_pDemuxer->GetStream(pPacket->iStreamId);
    if (!pStream) 
    {
      if(pPacket->iStreamId != -1)
        CLog::Log(LOGERROR, "%s - Error demux packet doesn't belong to any stream", __FUNCTION__);
      continue;
    }

    // it's a valid data packet, add some more information too it
    
    // this groupId stuff is getting a bit messy, need to find a better way
    // currently it is used to determine if a menu overlay is associated with a picture
    // for dvd's we use as a group id, the current cell and the current title
    // to be a bit more precise we alse count the number of disc's in case of a pts wrap back in the same cell / title
    pPacket->iGroupId = m_pInputStream->GetCurrentGroupId();
    try
    {
      if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
      {
        // Stream selection for DVD's this
        // should probably come as messages in the packet instead

        if (pStream->type == STREAM_SUBTITLE &&
            pStream->iPhysicalId == m_dvd.iSelectedSPUStream &&
            pStream->iId != m_CurrentSubtitle.id)
        {
          // dvd subtitle stream changed
          OpenSubtitleStream( pStream->iId );
        }
        else if (pStream->type == STREAM_AUDIO &&
            pStream->iPhysicalId == m_dvd.iSelectedAudioStream &&
            pStream->iId != m_CurrentAudio.id)
        {
          // dvd audio stream changed,          
          OpenAudioStream( pStream->iId );
        }
        else if (pStream->type == STREAM_VIDEO &&
            pStream->iPhysicalId == 0 &&
            pStream->iId != m_CurrentVideo.id)
        {
          // dvd video stream changed
          OpenVideoStream(pStream->iId);
        }

        // check so dvdnavigator didn't want us to close stream,
        // we allow lingering invalid audio/subtitle streams here to let player pass vts/cell borders more cleanly
        if (m_dvd.iSelectedAudioStream < 0 && m_CurrentAudio.id >= 0) CloseAudioStream( true );
        if (m_dvd.iSelectedSPUStream < 0 && m_CurrentVideo.id >= 0)   CloseSubtitleStream( true );
      }
      else
      {
        // for normal files, just open first stream
        if (m_CurrentSubtitle.id < 0 && pStream->type == STREAM_SUBTITLE) OpenSubtitleStream(pStream->iId);
        if (m_CurrentAudio.id < 0    && pStream->type == STREAM_AUDIO)    OpenAudioStream(pStream->iId);
        if (m_CurrentVideo.id < 0    && pStream->type == STREAM_VIDEO)    OpenVideoStream(pStream->iId);

        // check so that none of our streams has become invalid
        if (m_CurrentAudio.id >= 0 && m_pDemuxer->GetStream(m_CurrentAudio.id) == NULL)
          CloseAudioStream(false);
        if (m_CurrentVideo.id >= 0 && m_pDemuxer->GetStream(m_CurrentVideo.id) == NULL) 
          CloseVideoStream(false);
        if (m_CurrentSubtitle.id >= 0 && !(m_CurrentSubtitle.id & 0x100) && m_pDemuxer->GetStream(m_CurrentSubtitle.id) == NULL)
          CloseSubtitleStream(false);
      }
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s - Exception thrown when attempting to open stream", __FUNCTION__);
      break;
    }

    /* process packet if it belongs to selected stream. for dvd's down't allow automatic opening of streams*/
    LockStreams();

    try
    {
      if (pPacket->iStreamId == m_CurrentAudio.id && pStream->type == STREAM_AUDIO)
      {
        ProcessAudioData(pStream, pPacket);
      }
      else if (pPacket->iStreamId == m_CurrentVideo.id && pStream->type == STREAM_VIDEO)
      {
        ProcessVideoData(pStream, pPacket);
      }
      else if (pPacket->iStreamId == m_CurrentSubtitle.id && pStream->type == STREAM_SUBTITLE)
      {
        ProcessSubData(pStream, pPacket);
      }
      else CDVDDemuxUtils::FreeDemuxPacket(pPacket); // free it since we won't do anything with it
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "%s - Exception thrown when processing demux packet", __FUNCTION__);
      break;
    }

    UnlockStreams();
  }

  while (!m_bAbortRequest && 
         ((!m_dvdPlayerAudio.m_messageQueue.RecievedAbortRequest() && 
         m_dvdPlayerAudio.m_messageQueue.GetDataSize() > 0) ||
         (!m_dvdPlayerVideo.m_messageQueue.RecievedAbortRequest() && 
         m_dvdPlayerVideo.m_messageQueue.GetDataSize() > 0) ))
  {
    HandleMessages();
    Sleep(10);
  }
}

void CDVDPlayer::ProcessAudioData(CDemuxStream* pStream, CDVDDemux::DemuxPacket* pPacket)
{
  if (m_CurrentAudio.stream != (void*)pStream)
  {
    /* check so that dmuxer hints or extra data hasn't changed */
    /* if they have, reopen stream */

    if (m_CurrentAudio.hint != CDVDStreamInfo(*pStream, true))
    { 
      // we don't actually have to close audiostream here first, as 
      // we could send it as a stream message. only problem 
      // is how to notify player if a stream change failed.
      CloseAudioStream( true );
      OpenAudioStream( pPacket->iStreamId );
    }

    m_CurrentAudio.stream = (void*)pStream;
  }
  
  CheckContinuity(pPacket, DVDPLAYER_AUDIO);
  if(pPacket->dts != DVD_NOPTS_VALUE)
    m_CurrentAudio.dts = pPacket->dts;

  //If this is the first packet after a discontinuity, send it as a resync
  if (m_CurrentAudio.inited == false)
  {
    m_CurrentAudio.inited = true;

    bool setclock = m_CurrentVideo.id < 0 || m_playSpeed == DVD_PLAYSPEED_NORMAL;

    if(pPacket->dts != DVD_NOPTS_VALUE)
      m_dvdPlayerAudio.SendMessage(new CDVDMsgGeneralResync(pPacket->dts, setclock));
    else
      m_dvdPlayerAudio.SendMessage(new CDVDMsgGeneralResync(pPacket->pts, setclock));
  }

  if (m_CurrentAudio.id >= 0)
    m_dvdPlayerAudio.SendMessage(new CDVDMsgDemuxerPacket(pPacket, pPacket->iSize));
  else
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

void CDVDPlayer::ProcessVideoData(CDemuxStream* pStream, CDVDDemux::DemuxPacket* pPacket)
{
  if (m_CurrentVideo.stream != (void*)pStream)
  {
    /* check so that dmuxer hints or extra data hasn't changed */
    /* if they have reopen stream */

    if (m_CurrentVideo.hint != CDVDStreamInfo(*pStream, true))
    {
      CloseVideoStream(true);
      OpenVideoStream(pPacket->iStreamId);
    }

    m_CurrentVideo.stream = (void*)pStream;
  }

  if( pPacket->iSize != 4) //don't check the EOF_SEQUENCE of stillframes
  {
    CheckContinuity( pPacket, DVDPLAYER_VIDEO );
    if(pPacket->dts != DVD_NOPTS_VALUE)
      m_CurrentVideo.dts = pPacket->dts;
  }

  //If this is the first packet after a discontinuity, send it as a resync
  if (m_CurrentVideo.inited == false)
  {
    m_CurrentVideo.inited = true;
    
    bool setclock = m_CurrentAudio.id < 0 || m_playSpeed != DVD_PLAYSPEED_NORMAL;

    if(pPacket->dts != DVD_NOPTS_VALUE)
      m_dvdPlayerVideo.SendMessage(new CDVDMsgGeneralResync(pPacket->dts, setclock));
    else
      m_dvdPlayerVideo.SendMessage(new CDVDMsgGeneralResync(pPacket->pts, setclock));
  }

  if (m_CurrentVideo.id >= 0)
    m_dvdPlayerVideo.SendMessage(new CDVDMsgDemuxerPacket(pPacket, pPacket->iSize));
  else
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

void CDVDPlayer::ProcessSubData(CDemuxStream* pStream, CDVDDemux::DemuxPacket* pPacket)
{
  if (m_CurrentSubtitle.stream != (void*)pStream)
  {
    /* check so that dmuxer hints or extra data hasn't changed */
    /* if they have reopen stream */

    if (m_CurrentSubtitle.hint != CDVDStreamInfo(*pStream, true))
    {
        CloseSubtitleStream(true);
        OpenSubtitleStream(pPacket->iStreamId);
    }

    m_CurrentSubtitle.stream = (void*)pStream;
  }
  if(pPacket->dts != DVD_NOPTS_VALUE)
    m_CurrentSubtitle.dts = pPacket->dts;

  m_dvdPlayerSubtitle.SendMessage(new CDVDMsgDemuxerPacket(pPacket, pPacket->iSize));

  if(m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))    
    m_dvdPlayerSubtitle.UpdateOverlayInfo((CDVDInputStreamNavigator*)m_pInputStream, LIBDVDNAV_BUTTON_NORMAL);
}

void CDVDPlayer::CheckContinuity(CDVDDemux::DemuxPacket* pPacket, unsigned int source)
{
  if (m_playSpeed < DVD_PLAYSPEED_PAUSE)
    return;

  if( pPacket->dts == DVD_NOPTS_VALUE )
    return;

  if (source == DVDPLAYER_VIDEO
  && m_CurrentAudio.dts != DVD_NOPTS_VALUE 
  && m_CurrentVideo.dts != DVD_NOPTS_VALUE)
  {
    /* check for looping stillframes on non dvd's, dvd's will be detected by long duration check later */
    if( m_pInputStream && !m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    {
      /* special case for looping stillframes THX test discs*/
      /* only affect playback when not from dvd */
      if( (m_CurrentAudio.dts > m_CurrentVideo.dts + DVD_MSEC_TO_TIME(200)) 
      && (m_CurrentVideo.dts == pPacket->dts) )
      {
        CLog::Log(LOGDEBUG, "CDVDPlayer::CheckContinuity - Detected looping stillframe");
        SyncronizePlayers(SYNCSOURCE_VIDEO);
        return;
      }
    }

    /* if we haven't received video for a while, but we have been */
    /* getting audio much more recently, make sure video wait's  */
    /* this occurs especially on thx test disc */
    if( (pPacket->dts > m_CurrentVideo.dts + DVD_MSEC_TO_TIME(200)) 
     && (pPacket->dts < m_CurrentAudio.dts + DVD_MSEC_TO_TIME(50)) )
    {
      CLog::Log(LOGDEBUG, "CDVDPlayer::CheckContinuity - Potential long duration frame");
      SyncronizePlayers(SYNCSOURCE_VIDEO);
      return;
    }
  }

  double mindts, maxdts;
  if(m_CurrentAudio.dts == DVD_NOPTS_VALUE)
    maxdts = mindts = m_CurrentVideo.dts;
  else if(m_CurrentVideo.dts == DVD_NOPTS_VALUE)
    maxdts = mindts = m_CurrentAudio.dts;
  else
  {
    maxdts = max(m_CurrentAudio.dts, m_CurrentVideo.dts);
    mindts = min(m_CurrentAudio.dts, m_CurrentVideo.dts);
  }

  /* if we don't have max and min, we can't do anything more */
  if( mindts == DVD_NOPTS_VALUE || maxdts == DVD_NOPTS_VALUE )
    return;


  /* stream wrap back */
  if( pPacket->dts < mindts )
  {
    /* if video player is rendering a stillframe, we need to make sure */
    /* audio has finished processing it's data otherwise it will be */
    /* displayed too early */
 
    if (m_dvdPlayerVideo.IsStalled() && m_CurrentVideo.dts != DVD_NOPTS_VALUE)
      SyncronizePlayers(SYNCSOURCE_VIDEO);
    else if (m_dvdPlayerAudio.IsStalled() && m_CurrentAudio.dts != DVD_NOPTS_VALUE)
      SyncronizePlayers(SYNCSOURCE_AUDIO);

    m_CurrentAudio.inited = false;
    m_CurrentVideo.inited = false;
    m_CurrentSubtitle.inited = false;
  }

  /* stream jump forward */
  if( pPacket->dts > maxdts + DVD_MSEC_TO_TIME(1000) )
  {
    /* normally don't need to sync players since video player will keep playing at normal fps */
    /* after a discontinuity */
    //SyncronizePlayers(dts, pts, MSGWAIT_ALL);
    m_CurrentAudio.inited = false;
    m_CurrentVideo.inited = false;
    m_CurrentSubtitle.inited = false;
  }

}
void CDVDPlayer::SyncronizeDemuxer(DWORD timeout)
{
  if(CThread::ThreadId() == GetCurrentThreadId())
    return;
  if(!m_messenger.IsInited())
    return;

  CDVDMsgGeneralSynchronize* message = new CDVDMsgGeneralSynchronize(timeout, 0);
  message->Acquire();  
  m_messenger.Put(message);
  message->Wait(&m_bStop, 0);
  message->Release();
}

void CDVDPlayer::SyncronizePlayers(DWORD sources)
{

  /* we need a big timeout as audio queue is about 8seconds for 2ch ac3 */
  const int timeout = 10*1000; // in milliseconds

  CDVDMsgGeneralSynchronize* message = new CDVDMsgGeneralSynchronize(timeout, sources);
  if (m_CurrentAudio.id >= 0)
  {
    message->Acquire();
    m_dvdPlayerAudio.SendMessage(message);
  }
  if (m_CurrentVideo.id >= 0)
  {
    message->Acquire();
    m_dvdPlayerVideo.SendMessage(message);
  }
  message->Release();
}

void CDVDPlayer::OnExit()
{
  g_dvdPerformanceCounter.DisableMainPerformance();

  try
  {
    CLog::Log(LOGNOTICE, "CDVDPlayer::OnExit()");

    // if we are caching, start playing it agian
    if (m_caching && !m_bAbortRequest)
    {
      m_clock.SetSpeed(m_playSpeed);
      m_dvdPlayerAudio.SetSpeed(m_playSpeed);
      m_dvdPlayerVideo.SetSpeed(m_playSpeed);
      m_caching = false;
    }

    // close each stream
    if (!m_bAbortRequest) CLog::Log(LOGNOTICE, "DVDPlayer: eof, waiting for queues to empty");
    if (m_CurrentAudio.id >= 0)
    {
      CLog::Log(LOGNOTICE, "DVDPlayer: closing audio stream");
      CloseAudioStream(false);
    }
    if (m_CurrentVideo.id >= 0)
    {
      CLog::Log(LOGNOTICE, "DVDPlayer: closing video stream");
      CloseVideoStream(false);
    }
    if (m_CurrentSubtitle.id >= 0)
    {
      CLog::Log(LOGNOTICE, "DVDPlayer: closing video stream");
      CloseSubtitleStream(false);
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

    // if we didn't stop playing, advance to the next item in xbmc's playlist
    if (!m_bAbortRequest) m_callback.OnPlayBackEnded();

    m_messenger.End();

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when trying to close down player, memory leak will follow", __FUNCTION__);
    m_pInputStream = NULL;
    m_pDemuxer = NULL;   
  }

  // set event to inform openfile something went wrong in case openfile is still waiting for this event
  SetEvent(m_hReadyEvent);
}

void CDVDPlayer::HandleMessages()
{
  CDVDMsg* pMsg;

  MsgQueueReturnCode ret = m_messenger.Get(&pMsg, 0);
   
  while (ret == MSGQ_OK)
  {
    LockStreams();

    try
    {
      if (pMsg->IsType(CDVDMsg::PLAYER_SEEK))
      {
        CDVDMsgPlayerSeek* pMsgPlayerSeek = (CDVDMsgPlayerSeek*)pMsg;
        
        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          // need to get the seek based on file positition working in CDVDInputStreamNavigator
          // so that demuxers can control the stream (seeking in this case)
          // for now use time based seeking
          CLog::Log(LOGDEBUG, "CDVDInputStreamNavigator seek to: %d", pMsgPlayerSeek->GetTime());
          if (((CDVDInputStreamNavigator*)m_pInputStream)->Seek(pMsgPlayerSeek->GetTime()))
          {
            CLog::Log(LOGDEBUG, "CDVDInputStreamNavigator seek to: %d, success", pMsgPlayerSeek->GetTime());
            FlushBuffers(!pMsgPlayerSeek->GetFlush());
          }
          else
            CLog::Log(LOGWARNING, "error while seeking");
        }
        else
        {
          CLog::Log(LOGDEBUG, "demuxer seek to: %d", pMsgPlayerSeek->GetTime());
          if (m_pDemuxer && m_pDemuxer->Seek(pMsgPlayerSeek->GetTime(), pMsgPlayerSeek->GetBackward() ))
          {
            CLog::Log(LOGDEBUG, "demuxer seek to: %d, success", pMsgPlayerSeek->GetTime());
            FlushBuffers(!pMsgPlayerSeek->GetFlush());
          }
          else
          {
            // demuxer will return failure, if you seek to eof
            if (m_pInputStream && m_pInputStream->IsEOF())
            {
              CLog::Log(LOGDEBUG, "demuxer seek to: eof");
              FlushBuffers(!pMsgPlayerSeek->GetFlush());
            }
            else
              CLog::Log(LOGWARNING, "error while seeking");            
          }
        }
        // make sure video player displays next frame
        m_dvdPlayerVideo.StepFrame();

        // set flag to indicate we have finished a seeking request
        g_infoManager.m_performingSeek = false;
      }
      else if (pMsg->IsType(CDVDMsg::DEMUXER_RESET))
      {
          m_CurrentAudio.stream = NULL;
          m_CurrentVideo.stream = NULL;
          m_CurrentSubtitle.stream = NULL;

          // we need to reset the demuxer, probably because the streams have changed
          m_pDemuxer->Reset();
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_AUDIOSTREAM))
      {
        CDVDMsgPlayerSetAudioStream* pMsgPlayerSetAudioStream = (CDVDMsgPlayerSetAudioStream*)pMsg;        

        // for dvd's we set it using the navigater, it will then send a change audio to the dvdplayer
        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {          
          CDVDInputStreamNavigator* pInput = (CDVDInputStreamNavigator*)m_pInputStream;
          if( pInput->SetActiveAudioStream(pMsgPlayerSetAudioStream->GetStreamId()) )
          {
            m_dvd.iSelectedAudioStream = -1;
            CloseAudioStream(false);            
          }
        }
        else if (m_pDemuxer)
        {
          CDemuxStream* pStream = m_pDemuxer->GetStreamFromAudioId(pMsgPlayerSetAudioStream->GetStreamId());
          if (pStream) OpenAudioStream(pStream->iId);
        }
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_SUBTITLESTREAM))
      {
        CDVDMsgPlayerSetSubtitleStream* pMsgPlayerSetSubtileStream = (CDVDMsgPlayerSetSubtitleStream*)pMsg;

        // for dvd's we set it using the navigater, it will then send a change subtitle to the dvdplayer
        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
          if( pStream->SetActiveSubtitleStream(pMsgPlayerSetSubtileStream->GetStreamId()) )
          {
            m_dvd.iSelectedSPUStream = -1;
            CloseSubtitleStream(false);            
          }
        }
        else if (m_vecSubtitleFiles.size() > 0)
        {
          int index = pMsgPlayerSetSubtileStream->GetStreamId();
          if(index >= 0 && index < (int)m_vecSubtitleFiles.size())
          {
            CloseSubtitleStream(false);
            CDVDStreamInfo hints;
            m_dvdPlayerSubtitle.OpenStream(hints, m_vecSubtitleFiles[index]);
            m_CurrentSubtitle.id = index | 0x100;
          }
        }
        else if (m_pDemuxer)
        {
          CDemuxStream* pStream = m_pDemuxer->GetStreamFromSubtitleId(pMsgPlayerSetSubtileStream->GetStreamId());
          if (pStream) OpenSubtitleStream(pStream->iId);
        }        
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_SUBTITLESTREAM_VISIBLE))
      {
        CDVDMsgBool* pValue = (CDVDMsgBool*)pMsg;

        m_dvdPlayerVideo.EnableSubtitle(pValue->m_value);

        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
          pStream->EnableSubtitleStream(pValue->m_value);
        }
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_STATE))
      {
        CDVDMsgPlayerSetState* pMsgPlayerSetState = (CDVDMsgPlayerSetState*)pMsg;

        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          std::string s = pMsgPlayerSetState->GetState();
          ((CDVDInputStreamNavigator*)m_pInputStream)->SetNavigatorState(s);
          m_dvd.state = DVDSTATE_NORMAL;
          m_dvd.iDVDStillStartTime = 0;
          m_dvd.iDVDStillTime = 0;
        }
      }
      else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH))
      {
        FlushBuffers(false);
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
      {
        int speed = static_cast<CDVDMsgInt*>(pMsg)->m_value;
        // if playspeed is different then DVD_PLAYSPEED_NORMAL or DVD_PLAYSPEED_PAUSE
        // audioplayer, stops outputing audio to audiorendere, but still tries to
        // sleep an correct amount for each packet
        // videoplayer just plays faster after the clock speed has been increased
        // 1. disable audio
        // 2. skip frames and adjust their pts or the clock
        m_playSpeed = speed;
        m_caching = false;
        m_clock.SetSpeed(speed);
        m_dvdPlayerAudio.SetSpeed(speed);
        m_dvdPlayerVideo.SetSpeed(speed);

        // TODO - we really shouldn't pause demuxer 
        //        untill our buffers are somewhat filled
        if(m_pDemuxer)
          m_pDemuxer->SetSpeed(speed);
      } 
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s - Exception thrown when handling message", __FUNCTION__);
    }
    
    UnlockStreams();
    
    pMsg->Release();
    ret = m_messenger.Get(&pMsg, 0);
  }
}

void CDVDPlayer::SetPlaySpeed(int speed)
{
  m_messenger.Put(new CDVDMsgInt(CDVDMsg::PLAYER_SETSPEED, speed));
  SyncronizeDemuxer(100);
}

void CDVDPlayer::Pause()
{
  // return to normal speed if it was paused before, pause otherwise
  if (m_playSpeed == DVD_PLAYSPEED_PAUSE || m_caching) SetPlaySpeed(DVD_PLAYSPEED_NORMAL);
  else SetPlaySpeed(DVD_PLAYSPEED_PAUSE);
}

bool CDVDPlayer::IsPaused() const
{
  return (m_playSpeed == DVD_PLAYSPEED_PAUSE) || m_caching;
}

bool CDVDPlayer::HasVideo() const
{
  if (m_pInputStream)
  {
    if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) || m_CurrentVideo.id >= 0) return true;
  }
  return false;
}

bool CDVDPlayer::HasAudio() const
{
  return (m_CurrentAudio.id >= 0);
}

bool CDVDPlayer::CanSeek()
{
  return GetTotalTime() > 0;
}

void CDVDPlayer::Seek(bool bPlus, bool bLargeStep)
{
#if 0
  // sadly this doesn't work for now, audio player must
  // drop packets at the same rate as we play frames
  if( m_playSpeed == DVD_PLAYSPEED_PAUSE && bPlus && !bLargeStep)
  {
    m_dvdPlayerVideo.StepFrame();
    return;
  }
#endif

  if (g_advancedSettings.m_videoUseTimeSeeking && GetTotalTime() > 2*g_advancedSettings.m_videoTimeSeekForwardBig)
  {
    int seek = 0;
    if (bLargeStep)
      seek = bPlus ? g_advancedSettings.m_videoTimeSeekForwardBig : g_advancedSettings.m_videoTimeSeekBackwardBig;
    else
      seek = bPlus ? g_advancedSettings.m_videoTimeSeekForward : g_advancedSettings.m_videoTimeSeekBackward;
    // do the seek
    SeekTime(GetTime() + seek * 1000);
  }
  else
  {
    float percent = GetPercentage();
    if (bLargeStep)
      percent += bPlus ? g_advancedSettings.m_videoPercentSeekForwardBig : g_advancedSettings.m_videoPercentSeekBackwardBig;
    else
      percent += bPlus ? g_advancedSettings.m_videoPercentSeekForward : g_advancedSettings.m_videoPercentSeekBackward;

    if (percent >= 0 && percent <= 100)
    {
      // should be modified to seektime
      SeekPercentage(percent);
    }
  }

  m_tmLastSeek = time(NULL);
}

void CDVDPlayer::ToggleFrameDrop()
{
  m_dvdPlayerVideo.EnableFrameDrop( !m_dvdPlayerVideo.IsFrameDropEnabled() );
}

void CDVDPlayer::GetAudioInfo(CStdString& strAudioInfo)
{
  if( m_bStop ) return;

  string strDemuxerInfo;
  if (!m_bStop && m_CurrentAudio.id >= 0)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(m_CurrentAudio.id);
    if (pStream && pStream->type == STREAM_AUDIO)
      ((CDemuxStreamAudio*)pStream)->GetStreamInfo(strDemuxerInfo);
  }

  strAudioInfo.Format("D( %s ), P( %s )", strDemuxerInfo.c_str(), m_dvdPlayerAudio.GetPlayerInfo().c_str());
}

void CDVDPlayer::GetVideoInfo(CStdString& strVideoInfo)
{
  if( m_bStop ) return;

  string strDemuxerInfo;
  if (m_CurrentVideo.id >= 0)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(m_CurrentVideo.id);
    if (pStream && pStream->type == STREAM_VIDEO)
      ((CDemuxStreamVideo*)pStream)->GetStreamInfo(strDemuxerInfo);
  }

  strVideoInfo.Format("D( %s ), P( %s )", strDemuxerInfo.c_str(), m_dvdPlayerVideo.GetPlayerInfo().c_str());
}

void CDVDPlayer::GetGeneralInfo(CStdString& strGeneralInfo)
{
  if (!m_bStop)
  {
    double dDelay = (double)m_dvdPlayerVideo.GetDelay() / DVD_TIME_BASE;

    double apts = m_dvdPlayerAudio.GetCurrentPts();
    double vpts = m_dvdPlayerVideo.GetCurrentPts();
    double dDiff = 0;

    if( apts != DVD_NOPTS_VALUE && vpts != DVD_NOPTS_VALUE )
      dDiff = (apts - vpts) / DVD_TIME_BASE;

    int iFramesDropped = m_dvdPlayerVideo.GetNrOfDroppedFrames();

    strGeneralInfo.Format("DVD Player ad:%6.3f, a/v:%6.3f, dropped:%d, cpu: %i%%. source bitrate: %d Kbytes/s", dDelay, dDiff, iFramesDropped, (int)(CThread::GetRelativeUsage()*100), (GetSourceBitrate() / 8) / 1024);
  }
}

void CDVDPlayer::SeekPercentage(float iPercent)
{
  __int64 iTotalTime = GetTotalTimeInMsec();

  if (!iTotalTime)
    return;

  SeekTime((__int64)(iTotalTime * iPercent / 100));
  m_tmLastSeek = time(NULL);
}

float CDVDPlayer::GetPercentage()
{
  __int64 iTotalTime = GetTotalTimeInMsec();

  if (!iTotalTime)
    return 0.0f;

  return GetTime() * 100 / (float)iTotalTime;
}

//This is how much audio is delayed to video, we count the oposite in the dvdplayer
void CDVDPlayer::SetAVDelay(float fValue)
{
  m_dvdPlayerVideo.SetDelay( - (fValue * DVD_TIME_BASE) ) ;
}

float CDVDPlayer::GetAVDelay()
{
  return - m_dvdPlayerVideo.GetDelay() / (float)DVD_TIME_BASE;
}

void CDVDPlayer::SetSubTitleDelay(float fValue)
{
  m_dvdPlayerVideo.SetSubtitleDelay(fValue * DVD_TIME_BASE);
}

float CDVDPlayer::GetSubTitleDelay()
{
  return m_dvdPlayerVideo.GetSubtitleDelay() / DVD_TIME_BASE;
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
  int count = m_vecSubtitleFiles.size();
  if (count > 0) 
    return count;

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
  else if (m_vecSubtitleFiles.size())
  {
    if(m_CurrentSubtitle.id & 0x100)
      return m_CurrentSubtitle.id & 0xff;
  }
  // demuxer subttiles
  else if (m_pDemuxer)
  {
    int iSubtitleStreams = m_pDemuxer->GetNrOfSubtitleStreams();
    for (int i = 0; i < iSubtitleStreams; i++)
    {
      CDemuxStream* pStream = m_pDemuxer->GetStreamFromSubtitleId(i);
      if (pStream && pStream->iId == m_CurrentSubtitle.id) return i;
    }
  }

  return -1;
}

void CDVDPlayer::GetSubtitleName(int iStream, CStdString &strStreamName)
{
  strStreamName.Format("%d. ", iStream);

  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    // get stream name from dvd, don't check for demuxer stream
    // since the stream may not be avaiable yet (it is a few secs later)
    CDVDInputStreamNavigator* pStreamNav = (CDVDInputStreamNavigator*)m_pInputStream;
    strStreamName += pStreamNav->GetSubtitleStreamLanguage(iStream);
  }
  else if (m_vecSubtitleFiles.size())
  {
    if(iStream >= 0 && iStream < (int)m_vecSubtitleFiles.size())
      strStreamName += CUtil::GetFileName(m_vecSubtitleFiles[iStream]);
    else
      strStreamName += " (Invalid)";
  }
  else if (m_pDemuxer)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStreamFromSubtitleId(iStream);

    if (!pStream)
    {
      CLog::Log(LOGWARNING, "CDVDPlayer::GetSubtitleName: Invalid stream: id %i", iStream);
      strStreamName += " (Invalid)";
      return;
    }

    CStdString strName;
    pStream->GetStreamName(strName);
    strStreamName += strName;
  }
}

void CDVDPlayer::SetSubtitle(int iStream)
{
  m_messenger.Put(new CDVDMsgPlayerSetSubtitleStream(iStream));
}

bool CDVDPlayer::GetSubtitleVisible()
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    if(pStream->IsInMenu())
      return g_stSettings.m_currentVideoSettings.m_SubtitleOn;
    else
      return pStream->IsSubtitleStreamEnabled();
  }

  return m_dvdPlayerVideo.IsSubtitleEnabled();
}

void CDVDPlayer::SetSubtitleVisible(bool bVisible)
{
  g_stSettings.m_currentVideoSettings.m_SubtitleOn = bVisible;
  m_messenger.Put(new CDVDMsgBool(CDVDMsg::PLAYER_SET_SUBTITLESTREAM_VISIBLE, bVisible));
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
  else if (m_pDemuxer)
  {
    int iAudioStreams = m_pDemuxer->GetNrOfAudioStreams();
    for (int i = 0; i < iAudioStreams; i++)
    {
      CDemuxStream* pStream = m_pDemuxer->GetStreamFromAudioId(i);
      if (pStream && pStream->iId == m_CurrentAudio.id) return i;
    }
  }
  
  return -1;
}

void CDVDPlayer::GetAudioStreamName(int iStream, CStdString& strStreamName)
{
  strStreamName.Format("%d. ", iStream);
  CDemuxStreamAudio* pStream = NULL;
  if (m_pDemuxer) pStream = m_pDemuxer->GetStreamFromAudioId(iStream);

  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    // get stream name from dvd, don't check for demuxer stream
    // since the stream may not be avaiable yet (it is a few secs later)
    CDVDInputStreamNavigator* pStreamNav = (CDVDInputStreamNavigator*)m_pInputStream;
    strStreamName += pStreamNav->GetAudioStreamLanguage(iStream);
  }
  else
  {
    if (!pStream)
    {
      CLog::Log(LOGWARNING, "CDVDPlayer::GetAudioStreamName: Invalid stream: id %i", iStream);
      strStreamName += " (Invalid)";
      return;
    }
  
    CStdString strName;
    pStream->GetStreamName(strName);
    strStreamName += strName;
  }

  if (pStream)
  {
    std::string strType;
    pStream->GetStreamType(strType);
    if (strType.length() > 0)
    {
      strStreamName += " - ";
      strStreamName += strType;
    }
  }
}

void CDVDPlayer::SetAudioStream(int iStream)
{
  m_messenger.Put(new CDVDMsgPlayerSetAudioStream(iStream));
  SyncronizeDemuxer(100);
}

void CDVDPlayer::SeekTime(__int64 iTime)
{
  if(iTime<0) 
    iTime = 0;
  m_messenger.Put(new CDVDMsgPlayerSeek((int)iTime, false));
  SyncronizeDemuxer(100);
  m_tmLastSeek = time(NULL);
}

// return the time in milliseconds
__int64 CDVDPlayer::GetTime()
{
  // get timing and seeking from libdvdnav for dvd's
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    if(m_dvd.state == DVDSTATE_STILL)
      return GetTickCount() - m_dvd.iDVDStillStartTime;
    else
      return ((CDVDInputStreamNavigator*)m_pInputStream)->GetTime(); // we should take our buffers into account
  }

  __int64 iMsecs = (__int64)(m_clock.GetClock() * 1000 / DVD_TIME_BASE);
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
    if(m_dvd.state == DVDSTATE_STILL && m_dvd.iDVDStillTime > 0)
      return m_dvd.iDVDStillTime;
    else
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
  if (iSpeed < 0 && IsInMenu()) return;
  SetPlaySpeed(iSpeed * DVD_PLAYSPEED_NORMAL);
}

bool CDVDPlayer::GetSubtitleExtension(CStdString &strSubtitleExtension)
{
  return false;
}

bool CDVDPlayer::OpenAudioStream(int iStream)
{
  if (!m_pDemuxer)
  {
    CLog::Log(LOGERROR, "%s: No Demuxer", __FUNCTION__);
    return false;
  }
  
  CDemuxStream* pStream = m_pDemuxer->GetStream(iStream);
  if (!pStream)
  {
    CLog::Log(LOGERROR, "%s: Stream %d doesn't exist", __FUNCTION__, iStream);
    return false;
  }

  if (pStream->disabled)
    return false;

  if (pStream->type != STREAM_AUDIO)
  {
    CLog::Log(LOGERROR, "%s: Streamtype is not STREAM_AUDIO", __FUNCTION__);
    return false;
  }

  if( m_CurrentAudio.id >= 0 ) CloseAudioStream(true);

  CLog::Log(LOGNOTICE, "Opening audio stream: %i", iStream);

  if( m_CurrentAudio.id < 0 &&  m_CurrentVideo.id >= 0 )
  {
    // up until now we wheren't playing audio, but we did play video
    // this will change what is used to sync the dvdclock.
    // since the new audio data doesn't have to have any relation
    // to the current video data in the packet que, we have to
    // wait for it to empty

    // this happens if a new cell has audio data, but previous didn't
    // and both have video data

    SyncronizePlayers(SYNCSOURCE_AUDIO);
  }

  CDVDStreamInfo hint(*pStream, true);
//  if( m_CurrentAudio.id >= 0 )
//  {
//    CDVDStreamInfo* phint = new CDVDStreamInfo(hint);
//    m_dvdPlayerAudio.SendMessage(new CDVDMsgGeneralStreamChange(phint));
//  }
  bool success = false;
  try
  {
    success = m_dvdPlayerAudio.OpenStream( hint );
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown in player", __FUNCTION__);
    success = false;
  }

  if (!success)
  {
    /* mark stream as disabled, to disallaw further attempts*/
    CLog::Log(LOGWARNING, "%s - Unsupported stream %d. Stream disabled.", __FUNCTION__, iStream);
    pStream->disabled = true;

    return false;
  }

  /* store information about stream */
  m_CurrentAudio.id = iStream;
  m_CurrentAudio.hint = hint;
  m_CurrentAudio.stream = (void*)pStream;

  /* audio normally won't consume full cpu, so let it have prio */
  m_dvdPlayerAudio.SetPriority(GetThreadPriority(*this)+1);

  return true;
}

bool CDVDPlayer::OpenVideoStream(int iStream)
{
  if (!m_pDemuxer)
  {
    CLog::Log(LOGERROR, "%s: No Demuxer avilable", __FUNCTION__);
    return false;
  }
  
  CDemuxStream* pStream = m_pDemuxer->GetStream(iStream);
  if( !pStream )
  {
    CLog::Log(LOGERROR, "%s: Stream %d doesn't exist", __FUNCTION__, iStream);
    return false;
  }

  if( pStream->disabled )
    return false;

  if (pStream->type != STREAM_VIDEO)
  {
    CLog::Log(LOGERROR, "%s: Streamtype is not STREAM_VIDEO", __FUNCTION__);
    return false;
  }

  if ( m_CurrentVideo.id >= 0) CloseVideoStream(true);

  CLog::Log(LOGNOTICE, "Opening video stream: %i", iStream);

  CDVDStreamInfo hint(*pStream, true);

  /* set aspect ratio as requested by navigator for dvd's */
  if( m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) )
  {
    float aspect = static_cast<CDVDInputStreamNavigator*>(m_pInputStream)->GetVideoAspectRatio();
    if(aspect != 0.0)
      hint.aspect = aspect;
  }

  bool success = false;
  try
  {    
    success = m_dvdPlayerVideo.OpenStream(hint);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown in player", __FUNCTION__);
    success = false;
  }

  if (!success)
  {
    // mark stream as disabled, to disallaw further attempts
    CLog::Log(LOGWARNING, "%s - Unsupported stream %d. Stream disabled.", __FUNCTION__, iStream);
    pStream->disabled = true;
    return false;
  }

  /* store information about stream */
  m_CurrentVideo.id = iStream;
  m_CurrentVideo.hint = hint;
  m_CurrentVideo.stream = (void*)pStream;

  /* use same priority for video thread as demuxing thread, as */
  /* otherwise demuxer will starve if video consumes the full cpu */
  m_dvdPlayerVideo.SetPriority(GetThreadPriority(*this));
  return true;

}

bool CDVDPlayer::OpenSubtitleStream(int iStream)
{
  if (!m_pDemuxer)
  {
    CLog::Log(LOGERROR, "%s: No Demuxer avilable", __FUNCTION__);
    return false;
  }

  CDemuxStream* pStream = m_pDemuxer->GetStream(iStream);
  if( !pStream )
  {
    CLog::Log(LOGERROR, "%s: Stream %d doesn't exist", __FUNCTION__, iStream);
    return false;
  }

  if( pStream->disabled )
    return false;

  if (m_CurrentSubtitle.id >= 0) CloseSubtitleStream(true);

  CLog::Log(LOGNOTICE, "Opening Subtitle stream: %i", iStream);

  CDVDStreamInfo hint(*pStream, true);

  std::string filename;
  if(m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    filename = "dvd";
  if(!m_dvdPlayerSubtitle.OpenStream(hint, filename))
  {
    CLog::Log(LOGWARNING, "%s - Unsupported stream %d. Stream disabled.", __FUNCTION__, iStream);
    pStream->disabled = true;
    return false;
  }

  m_CurrentSubtitle.id = iStream;
  m_CurrentSubtitle.hint = hint;
  m_CurrentSubtitle.stream = (void*)pStream;

  return true;
}

bool CDVDPlayer::CloseAudioStream(bool bWaitForBuffers)
{
  if (m_CurrentAudio.id < 0)
    return false;

  CLog::Log(LOGNOTICE, "Closing audio stream");

  m_dvdPlayerAudio.CloseStream(bWaitForBuffers);

  m_CurrentAudio.id = -1;
  m_CurrentAudio.dts = DVD_NOPTS_VALUE;
  m_CurrentAudio.hint.Clear();

  return true;
}

bool CDVDPlayer::CloseVideoStream(bool bWaitForBuffers)
{
  if (m_CurrentVideo.id < 0) 
    return false;

  CLog::Log(LOGNOTICE, "Closing video stream");  

  m_dvdPlayerVideo.CloseStream(bWaitForBuffers);

  m_CurrentVideo.id = -1;
  m_CurrentVideo.dts = DVD_NOPTS_VALUE;
  m_CurrentVideo.hint.Clear();

  return true;
}

bool CDVDPlayer::CloseSubtitleStream(bool bKeepOverlays)
{
  if (m_CurrentSubtitle.id < 0) 
    return false;

  CLog::Log(LOGNOTICE, "Closing subtitle stream");

  m_dvdPlayerSubtitle.CloseStream(!bKeepOverlays);

  m_CurrentSubtitle.id = -1;
  m_CurrentSubtitle.dts = DVD_NOPTS_VALUE;
  m_CurrentSubtitle.hint.Clear();

  return true;
}

void CDVDPlayer::FlushBuffers(bool queued)
{
  if(queued) 
  {

    m_dvdPlayerAudio.SendMessage(new CDVDMsgGeneralFlush());
    m_dvdPlayerVideo.SendMessage(new CDVDMsgGeneralFlush());
    m_dvdPlayerSubtitle.SendMessage(new CDVDMsgGeneralFlush());
    SyncronizePlayers(1000);
  } 
  else
  {
    m_pDemuxer->Flush();
    m_dvdPlayerAudio.Flush();
    m_dvdPlayerVideo.Flush();
    m_dvdPlayerSubtitle.Flush();

    // clear subtitle and menu overlays
    m_overlayContainer.Clear();
  }
  m_CurrentAudio.inited = false;
  m_CurrentVideo.inited = false;
  m_CurrentSubtitle.inited = false;  
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

        if (m_dvd.state != DVDSTATE_STILL)
        {
          // else notify the player we have recieved a still frame

          if(still_event->length < 0xff)
            m_dvd.iDVDStillTime = still_event->length * 1000;
          else
            m_dvd.iDVDStillTime = 0;

          m_dvd.iDVDStillStartTime = GetTickCount();

          /* adjust for the output delay in the video queue */
          DWORD time = 0;
          if( m_CurrentVideo.stream && m_dvd.iDVDStillTime > 0 )
          {
            time = (DWORD)(m_dvdPlayerVideo.GetOutputDelay() / ( DVD_TIME_BASE / 1000 ));
            if( time < 10000 && time > 0 ) 
              m_dvd.iDVDStillTime += time;
          }
          m_dvd.state = DVDSTATE_STILL;
          CLog::Log(LOGDEBUG, "DVDNAV_STILL_FRAME - waiting %i sec, with delay of %ld sec", still_event->length, time / 1000);
        }
        return NAVRESULT_HOLD;
      }
      break;
    case DVDNAV_SPU_CLUT_CHANGE:
      {
        CLog::Log(LOGDEBUG, "DVDNAV_SPU_CLUT_CHANGE");
        m_dvdPlayerSubtitle.SendMessage(new CDVDMsgSubtitleClutChange((BYTE*)pData));
      }
      break;
    case DVDNAV_SPU_STREAM_CHANGE:
      {
        CLog::Log(LOGDEBUG, "DVDNAV_SPU_STREAM_CHANGE");

        dvdnav_spu_stream_change_event_t* event = (dvdnav_spu_stream_change_event_t*)pData;

        int iStream = event->physical_wide;
        bool visible = !(iStream & 0x80);

        m_dvdPlayerVideo.EnableSubtitle(visible);

        if (iStream >= 0)
          m_dvd.iSelectedSPUStream = (iStream & ~0x80);
        else
          m_dvd.iSelectedSPUStream = -1;

        m_CurrentSubtitle.stream = NULL;
      }
      break;
    case DVDNAV_AUDIO_STREAM_CHANGE:
      {
        CLog::Log(LOGDEBUG, "DVDNAV_AUDIO_STREAM_CHANGE");

        // This should be the correct way i think, however we don't have any streams right now
        // since the demuxer hasn't started so it doesn't change. not sure how to do this.
        dvdnav_audio_stream_change_event_t* event = (dvdnav_audio_stream_change_event_t*)pData;

        // Tell system what audiostream should be opened by default
        if (event->logical >= 0)
          m_dvd.iSelectedAudioStream = event->physical;
        else
          m_dvd.iSelectedAudioStream = -1;

        m_CurrentAudio.stream = NULL;
      }
      break;
    case DVDNAV_HIGHLIGHT:
      {
        //dvdnav_highlight_event_t* pInfo = (dvdnav_highlight_event_t*)pData;
        int iButton = pStream->GetCurrentButton();
        CLog::Log(LOGDEBUG, "DVDNAV_HIGHLIGHT: Highlight button %d\n", iButton);        
        m_dvdPlayerSubtitle.UpdateOverlayInfo((CDVDInputStreamNavigator*)m_pInputStream, LIBDVDNAV_BUTTON_NORMAL);
      }
      break;
    case DVDNAV_VTS_CHANGE:
      {
        //dvdnav_vts_change_event_t* vts_change_event = (dvdnav_vts_change_event_t*)pData;
        CLog::Log(LOGDEBUG, "DVDNAV_VTS_CHANGE");

        //Make sure we clear all the old overlays here, or else old forced items are left.
        m_overlayContainer.Clear();

        // reset the demuxer, this also imples closing the video and the audio system
        // this is a bit tricky cause it's the demuxer that's is making this call in the end
        // so we send a message to indicate the main loop that the demuxer needs a reset
        // this also means the libdvdnav may not return any data packets after this command
        m_messenger.Put(new CDVDMsgDemuxerReset());

        //Force an aspect ratio that is set in the dvdheaders if available
        m_CurrentVideo.hint.aspect = pStream->GetVideoAspectRatio();
        if( m_dvdPlayerAudio.m_messageQueue.IsInited() )
          m_dvdPlayerVideo.SendMessage(new CDVDMsgVideoSetAspect(m_CurrentVideo.hint.aspect));

        return NAVRESULT_HOLD;
      }
      break;
    case DVDNAV_CELL_CHANGE:
      {
        //dvdnav_cell_change_event_t* cell_change_event = (dvdnav_cell_change_event_t*)pData;
        CLog::Log(LOGDEBUG, "DVDNAV_CELL_CHANGE");

        m_dvd.state = DVDSTATE_NORMAL;        
        
        if( m_dvdPlayerVideo.m_messageQueue.IsInited() )
          m_dvdPlayerVideo.SendMessage(new CDVDMsgVideoNoSkip());        
      }
      break;
    case DVDNAV_NAV_PACKET:
      {
          //pci_t* pci = (pci_t*)pData;

          // this should be possible to use to make sure we get
          // seamless transitions over these boundaries
          // if we remember the old vobunits boundaries
          // when a packet comes out of demuxer that has
          // pts values outside that boundary, it belongs
          // to the new vobunit, wich has new timestamps
      }
      break;
    case DVDNAV_HOP_CHANNEL:
      {
        // This event is issued whenever a non-seamless operation has been executed.
        // Applications with fifos should drop the fifos content to speed up responsiveness.
        CLog::Log(LOGDEBUG, "DVDNAV_HOP_CHANNEL");
        m_messenger.Put(new CDVDMsgGeneralFlush());
        return NAVRESULT_ERROR;
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
  return NAVRESULT_NOP;
}

bool CDVDPlayer::OnAction(const CAction &action)
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;


    if( m_dvd.state == DVDSTATE_STILL && m_dvd.iDVDStillTime != 0 && pStream->GetTotalButtons() == 0 )
    {
      switch(action.wID)
      {
        case ACTION_NEXT_ITEM:
        case ACTION_MOVE_RIGHT:
        case ACTION_MOVE_UP:
        case ACTION_SELECT_ITEM:
          {
            /* this will force us out of the stillframe */
            CLog::Log(LOGDEBUG, "%s - User asked to exit stillframe", __FUNCTION__);
            m_dvd.iDVDStillStartTime = 0;
            m_dvd.iDVDStillTime = 1;
            return true;
          }
          break;
      }        
    }


    switch (action.wID)
    {
    case ACTION_PREV_ITEM:  // SKIP-:
      {
        CLog::Log(LOGDEBUG, " - pushed prev");
        pStream->OnPrevious();
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      break;
    case ACTION_NEXT_ITEM:  // SKIP+:
      {
        CLog::Log(LOGDEBUG, " - pushed next");
        pStream->OnNext();
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      break;
    case ACTION_SHOW_VIDEOMENU:   // start button
      {
        CLog::Log(LOGDEBUG, " - go to menu");
        pStream->OnMenu();
        // send a message to everyone that we've gone to the menu
        CGUIMessage msg(GUI_MSG_VIDEO_MENU_STARTED, 0, 0);
        g_graphicsContext.SendMessage(msg);
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
      case ACTION_SHOW_OSD:
      case ACTION_SELECT_ITEM:
        {
          CLog::Log(LOGDEBUG, " - button select");
          // show button pushed overlay
          m_dvdPlayerSubtitle.UpdateOverlayInfo((CDVDInputStreamNavigator*)m_pInputStream, LIBDVDNAV_BUTTON_CLICKED);

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
    if( m_dvd.state == DVDSTATE_STILL )
      return true;
    else
      return pStream->IsInMenu();
  }
  return false;
}

bool CDVDPlayer::HasMenu()
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    return true;
  else
    return false;
}

bool CDVDPlayer::GetCurrentSubtitle(CStdString& strSubtitle)
{
  double pts = m_clock.GetClock();

  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    return false;

  return m_dvdPlayerSubtitle.GetCurrentSubtitle(strSubtitle, pts);
}

CStdString CDVDPlayer::GetPlayerState()
{
  if (!m_pInputStream) return "";

  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;

    std::string buffer;
    if( pStream->GetNavigatorState(buffer) ) return buffer;
  }

  return "";
}

bool CDVDPlayer::SetPlayerState(CStdString state)
{
  m_messenger.Put(new CDVDMsgPlayerSetState(state));
  return true;
}

int CDVDPlayer::GetChapterCount()
{   
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    return pStream->GetChapterCount();
  }
  return 0;
}

int CDVDPlayer::GetChapter()
{   
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    return pStream->GetChapter();
  }
  return -1;
} 

bool CDVDPlayer::AddSubtitle(const CStdString& strSubPath)
{
  m_vecSubtitleFiles.push_back(strSubPath);
  return true;
}

int CDVDPlayer::GetCacheLevel() const
{
  int a = min(100,100 * m_dvdPlayerAudio.m_messageQueue.GetDataSize() / m_dvdPlayerAudio.m_messageQueue.GetMaxDataSize());
  int v = min(100,100 * m_dvdPlayerVideo.m_messageQueue.GetDataSize() / m_dvdPlayerVideo.m_messageQueue.GetMaxDataSize());
  return max(a, v);
}

int CDVDPlayer::GetAudioBitrate()
{
  return m_dvdPlayerAudio.GetAudioBitrate();
}

int CDVDPlayer::GetVideoBitrate()
{
  return m_dvdPlayerVideo.GetVideoBitrate();
}

int CDVDPlayer::GetSourceBitrate()
{
  if (m_pInputStream)
    return (int)m_pInputStream->GetBitstreamStats().GetBitrate();

  return 0;
}

