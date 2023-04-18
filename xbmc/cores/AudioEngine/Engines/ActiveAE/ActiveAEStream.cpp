/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ActiveAEStream.h"

#include "ActiveAE.h"
#include "cores/AudioEngine/AEResampleFactory.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/log.h"

#include <mutex>

using namespace ActiveAE;
using namespace std::chrono_literals;

CActiveAEStream::CActiveAEStream(AEAudioFormat* format, unsigned int streamid, CActiveAE* ae)
  : m_format(*format)
{
  m_activeAE = ae;
  m_id = streamid;
  m_bufferedTime = 0;
  m_currentBuffer = NULL;
  m_drain = false;
  m_paused = false;
  m_rgain = 1.0;
  m_volume = 1.0;
  SetVolume(1.0);
  m_amplify = 1.0;
  m_streamSpace = m_format.m_frameSize * m_format.m_frames;
  m_streamDraining = false;
  m_streamDrained = false;
  m_streamFading = false;
  m_streamFreeBuffers = 0;
  m_streamIsBuffering = false;
  m_streamIsFlushed = false;
  m_streamSlave = NULL;
  m_leftoverBytes = 0;
  m_forceResampler = false;
  m_streamResampleRatio = 1.0;
  m_streamResampleMode = 0;
  m_profile = 0;
  m_matrixEncoding = AV_MATRIX_ENCODING_NONE;
  m_audioServiceType = AV_AUDIO_SERVICE_TYPE_MAIN;
  m_pClock = NULL;
  m_lastPts = 0;
  m_lastPtsJump = 0;
  m_clockSpeed = 1.0;
}

void CActiveAEStream::IncFreeBuffers()
{
  std::unique_lock<CCriticalSection> lock(m_streamLock);
  m_streamFreeBuffers++;
}

void CActiveAEStream::DecFreeBuffers()
{
  std::unique_lock<CCriticalSection> lock(m_streamLock);
  m_streamFreeBuffers--;
}

void CActiveAEStream::ResetFreeBuffers()
{
  std::unique_lock<CCriticalSection> lock(m_streamLock);
  m_streamFreeBuffers = 0;
}

void CActiveAEStream::InitRemapper()
{
  // check if input format follows ffmpeg channel mask
  bool needRemap = false;
  unsigned int avLast, avCur = 0;
  for(unsigned int i=0; i<m_format.m_channelLayout.Count(); i++)
  {
    avLast = avCur;
    avCur = CAEUtil::GetAVChannelMask(m_format.m_channelLayout[i]);
    if(avCur < avLast)
    {
      needRemap = true;
      break;
    }
  }

  if(needRemap)
  {
    CLog::Log(LOGDEBUG, "CActiveAEStream::{} - initialize remapper", __FUNCTION__);

    m_remapper = CAEResampleFactory::Create();
    uint64_t avLayout = CAEUtil::GetAVChannelLayout(m_format.m_channelLayout);

    // build layout according to ffmpeg channel order
    // we need this for reference
    CAEChannelInfo ffmpegLayout;
    ffmpegLayout.Reset();
    int idx = 0;
    for(unsigned int i=0; i<m_format.m_channelLayout.Count(); i++)
    {
      for(unsigned int j=0; j<m_format.m_channelLayout.Count(); j++)
      {
        idx = CAEUtil::GetAVChannelIndex(m_format.m_channelLayout[j], avLayout);
        if (idx == (int)i)
        {
          ffmpegLayout += m_format.m_channelLayout[j];
          break;
        }
      }
    }

    // build remap layout we can pass to resampler as destination layout
    CAEChannelInfo remapLayout;
    remapLayout.Reset();
    for(unsigned int i=0; i<m_format.m_channelLayout.Count(); i++)
    {
      for(unsigned int j=0; j<m_format.m_channelLayout.Count(); j++)
      {
        idx = CAEUtil::GetAVChannelIndex(m_format.m_channelLayout[j], avLayout);
        if (idx == (int)i)
        {
          remapLayout += ffmpegLayout[j];
          break;
        }
      }
    }

    // initialize resampler for only doing remapping
    SampleConfig dstConfig, srcConfig;
    dstConfig.channel_layout = avLayout;
    dstConfig.channels = m_format.m_channelLayout.Count();
    dstConfig.sample_rate = m_format.m_sampleRate;
    dstConfig.fmt = CAEUtil::GetAVSampleFormat(m_format.m_dataFormat);
    dstConfig.bits_per_sample = CAEUtil::DataFormatToUsedBits(m_format.m_dataFormat);
    dstConfig.dither_bits = CAEUtil::DataFormatToDitherBits(m_format.m_dataFormat);

    srcConfig.channel_layout = avLayout;
    srcConfig.channels = m_format.m_channelLayout.Count();
    srcConfig.sample_rate = m_format.m_sampleRate;
    srcConfig.fmt = CAEUtil::GetAVSampleFormat(m_format.m_dataFormat);
    srcConfig.bits_per_sample = CAEUtil::DataFormatToUsedBits(m_format.m_dataFormat);
    srcConfig.dither_bits = CAEUtil::DataFormatToDitherBits(m_format.m_dataFormat);

    m_remapper->Init(dstConfig, srcConfig,
                     false,
                     false,
                     M_SQRT1_2,
                     &remapLayout,
                     AE_QUALITY_LOW, // not used for remapping
                     false);

    // extra sound packet, we can't resample to the same buffer
    m_remapBuffer =
        std::make_unique<CSoundPacket>(m_inputBuffers->m_allSamples[0]->pkt->config,
                                       m_inputBuffers->m_allSamples[0]->pkt->max_nb_samples);
  }
}

void CActiveAEStream::RemapBuffer()
{
  if(m_remapper)
  {
    int samples = m_remapper->Resample(m_remapBuffer->data, m_remapBuffer->max_nb_samples,
                                       m_currentBuffer->pkt->data, m_currentBuffer->pkt->nb_samples,
                                       1.0);

    if (samples != m_currentBuffer->pkt->nb_samples)
    {
      CLog::Log(LOGERROR, "CActiveAEStream::{} - error remapping", __FUNCTION__);
    }

    // swap sound packets
    std::swap(m_currentBuffer->pkt, m_remapBuffer);
  }
}

double CActiveAEStream::CalcResampleRatio(double error)
{
  //reset the integral on big errors, failsafe
  if (fabs(error) > 1000)
    m_resampleIntegral = 0;
  else if (fabs(error) > 5)
    m_resampleIntegral += error / 1000 / 50;

  double proportional = 0.0;

  double proportionaldiv = 2.0;
  proportional = error / GetErrorInterval().count() / proportionaldiv;

  double clockspeed = 1.0;
  if (m_pClock)
  {
    clockspeed = m_pClock->GetClockSpeed();
    if (m_clockSpeed != clockspeed)
      m_resampleIntegral = 0;
    m_clockSpeed = clockspeed;
  }

  double ret = 1.0 / clockspeed + proportional + m_resampleIntegral;
  //CLog::Log(LOGINFO,"----- error: {:f}, rr: {:f}, prop: {:f}, int: {:f}",
  //                    error, ret, proportional, m_resampleIntegral);
  return ret;
}

std::chrono::milliseconds CActiveAEStream::GetErrorInterval()
{
  std::chrono::milliseconds ret = m_errorInterval;
  double rr = m_processingBuffers->GetRR();
  if (rr > 1.02 || rr < 0.98)
    ret *= 3;
  return ret;
}

unsigned int CActiveAEStream::GetSpace()
{
  std::unique_lock<CCriticalSection> lock(m_streamLock);
  if (m_format.m_dataFormat == AE_FMT_RAW)
    return m_streamFreeBuffers;
  else
    return m_streamFreeBuffers * m_streamSpace;
}

unsigned int CActiveAEStream::AddData(const uint8_t* const *data, unsigned int offset, unsigned int frames, ExtData *extData)
{
  Message *msg;
  unsigned int copied = 0;
  int sourceFrames = frames;
  const uint8_t* const *buf = data;
  double pts = 0;

  if (extData)
  {
    pts = extData->pts;
  }

  m_streamIsFlushed = false;

  while(copied < frames)
  {
    sourceFrames = frames - copied;

    if (m_currentBuffer)
    {
      int start = m_currentBuffer->pkt->nb_samples *
                  m_currentBuffer->pkt->bytes_per_sample *
                  m_currentBuffer->pkt->config.channels /
                  m_currentBuffer->pkt->planes;

      int freeSpace = m_currentBuffer->pkt->max_nb_samples - m_currentBuffer->pkt->nb_samples;
      int minFrames = std::min(freeSpace, sourceFrames);
      int planes = m_currentBuffer->pkt->planes;
      int bufOffset = (offset + copied)*m_format.m_frameSize/planes;

      if (!copied)
      {
        if (pts < m_lastPts)
        {
          if (m_lastPtsJump != 0)
          {
            auto diff = std::chrono::milliseconds(static_cast<int>(pts - m_lastPtsJump));
            if (diff > m_errorInterval)
            {
              diff += 1s;
              diff = std::min(diff, 6000ms);
              CLog::Log(LOGINFO,
                        "CActiveAEStream::AddData - messy timestamps, increasing interval for "
                        "measuring average error to {} ms",
                        diff.count());
              m_errorInterval = diff;
            }
          }
          m_lastPtsJump = pts;
        }
        m_lastPts = pts;
        m_currentBuffer->timestamp = pts;
        m_currentBuffer->pkt_start_offset = m_currentBuffer->pkt->nb_samples;
      }

      for (int i=0; i<planes; i++)
      {
        memcpy(m_currentBuffer->pkt->data[i]+start, buf[i]+bufOffset, minFrames*m_format.m_frameSize/planes);
      }
      copied += minFrames;

      if (extData && extData->hasDownmix)
        m_currentBuffer->centerMixLevel = extData->centerMixLevel;

      bool rawPktComplete = false;
      {
        std::unique_lock<CCriticalSection> lock(m_statsLock);
        if (m_format.m_dataFormat != AE_FMT_RAW)
        {
          m_currentBuffer->pkt->nb_samples += minFrames;
          m_bufferedTime +=
              static_cast<float>(minFrames) / m_currentBuffer->pkt->config.sample_rate;
        }
        else
        {
          m_bufferedTime += static_cast<float>(m_format.m_streamInfo.GetDuration()) / 1000;
          m_currentBuffer->pkt->nb_samples += minFrames;
          rawPktComplete = true;
        }
      }

      if (m_currentBuffer->pkt->nb_samples == m_currentBuffer->pkt->max_nb_samples || rawPktComplete)
      {
        MsgStreamSample msgData;
        msgData.buffer = m_currentBuffer;
        msgData.stream = this;
        RemapBuffer();
        m_streamPort->SendOutMessage(CActiveAEDataProtocol::STREAMSAMPLE, &msgData, sizeof(MsgStreamSample));
        m_currentBuffer = nullptr;
      }
      continue;
    }
    else if (m_streamPort->ReceiveInMessage(&msg))
    {
      if (msg->signal == CActiveAEDataProtocol::STREAMBUFFER)
      {
        m_currentBuffer = *((CSampleBuffer**)msg->data);
        m_currentBuffer->timestamp = 0;
        m_currentBuffer->pkt->nb_samples = 0;
        m_currentBuffer->pkt->pause_burst_ms = 0;
        msg->Release();
        DecFreeBuffers();
        continue;
      }
      else
      {
        CLog::Log(LOGERROR, "CActiveAEStream::AddData - unknown signal");
        msg->Release();
        break;
      }
    }
    if (!m_inMsgEvent.Wait(200ms))
      break;
  }
  return copied;
}

double CActiveAEStream::GetDelay()
{
  AEDelayStatus status;
  m_activeAE->GetDelay(status, this);
  return status.GetDelay();
}

CAESyncInfo CActiveAEStream::GetSyncInfo()
{
  CAESyncInfo info;
  m_activeAE->GetSyncInfo(info, this);
  return info;
}

bool CActiveAEStream::IsBuffering()
{
  std::unique_lock<CCriticalSection> lock(m_streamLock);
  return m_streamIsBuffering;
}

double CActiveAEStream::GetCacheTime()
{
  return static_cast<double>(m_activeAE->GetCacheTime(this));
}

double CActiveAEStream::GetCacheTotal()
{
  return static_cast<double>(m_activeAE->GetCacheTotal());
}

double CActiveAEStream::GetMaxDelay()
{
  return static_cast<double>(m_activeAE->GetMaxDelay());
}

void CActiveAEStream::Pause()
{
  m_activeAE->PauseStream(this, true);
}

void CActiveAEStream::Resume()
{
  m_activeAE->PauseStream(this, false);
}

void CActiveAEStream::Drain(bool wait)
{
  Message *msg;
  CActiveAEStream *stream = this;

  m_streamDraining = true;
  m_streamDrained = false;

  Message *reply;
  if (m_streamPort->SendOutMessageSync(CActiveAEDataProtocol::DRAINSTREAM, &reply, 2s, &stream,
                                       sizeof(CActiveAEStream*)))
  {
    bool success = reply->signal == CActiveAEDataProtocol::ACC ? true : false;
    reply->Release();
    if (!success)
    {
      CLog::Log(LOGERROR, "CActiveAEStream::Drain - no acc");
    }
  }

  if (m_currentBuffer)
  {
    MsgStreamSample msgData;
    msgData.buffer = m_currentBuffer;
    msgData.stream = this;
    RemapBuffer();
    m_streamPort->SendOutMessage(CActiveAEDataProtocol::STREAMSAMPLE, &msgData, sizeof(MsgStreamSample));
    m_currentBuffer = NULL;
  }

  if (wait)
    Resume();

  XbmcThreads::EndTime<> timer(2000ms);
  while (!timer.IsTimePast())
  {
    if (m_streamPort->ReceiveInMessage(&msg))
    {
      if (msg->signal == CActiveAEDataProtocol::STREAMBUFFER)
      {
        MsgStreamSample msgData;
        msgData.stream = this;
        msgData.buffer = *((CSampleBuffer**)msg->data);
        msg->Reply(CActiveAEDataProtocol::STREAMSAMPLE, &msgData, sizeof(MsgStreamSample));
        DecFreeBuffers();
        continue;
      }
      else if (msg->signal == CActiveAEDataProtocol::STREAMDRAINED)
      {
        msg->Release();
        return;
      }
    }
    else if (!wait)
      return;

    m_inMsgEvent.Wait(timer.GetTimeLeft());
  }
  CLog::Log(LOGERROR, "CActiveAEStream::Drain - timeout out");
}

bool CActiveAEStream::IsDraining()
{
  std::unique_lock<CCriticalSection> lock(m_streamLock);
  return m_streamDraining;
}

bool CActiveAEStream::IsDrained()
{
  std::unique_lock<CCriticalSection> lock(m_streamLock);
  return m_streamDrained;
}

void CActiveAEStream::Flush()
{
  if (!m_streamIsFlushed)
  {
    m_currentBuffer = NULL;
    m_leftoverBytes = 0;
    m_activeAE->FlushStream(this);
    m_streamIsFlushed = true;
  }
}

float CActiveAEStream::GetAmplification()
{
  return m_streamAmplify;
}

void CActiveAEStream::SetAmplification(float amplify)
{
  m_streamAmplify = amplify;
  m_activeAE->SetStreamAmplification(this, m_streamAmplify);
}

float CActiveAEStream::GetReplayGain()
{
  return m_streamRgain;
}

void CActiveAEStream::SetReplayGain(float factor)
{
  m_streamRgain = std::max( 0.0f, factor);
  m_activeAE->SetStreamReplaygain(this, m_streamRgain);
}

float CActiveAEStream::GetVolume()
{
  return m_streamVolume;
}

void CActiveAEStream::SetVolume(float volume)
{
  m_streamVolume = std::max( 0.0f, std::min(1.0f, volume));
  m_activeAE->SetStreamVolume(this, m_streamVolume);
}

double CActiveAEStream::GetResampleRatio()
{
  return m_streamResampleRatio;
}

void CActiveAEStream::SetResampleRatio(double ratio)
{
  if (ratio != m_streamResampleRatio)
    m_activeAE->SetStreamResampleRatio(this, ratio);
  m_streamResampleRatio = ratio;
}

void CActiveAEStream::SetResampleMode(int mode)
{
  if (mode != m_streamResampleMode)
    m_activeAE->SetStreamResampleMode(this, mode);
  m_streamResampleMode = mode;
}

void CActiveAEStream::SetFFmpegInfo(int profile, enum AVMatrixEncoding matrix_encoding, enum AVAudioServiceType audio_service_type)
{
  m_activeAE->SetStreamFFmpegInfo(this, profile, matrix_encoding, audio_service_type);
}

void CActiveAEStream::FadeVolume(float from, float target, unsigned int time)
{
  if (time == 0 || (m_format.m_dataFormat == AE_FMT_RAW))
    return;

  m_streamFading = true;
  m_activeAE->SetStreamFade(this, from, target, time);
}

bool CActiveAEStream::IsFading()
{
  std::unique_lock<CCriticalSection> lock(m_streamLock);
  return m_streamFading;
}

unsigned int CActiveAEStream::GetFrameSize() const
{
  return m_format.m_frameSize;
}

unsigned int CActiveAEStream::GetChannelCount() const
{
  return m_format.m_channelLayout.Count();
}

unsigned int CActiveAEStream::GetSampleRate() const
{
  return m_format.m_sampleRate;
}

enum AEDataFormat CActiveAEStream::GetDataFormat() const
{
  return m_format.m_dataFormat;
}

void CActiveAEStream::RegisterAudioCallback(IAudioCallback* pCallback)
{
}

void CActiveAEStream::UnRegisterAudioCallback()
{
}

void CActiveAEStream::RegisterSlave(IAEStream *slave)
{
  std::unique_lock<CCriticalSection> lock(m_streamLock);
  m_streamSlave = slave;
}

//------------------------------------------------------------------------------
// CActiveAEStreamBuffers
//------------------------------------------------------------------------------

CActiveAEStreamBuffers::CActiveAEStreamBuffers(const AEAudioFormat& inputFormat,
                                               const AEAudioFormat& outputFormat,
                                               AEQuality quality)
  : m_inputFormat(inputFormat),
    m_resampleBuffers(
        std::make_unique<CActiveAEBufferPoolResample>(inputFormat, outputFormat, quality)),
    m_atempoBuffers(std::make_unique<CActiveAEBufferPoolAtempo>(outputFormat))
{
}

CActiveAEStreamBuffers::~CActiveAEStreamBuffers()
{
}

bool CActiveAEStreamBuffers::HasInputLevel(int level)
{
  if ((m_inputSamples.size() + m_resampleBuffers->m_inputSamples.size()) >
      (m_resampleBuffers->m_allSamples.size() * level / 100))
    return true;
  else
    return false;
}

bool CActiveAEStreamBuffers::Create(unsigned int totaltime, bool remap, bool upmix, bool normalize)
{
  if (!m_resampleBuffers->Create(totaltime, remap, upmix, normalize))
    return false;

  if (!m_atempoBuffers->Create(totaltime))
    return false;

  return true;
}

void CActiveAEStreamBuffers::SetExtraData(int profile, enum AVMatrixEncoding matrix_encoding, enum AVAudioServiceType audio_service_type)
{
  /*! @todo Implement set dsp config with new AudioDSP buffer implementation */
}

bool CActiveAEStreamBuffers::ProcessBuffers()
{
  bool busy = false;
  CSampleBuffer *buf;

  while (!m_inputSamples.empty())
  {
    buf = m_inputSamples.front();
    m_inputSamples.pop_front();
    m_resampleBuffers->m_inputSamples.push_back(buf);
    busy = true;
  }

  busy |= m_resampleBuffers->ResampleBuffers();

  while (!m_resampleBuffers->m_outputSamples.empty())
  {
    buf = m_resampleBuffers->m_outputSamples.front();
    m_resampleBuffers->m_outputSamples.pop_front();
    m_atempoBuffers->m_inputSamples.push_back(buf);
    busy = true;
  }

  busy |= m_atempoBuffers->ProcessBuffers();

  while (!m_atempoBuffers->m_outputSamples.empty())
  {
    buf = m_atempoBuffers->m_outputSamples.front();
    m_atempoBuffers->m_outputSamples.pop_front();
    m_outputSamples.push_back(buf);
    busy = true;
  }

  return busy;
}

void CActiveAEStreamBuffers::ConfigureResampler(bool normalizelevels, bool stereoupmix, AEQuality quality)
{
  m_resampleBuffers->ConfigureResampler(normalizelevels, stereoupmix, quality);
}

float CActiveAEStreamBuffers::GetDelay()
{
  float delay = 0;

  for (auto &buf : m_inputSamples)
  {
    delay += (float)buf->pkt->nb_samples / buf->pkt->config.sample_rate;
  }

  delay += m_resampleBuffers->GetDelay();
  delay += m_atempoBuffers->GetDelay();

  for (auto &buf : m_outputSamples)
  {
    delay += (float)buf->pkt->nb_samples / buf->pkt->config.sample_rate;
  }

  return delay;
}

void CActiveAEStreamBuffers::Flush()
{
  m_resampleBuffers->Flush();
  m_atempoBuffers->Flush();

  while (!m_inputSamples.empty())
  {
    m_inputSamples.front()->Return();
    m_inputSamples.pop_front();
  }
  while (!m_outputSamples.empty())
  {
    m_outputSamples.front()->Return();
    m_outputSamples.pop_front();
  }
}

void CActiveAEStreamBuffers::SetDrain(bool drain)
{
  m_resampleBuffers->SetDrain(drain);
  m_atempoBuffers->SetDrain(drain);
}

bool CActiveAEStreamBuffers::IsDrained()
{
  if (m_resampleBuffers->m_inputSamples.empty() &&
      m_resampleBuffers->m_outputSamples.empty() &&
      m_atempoBuffers->m_inputSamples.empty() &&
      m_atempoBuffers->m_outputSamples.empty() &&
      m_inputSamples.empty() &&
      m_outputSamples.empty())
    return true;
  else
    return false;
}

void CActiveAEStreamBuffers::SetRR(double rr, double atempoThreshold)
{
  if (fabs(rr - 1.0) < atempoThreshold)
  {
    m_resampleBuffers->SetRR(rr);
    m_atempoBuffers->SetTempo(1.0);
  }
  else
  {
    m_resampleBuffers->SetRR(1.0);
    m_atempoBuffers->SetTempo(1.0/rr);
  }
}

double CActiveAEStreamBuffers::GetRR()
{
  double tempo = m_resampleBuffers->GetRR();
  tempo /= static_cast<double>(m_atempoBuffers->GetTempo());
  return tempo;
}

void CActiveAEStreamBuffers::FillBuffer()
{
  m_resampleBuffers->FillBuffer();
  m_atempoBuffers->FillBuffer();
}

bool CActiveAEStreamBuffers::DoesNormalize()
{
  return m_resampleBuffers->DoesNormalize();
}

void CActiveAEStreamBuffers::ForceResampler(bool force)
{
  m_resampleBuffers->ForceResampler(force);
}

std::unique_ptr<CActiveAEBufferPool> CActiveAEStreamBuffers::GetResampleBuffers()
{
  return std::move(m_resampleBuffers);
}

std::unique_ptr<CActiveAEBufferPool> CActiveAEStreamBuffers::GetAtempoBuffers()
{
  return std::move(m_atempoBuffers);
}

bool CActiveAEStreamBuffers::HasWork()
{
  if (!m_inputSamples.empty())
    return true;
  if (!m_outputSamples.empty())
    return true;
  if (!m_resampleBuffers->m_inputSamples.empty())
    return true;
  if (!m_resampleBuffers->m_outputSamples.empty())
    return true;
  if (!m_atempoBuffers->m_inputSamples.empty())
    return true;
  if (!m_atempoBuffers->m_outputSamples.empty())
    return true;

  return false;
}
