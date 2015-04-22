/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "ActiveAEBuffer.h"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAE.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/AudioEngine/AEResampleFactory.h"

using namespace ActiveAE;

/* typecast AE to CActiveAE */
#define AE (*((CActiveAE*)CAEFactory::GetEngine()))

CSoundPacket::CSoundPacket(SampleConfig conf, int samples) : config(conf)
{
  data = AE.AllocSoundSample(config, samples, bytes_per_sample, planes, linesize);
  max_nb_samples = samples;
  nb_samples = 0;
}

CSoundPacket::~CSoundPacket()
{
  if (data)
    AE.FreeSoundSample(data);
}

CSampleBuffer::CSampleBuffer() : pkt(NULL), pool(NULL)
{
  refCount = 0;
  timestamp = 0;
  clockId = -1;
  pkt_start_offset = 0;
}

CSampleBuffer::~CSampleBuffer()
{
  delete pkt;
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

CActiveAEBufferPool::CActiveAEBufferPool(AEAudioFormat format)
{
  m_format = format;
  if (AE_IS_RAW(m_format.m_dataFormat))
    m_format.m_dataFormat = AE_FMT_S16NE;
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
  }
  return buf;
}

void CActiveAEBufferPool::ReturnBuffer(CSampleBuffer *buffer)
{
  buffer->pkt->nb_samples = 0;
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
  unsigned int n = 0;
  while (time < totaltime || n < 5)
  {
    buffer = new CSampleBuffer();
    buffer->pool = this;
    buffer->pkt = new CSoundPacket(config, m_format.m_frames);

    m_allSamples.push_back(buffer);
    m_freeSamples.push_back(buffer);
    time += buffertime;
    n++;
  }

  return true;
}

//-----------------------------------------------------------------------------

CActiveAEBufferPoolResample::CActiveAEBufferPoolResample(AEAudioFormat inputFormat, AEAudioFormat outputFormat, AEQuality quality)
  : CActiveAEBufferPool(outputFormat)
{
  m_inputFormat = inputFormat;
  if (AE_IS_RAW(m_inputFormat.m_dataFormat))
    m_inputFormat.m_dataFormat = AE_FMT_S16NE;
  m_resampler = NULL;
  m_fillPackets = false;
  m_drain = false;
  m_empty = true;
  m_procSample = NULL;
  m_resampleRatio = 1.0;
  m_resampleQuality = quality;
  m_changeResampler = false;
  m_forceResampler = false;
  m_stereoUpmix = false;
  m_normalize = true;
}

CActiveAEBufferPoolResample::~CActiveAEBufferPoolResample()
{
  delete m_resampler;
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
      m_forceResampler)
  {
    m_resampler = CAEResampleFactory::Create();
    m_resampler->Init(CAEUtil::GetAVChannelLayout(m_format.m_channelLayout),
                                m_format.m_channelLayout.Count(),
                                m_format.m_sampleRate,
                                CAEUtil::GetAVSampleFormat(m_format.m_dataFormat),
                                CAEUtil::DataFormatToUsedBits(m_format.m_dataFormat),
                                CAEUtil::DataFormatToDitherBits(m_format.m_dataFormat),
                                CAEUtil::GetAVChannelLayout(m_inputFormat.m_channelLayout),
                                m_inputFormat.m_channelLayout.Count(),
                                m_inputFormat.m_sampleRate,
                                CAEUtil::GetAVSampleFormat(m_inputFormat.m_dataFormat),
                                CAEUtil::DataFormatToUsedBits(m_inputFormat.m_dataFormat),
                                CAEUtil::DataFormatToDitherBits(m_inputFormat.m_dataFormat),
                                upmix,
                                m_normalize,
                                remap ? &m_format.m_channelLayout : NULL,
                                m_resampleQuality,
                                m_forceResampler);
  }

  m_changeResampler = false;

  return true;
}

void CActiveAEBufferPoolResample::ChangeResampler()
{
  delete m_resampler;

  m_resampler = CAEResampleFactory::Create();
  m_resampler->Init(CAEUtil::GetAVChannelLayout(m_format.m_channelLayout),
                                m_format.m_channelLayout.Count(),
                                m_format.m_sampleRate,
                                CAEUtil::GetAVSampleFormat(m_format.m_dataFormat),
                                CAEUtil::DataFormatToUsedBits(m_format.m_dataFormat),
                                CAEUtil::DataFormatToDitherBits(m_format.m_dataFormat),
                                CAEUtil::GetAVChannelLayout(m_inputFormat.m_channelLayout),
                                m_inputFormat.m_channelLayout.Count(),
                                m_inputFormat.m_sampleRate,
                                CAEUtil::GetAVSampleFormat(m_inputFormat.m_dataFormat),
                                CAEUtil::DataFormatToUsedBits(m_inputFormat.m_dataFormat),
                                CAEUtil::DataFormatToDitherBits(m_inputFormat.m_dataFormat),
                                m_stereoUpmix,
                                m_normalize,
                                m_remap ? &m_format.m_channelLayout : NULL,
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
        in->clockId = -1;
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
        m_inputSamples.pop_front();
      }
      else
        in = NULL;

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
          m_lastSamplePts = in->timestamp;
          m_procSample->clockId = in->clockId;
        }
        else
        {
          m_lastSamplePts = timestamp;
          in->pkt_start_offset = 0;
          m_procSample->clockId = -1;
        }

        // pts of last sample we added to the buffer
        m_lastSamplePts += (in->pkt->nb_samples-in->pkt_start_offset)/m_format.m_sampleRate * 1000;
      }

      // calculate pts for last sample in m_procSample
      int bufferedSamples = m_resampler->GetBufferedSamples();
      m_procSample->pkt_start_offset = m_procSample->pkt->nb_samples;
      m_procSample->timestamp = m_lastSamplePts - bufferedSamples/m_format.m_sampleRate*1000;

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

float CActiveAEBufferPoolResample::GetDelay()
{
  float delay = 0;
  std::deque<CSampleBuffer*>::iterator itBuf;

  if (m_procSample)
    delay += m_procSample->pkt->nb_samples / m_procSample->pkt->config.sample_rate;

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
