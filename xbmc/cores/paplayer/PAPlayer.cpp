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
#include "../../utils/GUIInfoManager.h"
#include "AudioContext.h"
#include "../../FileSystem/FileShoutcast.h"
#include "../../Application.h"
#include "FileItem.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "Settings.h"
#include "MusicInfoTag.h"
#include "../AudioRenderers/AudioRendererFactory.h"
#include "../../utils/TimeUtils.h"
#include "utils/log.h"

#include "utils/SingleLock.h"

#include "AudioEngine/AEUtil.h"

#define TIME_TO_CACHE_NEXT_FILE 5000 /* 5 seconds */
#define FAST_XFADE_TIME         2000 /* 2 seconds */

// PAP: Psycho-acoustic Audio Player
// Supporting all open  audio codec standards.
// First one being nullsoft's nsv audio decoder format

PAPlayer::PAPlayer(IPlayerCallback& callback) :
  IPlayer        (callback),
  m_current      (NULL    ),
  m_isPaused     (false   ),
  m_iSpeed       (1       ),
  m_fastOpen     (true    ),
  m_queueFailed  (false   ),
  m_playOnQueue  (false   )
{
}

PAPlayer::~PAPlayer()
{
  CloseFile();
}

bool PAPlayer::CloseFile()
{
  CSingleLock lock(m_critSection);

  if (m_current)
  {
    FreeStreamInfo(m_current);
    m_current = NULL;
  }

  while(!m_streams.empty())
  {
    FreeStreamInfo(m_streams.front());
    m_streams.pop_front();
  }

  while(!m_finishing.empty())
  {
    FreeStreamInfo(m_finishing.front());
    m_finishing.pop_front();
  }

  m_iSpeed = 1;

  return true;
}

void PAPlayer::OnExit()
{
}

void PAPlayer::FreeStreamInfo(StreamInfo *si)
{
  m_finishing.remove(si);
  si->m_decoder.Destroy();
  if (si->m_stream)
    si->m_stream->Destroy();
  delete si;
}

bool PAPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  m_iSpeed = 1;
  m_fastOpen = true;

  if (!QueueNextFile(file))
    return false;

  CSingleLock lock(m_critSection);
  return PlayNextStream();
}

void PAPlayer::StaticStreamOnData(CAEStream *sender, void *arg, unsigned int needed)
{
  StreamInfo *si  = (StreamInfo*)arg;
  PAPlayer   *pap = si->m_player;

  CSingleLock lock(pap->m_critSection);

  while(si->m_decoder.GetDataSize() == 0)
  {
    int status = si->m_decoder.GetStatus();
    if (status == STATUS_ENDED || status == STATUS_NO_FILE || si->m_decoder.ReadSamples(PACKET_SIZE) == RET_ERROR)
    {
      if (!si->m_triggered)
      {
        si->m_triggered = true;
        pap->PlayNextStream();
      }
      if (!si->m_stream->IsDraining())
        si->m_stream->Drain();
      return;
    }
  }

  /* convert needed frames to needed samples */
  needed *= sender->GetChannelCount();
  while(needed > 0) {
    unsigned int samples = std::min(si->m_decoder.GetDataSize(), needed);
    if (samples == 0) break;

    void *data = si->m_decoder.GetData(samples);
    si->m_stream->AddData(data, samples * si->m_bytesPerSample);
    si->m_sent += samples;
    needed     -= samples;
  }

  /* handle ff/rw */
  int speed = pap->m_iSpeed;
  if (!si->m_triggered && (speed != 1 && si->m_sent >= si->m_snippetEnd))
  {
    float step = (speed > 1 ? 0.5 : 1.0f) * ((float)speed / 2.0f);
    int   bps  = si->m_stream->GetSampleRate() * si->m_stream->GetChannelCount();
    float time = ((float)si->m_sent / (float)bps) + step;
    if (time <= 0.0f)
    {
      si->m_snippetEnd = 0;
      pap->m_iSpeed    = 1;

      pap->m_callback.OnPlayBackSpeedChanged(1);
      time = 0.0f;
    }

    float ttl = (float)si->m_decoder.TotalTime() / 1000.0f;
    if (time >= ttl)
      time = ttl;
      
    si->m_decoder.Seek(time * 1000.0f);
    si->m_sent       = time * bps;

    if (speed < 1) speed = -speed;
    si->m_snippetEnd = si->m_sent + (bps / speed);
  }

  /* if it is time to prepare the next stream */
  if (si->m_prepare > 0 && si->m_sent >= si->m_prepare)
  {
    si->m_prepare = 0;
    lock.Leave();
    pap->m_callback.OnQueueNextItem();
    return;
  }

  /* if it is time to move to the next stream */
  if (!si->m_triggered && si->m_sent >= si->m_change)
  {
    si->m_triggered = true;
    lock.Leave();
    pap->PlayNextStream();
    return;
  }
}

void PAPlayer::StaticStreamOnDrain(CAEStream *sender, void *arg, unsigned int unused)
{
  StreamInfo *si = (StreamInfo*)arg;
  PAPlayer *player = si->m_player;
  CSingleLock lock(player->m_critSection);

  if (si == player->m_current)
    player->m_current = NULL;

  player->FreeStreamInfo(si);
}

void PAPlayer::StaticFadeOnDone(CAEPPAnimationFade *sender, void *arg)
{
  StreamInfo *si = (StreamInfo*)arg;
  CSingleLock lock(si->m_player->m_critSection);

  /* the fadeout has completed rendering, so start draining */
  si->m_decoder.SetStatus(STATUS_ENDED);
  si->m_stream->Drain();
}

void PAPlayer::OnNothingToQueueNotify()
{
  m_queueFailed = true;
  if (m_playOnQueue)
    m_callback.OnPlayBackStopped();
}

bool PAPlayer::QueueNextFile(const CFileItem &file)
{
  StreamInfo *si = new StreamInfo();
  if (!si->m_decoder.Create(file, (file.m_lStartOffset * 1000) / 75))
  {
    delete si;
    return false;
  }

  unsigned int channels, sampleRate;
  enum AEDataFormat dataFormat;
  si->m_decoder.GetDataFormat(&channels, &sampleRate, &dataFormat);

  si->m_player         = this;
  si->m_sent           = 0;
  si->m_change         = 0;
  si->m_triggered      = false;
  si->m_bytesPerSample = CAEUtil::DataFormatToBits(dataFormat) >> 3;
  si->m_snippetEnd     = (sampleRate * channels) / (m_iSpeed > 1 ? m_iSpeed : -m_iSpeed);

  si->m_stream = AE.GetStream(
    dataFormat,
    sampleRate,
    channels,
    NULL, /* FIXME: channelLayout */
    true, /* free on drain */
    true  /* owns post-proc filters */
  );

  if (!si->m_stream)
  {
    FreeStreamInfo(si);
    return false;
  }

  /* pause the stream and set the callbacks */
  si->m_stream->Pause();
  si->m_stream->SetDataCallback (StaticStreamOnData , si);
  si->m_stream->SetDrainCallback(StaticStreamOnDrain, si);
  si->m_stream->SetReplayGain   (si->m_decoder.GetReplayGain());

  unsigned int crossFade = g_guiSettings.GetInt("musicplayer.crossfade") * 1000;
  unsigned int cacheTime = (crossFade * 1000) + TIME_TO_CACHE_NEXT_FILE;
  si->m_decoder.Start();
  si->m_change  = (si->m_decoder.TotalTime() - crossFade) * (sampleRate * channels) / 1000.0f;
  si->m_prepare = (si->m_decoder.TotalTime() - cacheTime) * (sampleRate * channels) / 1000.0f;

  /* buffer some audio packets */
  si->m_decoder.ReadSamples(PACKET_SIZE);

  /* queue the stream */  
  CSingleLock lock(m_critSection);
  m_streams.push_back(si);
  lock.Leave();

  if (m_playOnQueue)
  {
    PlayNextStream();
    m_playOnQueue = false;
  }

  return true;
}

bool PAPlayer::PlayNextStream()
{
  bool         fadeIn    = false;
  unsigned int crossFade = g_guiSettings.GetInt("musicplayer.crossfade") * 1000;

  /* if there is no more queued streams then flag to start on queue */
  CSingleLock lock(m_critSection);
  if (m_streams.empty())
  {
    if (!m_queueFailed) m_playOnQueue = true;
    else
      m_callback.OnPlayBackStopped();

    return false;
  }

  /* if there is a currently playing stream, stop it */
  if (m_current)
  {
    m_finishing.push_back(m_current);
    if (!crossFade) {
      if (!m_current->m_stream->IsDraining())
        m_current->m_stream->Drain();
      if (m_fastOpen) m_current->m_stream->Flush();
      m_fastOpen = false;
    }
    else
    {
      /* if the user is skipping tracks quickly, do a fast crossFade */
      if (!m_finishing.empty())
        crossFade = std::min((unsigned int)FAST_XFADE_TIME, crossFade);

      fadeIn = true;
      CAEPPAnimationFade *fade = new CAEPPAnimationFade(1.0f, 0.0f, crossFade);
      fade->SetDoneCallback(StaticFadeOnDone, m_current);
      fade->SetPosition(1.0f);
      m_current->m_stream->PrependPostProc(fade);
      fade->Run();
    }
  }

  /* get the next stream */
  m_current = m_streams.front();
  m_streams.pop_front();

  /* if we are crossFading, fade it in */
  if (fadeIn)
  {
    CAEPPAnimationFade *fade = new CAEPPAnimationFade(0.0f, 1.0f, crossFade);
    fade->SetPosition(0.0f);
    m_current->m_stream->PrependPostProc(fade);
    fade->Run();
  }

  m_isPaused  = false;

  /* start playback */
  m_callback.OnPlayBackStarted();
  m_current->m_stream->Resume();
  return true;
}

void PAPlayer::Pause()
{
  if (!IsPlaying()) return;
  CSingleLock lock(m_critSection);

  m_isPaused = !m_isPaused;
  if (m_isPaused)
  {
    m_current->m_stream->Pause();
    list<StreamInfo*>::iterator itt;
    for(itt = m_finishing.begin(); itt != m_finishing.end(); ++itt)
      (*itt)->m_stream->Pause();
    m_callback.OnPlayBackPaused();
    CLog::Log(LOGDEBUG, "PAPlayer: Playback paused");
  }
  else
  {
    m_current->m_stream->Resume();
    list<StreamInfo*>::iterator itt;
    for(itt = m_finishing.begin(); itt != m_finishing.end(); ++itt)
      (*itt)->m_stream->Resume();
    m_callback.OnPlayBackResumed();
    CLog::Log(LOGDEBUG, "PAPlayer: Playback resumed");
  }
}

void PAPlayer::SetVolume(float volume)
{
}

void PAPlayer::SetDynamicRangeCompression(long drc)
{
}

void PAPlayer::Process()
{
  /* we dont use this, this player uses a pull model */
}

void PAPlayer::ToFFRW(int iSpeed)
{
  CSingleLock lock(m_critSection);
  m_iSpeed = iSpeed;
  if (!m_current) return;

  m_current->m_snippetEnd = m_current->m_sent;
  m_callback.OnPlayBackSpeedChanged(m_iSpeed);
}

#if 0
bool PAPlayer::ProcessPAP()
{
  /*
   * Here's what we should be doing in each player loop:
   *
   * 1.  Run DoWork() on our audio device to actually output audio.
   *
   * 2.  Pass our current buffer to the audio device to see if it wants anything,
   *     and if so, reduce our buffer size accordingly.
   *
   * 3.  Check whether we have space in our buffer for more data, and if so,
   *     read some more in.
   *
   * 4.  Check for end of file and return false if we reach it.
   *
   * 5.  Perform any seeking and ffwd/rewding as necessary.
   *
   * 6.  If we don't do anything in 2...5, we can take a breather and break out for sleeping.
   */
  while (true)
  {
    if (m_bStop) return false;

    // Check for .cue sheet item end
    if (m_currentFile->m_lEndOffset && GetTime() >= GetTotalTime64())
    {
      CLog::Log(LOGINFO, "PAPlayer: Passed end of track in a .cue sheet item");
      m_decoder[m_currentDecoder].SetStatus(STATUS_ENDED);
    }

    // check whether we need to send off our callbacks etc.
    int status = m_decoder[m_currentDecoder].GetStatus();
    if (status == STATUS_NO_FILE)
      return false;

    UpdateCacheLevel();

    // check whether we should queue the next file up
    if ((GetTotalTime64() > 0) && GetTotalTime64() - GetTime() < TIME_TO_CACHE_NEXT_FILE + m_crossFading * 1000L && !m_cachingNextFile)
    { // request the next file from our application
      m_callback.OnQueueNextItem();
      m_cachingNextFile = true;
    }

    if (m_crossFading && m_decoder[0].GetChannels() == m_decoder[1].GetChannels())
    {
      if (((GetTotalTime64() - GetTime() < m_crossFading * 1000L) || (m_forceFadeToNext)) && !m_currentlyCrossFading)
      { // request the next file from our application
        if (m_decoder[1 - m_currentDecoder].GetStatus() == STATUS_QUEUED && m_pAudioStream[1 - m_currentStream])
        {
          m_currentlyCrossFading = true;
          m_currentDecoder = 1 - m_currentDecoder;
          m_decoder[m_currentDecoder].Start();
          m_currentStream = 1 - m_currentStream;
          CLog::Log(LOGDEBUG, "Starting Crossfade - resuming stream %i", m_currentStream);

          m_pAudioStream[m_currentStream]->Resume();

          m_callback.OnPlayBackStarted();
          m_timeOffset = m_nextFile->m_lStartOffset * 1000 / 75;
          m_bytesSentOut = 0;
          *m_currentFile = *m_nextFile;
          m_nextFile->Reset();
          m_cachingNextFile = false;
        }
      }
    }

    // Check for EOF and queue the next track if applicable
    if (m_decoder[m_currentDecoder].GetStatus() == STATUS_ENDED)
    { // time to swap tracks
      if (m_nextFile->m_strPath != m_currentFile->m_strPath ||
          !m_nextFile->m_lStartOffset ||
          m_nextFile->m_lStartOffset != m_currentFile->m_lEndOffset)
      { // don't have a .cue sheet item
        int nextstatus = m_decoder[1 - m_currentDecoder].GetStatus();
        if (nextstatus == STATUS_QUEUED || nextstatus == STATUS_QUEUING || nextstatus == STATUS_PLAYING)
        { // swap streams
          CLog::Log(LOGDEBUG, "PAPlayer: Swapping tracks %i to %i", m_currentDecoder, 1-m_currentDecoder);
          if (!m_crossFading || m_decoder[0].GetChannels() != m_decoder[1].GetChannels())
          { // playing gapless (we use only the 1 output stream in this case)
            int prefixAmount = m_decoder[m_currentDecoder].GetDataSize();
            CLog::Log(LOGDEBUG, "PAPlayer::Prefixing %i samples of old data to new track for gapless playback", prefixAmount);
            m_decoder[1 - m_currentDecoder].PrefixData(m_decoder[m_currentDecoder].GetData(prefixAmount), prefixAmount);
            // check if we need to change the resampler (due to format change)
            unsigned int channels, samplerate, bitspersample;
            m_decoder[m_currentDecoder].GetDataFormat(&channels, &samplerate, &bitspersample);
            unsigned int channels2, samplerate2, bitspersample2;
            m_decoder[1 - m_currentDecoder].GetDataFormat(&channels2, &samplerate2, &bitspersample2);
            // change of channels - reinitialize our speaker configuration
            if (channels != channels2 || samplerate != samplerate2 || bitspersample != bitspersample2)
            {
              CLog::Log(LOGINFO, "PAPlayer: Stream properties have changed, restarting stream");
              FreeStream(m_currentStream);
              if (!CreateStream(m_currentStream, channels2, samplerate2, bitspersample2))
              {
                CLog::Log(LOGERROR, "PAPlayer: Error creating stream!");
                return false;
              }
              m_pAudioStream[m_currentStream]->Resume();
            }
            CLog::Log(LOGINFO, "PAPlayer: Starting new track");

            m_decoder[m_currentDecoder].Destroy();
            m_decoder[1 - m_currentDecoder].Start();
            m_callback.OnPlayBackStarted();
            m_timeOffset = m_nextFile->m_lStartOffset * 1000 / 75;
            m_bytesSentOut = 0;
            *m_currentFile = *m_nextFile;
            m_nextFile->Reset();
            m_cachingNextFile = false;
            m_currentDecoder = 1 - m_currentDecoder;
          }
          else
          { // cross fading - shouldn't ever get here - if we do, return false
            if (!m_currentlyCrossFading)
            {
              CLog::Log(LOGERROR, "End of file Reached before crossfading kicked in!");
              return false;
            }
            else
            {
              CLog::Log(LOGINFO, "End of file reached before crossfading finished!");
              return false;
            }
          }
        }
        else
        {
          if (GetTotalTime64() <= 0 && !m_bQueueFailed)
          { //we did not know the duration so didn't queue the next song, try queueing it now
            if (!m_cachingNextFile)
            {// request the next file from our application
              m_callback.OnQueueNextItem();
              m_cachingNextFile = true;
            }
          }
          else
          {
            // no track queued - return and get another one once we are finished
            // with the current stream
            WaitForStream();
            return false;
          }
        }
      }
      else
      {
        // set the next track playing (.cue sheet)
        m_decoder[m_currentDecoder].SetStatus(STATUS_PLAYING);
        m_callback.OnPlayBackStarted();
        m_timeOffset = m_nextFile->m_lStartOffset * 1000 / 75;
        m_bytesSentOut = 0;
        *m_currentFile = *m_nextFile;
        m_nextFile->Reset();
        m_cachingNextFile = false;
      }
    }

    // handle seeking and ffwd/rewding.
    HandleSeeking();
    if (!HandleFFwdRewd())
    {
      // need to skip to the next track - let's see if we already have another one
      m_decoder[m_currentDecoder].SetStatus(STATUS_ENDED);
      continue; // loop around to start the next track
    }

    if (!m_bPaused)
    {

      // Let our decoding stream(s) do their thing
      int retVal = m_decoder[m_currentDecoder].ReadSamples(PACKET_SIZE);
      if (retVal == RET_ERROR)
      {
        m_decoder[m_currentDecoder].Destroy();
        return false;
      }

      int retVal2 = m_decoder[1 - m_currentDecoder].ReadSamples(PACKET_SIZE);
      if (retVal2 == RET_ERROR)
      {
        m_decoder[1 - m_currentDecoder].Destroy();
      }

      // add packets as necessary
      if (AddPacketsToStream(m_currentStream, m_decoder[m_currentDecoder]))
        retVal = RET_SUCCESS;

      if (retVal == RET_SLEEP && retVal2 == RET_SLEEP)
      {
        float maximumSleepTime = 0; /* m_pAudioStream[m_currentStream]->GetCacheTime(); FIXME */
        
        if (m_pAudioStream[1 - m_currentStream])
          maximumSleepTime = std::min(maximumSleepTime, 0.0f); /*m_pAudioStream[1 - m_currentStream]->GetCacheTime()); FIXME*/

        int sleep = std::max((int)((maximumSleepTime / 2.0f) * 1000.0f), 1);

        Sleep(std::min(sleep, 15));
      }
    }
    else
      Sleep(100);
  }
  return true;
}
#endif

void PAPlayer::ResetTime()
{
  if (!m_current) return;
  m_current->m_sent = 0;
}

__int64 PAPlayer::GetTime()
{
  if (!m_current) return 0;
  return (float)m_current->m_sent / (float)(m_current->m_stream->GetSampleRate() * m_current->m_stream->GetChannelCount()) * 1000.0f;
}

int PAPlayer::GetTotalTime()
{
  if (!m_current) return 0;
  return m_current->m_decoder.TotalTime();
}

int PAPlayer::GetCacheLevel() const
{
  if (!m_current) return -1;
  const ICodec* codec = m_current->m_decoder.GetCodec();
  if (codec)
    return codec->GetCacheLevel();

  return -1;
}

int PAPlayer::GetChannels()
{
  if (!m_current) return 0;
  const ICodec* codec = m_current->m_decoder.GetCodec();
  if (codec)
    return codec->m_Channels;

  return 0;
}

int PAPlayer::GetBitsPerSample()
{
  if (!m_current) return 0;
  const ICodec* codec = m_current->m_decoder.GetCodec();
  if (codec)
    return codec->m_BitsPerSample;

  return 0;
}

int PAPlayer::GetSampleRate()
{
  if (!m_current) return 0;
  const ICodec* codec = m_current->m_decoder.GetCodec();
  if (codec)
    return (codec->m_SampleRate / 1000) + 0.5;

  return 0;
}

CStdString PAPlayer::GetAudioCodecName()
{
  if (!m_current) return "";
  const ICodec* codec = m_current->m_decoder.GetCodec();
  if (codec)
    return codec->m_CodecName;

  return "";
}

int PAPlayer::GetAudioBitrate()
{
  if (!m_current) return 0;
  const ICodec* codec = m_current->m_decoder.GetCodec();
  if (codec)
    return codec->m_Bitrate;
  return 0;
}

bool PAPlayer::CanSeek()
{
  return m_current && m_current->m_decoder.CanSeek();
}

void PAPlayer::Seek(bool bPlus, bool bLargeStep)
{
  __int64 seek;
  if (g_advancedSettings.m_musicUseTimeSeeking && GetTotalTime() > 2*g_advancedSettings.m_musicTimeSeekForwardBig)
  {
    if (bLargeStep)
      seek = bPlus ? g_advancedSettings.m_musicTimeSeekForwardBig : g_advancedSettings.m_musicTimeSeekBackwardBig;
    else
      seek = bPlus ? g_advancedSettings.m_musicTimeSeekForward : g_advancedSettings.m_musicTimeSeekBackward;
    seek *= 1000;
    seek += GetTime();
  }
  else
  {
    float percent;
    if (bLargeStep)
      percent = bPlus ? (float)g_advancedSettings.m_musicPercentSeekForwardBig : (float)g_advancedSettings.m_musicPercentSeekBackwardBig;
    else
      percent = bPlus ? (float)g_advancedSettings.m_musicPercentSeekForward : (float)g_advancedSettings.m_musicPercentSeekBackward;
    seek = (int64_t)(GetTotalTime() * (GetPercentage() + percent) / 100);
  }

  SeekTime(seek);
}

void PAPlayer::SeekTime(__int64 iTime /*=0*/)
{
  CSingleLock lock(m_critSection);
  if (!CanSeek() || !m_current) return;

  int seekOffset  = (int)(iTime - GetTime());

  int seekSamples = ((float)seekOffset / 1000.0f) * ((float)(m_current->m_stream->GetSampleRate() * m_current->m_stream->GetChannelCount()));
  seekSamples = std::min(-(int)m_current->m_sent, seekSamples);

  m_callback.OnPlayBackSeek(iTime, seekOffset);
  CLog::Log(LOGDEBUG, "PAPlayer::Seeking to time %f (%d)", 0.001f * iTime, seekSamples);
  m_current->m_decoder.Seek(seekOffset);
  m_current->m_stream->Flush();
  m_current->m_sent += seekSamples;
  g_infoManager.m_performingSeek = false;
}

void PAPlayer::SeekPercentage(float fPercent /*=0*/)
{
  if (fPercent < 0.0f) fPercent = 0.0f;
  if (fPercent > 100.0f) fPercent = 100.0f;
  SeekTime((int64_t)(fPercent * 0.01f * (float)GetTotalTime()));
}

float PAPlayer::GetPercentage()
{
  float percent = (float)GetTime() * 100.0f / GetTotalTime();
  return percent;
}

/*
void PAPlayer::HandleSeeking()
{
  if (m_SeekTime != -1)
  {
    DWORD time = CTimeUtils::GetTimeMS();
    m_timeOffset = m_decoder[m_currentDecoder].Seek(m_SeekTime);
    CLog::Log(LOGDEBUG, "Seek to time %f took %i ms", 0.001f * m_SeekTime, CTimeUtils::GetTimeMS() - time);
    FlushStreams();
    m_SeekTime = -1;
  }
  g_infoManager.m_performingSeek = false;
}
*/

#if 0
void PAPlayer::FlushStreams()
{
  m_bytesSentOut = 0;
  for (int stream = 0; stream < 2; stream++)
  {
    if (m_pAudioStream[stream] && m_packet[stream])
    {
      m_pAudioStream[stream]->Flush();
      m_bufferPos[stream] = 0;
    }
  }
}
#endif

#if 0
bool PAPlayer::HandleFFwdRewd()
{
  if (!m_IsFFwdRewding && m_iSpeed == 1)
    return true;  // nothing to do
  if (m_IsFFwdRewding && m_iSpeed == 1)
  { // stop ffwd/rewd
    m_IsFFwdRewding = false;
    SetVolume(g_settings.m_fVolumeLevel);
    FlushStreams();
    return true;
  }
  // we're definitely fastforwarding or rewinding
  int snippet = m_BytesPerSecond / 2;
  if ( m_bytesSentOut >= snippet )
  {
    // Calculate offset to seek if we do FF/RW
    __int64 time = GetTime();
    if (m_IsFFwdRewding) snippet = (int)m_bytesSentOut;
    time += (int64_t)((double)snippet * (m_iSpeed - 1.0) / m_BytesPerSecond * 1000.0);

    // Is our offset inside the track range?
    if (time >= 0 && time <= m_decoder[m_currentDecoder].TotalTime())
    { // just set next position to read
      m_IsFFwdRewding = true;
      time += m_currentFile->m_lStartOffset * 1000 / 75;
      m_timeOffset = m_decoder[m_currentDecoder].Seek(time);
      FlushStreams();
      SetVolume(g_settings.m_fVolumeLevel - VOLUME_FFWD_MUTE); // override xbmc mute
    }
    else if (time < 0)
    { // ...disable seeking and start the track again
      time = m_currentFile->m_lStartOffset * 1000 / 75;
      m_timeOffset = m_decoder[m_currentDecoder].Seek(time);
      FlushStreams();
      m_iSpeed = 1;
      SetVolume(g_settings.m_fVolumeLevel); // override xbmc mute
    } // is our next position greater then the end sector...
    else //if (time > m_codec->m_TotalTime)
    {
      // restore volume level so the next track isn't muted
      SetVolume(g_settings.m_fVolumeLevel);
      CLog::Log(LOGDEBUG, "PAPlayer: End of track reached while seeking");
      return false;
    }
  }
  return true;
}
#endif

bool PAPlayer::HandlesType(const CStdString &type)
{
  ICodec* codec=CodecFactory::CreateCodec(type);

  if (codec && codec->CanInit())
  {
    delete codec;
    return true;
  }

  if (codec)
    delete codec;

  return false;
}

bool PAPlayer::SkipNext()
{
  /* Skip to next track/item inside the current media (if supported). */
  if (!m_current) return false;
  return (m_current->m_decoder.GetCodec() && m_current->m_decoder.GetCodec()->SkipNext());
}
