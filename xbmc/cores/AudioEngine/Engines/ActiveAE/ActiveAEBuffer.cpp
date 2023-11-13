/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ActiveAEBuffer.h"

#include "ActiveAE.h"
#include "ActiveAEFilter.h"
#include "cores/AudioEngine/AEResampleFactory.h"
#include "cores/AudioEngine/Utils/AEUtil.h"

#include <memory>

using namespace ActiveAE;

CSoundPacket::CSoundPacket(const SampleConfig& conf, int samples) : config(conf)
{
  data = CActiveAE::AllocSoundSample(config, samples, bytes_per_sample, planes, linesize);
  max_nb_samples = samples;
  nb_samples = 0;
  pause_burst_ms = 0;
}

CSoundPacket::~CSoundPacket()
{
  if (data)
    CActiveAE::FreeSoundSample(data);
}

CSampleBuffer* CSampleBuffer::Acquire()
{
  refCount++;
  return this;
}

void CSampleBuffer::Return()
{
  refCount--;
  if (pool && refCount <= 0)
    pool->ReturnBuffer(this);
}

CActiveAEBufferPool::CActiveAEBufferPool(const AEAudioFormat& format) : m_format(format)
{
  if (m_format.m_dataFormat == AE_FMT_RAW)
  {
    m_format.m_frameSize = 1;
    m_format.m_frames = 61440;
    m_format.m_channelLayout.Reset();
    m_format.m_channelLayout += AE_CH_FC;
  }
}

CActiveAEBufferPool::~CActiveAEBufferPool()
{
  CSampleBuffer *buffer;
  while(!m_allSamples.empty())
  {
    buffer = m_allSamples.front();
    m_allSamples.pop_front();
    delete buffer;
  }
}

CSampleBuffer* CActiveAEBufferPool::GetFreeBuffer()
{
  CSampleBuffer* buf = NULL;

  if (!m_freeSamples.empty())
  {
    buf = m_freeSamples.front();
    m_freeSamples.pop_front();
    buf->refCount = 1;
    buf->centerMixLevel = M_SQRT1_2;
  }
  return buf;
}

void CActiveAEBufferPool::ReturnBuffer(CSampleBuffer *buffer)
{
  buffer->pkt->nb_samples = 0;
  buffer->pkt->pause_burst_ms = 0;
  m_freeSamples.push_back(buffer);
}

bool CActiveAEBufferPool::Create(unsigned int totaltime)
{
  CSampleBuffer *buffer;
  SampleConfig config;
  config.fmt = CAEUtil::GetAVSampleFormat(m_format.m_dataFormat);
  config.bits_per_sample = CAEUtil::DataFormatToUsedBits(m_format.m_dataFormat);
  config.dither_bits = CAEUtil::DataFormatToDitherBits(m_format.m_dataFormat);
  config.channels = m_format.m_channelLayout.Count();
  config.sample_rate = m_format.m_sampleRate;
  config.channel_layout = CAEUtil::GetAVChannelLayout(m_format.m_channelLayout);

  unsigned int time = 0;
  unsigned int buffertime = (m_format.m_frames*1000) / m_format.m_sampleRate;
  if (m_format.m_dataFormat == AE_FMT_RAW)
  {
    buffertime = m_format.m_streamInfo.GetDuration();
  }
  unsigned int n = 0;
  while (time < totaltime || n < 5)
  {
    buffer = new CSampleBuffer();
    buffer->pool = this;
    buffer->pkt = std::make_unique<CSoundPacket>(config, m_format.m_frames);

    m_allSamples.push_back(buffer);
    m_freeSamples.push_back(buffer);
    time += buffertime;
    n++;
  }

  return true;
}

// ----------------------------------------------------------------------------------
// Resample
// ----------------------------------------------------------------------------------

CActiveAEBufferPoolResample::CActiveAEBufferPoolResample(const AEAudioFormat& inputFormat,
                                                         const AEAudioFormat& outputFormat,
                                                         AEQuality quality)
  : CActiveAEBufferPool(outputFormat), m_inputFormat(inputFormat)
{
  if (m_inputFormat.m_dataFormat == AE_FMT_RAW)
  {
    m_format.m_frameSize = 1;
    m_format.m_frames = 61440;
    m_inputFormat.m_channelLayout.Reset();
    m_inputFormat.m_channelLayout += AE_CH_FC;
  }
  m_resampleQuality = quality;
}

CActiveAEBufferPoolResample::~CActiveAEBufferPoolResample()
{
  Flush();
}

bool CActiveAEBufferPoolResample::Create(unsigned int totaltime, bool remap, bool upmix, bool normalize)
{
  CActiveAEBufferPool::Create(totaltime);

  m_remap = remap;
  m_stereoUpmix = upmix;

  m_normalize = true;
  if ((m_format.m_channelLayout.Count() < m_inputFormat.m_channelLayout.Count() && !normalize))
    m_normalize = false;

  if (m_inputFormat.m_channelLayout != m_format.m_channelLayout ||
      m_inputFormat.m_sampleRate != m_format.m_sampleRate ||
      m_inputFormat.m_dataFormat != m_format.m_dataFormat ||
      m_changeResampler)
  {
    ChangeResampler();
  }
  return true;
}

void CActiveAEBufferPoolResample::ChangeResampler()
{
  m_resampler = CAEResampleFactory::Create();

  SampleConfig dstConfig, srcConfig;
  dstConfig.channel_layout = CAEUtil::GetAVChannelLayout(m_format.m_channelLayout);
  dstConfig.channels = m_format.m_channelLayout.Count();
  dstConfig.sample_rate = m_format.m_sampleRate;
  dstConfig.fmt = CAEUtil::GetAVSampleFormat(m_format.m_dataFormat);
  dstConfig.bits_per_sample = CAEUtil::DataFormatToUsedBits(m_format.m_dataFormat);
  dstConfig.dither_bits = CAEUtil::DataFormatToDitherBits(m_format.m_dataFormat);

  srcConfig.channel_layout = CAEUtil::GetAVChannelLayout(m_inputFormat.m_channelLayout);
  srcConfig.channels = m_inputFormat.m_channelLayout.Count();
  srcConfig.sample_rate = m_inputFormat.m_sampleRate;
  srcConfig.fmt = CAEUtil::GetAVSampleFormat(m_inputFormat.m_dataFormat);
  srcConfig.bits_per_sample = CAEUtil::DataFormatToUsedBits(m_inputFormat.m_dataFormat);
  srcConfig.dither_bits = CAEUtil::DataFormatToDitherBits(m_inputFormat.m_dataFormat);

  m_resampler->Init(dstConfig, srcConfig,
                    m_stereoUpmix,
                    m_normalize,
                    m_centerMixLevel,
                    m_remap ? &m_format.m_channelLayout : nullptr,
                    m_resampleQuality,
                    m_forceResampler);

  m_changeResampler = false;
}

bool CActiveAEBufferPoolResample::ResampleBuffers(int64_t timestamp)
{
  bool busy = false;
  CSampleBuffer *in;

  if (!m_resampler)
  {
    if (m_changeResampler)
    {
      if (m_changeResampler)
        ChangeResampler();
      return true;
    }
    while(!m_inputSamples.empty())
    {
      in = m_inputSamples.front();
      m_inputSamples.pop_front();
      if (timestamp)
      {
        in->timestamp = timestamp;
      }
      m_outputSamples.push_back(in);
      busy = true;
    }
  }
  else if (m_procSample || !m_freeSamples.empty())
  {
    int free_samples;
    if (m_procSample)
      free_samples = m_procSample->pkt->max_nb_samples - m_procSample->pkt->nb_samples;
    else
      free_samples = m_format.m_frames;

    bool skipInput = false;
    // avoid that ffmpeg resample buffer grows too large
    if (!m_resampler->WantsNewSamples(free_samples) && !m_empty)
      skipInput = true;

    bool hasInput = !m_inputSamples.empty();

    if (hasInput || skipInput || m_drain || m_changeResampler)
    {
      if (!m_procSample)
      {
        m_procSample = GetFreeBuffer();
      }

      if (hasInput && !skipInput && !m_changeResampler)
      {
        in = m_inputSamples.front();
        if (in->centerMixLevel != m_centerMixLevel)
        {
          m_centerMixLevel = in->centerMixLevel;
          m_changeResampler = true;
          in = nullptr;
        }
        else
          m_inputSamples.pop_front();
      }
      else
        in = nullptr;

      int start = m_procSample->pkt->nb_samples *
                  m_procSample->pkt->bytes_per_sample *
                  m_procSample->pkt->config.channels /
                  m_procSample->pkt->planes;

      for(int i=0; i<m_procSample->pkt->planes; i++)
      {
        m_planes[i] = m_procSample->pkt->data[i] + start;
      }

      int out_samples = m_resampler->Resample(m_planes,
                                              m_procSample->pkt->max_nb_samples - m_procSample->pkt->nb_samples,
                                              in ? in->pkt->data : NULL,
                                              in ? in->pkt->nb_samples : 0,
                                              m_resampleRatio);
      // in case of error, trigger re-create of resampler
      if (out_samples < 0)
      {
        out_samples = 0;
        m_changeResampler = true;
      }

      m_procSample->pkt->nb_samples += out_samples;
      busy = true;
      m_empty = (out_samples == 0);

      if (in)
      {
        if (!timestamp)
        {
          if (in->timestamp)
            m_lastSamplePts = in->timestamp;
          else
            in->pkt_start_offset = 0;
        }
        else
        {
          m_lastSamplePts = timestamp;
          in->pkt_start_offset = 0;
        }

        // pts of last sample we added to the buffer
        m_lastSamplePts +=
            (in->pkt->nb_samples - in->pkt_start_offset) * 1000 / in->pkt->config.sample_rate;
      }

      // calculate pts for last sample in m_procSample
      int bufferedSamples = m_resampler->GetBufferedSamples();
      m_procSample->pkt_start_offset = m_procSample->pkt->nb_samples;
      m_procSample->timestamp = m_lastSamplePts - bufferedSamples * 1000 / m_format.m_sampleRate;

      if ((m_drain || m_changeResampler) && m_empty)
      {
        if (m_fillPackets && m_procSample->pkt->nb_samples != 0)
        {
          // pad with zero
          start = m_procSample->pkt->nb_samples *
                  m_procSample->pkt->bytes_per_sample *
                  m_procSample->pkt->config.channels /
                  m_procSample->pkt->planes;
          for(int i=0; i<m_procSample->pkt->planes; i++)
          {
            memset(m_procSample->pkt->data[i]+start, 0, m_procSample->pkt->linesize-start);
          }
        }

        // check if draining is finished
        if (m_drain && m_procSample->pkt->nb_samples == 0)
        {
          m_procSample->Return();
          busy = false;
        }
        else
          m_outputSamples.push_back(m_procSample);

        m_procSample = NULL;
        if (m_changeResampler)
          ChangeResampler();
      }
      // some methods like encode require completely filled packets
      else if (!m_fillPackets || (m_procSample->pkt->nb_samples == m_procSample->pkt->max_nb_samples))
      {
        m_outputSamples.push_back(m_procSample);
        m_procSample = NULL;
      }

      if (in)
        in->Return();
    }
  }
  return busy;
}

void CActiveAEBufferPoolResample::ConfigureResampler(bool normalizelevels, bool stereoupmix, AEQuality quality)
{
  bool normalize = true;
  if ((m_format.m_channelLayout.Count() < m_inputFormat.m_channelLayout.Count()) && !normalizelevels)
  {
    normalize = false;
  }

  if (m_normalize != normalize || m_resampleQuality != quality)
  {
    m_changeResampler = true;
  }

  m_resampleQuality = quality;
  m_normalize = normalize;
}

float CActiveAEBufferPoolResample::GetDelay()
{
  float delay = 0;
  std::deque<CSampleBuffer*>::iterator itBuf;

  if (m_procSample)
    delay += (float)m_procSample->pkt->nb_samples / m_procSample->pkt->config.sample_rate;

  for(itBuf=m_inputSamples.begin(); itBuf!=m_inputSamples.end(); ++itBuf)
  {
    delay += (float)(*itBuf)->pkt->nb_samples / (*itBuf)->pkt->config.sample_rate;
  }

  for(itBuf=m_outputSamples.begin(); itBuf!=m_outputSamples.end(); ++itBuf)
  {
    delay += (float)(*itBuf)->pkt->nb_samples / (*itBuf)->pkt->config.sample_rate;
  }

  if (m_resampler)
  {
    int samples = m_resampler->GetBufferedSamples();
    delay += (float)samples / m_format.m_sampleRate;
  }

  return delay;
}

void CActiveAEBufferPoolResample::Flush()
{
  if (m_procSample)
  {
    m_procSample->Return();
    m_procSample = NULL;
  }
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
  if (m_resampler)
    ChangeResampler();
}

void CActiveAEBufferPoolResample::SetDrain(bool drain)
{
  m_drain = drain;
}

void CActiveAEBufferPoolResample::SetRR(double rr)
{
  m_resampleRatio = rr;
}

double CActiveAEBufferPoolResample::GetRR() const
{
  return m_resampleRatio;
}

void CActiveAEBufferPoolResample::FillBuffer()
{
  m_fillPackets = true;
}

bool CActiveAEBufferPoolResample::DoesNormalize() const
{
  return m_normalize;
}

void CActiveAEBufferPoolResample::ForceResampler(bool force)
{
  m_forceResampler = force;
}


// ----------------------------------------------------------------------------------
// Atempo
// ----------------------------------------------------------------------------------

CActiveAEBufferPoolAtempo::CActiveAEBufferPoolAtempo(const AEAudioFormat& format) : CActiveAEBufferPool(format)
{
  m_drain = false;
  m_empty = true;
  m_tempo = 1.0;
  m_changeFilter = false;
  m_procSample = nullptr;
  m_fillPackets = false;
}

CActiveAEBufferPoolAtempo::~CActiveAEBufferPoolAtempo()
{
  Flush();
}

bool CActiveAEBufferPoolAtempo::Create(unsigned int totaltime)
{
  CActiveAEBufferPool::Create(totaltime);

  m_pTempoFilter = std::make_unique<CActiveAEFilter>();
  m_pTempoFilter->Init(CAEUtil::GetAVSampleFormat(m_format.m_dataFormat), m_format.m_sampleRate, CAEUtil::GetAVChannelLayout(m_format.m_channelLayout));

  return true;
}

void CActiveAEBufferPoolAtempo::ChangeFilter()
{
  m_pTempoFilter->SetTempo(m_tempo);
  m_changeFilter = false;
}

bool CActiveAEBufferPoolAtempo::ProcessBuffers()
{
  bool busy = false;
  CSampleBuffer *in;

  if (!m_pTempoFilter->IsActive())
  {
    if (m_changeFilter)
    {
      if (m_changeFilter)
        ChangeFilter();
      return true;
    }
    while(!m_inputSamples.empty())
    {
      in = m_inputSamples.front();
      m_inputSamples.pop_front();
      m_outputSamples.push_back(in);
      busy = true;
    }
  }
  else if (m_procSample || !m_freeSamples.empty())
  {
    bool skipInput = false;

    // avoid that bufferscr grows too large
    if (!m_pTempoFilter->NeedData())
      skipInput = true;

    bool hasInput = !m_inputSamples.empty();

    if (hasInput || skipInput || m_drain || m_changeFilter)
    {
      if (!m_procSample)
      {
        m_procSample = GetFreeBuffer();
      }

      if (hasInput && !skipInput && !m_changeFilter)
      {
        in = m_inputSamples.front();
        m_inputSamples.pop_front();
      }
      else
        in = nullptr;

      int start = m_procSample->pkt->nb_samples *
                  m_procSample->pkt->bytes_per_sample *
                  m_procSample->pkt->config.channels /
                  m_procSample->pkt->planes;

      for (int i=0; i<m_procSample->pkt->planes; i++)
      {
        m_planes[i] = m_procSample->pkt->data[i] + start;
      }

      int out_samples = m_pTempoFilter->ProcessFilter(m_planes,
                                                      m_procSample->pkt->max_nb_samples - m_procSample->pkt->nb_samples,
                                                      in ? in->pkt->data : nullptr,
                                                      in ? in->pkt->nb_samples : 0,
                                                      in ? in->pkt->linesize * in->pkt->planes : 0);

      // in case of error, trigger re-create of filter
      if (out_samples < 0)
      {
        out_samples = 0;
        m_changeFilter = true;
      }

      m_procSample->pkt->nb_samples += out_samples;
      busy = true;
      m_empty = m_pTempoFilter->IsEof();

      if (in)
      {
        if (in->timestamp)
          m_lastSamplePts = in->timestamp;
        else
          in->pkt_start_offset = 0;

        // pts of last sample we added to the buffer
        m_lastSamplePts += (in->pkt->nb_samples-in->pkt_start_offset) * 1000 / m_format.m_sampleRate;
      }

      // calculate pts for last sample in m_procSample
      int bufferedSamples = m_pTempoFilter->GetBufferedSamples();
      m_procSample->pkt_start_offset = m_procSample->pkt->nb_samples;
      m_procSample->timestamp = m_lastSamplePts - bufferedSamples * 1000 / m_format.m_sampleRate;

      if ((m_drain || m_changeFilter) && m_empty)
      {
        if (m_fillPackets && m_procSample->pkt->nb_samples != 0)
        {
          // pad with zero
          start = m_procSample->pkt->nb_samples *
          m_procSample->pkt->bytes_per_sample *
          m_procSample->pkt->config.channels /
          m_procSample->pkt->planes;
          for (int i=0; i<m_procSample->pkt->planes; i++)
          {
            memset(m_procSample->pkt->data[i]+start, 0, m_procSample->pkt->linesize-start);
          }
        }

        // check if draining is finished
        if (m_drain && m_procSample->pkt->nb_samples == 0)
        {
          m_procSample->Return();
          busy = false;
        }
        else
          m_outputSamples.push_back(m_procSample);

        m_procSample = nullptr;

        if (m_changeFilter)
        {
          ChangeFilter();
        }
      }
      // some methods like encode require completely filled packets
      else if (!m_fillPackets || (m_procSample->pkt->nb_samples == m_procSample->pkt->max_nb_samples))
      {
        m_outputSamples.push_back(m_procSample);
        m_procSample = nullptr;
      }

      if (in)
        in->Return();
    }
  }
  return busy;
}

void CActiveAEBufferPoolAtempo::Flush()
{
  if (m_procSample)
  {
    m_procSample->Return();
    m_procSample = nullptr;
  }
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
  if (m_pTempoFilter)
    ChangeFilter();
}

float CActiveAEBufferPoolAtempo::GetDelay()
{
  float delay = 0;

  if (m_procSample)
    delay += (float)m_procSample->pkt->nb_samples / m_procSample->pkt->config.sample_rate;

  for (auto &buf : m_inputSamples)
  {
    delay += (float)buf->pkt->nb_samples / buf->pkt->config.sample_rate;
  }

  for (auto &buf : m_outputSamples)
  {
    delay += (float)buf->pkt->nb_samples / buf->pkt->config.sample_rate;
  }

  if (m_pTempoFilter->IsActive())
  {
    int samples = m_pTempoFilter->GetBufferedSamples();
    delay += (float)samples / m_format.m_sampleRate;
  }

  return delay;
}

void CActiveAEBufferPoolAtempo::SetTempo(float tempo)
{
  if (tempo > 2.0f)
    tempo = 2.0;
  else if (tempo < 0.5f)
    tempo = 0.5;

  if (tempo != m_tempo)
    m_changeFilter = true;

  m_tempo = tempo;
}

float CActiveAEBufferPoolAtempo::GetTempo() const
{
  return m_tempo;
}

void CActiveAEBufferPoolAtempo::FillBuffer()
{
  m_fillPackets = true;
}

void CActiveAEBufferPoolAtempo::SetDrain(bool drain)
{
  m_drain = drain;
  if (!m_drain)
    m_changeFilter = true;
}
