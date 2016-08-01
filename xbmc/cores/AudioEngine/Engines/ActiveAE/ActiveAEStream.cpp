/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include "system.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/AudioEngine/AEResampleFactory.h"

#include "ActiveAE.h"
#include "ActiveAEStream.h"

using namespace ActiveAE;

/* typecast AE to CActiveAE */
#define AE (*((CActiveAE*)CAEFactory::GetEngine()))


CActiveAEStream::CActiveAEStream(AEAudioFormat *format, unsigned int streamid)
{
  m_format = *format;
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
  m_bypassDSP = false;
  m_streamSlave = NULL;
  m_leftoverBuffer = new uint8_t[m_format.m_frameSize];
  m_leftoverBytes = 0;
  m_forceResampler = false;
  m_remapper = NULL;
  m_remapBuffer = NULL;
  m_streamResampleRatio = 1.0;
  m_streamResampleMode = 0;
  m_profile = 0;
  m_matrixEncoding = AV_MATRIX_ENCODING_NONE;
  m_audioServiceType = AV_AUDIO_SERVICE_TYPE_MAIN;
  m_pClock = NULL;
  m_lastPts = 0;
  m_lastPtsJump = 0;
  m_errorInterval = 1000;
  m_clockSpeed = 1.0;
}

CActiveAEStream::~CActiveAEStream()
{
  delete [] m_leftoverBuffer;
  delete m_remapper;
  delete m_remapBuffer;
}

void CActiveAEStream::IncFreeBuffers()
{
  CSingleLock lock(m_streamLock);
  m_streamFreeBuffers++;
}

void CActiveAEStream::DecFreeBuffers()
{
  CSingleLock lock(m_streamLock);
  m_streamFreeBuffers--;
}

void CActiveAEStream::ResetFreeBuffers()
{
  CSingleLock lock(m_streamLock);
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
    avCur = CAEUtil::GetAVChannel(m_format.m_channelLayout[i]);
    if(avCur < avLast)
    {
      needRemap = true;
      break;
    }
  }

  if(needRemap)
  {
    CLog::Log(LOGDEBUG, "CActiveAEStream::%s - initialize remapper", __FUNCTION__);

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
    m_remapper->Init(avLayout,
                     m_format.m_channelLayout.Count(),
                     m_format.m_sampleRate,
                     CAEUtil::GetAVSampleFormat(m_format.m_dataFormat),
                     CAEUtil::DataFormatToUsedBits(m_format.m_dataFormat),
                     CAEUtil::DataFormatToDitherBits(m_format.m_dataFormat),
                     avLayout,
                     m_format.m_channelLayout.Count(),
                     m_format.m_sampleRate,
                     CAEUtil::GetAVSampleFormat(m_format.m_dataFormat),
                     CAEUtil::DataFormatToUsedBits(m_format.m_dataFormat),
                     CAEUtil::DataFormatToDitherBits(m_format.m_dataFormat),
                     false,
                     false,
                     &remapLayout,
                     AE_QUALITY_LOW, // not used for remapping
                     false);

    // extra sound packet, we can't resample to the same buffer
    m_remapBuffer = new CSoundPacket(m_inputBuffers->m_allSamples[0]->pkt->config, m_inputBuffers->m_allSamples[0]->pkt->max_nb_samples);
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
      CLog::Log(LOGERROR, "CActiveAEStream::%s - error remapping", __FUNCTION__);
    }

    // swap sound packets
    CSoundPacket *tmp = m_currentBuffer->pkt;
    m_currentBuffer->pkt = m_remapBuffer;
    m_remapBuffer = tmp;
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
  proportional = error / m_errorInterval / proportionaldiv;

  double clockspeed = 1.0;
  if (m_pClock)
  {
    clockspeed = m_pClock->GetClockSpeed();
    if (m_clockSpeed != clockspeed)
      m_resampleIntegral = 0;
    m_clockSpeed = clockspeed;
  }

  double ret = 1.0 / clockspeed + proportional + m_resampleIntegral;
  //CLog::Log(LOGNOTICE,"----- error: %f, rr: %f, prop: %f, int: %f",
  //                    error, ret, proportional, m_resampleIntegral);
  return ret;
}

unsigned int CActiveAEStream::GetSpace()
{
  CSingleLock lock(m_streamLock);
  if (m_format.m_dataFormat == AE_FMT_RAW)
    return m_streamFreeBuffers;
  else
    return m_streamFreeBuffers * m_streamSpace;
}

unsigned int CActiveAEStream::AddData(const uint8_t* const *data, unsigned int offset, unsigned int frames, double pts)
{
  Message *msg;
  unsigned int copied = 0;
  int sourceFrames = frames;
  const uint8_t* const *buf = data;

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
            int diff = pts - m_lastPtsJump;
            if (diff > m_errorInterval)
            {
              diff += 1000;
              diff = std::min(diff, 6000);
              CLog::Log(LOGNOTICE, "CActiveAEStream::AddData - messy timestamps, increasing interval for measuring average error to %d ms", diff);
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

      bool rawPktComplete = false;
      {
        CSingleLock lock(m_statsLock);
        if (m_format.m_dataFormat != AE_FMT_RAW)
        {
          m_currentBuffer->pkt->nb_samples += minFrames;
          m_bufferedTime += (double)minFrames / m_currentBuffer->pkt->config.sample_rate;
        }
        else
        {
          m_bufferedTime += m_format.m_streamInfo.GetDuration() / 1000;
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
        m_currentBuffer = NULL;
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
    if (!m_inMsgEvent.WaitMSec(200))
      break;
  }
  return copied;
}

double CActiveAEStream::GetDelay()
{
  AEDelayStatus status;
  AE.GetDelay(status, this);
  return status.GetDelay();
}

CAESyncInfo CActiveAEStream::GetSyncInfo()
{
  CAESyncInfo info;
  AE.GetSyncInfo(info, this);
  return info;
}

bool CActiveAEStream::IsBuffering()
{
  CSingleLock lock(m_streamLock);
  return m_streamIsBuffering;
}

double CActiveAEStream::GetCacheTime()
{
  return AE.GetCacheTime(this);
}

double CActiveAEStream::GetCacheTotal()
{
  return AE.GetCacheTotal(this);
}

void CActiveAEStream::Pause()
{
  AE.PauseStream(this, true);
}

void CActiveAEStream::Resume()
{
  AE.PauseStream(this, false);
}

void CActiveAEStream::Drain(bool wait)
{
  Message *msg;
  CActiveAEStream *stream = this;

  m_streamDraining = true;
  m_streamDrained = false;

  Message *reply;
  if (m_streamPort->SendOutMessageSync(CActiveAEDataProtocol::DRAINSTREAM,
                                       &reply,2000,
                                       &stream, sizeof(CActiveAEStream*)))
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

  XbmcThreads::EndTime timer(2000);
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

    m_inMsgEvent.WaitMSec(timer.MillisLeft());
  }
  CLog::Log(LOGERROR, "CActiveAEStream::Drain - timeout out");
}

bool CActiveAEStream::IsDraining()
{
  CSingleLock lock(m_streamLock);
  return m_streamDraining;
}

bool CActiveAEStream::IsDrained()
{
  CSingleLock lock(m_streamLock);
  return m_streamDrained;
}

void CActiveAEStream::Flush()
{
  if (!m_streamIsFlushed)
  {
    m_currentBuffer = NULL;
    m_leftoverBytes = 0;
    AE.FlushStream(this);
    ResetFreeBuffers();
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
  AE.SetStreamAmplification(this, m_streamAmplify);
}

float CActiveAEStream::GetReplayGain()
{
  return m_streamRgain;
}

void CActiveAEStream::SetReplayGain(float factor)
{
  m_streamRgain = std::max( 0.0f, factor);
  AE.SetStreamReplaygain(this, m_streamRgain);
}

float CActiveAEStream::GetVolume()
{
  return m_streamVolume;
}

void CActiveAEStream::SetVolume(float volume)
{
  m_streamVolume = std::max( 0.0f, std::min(1.0f, volume));
  AE.SetStreamVolume(this, m_streamVolume);
}

double CActiveAEStream::GetResampleRatio()
{
  return m_streamResampleRatio;
}

void CActiveAEStream::SetResampleRatio(double ratio)
{
  if (ratio != m_streamResampleRatio)
    AE.SetStreamResampleRatio(this, ratio);
  m_streamResampleRatio = ratio;
}

void CActiveAEStream::SetResampleMode(int mode)
{
  if (mode != m_streamResampleMode)
    AE.SetStreamResampleMode(this, mode);
  m_streamResampleMode = mode;
}

void CActiveAEStream::SetFFmpegInfo(int profile, enum AVMatrixEncoding matrix_encoding, enum AVAudioServiceType audio_service_type)
{
  AE.SetStreamFFmpegInfo(this, profile, matrix_encoding, audio_service_type);
}

void CActiveAEStream::FadeVolume(float from, float target, unsigned int time)
{
  if (time == 0 || (m_format.m_dataFormat == AE_FMT_RAW))
    return;

  m_streamFading = true;
  AE.SetStreamFade(this, from, target, time);
}

bool CActiveAEStream::IsFading()
{
  CSingleLock lock(m_streamLock);
  return m_streamFading;
}

bool CActiveAEStream::HasDSP()
{
  return AE.HasDSP();
}

const unsigned int CActiveAEStream::GetFrameSize() const
{
  return m_format.m_frameSize;
}

const unsigned int CActiveAEStream::GetChannelCount() const
{
  return m_format.m_channelLayout.Count();
}

const unsigned int CActiveAEStream::GetSampleRate() const
{
  return m_format.m_sampleRate;
}

const enum AEDataFormat CActiveAEStream::GetDataFormat() const
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
  CSingleLock lock(m_streamLock);
  m_streamSlave = slave;
}

//------------------------------------------------------------------------------
// CActiveAEStreamBuffers
//------------------------------------------------------------------------------

CActiveAEStreamBuffers::CActiveAEStreamBuffers(AEAudioFormat inputFormat, AEAudioFormat outputFormat, AEQuality quality)
{
  m_inputFormat = inputFormat;
  m_resampleBuffers = new CActiveAEBufferPoolResample(inputFormat, outputFormat, quality);
  m_atempoBuffers = new CActiveAEBufferPoolAtempo(outputFormat);
}

CActiveAEStreamBuffers::~CActiveAEStreamBuffers()
{
  delete m_resampleBuffers;
  delete m_atempoBuffers;
}

bool CActiveAEStreamBuffers::HasInputLevel(int level)
{
  if ((m_inputSamples.size() + m_resampleBuffers->m_inputSamples.size()) >
      (m_resampleBuffers->m_allSamples.size() * level / 100))
    return true;
  else
    return false;
}

bool CActiveAEStreamBuffers::Create(unsigned int totaltime, bool remap, bool upmix, bool normalize, bool useDSP)
{
  if (!m_resampleBuffers->Create(totaltime, remap, upmix, normalize, useDSP))
    return false;

  if (!m_atempoBuffers->Create(totaltime))
    return false;

  return true;
}

void CActiveAEStreamBuffers::SetExtraData(int profile, enum AVMatrixEncoding matrix_encoding, enum AVAudioServiceType audio_service_type)
{
  m_resampleBuffers->SetExtraData(profile, matrix_encoding, audio_service_type);
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

void CActiveAEStreamBuffers::ConfigureResampler(bool normalizelevels, bool dspenabled, bool stereoupmix, AEQuality quality)
{
  m_resampleBuffers->ConfigureResampler(normalizelevels, dspenabled, stereoupmix, quality);
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

void CActiveAEStreamBuffers::SetRR(double rr)
{
  if (rr < 1.02 && rr > 0.98)
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
  tempo /= m_atempoBuffers->GetTempo();
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

void CActiveAEStreamBuffers::SetDSPConfig(bool usedsp, bool bypassdsp)
{
  m_resampleBuffers->SetDSPConfig(usedsp, bypassdsp);
}

CActiveAEBufferPool* CActiveAEStreamBuffers::GetResampleBuffers()
{
  CActiveAEBufferPool *ret = m_resampleBuffers;
  m_resampleBuffers = nullptr;
  return ret;
}

CActiveAEBufferPool* CActiveAEStreamBuffers::GetAtempoBuffers()
{
  CActiveAEBufferPool *ret = m_atempoBuffers;
  m_atempoBuffers = nullptr;
  return ret;
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
