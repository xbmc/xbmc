/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "PAPlayer.h"
#include "CodecFactory.h"
#include "GUIInfoManager.h"
#include "Application.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "utils/MathUtils.h"

#include "threads/SingleLock.h"
#include "cores/AudioEngine/Utils/AEUtil.h"

#define TIME_TO_CACHE_NEXT_FILE 5000 /* 5 seconds before end of song, start caching the next song */
#define FAST_XFADE_TIME           80 /* 80 milliseconds */

// PAP: Psycho-acoustic Audio Player
// Supporting all open  audio codec standards.
// First one being nullsoft's nsv audio decoder format

PAPlayer::PAPlayer(IPlayerCallback& callback) :
  IPlayer            (callback),
  CThread            ("PAPlayer"),
  m_signalSpeedChange(false),
  m_playbackSpeed    (1    ),
  m_isPlaying        (false),
  m_isPaused         (false),
  m_isFinished       (false),
  m_currentStream    (NULL ),
  m_audioCallback    (NULL ),
  m_FileItem         (new CFileItem() )
{
}

PAPlayer::~PAPlayer()
{
  if (!m_isPaused)
    SoftStop(true, true);
  CloseAllStreams(false);  

  /* wait for the thread to terminate */
  StopThread(true);//true - wait for end of thread
  delete m_FileItem;
}

bool PAPlayer::HandlesType(const CStdString &type)
{
  ICodec* codec = CodecFactory::CreateCodec(type);
  if (codec && codec->CanInit())
  {
    delete codec;
    return true;
  }

  return false;
}

void PAPlayer::SoftStart(bool wait/* = false */)
{
  CSharedLock lock(m_streamsLock);
  for(StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
  {
    StreamInfo* si = *itt;
    if (si->m_fadeOutTriggered)
      continue;

    si->m_stream->FadeVolume(0.0f, 1.0f, FAST_XFADE_TIME);
    si->m_stream->Resume();
  }
  
  if (wait)
  {
    /* wait for them to fade in */
    lock.Leave();
    Sleep(FAST_XFADE_TIME);
    lock.Enter();

    /* be sure they have faded in */
    while(wait)
    {
      wait = false;
      for(StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
      {
        StreamInfo* si = *itt;
        if (si->m_stream->IsFading())
        {
          lock.Leave();	  
          wait = true;
          Sleep(1);
          lock.Enter();
          break;
        }
      }
    }
  }
}

void PAPlayer::SoftStop(bool wait/* = false */, bool close/* = true */)
{
  /* fade all the streams out fast for a nice soft stop */
  CSharedLock lock(m_streamsLock);
  for(StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
  {
    StreamInfo* si = *itt;
    if (si->m_stream)
      si->m_stream->FadeVolume(1.0f, 0.0f, FAST_XFADE_TIME);

    if (close)
    {
      si->m_prepareTriggered  = true;
      si->m_playNextTriggered = true;
      si->m_fadeOutTriggered  = true;
    }
  }

  /* if we are going to wait for them to finish fading */
  if(wait)
  {
    /* wait for them to fade out */
    lock.Leave();
    Sleep(FAST_XFADE_TIME);
    lock.Enter();

    /* be sure they have faded out */
    while(wait)
    {
      wait = false;
      for(StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
      {
        StreamInfo* si = *itt;
        if (si->m_stream && si->m_stream->IsFading())
        {
          lock.Leave();	  
          wait = true;
          Sleep(1);
          lock.Enter();
          break;
        }
      }
    }

    /* if we are not closing the streams, pause them */
    if (!close)
    {
      for(StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
      {
        StreamInfo* si = *itt;
        si->m_stream->Pause();
      }
    }
  }
}

void PAPlayer::CloseAllStreams(bool fade/* = true */)
{
  if (!fade) 
  {
    CExclusiveLock lock(m_streamsLock);    
    while(!m_streams.empty())
    {
      StreamInfo* si = m_streams.front();
      m_streams.pop_front();
      
      if (si->m_stream)
      {
        CAEFactory::AE->FreeStream(si->m_stream);
        si->m_stream = NULL;
      }

      si->m_decoder.Destroy();
      delete si;
    }

    while(!m_finishing.empty())
    {
      StreamInfo* si = m_finishing.front();
      m_finishing.pop_front();

      if (si->m_stream)
      {
        CAEFactory::AE->FreeStream(si->m_stream);
        si->m_stream = NULL;
      }

      si->m_decoder.Destroy();
      delete si;
    }
    m_currentStream = NULL;
  }
  else
  {
    SoftStop(false, true);
    CExclusiveLock lock(m_streamsLock);
    m_currentStream = NULL;
  }  
}

bool PAPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  CloseAllStreams();
  m_crossFadeTime = g_guiSettings.GetInt("musicplayer.crossfade") * 1000;

  if (!QueueNextFileEx(file, false))
    return false;

  if (!IsRunning())
    Create();

  /* trigger playback start */
  m_isPlaying = true;
  m_startEvent.Set();
  return true;
}

void PAPlayer::UpdateCrossFadingTime(const CFileItem& file)
{
  if ((m_crossFadeTime = g_guiSettings.GetInt("musicplayer.crossfade") * 1000))
  {
    if (
        m_streams.size() == 0 ||
        (
          file.HasMusicInfoTag() && !g_guiSettings.GetBool("musicplayer.crossfadealbumtracks") &&
          m_FileItem->HasMusicInfoTag() &&
          (m_FileItem->GetMusicInfoTag()->GetAlbum() != "") &&
          (m_FileItem->GetMusicInfoTag()->GetAlbum() == file.GetMusicInfoTag()->GetAlbum()) &&
          (m_FileItem->GetMusicInfoTag()->GetDiscNumber() == file.GetMusicInfoTag()->GetDiscNumber()) &&
          (m_FileItem->GetMusicInfoTag()->GetTrackNumber() == file.GetMusicInfoTag()->GetTrackNumber() - 1)
        )
    )
    {
      //do not crossfade when playing consecutive albumtracks
      m_crossFadeTime = 0;
    }
  }
}

bool PAPlayer::QueueNextFile(const CFileItem &file)
{
  return QueueNextFileEx(file);
}

bool PAPlayer::QueueNextFileEx(const CFileItem &file, bool fadeIn/* = true */)
{
  //set crossfade time for the file being queued
  UpdateCrossFadingTime(file);

  StreamInfo *si = new StreamInfo();

  if (!si->m_decoder.Create(file, (file.m_lStartOffset * 1000) / 75))
  {
    CLog::Log(LOGWARNING, "PAPlayer::QueueNextFileEx - Failed to create the decoder");

    delete si;
    m_callback.OnQueueNextItem();
    return false;
  }

  /* decode until there is data-available */
  si->m_decoder.Start();
  while(si->m_decoder.GetDataSize() == 0)
  {
    int status = si->m_decoder.GetStatus();
    if (status == STATUS_ENDED   ||
        status == STATUS_NO_FILE ||
        si->m_decoder.ReadSamples(PACKET_SIZE) == RET_ERROR)
    {
      CLog::Log(LOGINFO, "PAPlayer::QueueNextFileEx - Error reading samples");

      si->m_decoder.Destroy();
      delete si;
      m_callback.OnQueueNextItem();
      return false;
    }

    /* yield our time so that the main PAP thread doesnt stall */
    CThread::Sleep(1);
  }

  /* init the streaminfo struct */
  si->m_decoder.GetDataFormat(&si->m_channelInfo, &si->m_sampleRate, &si->m_encodedSampleRate, &si->m_dataFormat);
  si->m_startOffset        = file.m_lStartOffset * 1000 / 75;
  si->m_endOffset          = file.m_lEndOffset   * 1000 / 75;
  si->m_bytesPerSample     = CAEUtil::DataFormatToBits(si->m_dataFormat) >> 3;
  si->m_bytesPerFrame      = si->m_bytesPerSample * si->m_channelInfo.Count();
  si->m_started            = false;
  si->m_finishing          = false;
  si->m_framesSent         = 0;
  si->m_seekNextAtFrame    = 0;
  si->m_seekFrame          = -1;
  si->m_stream             = NULL;
  si->m_volume             = (fadeIn && m_crossFadeTime) ? 0.0f : 1.0f;
  si->m_fadeOutTriggered   = false;
  si->m_isSlaved           = false;

  int64_t streamTotalTime = si->m_decoder.TotalTime();
  if (si->m_endOffset)
    streamTotalTime = si->m_endOffset;

  unsigned int firstFrame = 0;
  firstFrame = (int)(si->m_startOffset * si->m_sampleRate / 1000.0f);

  if (streamTotalTime >= TIME_TO_CACHE_NEXT_FILE + m_crossFadeTime)
    si->m_prepareNextAtFrame = (int)((streamTotalTime - TIME_TO_CACHE_NEXT_FILE - m_crossFadeTime) * si->m_sampleRate / 1000.0f);
  si->m_prepareNextAtFrame -= firstFrame;
  
  si->m_prepareTriggered = false;

  if (streamTotalTime < m_crossFadeTime)
    si->m_playNextAtFrame = (int)((streamTotalTime / 2) * si->m_sampleRate / 1000.0f);
  else
    si->m_playNextAtFrame = (int)((streamTotalTime - m_crossFadeTime) * si->m_sampleRate / 1000.0f);
  si->m_playNextAtFrame -= firstFrame;
  si->m_playNextTriggered = false;

  PrepareStream(si);

  /* add the stream to the list */
  CExclusiveLock lock(m_streamsLock);
  m_streams.push_back(si);

  *m_FileItem = file;

  return true;
}

inline bool PAPlayer::PrepareStream(StreamInfo *si)
{
  /* if we have a stream we are already prepared */
  if (si->m_stream)
    return true;

  /* get a paused stream */
  si->m_stream = CAEFactory::AE->MakeStream(
    si->m_dataFormat,
    si->m_sampleRate,
    si->m_encodedSampleRate,
    si->m_channelInfo,
    AESTREAM_PAUSED
  );

  if (!si->m_stream)
  {
    CLog::Log(LOGDEBUG, "PAPlayer::PrepareStream - Failed to get IAEStream");
    return false;
  }

  si->m_stream->SetVolume    (si->m_volume);
  si->m_stream->SetReplayGain(si->m_decoder.GetReplayGain());

  /* if its not the first stream and crossfade is not enabled */
  if (m_currentStream && m_currentStream != si && !m_crossFadeTime)
  {
    /* slave the stream for gapless */
    si->m_isSlaved = true;
    m_currentStream->m_stream->RegisterSlave(si->m_stream);
  }

  /* fill the stream's buffer */
  while(si->m_stream->IsBuffering())
  {
    int status = si->m_decoder.GetStatus();
    if (status == STATUS_ENDED   ||
        status == STATUS_NO_FILE ||
        si->m_decoder.ReadSamples(PACKET_SIZE) == RET_ERROR)
    {
      CLog::Log(LOGINFO, "PAPlayer::PrepareStream - Stream Finished");
      break;
    }

    if (!QueueData(si))
      break;

    /* yield our time so that the main PAP thread doesnt stall */
    CThread::Sleep(1);
  }

  CLog::Log(LOGINFO, "PAPlayer::PrepareStream - Ready");

  return true;
}

bool PAPlayer::CloseFile()
{
  m_callback.OnPlayBackStopped();
  return true;
}

void PAPlayer::Process()
{
  if (!m_startEvent.WaitMSec(100))
  {
    CLog::Log(LOGDEBUG, "PAPlayer::Process - Failed to receive start event");
    return;
  }

  CLog::Log(LOGDEBUG, "PAPlayer::Process - Playback started");  
  while(m_isPlaying && !m_bStop)
  {
    /* this needs to happen outside of any locks to prevent deadlocks */
    if (m_signalSpeedChange)
    {
      m_callback.OnPlayBackSpeedChanged(m_playbackSpeed);
      m_signalSpeedChange = false;
    }

    double delay  = 100.0;
    double buffer = 100.0;
    ProcessStreams(delay, buffer);

    if (delay < buffer && delay > 0.75 * buffer)
      CThread::Sleep(MathUtils::round_int((buffer - delay) * 1000.0));
  }
}

inline void PAPlayer::ProcessStreams(double &delay, double &buffer)
{
  CSharedLock sharedLock(m_streamsLock);
  if (m_isFinished && m_streams.empty() && m_finishing.empty())
  {
    m_isPlaying = false;
    delay       = 0;
    m_callback.OnPlayBackEnded();
    return;
  }

  /* destroy any drained streams */
  for(StreamList::iterator itt = m_finishing.begin(); itt != m_finishing.end();)
  {
    StreamInfo* si = *itt;
    if (si->m_stream->IsDrained())
    {      
      itt = m_finishing.erase(itt);
      CAEFactory::AE->FreeStream(si->m_stream);
      delete si;
      CLog::Log(LOGDEBUG, "PAPlayer::ProcessStreams - Stream Freed");
    }
    else
      ++itt;
  }

  sharedLock.Leave();
  CExclusiveLock lock(m_streamsLock);

  for(StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
  {
    StreamInfo* si = *itt;
    if (!m_currentStream && !si->m_started)
      m_currentStream = si;
    /* if the stream is finishing */
    if ((si->m_playNextTriggered && si->m_stream && !si->m_stream->IsFading()) || !ProcessStream(si, delay, buffer))
    {
      if (!si->m_prepareTriggered)
      {
        si->m_prepareTriggered = true;
        m_callback.OnQueueNextItem();
      }

      /* remove the stream */
      itt = m_streams.erase(itt);
      /* if its the current stream */
      if (si == m_currentStream)
      {
        /* if it was the last stream */
        if (itt == m_streams.end())
        {
          /* if it didnt trigger the next queue item */
          if (!si->m_prepareTriggered)
          {
            m_callback.OnQueueNextItem();
            si->m_prepareTriggered = true;
          }
          m_currentStream = NULL;
        }
        else
        {
          m_currentStream = *itt;
        }
      }

      /* unregister the audio callback */
      si->m_stream->UnRegisterAudioCallback();
      si->m_decoder.Destroy();      
      si->m_stream->Drain();
      m_finishing.push_back(si);
      return;
    }

    if (!si->m_started)
      continue;

    /* is it time to prepare the next stream? */
    if (si->m_prepareNextAtFrame > 0 && !si->m_prepareTriggered && si->m_framesSent >= si->m_prepareNextAtFrame)
    {
      si->m_prepareTriggered = true;
      m_callback.OnQueueNextItem();
    }

    /* it is time to start playing the next stream? */
    if (si->m_playNextAtFrame > 0 && !si->m_playNextTriggered && si->m_framesSent >= si->m_playNextAtFrame)
    {
      if (!si->m_prepareTriggered)
      {
        si->m_prepareTriggered = true;
        m_callback.OnQueueNextItem();
      }

      if (!m_isFinished)
      {
        if (m_crossFadeTime)
        {
          si->m_stream->FadeVolume(1.0f, 0.0f, m_crossFadeTime);
          si->m_fadeOutTriggered = true;
        }
        m_currentStream = NULL;

        /* unregister the audio callback */
        si->m_stream->UnRegisterAudioCallback();
      }

      si->m_playNextTriggered = true;      
    }
  }
}

inline bool PAPlayer::ProcessStream(StreamInfo *si, double &delay, double &buffer)
{
  /* if playback needs to start on this stream, do it */
  if (si == m_currentStream && !si->m_started)
  {
    si->m_started = true;
    si->m_stream->RegisterAudioCallback(m_audioCallback);
    if (!si->m_isSlaved)
      si->m_stream->Resume();
    si->m_stream->FadeVolume(0.0f, 1.0f, m_crossFadeTime);
    m_callback.OnPlayBackStarted();
  }

  /* if we have not started yet and the stream has been primed */
  unsigned int space = si->m_stream->GetSpace();
  if (!si->m_started && !space)
    return true;

  /* see if it is time yet to FF/RW or a direct seek */
  if (!si->m_playNextTriggered && ((m_playbackSpeed != 1 && si->m_framesSent >= si->m_seekNextAtFrame) || si->m_seekFrame > -1))
  {
    /* if its a direct seek */
    if (si->m_seekFrame > -1)
    {
      si->m_framesSent = si->m_seekFrame;
      si->m_seekFrame  = -1;
    }
    /* if its FF/RW */
    else
    {
      si->m_framesSent      += si->m_sampleRate * (m_playbackSpeed  - 1);
      si->m_seekNextAtFrame  = si->m_framesSent + si->m_sampleRate / 2;
    }

    int64_t time = (int64_t)(si->m_startOffset + ((float)si->m_framesSent / (float)si->m_sampleRate * 1000.0f));

    /* if we are seeking back before the start of the track start normal playback */
    if (time < si->m_startOffset || si->m_framesSent < 0)
    {
      time = si->m_startOffset;
      si->m_framesSent	    = 0;
      si->m_seekNextAtFrame = 0;
      ToFFRW(1);
    }

    si->m_decoder.Seek(time);
  }

  int status = si->m_decoder.GetStatus();
  if (status == STATUS_ENDED   ||
      status == STATUS_NO_FILE ||
      si->m_decoder.ReadSamples(PACKET_SIZE) == RET_ERROR)
  {
    CLog::Log(LOGINFO, "PAPlayer::ProcessStream - Stream Finished");
    return false;
  }

  if (!QueueData(si))
    return false;

  /* update the delay time if we are running */
  if (si->m_started)
  {
    if (si->m_stream->IsBuffering())
      delay = 0.0;
    else
      delay = std::min(delay , si->m_stream->GetDelay());
    buffer = std::min(buffer, si->m_stream->GetCacheTotal());
  }

  return true;
}

bool PAPlayer::QueueData(StreamInfo *si)
{
  unsigned int space   = si->m_stream->GetSpace();
  unsigned int samples = std::min(si->m_decoder.GetDataSize(), space / si->m_bytesPerSample);
  if (!samples)
    return true;

  void* data = si->m_decoder.GetData(samples);
  if (!data)
  {
    CLog::Log(LOGERROR, "PAPlayer::QueueData - Failed to get data from the decoder");
    return false;
  }

  unsigned int added = si->m_stream->AddData(data, samples * si->m_bytesPerSample);
  si->m_framesSent += added / si->m_bytesPerFrame;

  CLog::Log(LOGDEBUG, "PAPlayer::QueueData - m_frame_set %d - %d", si->m_framesSent, si->m_prepareNextAtFrame);

  return true;
}

void PAPlayer::OnExit()
{

}

void PAPlayer::RegisterAudioCallback(IAudioCallback* pCallback)
{
  CSharedLock lock(m_streamsLock);
  m_audioCallback = pCallback;
  if (m_currentStream && m_currentStream->m_stream)
    m_currentStream->m_stream->RegisterAudioCallback(pCallback);
}

void PAPlayer::UnRegisterAudioCallback()
{
  CSharedLock lock(m_streamsLock);
  /* only one stream should have the callback, but we do it to all just incase */
  for(StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
    if ((*itt)->m_stream)
      (*itt)->m_stream->UnRegisterAudioCallback();
  m_audioCallback = NULL;
}

void PAPlayer::OnNothingToQueueNotify()
{
  m_isFinished = true;
}

bool PAPlayer::IsPlaying() const
{
  return m_isPlaying;
}

bool PAPlayer::IsPaused() const
{
  return m_isPaused;
}

void PAPlayer::Pause()
{
  if (m_isPaused)
  {
    m_isPaused = false;
    SoftStart();
  }
  else
  {
    m_isPaused = true;    
    SoftStop(true, false);
  }
}

void PAPlayer::SetVolume(float volume)
{

}

void PAPlayer::SetDynamicRangeCompression(long drc)
{

}

void PAPlayer::ToFFRW(int iSpeed)
{
  m_playbackSpeed     = iSpeed;
  m_signalSpeedChange = true;
}

int64_t PAPlayer::GetTime()
{
  CSharedLock lock(m_streamsLock);
  if (!m_currentStream)
    return 0;

  double time = (double)m_currentStream->m_framesSent / (double)m_currentStream->m_sampleRate;
  if (m_currentStream->m_stream)
    time -= m_currentStream->m_stream->GetDelay();

  return (int64_t)(time * 1000.0);
}

int64_t PAPlayer::GetTotalTime64()
{
  CSharedLock lock(m_streamsLock);
  if (!m_currentStream)
    return 0;

  int64_t total = m_currentStream->m_decoder.TotalTime();
  if (m_currentStream->m_endOffset)
    total = m_currentStream->m_endOffset;
  total -= m_currentStream->m_startOffset;
  return total;
}

int PAPlayer::GetTotalTime()
{
  return (int)(GetTotalTime64() / 1000);
}

int PAPlayer::GetCacheLevel() const
{
  CSharedLock lock(m_streamsLock);
  if (!m_currentStream)
    return -1;

  const ICodec* codec = m_currentStream->m_decoder.GetCodec();
  if (codec)
    return codec->GetCacheLevel();
  return -1;
}

int PAPlayer::GetChannels()
{
  CSharedLock lock(m_streamsLock);
  if (!m_currentStream)
    return 0;

  return m_currentStream->m_channelInfo.Count();
}

int PAPlayer::GetBitsPerSample()
{
  CSharedLock lock(m_streamsLock);
  if (!m_currentStream)
    return 0;

  return m_currentStream->m_bytesPerSample >> 3;
}

int PAPlayer::GetSampleRate()
{
  CSharedLock lock(m_streamsLock);
  if (!m_currentStream)
    return 0;

  return m_currentStream->m_sampleRate;
}

CStdString PAPlayer::GetAudioCodecName()
{
  CSharedLock lock(m_streamsLock);
  if (!m_currentStream)
    return "";

  const ICodec* codec = m_currentStream->m_decoder.GetCodec();
  if (codec)
    return codec->m_CodecName;
  return "";
}

int PAPlayer::GetAudioBitrate()
{
  CSharedLock lock(m_streamsLock);
  if (!m_currentStream)
    return 0;

  const ICodec* codec = m_currentStream->m_decoder.GetCodec();
  if (codec)
    return codec->m_Bitrate;
  return 0;
}

bool PAPlayer::CanSeek()
{
  CSharedLock lock(m_streamsLock);
  if (!m_currentStream)
    return false;

  return m_currentStream->m_decoder.CanSeek();
}

void PAPlayer::Seek(bool bPlus, bool bLargeStep)
{
}

void PAPlayer::SeekTime(int64_t iTime /*=0*/)
{
  if (!CanSeek()) return;

  CSharedLock lock(m_streamsLock);
  if (!m_currentStream)
    return;

  int seekOffset = (int)(iTime - GetTime());
  if (m_currentStream->m_startOffset)
    iTime += m_currentStream->m_startOffset;

  if (m_playbackSpeed != 1)
    ToFFRW(1);

  m_currentStream->m_seekFrame = (int)(m_currentStream->m_sampleRate * (iTime / 1000));
  m_callback.OnPlayBackSeek((int)iTime, seekOffset);
}

void PAPlayer::SeekPercentage(float fPercent /*=0*/)
{
  if (fPercent < 0.0f  ) fPercent = 0.0f;
  if (fPercent > 100.0f) fPercent = 100.0f;
  SeekTime((int64_t)(fPercent * 0.01f * (float)GetTotalTime64()));
}

float PAPlayer::GetPercentage()
{
  return GetTime() * 100.0f / GetTotalTime64();
}

bool PAPlayer::SkipNext()
{
  return false;
}
