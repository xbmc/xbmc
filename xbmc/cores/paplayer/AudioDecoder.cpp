/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AudioDecoder.h"

#include "CodecFactory.h"
#include "FileItem.h"
#include "ICodec.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationVolumeHandling.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <cmath>
#include <mutex>

using namespace KODI;

CAudioDecoder::CAudioDecoder()
{
  m_codec = NULL;
  m_rawBuffer = nullptr;

  m_eof = false;

  m_status = STATUS_NO_FILE;
  m_canPlay = false;
  m_startThresholdBytes = 0;

  // output buffer (for transferring data from the Pcm Buffer to the rest of the audio chain)
  memset(&m_outputBuffer, 0, OUTPUT_SAMPLES * sizeof(float));
  memset(&m_pcmInputBuffer, 0, INPUT_SIZE * sizeof(unsigned char));
  memset(&m_inputBuffer, 0, INPUT_SAMPLES * sizeof(float));

  m_rawBufferSize = 0;
}

CAudioDecoder::~CAudioDecoder()
{
  Destroy();
}

void CAudioDecoder::Destroy()
{
  std::unique_lock lock(m_critSection);
  m_status = STATUS_NO_FILE;

  m_pcmBuffer.Destroy();

  if ( m_codec )
    delete m_codec;
  m_codec = NULL;

  m_canPlay = false;
}

bool CAudioDecoder::Create(const CFileItem& file, int64_t seekOffset, int streamIndex)
{
  Destroy();

  std::unique_lock lock(m_critSection);

  // reset our playback timing variables
  m_eof = false;

  // get correct cache size
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  unsigned int filecache = settings->GetInt(CSettings::SETTING_CACHEAUDIO_INTERNET);
  if ( file.IsHD() )
    filecache = settings->GetInt(CSettings::SETTING_CACHE_HARDDISK);
  else if ( file.IsOnDVD() )
    filecache = settings->GetInt(CSettings::SETTING_CACHEAUDIO_DVDROM);
  else if (URIUtils::IsOnLAN(file.GetPath()))
    filecache = settings->GetInt(CSettings::SETTING_CACHEAUDIO_LAN);

  // create our codec
  m_codec=CodecFactory::CreateCodecDemux(file, filecache * 1024);

  if (!m_codec || !m_codec->Init(file, filecache * 1024))
  {
    CLog::Log(LOGERROR, "CAudioDecoder: Unable to Init Codec while loading file {}",
              file.GetDynPath());
    Destroy();
    return false;
  }
  // select the requested stream before buffer sizing (assumes Init() starts on stream 0)
  m_streamIndex = 0;
  if (streamIndex > 0 && streamIndex < m_codec->GetStreamCount() && m_codec->SetStream(streamIndex))
    m_streamIndex = streamIndex;

  if (!CreatePcmBuffer())
    return false;

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

  // Selecting a stream probes it, which consumes packets, so seek back
  if (seekOffset || streamIndex > 0)
    m_codec->Seek(seekOffset);

  UpdateStartThreshold();

  m_status = STATUS_QUEUING;

  m_rawBufferSize = 0;

  return true;
}

bool CAudioDecoder::CreatePcmBuffer()
{
  // Enough for two seconds of what is being decoded
  constexpr unsigned int PCM_BUFFER_SECONDS = 2;

  const unsigned int blockSize =
      (m_codec->m_bitsPerSample >> 3) * m_codec->m_format.m_channelLayout.Count();
  if (blockSize == 0)
  {
    CLog::Log(LOGERROR, "CAudioDecoder: Codec provided invalid parameters ({}-bit, {} channels)",
              m_codec->m_bitsPerSample, m_codec->m_format.m_channelLayout.Count());
    return false;
  }

  // CRingBuffer::Create() allocates without freeing what it is holding
  m_pcmBuffer.Destroy();
  m_pcmBuffer.Create(PCM_BUFFER_SECONDS * blockSize * m_codec->m_format.m_sampleRate);

  return true;
}

void CAudioDecoder::UpdateStartThreshold()
{
  // Pre-compute the startup-buffer threshold, so that the per-packet recomputation in ReadSamples
  // is not wasted work. 64-bit intermediate prevents wrap for extreme hi-res multichannel (see
  // ReadSamples for the original sizing rationale).
  constexpr unsigned int STARTUP_BUFFER_MS = 200;
  m_startThresholdBytes = (static_cast<uint64_t>(STARTUP_BUFFER_MS) *
                           static_cast<uint64_t>(m_codec->m_bitsPerSample >> 3) *
                           static_cast<uint64_t>(m_codec->m_format.m_channelLayout.Count()) *
                           static_cast<uint64_t>(m_codec->m_format.m_sampleRate)) /
                          1000;
}

AEAudioFormat CAudioDecoder::GetFormat()
{
  AEAudioFormat format;
  if (!m_codec)
    return format;
  return m_codec->m_format;
}

unsigned int CAudioDecoder::GetChannels()
{
  return GetFormat().m_channelLayout.Count();
}

int64_t CAudioDecoder::Seek(int64_t time)
{
  m_pcmBuffer.Clear();
  m_rawBufferSize = 0;
  if (!m_codec)
    return 0;
  if (time < 0) time = 0;
  if (time > m_codec->m_TotalTime) time = m_codec->m_TotalTime;
  return m_codec->Seek(time);
}

void CAudioDecoder::SetTotalTime(int64_t time)
{
  if (m_codec)
    m_codec->m_TotalTime = time;
}

int64_t CAudioDecoder::TotalTime()
{
  if (m_codec)
    return m_codec->m_TotalTime;
  return 0;
}

unsigned int CAudioDecoder::GetDataSize(bool checkPktSize)
{
  if (m_status == STATUS_QUEUING || m_status == STATUS_NO_FILE)
    return 0;

  if (m_codec->m_format.m_dataFormat != AE_FMT_RAW)
  {
    // check for end of file and end of buffer
    if (m_status == STATUS_ENDING)
    {
      if (m_pcmBuffer.getMaxReadSize() == 0)
        m_status = STATUS_ENDED;
      else if (checkPktSize && m_pcmBuffer.getMaxReadSize() < PACKET_SIZE)
        m_status = STATUS_ENDED;
    }
    const unsigned int bytesPerSample = m_codec->m_bitsPerSample >> 3;
    if (bytesPerSample == 0)
    {
      CLog::Log(LOGERROR, "CAudioDecoder::GetDataSize - Codec reports {} bits per sample",
                m_codec->m_bitsPerSample);
      return 0;
    }

    return std::min(m_pcmBuffer.getMaxReadSize() / bytesPerSample, (unsigned int)OUTPUT_SAMPLES);
  }
  else
  {
    if (m_status == STATUS_ENDING)
      m_status = STATUS_ENDED;
    return m_rawBufferSize;
  }
}

void *CAudioDecoder::GetData(unsigned int samples)
{
  unsigned int size  = samples * (m_codec->m_bitsPerSample >> 3);
  if (size > sizeof(m_outputBuffer))
  {
    CLog::Log(LOGERROR, "CAudioDecoder::GetData - More data was requested then we have space to buffer!");
    return NULL;
  }

  if (size > m_pcmBuffer.getMaxReadSize())
  {
    CLog::Log(
        LOGWARNING,
        "CAudioDecoder::GetData() more bytes/samples ({}) requested than we have to give ({})!",
        size, m_pcmBuffer.getMaxReadSize());
    size = m_pcmBuffer.getMaxReadSize();
  }

  if (m_pcmBuffer.ReadData((char *)m_outputBuffer, size))
  {
    if (m_status == STATUS_ENDING && m_pcmBuffer.getMaxReadSize() == 0)
      m_status = STATUS_ENDED;

    return m_outputBuffer;
  }

  CLog::Log(LOGERROR, "CAudioDecoder::GetData() ReadBinary failed with {} samples", samples);
  return NULL;
}

uint8_t *CAudioDecoder::GetRawData(int &size)
{
  if (m_status == STATUS_ENDING)
    m_status = STATUS_ENDED;

  if (m_rawBufferSize)
  {
    size = m_rawBufferSize;
    m_rawBufferSize = 0;
    return m_rawBuffer;
  }
  return nullptr;
}

int CAudioDecoder::ReadSamples(int numsamples)
{
  if (m_status == STATUS_NO_FILE || m_status == STATUS_ENDING || m_status == STATUS_ENDED)
    return RET_SLEEP;             // nothing loaded yet

  // start playing once we're fully queued and we're ready to go
  if (m_status == STATUS_QUEUED && m_canPlay)
    m_status = STATUS_PLAYING;

  // grab a lock to ensure the codec is created at this point.
  std::unique_lock lock(m_critSection);

  if (m_codec->m_format.m_dataFormat != AE_FMT_RAW)
  {
    // Read in more data
    int maxsize = std::min<int>(INPUT_SAMPLES, m_pcmBuffer.getMaxWriteSize() / (m_codec->m_bitsPerSample >> 3));
    numsamples = std::min<int>(numsamples, maxsize);
    numsamples -= (numsamples % GetFormat().m_channelLayout.Count());  // make sure it's divisible by our number of channels
    if (numsamples)
    {
      size_t readSize = 0;
      int result = m_codec->ReadPCM(
          m_pcmInputBuffer, static_cast<size_t>(numsamples * (m_codec->m_bitsPerSample >> 3)),
          &readSize);

      if (result != READ_ERROR && readSize)
      {
        // move it into our buffer
        m_pcmBuffer.WriteData((char *)m_pcmInputBuffer, readSize);

        // Declare queued once we have a fixed amount of decoded PCM ready.
        // Time-based (not a percentage of the 2-s buffer) so startup latency is
        // decoupled from buffer-capacity choices: enlarging the buffer for
        // playback resilience does not silently increase the play-start delay.
        // The original threshold required 90% of the 2-s buffer (~1.8 s of PCM)
        // which on multichannel hi-res content (Atmos/TrueHD) over NFS at a
        // chapter offset took ~4 s to fill. VideoPlayer has no equivalent
        // threshold and starts on the first decoded frame; this brings PaPlayer
        // closer to that feel without sacrificing the resilience the remaining
        // buffer-fill provides during playback. Threshold is computed once in
        // Create() since the contributing format values are immutable.
        if (m_status == STATUS_QUEUING && m_pcmBuffer.getMaxReadSize() > m_startThresholdBytes)
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
        CLog::Log(LOGERROR, "CAudioDecoder: Error while decoding {}", result);
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
  }
  else
  {
    if (m_rawBufferSize == 0)
    {
      int result = m_codec->ReadRaw(&m_rawBuffer, &m_rawBufferSize);
      if (result == READ_SUCCESS && m_rawBufferSize)
      {
        //! @todo trash this useless ringbuffer
        if (m_status == STATUS_QUEUING)
        {
          m_status = STATUS_QUEUED;
        }
        return RET_SUCCESS;
      }
      else if (result == READ_ERROR)
      {
        // error decoding, lets finish up and get out
        CLog::Log(LOGERROR, "CAudioDecoder: Error while decoding {}", result);
        return RET_ERROR;
      }
      else if (result == READ_EOF)
      {
        m_eof = true;
        // setup ending if we're within set time of the end (currently just EOF)
        if (m_status < STATUS_ENDING)
          m_status = STATUS_ENDING;
      }
    }
  }
  return RET_SLEEP; // nothing to do
}

bool CAudioDecoder::CanSeek()
{
  if (m_codec)
    return m_codec->CanSeek();
  else
    return false;
}

float CAudioDecoder::GetReplayGain(float &peakVal)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();

  const auto& replayGainSettings = appVolume->GetReplayGainSettings();
  if (replayGainSettings.m_type == ReplayGain::NONE)
    return 1.0f;

  // Compute amount of gain
  float replaydB = replayGainSettings.m_noGainPreAmp;
  float peak = 1.0f;
  const ReplayGain& rgInfo = m_codec->m_tag.GetReplayGain();
  if (replayGainSettings.m_type == ReplayGain::ALBUM)
  {
    if (rgInfo.Get(ReplayGain::ALBUM).HasGain())
    {
      replaydB = replayGainSettings.m_preAmp + rgInfo.Get(ReplayGain::ALBUM).Gain();
      if (rgInfo.Get(ReplayGain::ALBUM).HasPeak())
        peak = rgInfo.Get(ReplayGain::ALBUM).Peak();
    }
    else if (rgInfo.Get(ReplayGain::TRACK).HasGain())
    {
      replaydB = replayGainSettings.m_preAmp + rgInfo.Get(ReplayGain::TRACK).Gain();
      if (rgInfo.Get(ReplayGain::TRACK).HasPeak())
        peak = rgInfo.Get(ReplayGain::TRACK).Peak();
    }
  }
  else if (replayGainSettings.m_type == ReplayGain::TRACK)
  {
    if (rgInfo.Get(ReplayGain::TRACK).HasGain())
    {
      replaydB = replayGainSettings.m_preAmp + rgInfo.Get(ReplayGain::TRACK).Gain();
      if (rgInfo.Get(ReplayGain::TRACK).HasPeak())
        peak = rgInfo.Get(ReplayGain::TRACK).Peak();
    }
    else if (rgInfo.Get(ReplayGain::ALBUM).HasGain())
    {
      replaydB = replayGainSettings.m_preAmp + rgInfo.Get(ReplayGain::ALBUM).Gain();
      if (rgInfo.Get(ReplayGain::ALBUM).HasPeak())
        peak = rgInfo.Get(ReplayGain::ALBUM).Peak();
    }
  }
  // convert to a gain type
  float replaygain = std::pow(10.0f, replaydB * 0.05f);

  CLog::Log(LOGDEBUG,
            "AudioDecoder::GetReplayGain - Final Replaygain applied: {:f} ({:.2f} dB), "
            "Peak {:f}",
            replaygain, replaydB, peak);

  peakVal = peak;
  return replaygain;
}

int CAudioDecoder::GetStreamCount() const
{
  std::unique_lock lock(m_critSection);
  if (m_codec)
    return m_codec->GetStreamCount();
  return 0;
}

int CAudioDecoder::GetStreamIndex() const
{
  std::unique_lock lock(m_critSection);
  return m_streamIndex;
}

bool CAudioDecoder::IsUsable() const
{
  std::unique_lock lock(m_critSection);

  // A codec describing neither a rate nor a sample size has nothing to decode through - which is
  // what one left behind by a failed stream switch reports.
  return m_codec && m_codec->m_format.m_sampleRate != 0 && m_codec->m_bitsPerSample != 0;
}

bool CAudioDecoder::SetStream(int index)
{
  std::unique_lock lock(m_critSection);
  if (!m_codec)
    return false;

  const int64_t totalTime = m_codec->m_TotalTime;

  if (!m_codec->SetStream(index))
  {
    m_codec->m_TotalTime = totalTime;
    return false;
  }

  m_codec->m_TotalTime = totalTime;

  m_streamIndex = index;

  // Reset for new stream
  m_eof = false;
  if (m_status == STATUS_ENDING || m_status == STATUS_ENDED)
    m_status = STATUS_PLAYING;

  m_rawBufferSize = 0;
  if (!CreatePcmBuffer())
    return false;

  UpdateStartThreshold();

  return true;
}

void CAudioDecoder::GetStreamInfo(int index, AudioStreamInfo& info) const
{
  std::unique_lock lock(m_critSection);
  if (m_codec)
    m_codec->GetStreamInfo(index, info);
  else
    info.valid = false;
}
