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
  m_audioCallback(NULL    ),
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
    FreeStreamInfo(m_current);

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
  m_callback.OnPlayBackStopped();
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

  if (m_current == si)
    m_current = NULL;

  delete si;
}

void PAPlayer::RegisterAudioCallback(IAudioCallback* pCallback)
{
  CSingleLock lock(m_critSection);
  m_audioCallback = pCallback;
  if (m_current)
  {
    m_current->m_stream->UnRegisterAudioCallback();
    m_current->m_stream->RegisterAudioCallback(pCallback);
  }
}

void PAPlayer::UnRegisterAudioCallback()
{
  CSingleLock lock(m_critSection);
  if (m_current)
    m_current->m_stream->UnRegisterAudioCallback();

  m_audioCallback = NULL;
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

void PAPlayer::StaticStreamOnData(IAEStream *sender, void *arg, unsigned int needed)
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
  while(needed > 0)
  {
    unsigned int samples = std::min(std::min(si->m_decoder.GetDataSize(), needed), (unsigned int)OUTPUT_SAMPLES);
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
    float step = (speed > 1 ? 0.5f : 1.0f) * ((float)speed / 2.0f);
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

void PAPlayer::StaticStreamOnDrain(IAEStream *sender, void *arg, unsigned int unused)
{
  StreamInfo *si = (StreamInfo*)arg;
  si->m_stream->UnRegisterAudioCallback();
  PAPlayer *player = si->m_player;
  CSingleLock lock(player->m_critSection);
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
    AESTREAM_FREE_ON_DRAIN | AESTREAM_OWNS_POST_PROC
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
    m_current->m_stream->UnRegisterAudioCallback();
    m_finishing.push_back(m_current);
    if (!crossFade)
    {
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

  if (m_audioCallback)
    m_current->m_stream->RegisterAudioCallback(m_audioCallback);

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
    std::list<StreamInfo*>::iterator itt;
    for(itt = m_finishing.begin(); itt != m_finishing.end(); ++itt)
      (*itt)->m_stream->Pause();
    m_callback.OnPlayBackPaused();
    CLog::Log(LOGDEBUG, "PAPlayer: Playback paused");
  }
  else
  {
    m_current->m_stream->Resume();
    std::list<StreamInfo*>::iterator itt;
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

void PAPlayer::ResetTime()
{
  if (!m_current) return;
  m_current->m_sent       = 0;
  m_current->m_snippetEnd = 0;
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
