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
#include <visualizations/Goom/goom2k4-0/src/goom_hash.h>

#define TIME_TO_CACHE_NEXT_FILE 5000 /* 5 seconds before end of song, start caching the next song */
#define FAST_XFADE_TIME           80 /* 80 milliseconds */

// PAP: Psycho-acoustic Audio Player
// Supporting all open  audio codec standards.
// First one being nullsoft's nsv audio decoder format

PAPlayer::PAPlayer(IPlayerCallback& callback) :
  IPlayer         (callback),
  m_isPlaying     (false),
  m_isPaused      (false),
  m_isFinished    (false),
  m_currentStream (NULL ),
  m_audioCallback (NULL )
{
  m_startEvent.Reset();
}

PAPlayer::~PAPlayer()
{
  if (!m_isPaused)
    SoftStop(true, true);
  CloseAllStreams(false);  
  
  /* wait for the thread to terminate */
  m_isPlaying = false;
  CSingleLock lock(m_threadLock);
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
        si->m_stream->Destroy();
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
        si->m_stream->Destroy();
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
  
  if (ThreadHandle() == NULL)
    Create();
  
  /* trigger playback start */
  m_isPlaying = true;
  m_startEvent.Set();
  return true;
}

bool PAPlayer::QueueNextFile(const CFileItem &file)
{
  return QueueNextFileEx(file);
}

bool PAPlayer::QueueNextFileEx(const CFileItem &file, bool fadeIn/* = true */)
{
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
  }
  
  /* init the streaminfo struct */
  si->m_decoder.GetDataFormat(&si->m_channels, &si->m_sampleRate, &si->m_dataFormat);
  si->m_bytesPerSample     = CAEUtil::DataFormatToBits(si->m_dataFormat) >> 3;
  si->m_started            = false;
  si->m_finishing          = false;
  si->m_samplesSent        = 0;
  si->m_stream             = NULL;
  si->m_volume             = (fadeIn && m_crossFadeTime) ? 0.0f : 1.0f;
  
  si->m_prepareNextAtSample = (si->m_decoder.TotalTime() - TIME_TO_CACHE_NEXT_FILE - m_crossFadeTime) * (si->m_sampleRate * si->m_channels) / 1000.0f;
  si->m_prepareTriggered    = false;
  si->m_playNextAtSample    = (si->m_decoder.TotalTime() - m_crossFadeTime) * (si->m_sampleRate * si->m_channels) / 1000.0f;
  si->m_playNextTriggered   = false;
   
  /* add the stream to the list */
  CExclusiveLock lock(m_streamsLock);
  m_streams.push_back(si);

  return true;
}

inline bool PAPlayer::PrepareStream(StreamInfo *si)
{
  /* if we have a stream we are already prepared */
  if (si->m_stream)
    return true;
  
  /* get a paused stream */
  si->m_stream = CAEFactory::AE->GetStream(
    si->m_dataFormat,
    si->m_sampleRate,
    CAEUtil::GuessChLayout(si->m_channels), /* FIXME: channelLayout */
    AESTREAM_PAUSED
  );

  if (!si->m_stream)
  {
    CLog::Log(LOGDEBUG, "PAPlayer::PrepareStream - Failed to get IAEStream");
    return false;
  }
  
  si->m_stream->SetVolume    (si->m_volume);
  si->m_stream->SetReplayGain(si->m_decoder.GetReplayGain());
  
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
  CSingleLock lock(m_threadLock);
  if (!m_startEvent.WaitMSec(100))
  {
    CLog::Log(LOGDEBUG, "PAPlayer::Process - Failed to receive start event");
    return;
  }

  CLog::Log(LOGDEBUG, "PAPlayer::Process - Playback started");  

  while(m_isPlaying)
  {
    float delay = 1.0f;
    ProcessStreams(delay);

    /* try to keep the buffer 75% full */
    const float mul = 13.333333333f;
    delay = delay * mul;    
    Sleep(MathUtils::round_int(delay));
  }
  
  m_callback.OnPlayBackEnded();
}

inline void PAPlayer::ProcessStreams(float &delay)
{
  CSharedLock sharedLock(m_streamsLock);
  if (m_isFinished && m_streams.empty() && m_finishing.empty())
  {
    m_callback.OnPlayBackStopped();    
    m_isPlaying = false;
    delay       = 0;
    return;
  }

  /* destroy any drained streams */
  for(StreamList::iterator itt = m_finishing.begin(); itt != m_finishing.end();)
  {
    StreamInfo* si = *itt;
    if (si->m_stream->IsDrained())
    {      
      itt = m_finishing.erase(itt);
      si->m_stream->Destroy();
      delete si;
    }
    else
      ++itt;    
  }
    
  for(StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
  {
    StreamInfo* si = *itt;
    if (!m_currentStream && !si->m_started)
      m_currentStream = si;
        
    if ((si->m_fadeOutTriggered && si->m_stream && !si->m_stream->IsFading()) || !PrepareStream(si) || !ProcessStream(si, delay))
    {
      /* if the stream is finshed */
      sharedLock.Leave();
      CExclusiveLock lock(m_streamsLock);

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
    
    /* is it time to prepare the next stream? */
    if (!si->m_prepareTriggered && si->m_samplesSent >= si->m_prepareNextAtSample)
    {
      si->m_prepareTriggered = true;
      m_callback.OnQueueNextItem();
    }
    
    /* it is time to start playing the next stream? */
    if (!si->m_playNextTriggered && si->m_samplesSent >= si->m_playNextAtSample)
    {
      if (!m_isFinished)
      {
        if (m_crossFadeTime)
          si->m_stream->FadeVolume(1.0f, 0.0f, m_crossFadeTime);
        m_currentStream = NULL;

        /* unregister the audio callback */
        si->m_stream->UnRegisterAudioCallback();
      }

      si->m_playNextTriggered = true;      
    }
  }
}

inline bool PAPlayer::ProcessStream(StreamInfo *si, float &delay)
{
  /* if playback needs to start on this stream, do it */
  if (si == m_currentStream && !si->m_started)
  {
    si->m_started = true;
    si->m_stream->RegisterAudioCallback(m_audioCallback);
    si->m_stream->Resume();
    si->m_stream->FadeVolume(0.0f, 1.0f, m_crossFadeTime);
    m_callback.OnPlayBackStarted();
  }

  /* if we have not started yet and the stream has been primed */
  unsigned int space = si->m_stream->GetSpace();
  if (!si->m_started && !space)
    return true;

  int status = si->m_decoder.GetStatus();
  if (status == STATUS_ENDED   ||
      status == STATUS_NO_FILE ||
      si->m_decoder.ReadSamples(PACKET_SIZE) == RET_ERROR)
  {
    CLog::Log(LOGINFO, "PAPlayer::ProcessStream - Stream Finished");
    return false;
  }

  /* calculate the data size */
  unsigned int size = std::min(si->m_decoder.GetDataSize(), space / si->m_bytesPerSample);
  if (!size)
  {
    /* update the delay time if we are running */
    if (si->m_started)
      delay = std::min(delay, si->m_stream->GetCacheTime());  
    return true;
  }
  
  void* data = si->m_decoder.GetData(size);
  if (!data)
  {
    CLog::Log(LOGERROR, "PAPlayer::ProcessStream - Failed to get data from the decoder");
    return false;
  }
  
  unsigned int added = si->m_stream->AddData(data, size * si->m_bytesPerSample);
  si->m_samplesSent += added / si->m_bytesPerSample;

  /* update the delay time if we are running */
  if (si->m_started)  
    delay = std::min(delay, si->m_stream->GetCacheTime());  
  
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

}

__int64 PAPlayer::GetTime()
{
  CSharedLock lock(m_streamsLock);
  if (m_currentStream)
  {
    float time = (float)m_currentStream->m_samplesSent / (float)(m_currentStream->m_sampleRate * m_currentStream->m_channels) * 1000.0f;
    if (m_currentStream->m_stream)
      time -= m_currentStream->m_stream->GetDelay();
    return time;
  }
  return 0;
}

int PAPlayer::GetTotalTime()
{
  CSharedLock lock(m_streamsLock);
  if (m_currentStream)
    return m_currentStream->m_decoder.TotalTime();
  return 0;
}

int PAPlayer::GetCacheLevel() const
{
  CSharedLock lock(m_streamsLock);
  if (m_currentStream)
  {
    const ICodec* codec = m_currentStream->m_decoder.GetCodec();
    if (codec)
      return codec->GetCacheLevel();
  }
  
  return -1;
}

int PAPlayer::GetChannels()
{
  CSharedLock lock(m_streamsLock);
  if (m_currentStream)
    return m_currentStream->m_channels;
  return 0;
}

int PAPlayer::GetBitsPerSample()
{
  CSharedLock lock(m_streamsLock);
  if (m_currentStream)
    return m_currentStream->m_bytesPerSample >> 3;
  
  return 0;
}

int PAPlayer::GetSampleRate()
{
  CSharedLock lock(m_streamsLock);
  if (m_currentStream)
    return m_currentStream->m_sampleRate;
  
  return 0;
}

CStdString PAPlayer::GetAudioCodecName()
{
  CSharedLock lock(m_streamsLock);
  if (m_currentStream)
  {
    const ICodec* codec = m_currentStream->m_decoder.GetCodec();
    if (codec)
      return codec->m_CodecName;
  }
  
  return "";
}

int PAPlayer::GetAudioBitrate()
{
  CSharedLock lock(m_streamsLock);
  if (m_currentStream)
  {
    const ICodec* codec = m_currentStream->m_decoder.GetCodec();
    if (codec)
      return codec->m_Bitrate;
  }

  return 0;
}

bool PAPlayer::CanSeek()
{
  return false;
}

void PAPlayer::Seek(bool bPlus, bool bLargeStep)
{

}

void PAPlayer::SeekTime(__int64 iTime /*=0*/)
{

}

void PAPlayer::SeekPercentage(float fPercent /*=0*/)
{

}

float PAPlayer::GetPercentage()
{
  return GetTime() * 100.0f / GetTotalTime();
}

bool PAPlayer::SkipNext()
{
  return false;
}
