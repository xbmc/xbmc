/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AudioDecoder.h"
#include "CodecFactory.h"
#include "Application.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "music/tags/MusicInfoTag.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include <math.h>

CAudioDecoder::CAudioDecoder()
{
  m_codec = NULL;

  m_eof = false;

  m_status = STATUS_NO_FILE;
  m_canPlay = false;

  // output buffer (for transferring data from the Pcm Buffer to the rest of the audio chain)
  memset(&m_outputBuffer, 0, OUTPUT_SAMPLES * sizeof(float));
  memset(&m_pcmInputBuffer, 0, INPUT_SIZE * sizeof(BYTE));
  memset(&m_inputBuffer, 0, INPUT_SAMPLES * sizeof(float));

}

CAudioDecoder::~CAudioDecoder()
{
  Destroy();
}

void CAudioDecoder::Destroy()
{
  CSingleLock lock(m_critSection);
  m_status = STATUS_NO_FILE;

  m_pcmBuffer.Destroy();

  if ( m_codec )
    delete m_codec;
  m_codec = NULL;

  m_canPlay = false;
}

bool CAudioDecoder::Create(const CFileItem &file, int64_t seekOffset)
{
  Destroy();

  CSingleLock lock(m_critSection);

  // reset our playback timing variables
  m_eof = false;

  // get correct cache size
  unsigned int filecache = CSettings::Get().GetInt("cacheaudio.internet");
  if ( file.IsHD() )
    filecache = CSettings::Get().GetInt("cache.harddisk");
  else if ( file.IsOnDVD() )
    filecache = CSettings::Get().GetInt("cacheaudio.dvdrom");
  else if ( file.IsOnLAN() )
    filecache = CSettings::Get().GetInt("cacheaudio.lan");

  // create our codec
  m_codec=CodecFactory::CreateCodecDemux(file.GetPath(), file.GetMimeType(), filecache * 1024);

  if (!m_codec || !m_codec->Init(file.GetPath(), filecache * 1024))
  {
    CLog::Log(LOGERROR, "CAudioDecoder: Unable to Init Codec while loading file %s", file.GetPath().c_str());
    Destroy();
    return false;
  }
  unsigned int blockSize = (m_codec->m_BitsPerSample >> 3) * m_codec->GetChannelInfo().Count();

  if (blockSize == 0)
  {
    CLog::Log(LOGERROR, "CAudioDecoder: Codec provided invalid parameters (%d-bit, %u channels)",
              m_codec->m_BitsPerSample, m_codec->GetChannelInfo().Count());
    return false;
  }

  /* allocate the pcmBuffer for 2 seconds of audio */
  m_pcmBuffer.Create(2 * blockSize * m_codec->m_SampleRate);

  if (file.HasMusicInfoTag())
  {
    // set total time from the given tag
    if (file.GetMusicInfoTag()->GetDuration())
      m_codec->SetTotalTime(file.GetMusicInfoTag()->GetDuration());

    // update ReplayGain from the given tag if it's better then original (cuesheet)
    ReplayGain rgInfo = m_codec->m_tag.GetReplayGain();
    bool anySet = false;
    if (!rgInfo.Get(ReplayGain::ALBUM).Valid()
      && file.GetMusicInfoTag()->GetReplayGain().Get(ReplayGain::ALBUM).Valid())
    {
      rgInfo.Set(ReplayGain::ALBUM, file.GetMusicInfoTag()->GetReplayGain().Get(ReplayGain::ALBUM));
      anySet = true;
    }
    if (!rgInfo.Get(ReplayGain::TRACK).Valid()
      && file.GetMusicInfoTag()->GetReplayGain().Get(ReplayGain::TRACK).Valid())
    {
      rgInfo.Set(ReplayGain::TRACK, file.GetMusicInfoTag()->GetReplayGain().Get(ReplayGain::TRACK));
      anySet = true;
    }
    if (anySet)
      m_codec->m_tag.SetReplayGain(rgInfo);
  }

  if (seekOffset)
    m_codec->Seek(seekOffset);

  m_status = STATUS_QUEUING;

  return true;
}

void CAudioDecoder::GetDataFormat(CAEChannelInfo *channelInfo, unsigned int *samplerate, unsigned int *encodedSampleRate, enum AEDataFormat *dataFormat)
{
  if (!m_codec)
    return;

  if (channelInfo      ) *channelInfo       = m_codec->GetChannelInfo();
  if (samplerate       ) *samplerate        = m_codec->m_SampleRate;
  if (encodedSampleRate) *encodedSampleRate = m_codec->m_EncodedSampleRate;
  if (dataFormat       ) *dataFormat        = m_codec->m_DataFormat;
}

int64_t CAudioDecoder::Seek(int64_t time)
{
  m_pcmBuffer.Clear();
  if (!m_codec)
    return 0;
  if (time < 0) time = 0;
  if (time > m_codec->m_TotalTime) time = m_codec->m_TotalTime;
  return m_codec->Seek(time);
}

int64_t CAudioDecoder::TotalTime()
{
  if (m_codec)
    return m_codec->m_TotalTime;
  return 0;
}

unsigned int CAudioDecoder::GetDataSize()
{
  if (m_status == STATUS_QUEUING || m_status == STATUS_NO_FILE)
    return 0;
  // check for end of file and end of buffer
  if (m_status == STATUS_ENDING && m_pcmBuffer.getMaxReadSize() < PACKET_SIZE)
    m_status = STATUS_ENDED;
  return std::min(m_pcmBuffer.getMaxReadSize() / (m_codec->m_BitsPerSample >> 3), (unsigned int)OUTPUT_SAMPLES);
}

void *CAudioDecoder::GetData(unsigned int samples)
{
  unsigned int size  = samples * (m_codec->m_BitsPerSample >> 3);
  if (size > sizeof(m_outputBuffer))
  {
    CLog::Log(LOGERROR, "CAudioDecoder::GetData - More data was requested then we have space to buffer!");
    return NULL;
  }
  
  if (size > m_pcmBuffer.getMaxReadSize())
  {
    CLog::Log(LOGWARNING, "CAudioDecoder::GetData() more bytes/samples (%i) requested than we have to give (%i)!", size, m_pcmBuffer.getMaxReadSize());
    size = m_pcmBuffer.getMaxReadSize();
  }

  if (m_pcmBuffer.ReadData((char *)m_outputBuffer, size))
  {
    if (m_status == STATUS_ENDING && m_pcmBuffer.getMaxReadSize() == 0)
      m_status = STATUS_ENDED;
    
    return m_outputBuffer;
  }
  
  CLog::Log(LOGERROR, "CAudioDecoder::GetData() ReadBinary failed with %i samples", samples);
  return NULL;
}

int CAudioDecoder::ReadSamples(int numsamples)
{
  if (m_status == STATUS_NO_FILE || m_status == STATUS_ENDING || m_status == STATUS_ENDED)
    return RET_SLEEP;             // nothing loaded yet

  // start playing once we're fully queued and we're ready to go
  if (m_status == STATUS_QUEUED && m_canPlay)
    m_status = STATUS_PLAYING;

  // grab a lock to ensure the codec is created at this point.
  CSingleLock lock(m_critSection);

  // Read in more data
  int maxsize = std::min<int>(INPUT_SAMPLES, m_pcmBuffer.getMaxWriteSize() / (m_codec->m_BitsPerSample >> 3));
  numsamples = std::min<int>(numsamples, maxsize);
  numsamples -= (numsamples % m_codec->GetChannelInfo().Count());  // make sure it's divisible by our number of channels
  if ( numsamples )
  {
    int readSize = 0;
    int result = m_codec->ReadPCM(m_pcmInputBuffer, numsamples * (m_codec->m_BitsPerSample >> 3), &readSize);

    if (result != READ_ERROR && readSize)
    {
      // move it into our buffer
      m_pcmBuffer.WriteData((char *)m_pcmInputBuffer, readSize);

      // update status
      if (m_status == STATUS_QUEUING && m_pcmBuffer.getMaxReadSize() > m_pcmBuffer.getSize() * 0.9)
      {
        CLog::Log(LOGINFO, "AudioDecoder: File is queued");
        m_status = STATUS_QUEUED;
      }

      if (result == READ_EOF) // EOF reached
      {
        // setup ending if we're within set time of the end (currently just EOF)
        m_eof = true;
        if (m_status < STATUS_ENDING)
          m_status = STATUS_ENDING;
      }

      return RET_SUCCESS;
    }
    if (result == READ_ERROR)
    {
      // error decoding, lets finish up and get out
      CLog::Log(LOGERROR, "CAudioDecoder: Error while decoding %i", result);
      return RET_ERROR;
    }
    if (result == READ_EOF)
    {
      m_eof = true;
      // setup ending if we're within set time of the end (currently just EOF)
      if (m_status < STATUS_ENDING)
        m_status = STATUS_ENDING;
    }
  }
  return RET_SLEEP; // nothing to do
}

float CAudioDecoder::GetReplayGain()
{
#define REPLAY_GAIN_DEFAULT_LEVEL 89.0f
  const ReplayGainSettings &replayGainSettings = g_application.GetReplayGainSettings();
  if (replayGainSettings.iType == ReplayGain::NONE)
    return 1.0f;

  // Compute amount of gain
  float replaydB = (float)replayGainSettings.iNoGainPreAmp;
  float peak = 0.0f;
  const ReplayGain& rgInfo = m_codec->m_tag.GetReplayGain();
  if (replayGainSettings.iType == ReplayGain::ALBUM)
  {
    if (rgInfo.Get(ReplayGain::ALBUM).Valid())
    {
      replaydB = (float)replayGainSettings.iPreAmp + rgInfo.Get(ReplayGain::ALBUM).Gain();
      peak = rgInfo.Get(ReplayGain::ALBUM).Peak();
    }
    else if (rgInfo.Get(ReplayGain::TRACK).Valid())
    {
      replaydB = (float)replayGainSettings.iPreAmp + rgInfo.Get(ReplayGain::TRACK).Gain();
      peak = rgInfo.Get(ReplayGain::TRACK).Peak();
    }
  }
  else if (replayGainSettings.iType == ReplayGain::TRACK)
  {
    if (rgInfo.Get(ReplayGain::TRACK).Valid())
    {
      replaydB = (float)replayGainSettings.iPreAmp + rgInfo.Get(ReplayGain::TRACK).Gain();
      peak = rgInfo.Get(ReplayGain::TRACK).Peak();
    }
    else if (rgInfo.Get(ReplayGain::ALBUM).Valid())
    {
      replaydB = (float)replayGainSettings.iPreAmp + rgInfo.Get(ReplayGain::ALBUM).Gain();
      peak = rgInfo.Get(ReplayGain::ALBUM).Peak();
    }
  }
  // convert to a gain type
  float replaygain = pow(10.0f, (replaydB - REPLAY_GAIN_DEFAULT_LEVEL)* 0.05f);
  // check peaks
  if (replayGainSettings.bAvoidClipping)
  {
    if (fabs(peak * replaygain) > 1.0f)
      replaygain = 1.0f / fabs(peak);
  }

  CLog::Log(LOGDEBUG, "AudioDecoder::GetReplayGain - Final Replaygain applied: %f, Track/Album Gain %f, Peak %f", replaygain, replaydB, peak);

  return replaygain;
}

