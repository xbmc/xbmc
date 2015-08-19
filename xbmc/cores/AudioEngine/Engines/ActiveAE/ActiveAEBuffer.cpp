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
#include "cores/AudioEngine/DSPAddons/ActiveAEDSPProcess.h"
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
  m_dspSample = NULL;
  m_dspBuffer = NULL;
  m_resampleRatio = 1.0;
  m_resampleQuality = quality;
  m_forceResampler = false;
  m_stereoUpmix = false;
  m_normalize = true;
  m_useResampler = false;
  m_useDSP = false;
  m_bypassDSP = false;
  m_changeResampler = false;
  m_changeDSP = false;
}

CActiveAEBufferPoolResample::~CActiveAEBufferPoolResample()
{
  delete m_resampler;
  if (m_useDSP)
    CActiveAEDSP::GetInstance().DestroyDSPs(m_streamId);
  if (m_dspBuffer)
    delete m_dspBuffer;
}

void CActiveAEBufferPoolResample::SetExtraData(int profile, enum AVMatrixEncoding matrix_encoding, enum AVAudioServiceType audio_service_type)
{
  m_Profile = profile;
  m_MatrixEncoding = matrix_encoding;
  m_AudioServiceType = audio_service_type;

  if (m_useDSP)
    ChangeAudioDSP();
}

bool CActiveAEBufferPoolResample::Create(unsigned int totaltime, bool remap, bool upmix, bool normalize, bool useDSP)
{
  CActiveAEBufferPool::Create(totaltime);

  m_remap = remap;
  m_stereoUpmix = upmix;
  m_useResampler = m_changeResampler;                                       /* if m_changeResampler is true on Create, system require the usage of resampler */

  /*
   * On first used resample class, DSP signal processing becomes performed.
   * For the signal processing the input data for CActiveAEResample must be
   * modified to have a full supported audio stream with all available
   * channels, for this reason also some in resample used functions must be
   * disabled.
   *
   * The reason to perform this before CActiveAEResample is to have a unmodified
   * stream with the basic data to have best quality like for surround upmix.
   *
   * The value m_streamId and address pointer m_processor are passed a pointers
   * to CActiveAEDSP::GetInstance().CreateDSPs and set from it.
   */
  if ((useDSP || m_changeDSP) && !m_bypassDSP)
  {
    m_dspFormat = m_inputFormat;
    m_useDSP = CActiveAEDSP::GetInstance().CreateDSPs(m_streamId, m_processor, m_dspFormat, m_format, upmix, m_resampleQuality, m_MatrixEncoding, m_AudioServiceType, m_Profile);
    if (m_useDSP)
    {

      m_inputFormat.m_channelLayout = m_processor->GetChannelLayout();    /* Overide input format with DSP's supported format */
      m_inputFormat.m_sampleRate    = m_processor->GetOutputSamplerate(); /* Overide input format with DSP's generated samplerate */
      m_inputFormat.m_dataFormat    = m_processor->GetDataFormat();       /* Overide input format with DSP's processed data format, normally it is float */
      m_inputFormat.m_frames        = m_processor->GetOutputFrames();
      m_forceResampler              = true;                               /* Force load of ffmpeg resampler, required to detect exact input and output channel alignment pointers */
      if (m_processor->GetChannelLayout().Count() > 2)                    /* Disable upmix for CActiveAEResample if DSP layout > 2.0, becomes perfomed by DSP */
        upmix = false;

      m_dspBuffer = new CActiveAEBufferPool(m_inputFormat);               /* Get dsp processing buffer class, based on dsp output format */
      m_dspBuffer->Create(totaltime);
    }
  }
  m_changeDSP  = false;

  m_normalize = true;
  if ((m_format.m_channelLayout.Count() < m_inputFormat.m_channelLayout.Count() && !normalize))
    m_normalize = false;

  /*
   * Compare the input format with output, if there is one different format, perform the
   * kodi based resample processing.
   *
   * The input format can be become modified if addon dsp processing is enabled. For this
   * reason somethings are no more required for it. As example, if resampling is performed
   * by the addons it is no more required here, also the channel layout can be modified
   * from addons. The output data format from dsp is always float and if something other
   * is required here, the CActiveAEResample must change it.
   */
  if (m_inputFormat.m_channelLayout != m_format.m_channelLayout ||
      m_inputFormat.m_sampleRate != m_format.m_sampleRate ||
      m_inputFormat.m_dataFormat != m_format.m_dataFormat ||
      m_forceResampler)
    m_useResampler = true;

  if (m_useResampler || m_changeResampler)
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
  if (m_resampler)
  {
    delete m_resampler;
    m_resampler = NULL;
  }

  bool upmix = m_stereoUpmix;
  if (m_useDSP && m_processor && m_processor->GetChannelLayout().Count() > 2)
    upmix = false;

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
                                m_remap ? &m_format.m_channelLayout : NULL,
                                m_resampleQuality,
                                m_forceResampler);

  m_changeResampler = false;
}

void CActiveAEBufferPoolResample::ChangeAudioDSP()
{
  /* if dsp was enabled before reset input format, also used for failed dsp creation */
  bool wasActive = false;
  if (m_useDSP && m_processor != NULL)
  {
    m_inputFormat = m_processor->GetInputFormat();
    wasActive = true;
  }

  m_useDSP = CActiveAEDSP::GetInstance().CreateDSPs(m_streamId, m_processor, m_dspFormat, m_format, m_stereoUpmix, m_resampleQuality, m_MatrixEncoding, m_AudioServiceType, m_Profile, wasActive);
  if (m_useDSP)
  {
    m_inputFormat.m_channelLayout = m_processor->GetChannelLayout();    /* Overide input format with DSP's supported format */
    m_inputFormat.m_sampleRate    = m_processor->GetOutputSamplerate(); /* Overide input format with DSP's generated samplerate */
    m_inputFormat.m_dataFormat    = m_processor->GetDataFormat();       /* Overide input format with DSP's processed data format, normally it is float */
    m_inputFormat.m_frames        = m_processor->GetOutputFrames();
    m_changeResampler             = true;                               /* Force load of ffmpeg resampler, required to detect exact input and output channel alignment pointers */
  }
  else if (wasActive)
  {
    /*
     * Check now after the dsp processing becomes disabled, that the resampler is still
     * required, if not unload it also.
     */
    if (m_inputFormat.m_channelLayout == m_format.m_channelLayout &&
        m_inputFormat.m_sampleRate == m_format.m_sampleRate &&
        m_inputFormat.m_dataFormat == m_format.m_dataFormat &&
        !m_useResampler)
    {
      delete m_resampler;
      m_resampler = NULL;
      delete m_dspBuffer;
      m_dspBuffer = NULL;
      m_changeResampler = false;
    }
    else
      m_changeResampler = true;

    m_useDSP = false;
    CActiveAEDSP::GetInstance().DestroyDSPs(m_streamId);
  }
  else
  {
    m_useDSP = false;
  }

  m_changeDSP = false;
}

bool CActiveAEBufferPoolResample::ResampleBuffers(int64_t timestamp)
{
  bool busy = false;
  CSampleBuffer *in;

  if (!m_resampler)
  {
    if (m_changeDSP || m_changeResampler)
    {
      if (m_changeDSP)
        ChangeAudioDSP();
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

    if (hasInput || skipInput || m_drain || m_changeResampler || m_changeDSP)
    {
      if (!m_procSample)
      {
        m_procSample = GetFreeBuffer();
      }

      if (hasInput && !skipInput && !m_changeResampler && !m_changeDSP)
      {
        in = m_inputSamples.front();
        m_inputSamples.pop_front();
      }
      else
        in = NULL;

      /*
       * DSP need always a available input packet! To pass it step by step
       * over all enabled addons and processing segments.
       */
      if (m_useDSP && in)
      {
        if (!m_dspSample)
          m_dspSample = m_dspBuffer->GetFreeBuffer();

        if (m_dspSample && m_processor->Process(in, m_dspSample))
        {
          in->Return();
          in = m_dspSample;
          m_dspSample = NULL;
        }
        else
        {
          in->Return();
          in = NULL;
        }
      }

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

      if ((m_drain || m_changeResampler || m_changeDSP) && m_empty)
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
        if (m_changeDSP)
          ChangeAudioDSP();
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
  if (m_dspSample)
    delay += m_dspSample->pkt->nb_samples / m_dspSample->pkt->config.sample_rate;

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

  if (m_useDSP)
  {
    delay += m_processor->GetDelay();
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
  if (m_dspSample)
  {
    m_dspSample->Return();
    m_dspSample = NULL;
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
