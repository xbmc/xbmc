/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <cassert>

#include "cores/AudioEngine/Utils/AEUtil.h"
#include "ActiveAEResamplePi.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "platform/linux/RBP.h"

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

//#define DEBUG_VERBOSE

#define CLASSNAME "CActiveAEResamplePi"

#define BUFFERSIZE (32*1024*2*8)

//#define BENCHMARKING
#ifdef BENCHMARKING
#define LOGTIMEINIT(f) \
  struct timespec now; \
  uint64_t  Start, End; \
  clock_gettime(CLOCK_MONOTONIC, &now); \
  Start = ((int64_t)now.tv_sec * 1000000000L) + now.tv_nsec; \
  const char *_filename = f;

#define LOGTIME(n) \
  clock_gettime(CLOCK_MONOTONIC, &now); \
  End = ((int64_t)now.tv_sec * 1000000000L) + now.tv_nsec; \
  CLog::Log(LOGNOTICE, "ActiveAE::%s %d - resample %s took %.0fms", __FUNCTION__, n, _filename, (End-Start)*1e-6); \
  Start=End;
#else
#define LOGTIMEINIT(f)
#define LOGTIME(n)
#endif

using namespace ActiveAE;

CActiveAEResamplePi::CActiveAEResamplePi()
{
  CLog::Log(LOGINFO, "%s::%s", CLASSNAME, __func__);

  m_Initialized = false;
  m_encoded_buffer = NULL;
  m_offset = 0;
  m_ratio = 0.0;
}

CActiveAEResamplePi::~CActiveAEResamplePi()
{
  CLog::Log(LOGINFO, "%s::%s", CLASSNAME, __func__);
  DeInit();
}

void CActiveAEResamplePi::DeInit()
{
  CLog::Log(LOGDEBUG, "%s:%s", CLASSNAME, __func__);
  if (m_Initialized)
  {
    m_omx_mixer.FlushAll();
    m_omx_mixer.Deinitialize();
    m_Initialized = false;
  }
}

static int format_to_bits(AVSampleFormat fmt)
{
  switch (fmt)
  {
  case AV_SAMPLE_FMT_U8:
  case AV_SAMPLE_FMT_U8P:
    return 8;
  case AV_SAMPLE_FMT_S16:
  case AV_SAMPLE_FMT_S16P:
    return 16;
  case AV_SAMPLE_FMT_S32:
  case AV_SAMPLE_FMT_S32P:
  case AV_SAMPLE_FMT_FLT:
  case AV_SAMPLE_FMT_FLTP:
    return 32;
  default:
    assert(0);
  }
  return 0;
}

bool CActiveAEResamplePi::Init(SampleConfig dstConfig, SampleConfig srcConfig, bool upmix, bool normalize, double centerMix,
                               CAEChannelInfo *remapLayout, AEQuality quality, bool force_resample)
{
  LOGTIMEINIT("x");

  CLog::Log(LOGINFO,
            "%s::%s remap:%p chan:%d->%d rate:%d->%d format:%d->%d bits:%d->%d dither:%d->%d "
            "norm:%d upmix:%d",
            CLASSNAME, __func__, static_cast<void*>(remapLayout), srcConfig.channels, dstConfig.channels,
            srcConfig.sample_rate, dstConfig.sample_rate, srcConfig.fmt, dstConfig.fmt,
            srcConfig.bits_per_sample, dstConfig.bits_per_sample, srcConfig.dither_bits, dstConfig.dither_bits,
            normalize, upmix);

  m_dst_chan_layout = dstConfig.channel_layout;
  m_dst_channels = dstConfig.channels;
  m_dst_rate = dstConfig.sample_rate;
  m_dst_fmt = dstConfig.fmt;
  m_dst_bits = dstConfig.bits_per_sample;
  m_dst_dither_bits = dstConfig.dither_bits;
  m_src_chan_layout = srcConfig.channel_layout;
  m_src_channels = srcConfig.channels;
  m_src_rate = srcConfig.sample_rate;
  m_src_fmt = srcConfig.fmt;
  m_src_bits = srcConfig.bits_per_sample;
  m_src_dither_bits = srcConfig.dither_bits;
  m_offset = 0;
  m_src_pitch = format_to_bits(m_src_fmt) >> 3;
  m_dst_pitch = format_to_bits(m_dst_fmt) >> 3;
  m_force_resample = force_resample;

  // special handling for S24 formats which are carried in S32 (S24NE3)
  if ((m_dst_fmt == AV_SAMPLE_FMT_S32 || m_dst_fmt == AV_SAMPLE_FMT_S32P) && m_dst_bits == 24 && m_dst_dither_bits == -8)
    m_dst_pitch = 24;

  if (m_dst_chan_layout == 0)
    m_dst_chan_layout = av_get_default_channel_layout(m_dst_channels);
  if (m_src_chan_layout == 0)
    m_src_chan_layout = av_get_default_channel_layout(m_src_channels);

  OMX_CONFIG_BRCMAUDIODOWNMIXCOEFFICIENTS8x8 mix;
  OMX_INIT_STRUCTURE(mix);

  assert(sizeof(mix.coeff)/sizeof(mix.coeff[0]) == 64);

  LOGTIME(1);
// this code is just uses ffmpeg to produce the 8x8 mixing matrix
{
  // dummy sample rate and format, as we only care about channel mapping
  SwrContext *m_pContext = swr_alloc_set_opts(NULL, m_dst_chan_layout, AV_SAMPLE_FMT_FLT, 48000,
                                                        m_src_chan_layout, AV_SAMPLE_FMT_FLT, 48000, 0, NULL);
  if (!m_pContext)
  {
    CLog::Log(LOGERROR, "CActiveAEResamplePi::Init - create context failed");
    return false;
  }
  // tell resampler to clamp float values
  // not required for sink stage (remapLayout == true)
  if (!remapLayout && normalize)
  {
    av_opt_set_double(m_pContext, "rematrix_maxval", 1.0, 0);
  }

  if (remapLayout)
  {
    // one-to-one mapping of channels
    // remapLayout is the layout of the sink, if the channel is in our src layout
    // the channel is mapped by setting coef 1.0
    double m_rematrix[AE_CH_MAX][AE_CH_MAX];
    memset(m_rematrix, 0, sizeof(m_rematrix));
    m_dst_chan_layout = 0;
    for (unsigned int out=0; out<remapLayout->Count(); out++)
    {
      m_dst_chan_layout += (uint64_t) (1 << out);
      int idx = CAEUtil::GetAVChannelIndex((*remapLayout)[out], m_src_chan_layout);
      if (idx >= 0)
      {
        m_rematrix[out][idx] = 1.0;
      }
    }

    av_opt_set_int(m_pContext, "out_channel_count", m_dst_channels, 0);
    av_opt_set_int(m_pContext, "out_channel_layout", m_dst_chan_layout, 0);

    if (swr_set_matrix(m_pContext, (const double*)m_rematrix, AE_CH_MAX) < 0)
    {
      CLog::Log(LOGERROR, "CActiveAEResamplePi::Init - setting channel matrix failed");
      return false;
    }
  }
  // stereo upmix
  else if (upmix && m_src_channels == 2 && m_dst_channels > 2)
  {
    double m_rematrix[AE_CH_MAX][AE_CH_MAX];
    memset(m_rematrix, 0, sizeof(m_rematrix));
    for (int out=0; out<m_dst_channels; out++)
    {
      uint64_t out_chan = av_channel_layout_extract_channel(m_dst_chan_layout, out);
      switch(out_chan)
      {
        case AV_CH_FRONT_LEFT:
        case AV_CH_BACK_LEFT:
        case AV_CH_SIDE_LEFT:
          m_rematrix[out][0] = 1.0;
          break;
        case AV_CH_FRONT_RIGHT:
        case AV_CH_BACK_RIGHT:
        case AV_CH_SIDE_RIGHT:
          m_rematrix[out][1] = 1.0;
          break;
        case AV_CH_FRONT_CENTER:
          m_rematrix[out][0] = 0.5;
          m_rematrix[out][1] = 0.5;
          break;
        case AV_CH_LOW_FREQUENCY:
          m_rematrix[out][0] = 0.5;
          m_rematrix[out][1] = 0.5;
          break;
        default:
          break;
      }
    }

    if (swr_set_matrix(m_pContext, (const double*)m_rematrix, AE_CH_MAX) < 0)
    {
      CLog::Log(LOGERROR, "CActiveAEResamplePi::Init - setting channel matrix failed");
      return false;
    }
  }

  if (swr_init(m_pContext) < 0)
  {
    CLog::Log(LOGERROR, "CActiveAEResamplePi::Init - init resampler failed");
    return false;
  }

  const int samples = 8;
  uint8_t *output, *input;
  av_samples_alloc(&output, NULL, m_dst_channels, samples, AV_SAMPLE_FMT_FLT, 1);
  av_samples_alloc(&input , NULL, m_src_channels, samples, AV_SAMPLE_FMT_FLT, 1);

  // Produce "identity" samples
  float *f = (float *)input;
  for (int j=0; j < samples; j++)
    for (int i=0; i < m_src_channels; i++)
      *f++ = i == j ? 1.0f : 0.0f;

  int ret = swr_convert(m_pContext, &output, samples, (const uint8_t **)&input, samples);
  if (ret < 0)
    CLog::Log(LOGERROR, "CActiveAEResamplePi::Resample - resample failed");

  f = (float *)output;
  for (int j=0; j < samples; j++)
    for (int i=0; i < m_dst_channels; i++)
      mix.coeff[8*i+j] = *f++ * (1<<16);

  for (int j=0; j < 8; j++)
  {
    char s[128] = {}, *t=s;
    for (int i=0; i < 8; i++)
      t += sprintf(t, "% 6.2f ", mix.coeff[j*8+i] * (1.0/0x10000));
    CLog::Log(LOGINFO, "%s::%s  %s", CLASSNAME, __func__, s);
  }
  av_freep(&input);
  av_freep(&output);
  swr_free(&m_pContext);
}
  LOGTIME(2);

  // This may be called before Application calls g_RBP.Initialise, so call it here too
  g_RBP.Initialize();

  OMX_ERRORTYPE omx_err   = OMX_ErrorNone;

  if (!m_omx_mixer.Initialize("OMX.broadcom.audio_mixer", OMX_IndexParamAudioInit))
    CLog::Log(LOGERROR, "%s::%s - m_omx_mixer.Initialize omx_err(0x%08x)", CLASSNAME, __func__, omx_err);

  LOGTIME(3);

  if (m_force_resample)
  {
    OMX_PARAM_U32TYPE scaleType;
    OMX_INIT_STRUCTURE(scaleType);

    scaleType.nPortIndex            = m_omx_mixer.GetInputPort();
    scaleType.nU32 = (1 << 16);
    omx_err = m_omx_mixer.SetConfig(OMX_IndexParamBrcmTimeScale, &scaleType);
    if (omx_err != OMX_ErrorNone)
      CLog::Log(LOGERROR, "%s::%s - error m_omx_mixer Failed to set OMX_IndexParamBrcmTimeScale omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
    m_ratio = 1.0;
  }
  // audio_mixer only supports up to 192kHz, however as long as ratio of samplerates remains the same we can lie
  while (srcConfig.sample_rate > 192000 || dstConfig.sample_rate > 192000)
    srcConfig.sample_rate >>= 1, dstConfig.sample_rate >>= 1;

  OMX_INIT_STRUCTURE(m_pcm_input);
  m_pcm_input.nPortIndex            = m_omx_mixer.GetInputPort();
  m_pcm_input.eNumData              = OMX_NumericalDataSigned;
  m_pcm_input.eEndian               = OMX_EndianLittle;
  m_pcm_input.bInterleaved          = OMX_TRUE;
  m_pcm_input.nBitPerSample         = m_src_pitch << 3;
  // 0x8000 = float, 0x10000 = planar
  uint32_t flags = 0;
  if (m_src_fmt == AV_SAMPLE_FMT_FLT || m_src_fmt == AV_SAMPLE_FMT_FLTP)
   flags |= 0x8000;
  if (m_src_fmt >= AV_SAMPLE_FMT_U8P)
   flags |= 0x10000;
  m_pcm_input.ePCMMode              = flags == 0 ? OMX_AUDIO_PCMModeLinear : (OMX_AUDIO_PCMMODETYPE)flags;
  m_pcm_input.nChannels             = srcConfig.channels;
  m_pcm_input.nSamplingRate         = srcConfig.sample_rate;

  omx_err = m_omx_mixer.SetParameter(OMX_IndexParamAudioPcm, &m_pcm_input);
  if (omx_err != OMX_ErrorNone)
    CLog::Log(LOGERROR, "%s::%s - error m_omx_mixer in SetParameter omx_err(0x%08x)", CLASSNAME, __func__, omx_err);

  OMX_INIT_STRUCTURE(m_pcm_output);
  m_pcm_output.nPortIndex            = m_omx_mixer.GetOutputPort();
  m_pcm_output.eNumData              = OMX_NumericalDataSigned;
  m_pcm_output.eEndian               = OMX_EndianLittle;
  m_pcm_output.bInterleaved          = OMX_TRUE;
  m_pcm_output.nBitPerSample         = m_dst_pitch << 3;
  flags = 0;
  if (m_dst_fmt == AV_SAMPLE_FMT_FLT || m_dst_fmt == AV_SAMPLE_FMT_FLTP)
   flags |= 0x8000;
  if (m_dst_fmt >= AV_SAMPLE_FMT_U8P)
   flags |= 0x10000;
  // shift bits if destination format requires it, swr_resamples aligns to the left
  if (m_dst_bits != 32 && (m_dst_dither_bits + m_dst_bits) != 32)
    flags |= (32 - m_dst_bits - m_dst_dither_bits) << 8;

  m_pcm_output.ePCMMode              = flags == 0 ? OMX_AUDIO_PCMModeLinear : (OMX_AUDIO_PCMMODETYPE)flags;
  m_pcm_output.nChannels             = dstConfig.channels;
  m_pcm_output.nSamplingRate         = dstConfig.sample_rate;

  omx_err = m_omx_mixer.SetParameter(OMX_IndexParamAudioPcm, &m_pcm_output);
  if (omx_err != OMX_ErrorNone)
    CLog::Log(LOGERROR, "%s::%s - error m_omx_mixer out SetParameter omx_err(0x%08x)", CLASSNAME, __func__, omx_err);

  LOGTIME(4);

  mix.nPortIndex = m_omx_mixer.GetInputPort();
  omx_err = m_omx_mixer.SetConfig(OMX_IndexConfigBrcmAudioDownmixCoefficients8x8, &mix);
  if (omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "%s::%s - error setting mixer OMX_IndexConfigBrcmAudioDownmixCoefficients, error 0x%08x",
              CLASSNAME, __func__, omx_err);
    return false;
  }

  // set up the number/size of buffers for decoder input
  OMX_PARAM_PORTDEFINITIONTYPE port_param;
  OMX_INIT_STRUCTURE(port_param);
  port_param.nPortIndex = m_omx_mixer.GetInputPort();

  omx_err = m_omx_mixer.GetParameter(OMX_IndexParamPortDefinition, &port_param);
  if (omx_err != OMX_ErrorNone)
    CLog::Log(LOGERROR, "%s:%s - error get OMX_IndexParamPortDefinition (input) omx_err(0x%08x)", CLASSNAME, __func__, omx_err);

  port_param.nBufferCountActual = std::max((unsigned int)port_param.nBufferCountMin, (unsigned int)1);
  port_param.nBufferSize = BUFFERSIZE;

  omx_err = m_omx_mixer.SetParameter(OMX_IndexParamPortDefinition, &port_param);
  if (omx_err != OMX_ErrorNone)
    CLog::Log(LOGERROR, "%s:%s - error set OMX_IndexParamPortDefinition (input) omx_err(0x%08x)", CLASSNAME, __func__, omx_err);

  LOGTIME(5);

  omx_err = m_omx_mixer.AllocInputBuffers();
  if (omx_err != OMX_ErrorNone)
    CLog::Log(LOGERROR, "%s:%s - Error alloc buffers 0x%08x", CLASSNAME, __func__, omx_err);

  LOGTIME(6);

  // set up the number/size of buffers for decoder output
  OMX_INIT_STRUCTURE(port_param);
  port_param.nPortIndex = m_omx_mixer.GetOutputPort();

  omx_err = m_omx_mixer.GetParameter(OMX_IndexParamPortDefinition, &port_param);
  if (omx_err != OMX_ErrorNone)
    CLog::Log(LOGERROR, "%s:%s - error get OMX_IndexParamPortDefinition (input) omx_err(0x%08x)", CLASSNAME, __func__, omx_err);

  port_param.nBufferCountActual = std::max((unsigned int)port_param.nBufferCountMin, (unsigned int)1);
  port_param.nBufferSize = BUFFERSIZE;

  omx_err = m_omx_mixer.SetParameter(OMX_IndexParamPortDefinition, &port_param);
  if (omx_err != OMX_ErrorNone)
    CLog::Log(LOGERROR, "%s:%s - error set OMX_IndexParamPortDefinition (input) omx_err(0x%08x)", CLASSNAME, __func__, omx_err);

  LOGTIME(7);

  omx_err = m_omx_mixer.AllocOutputBuffers();
  if (omx_err != OMX_ErrorNone)
    CLog::Log(LOGERROR, "%s:%s - Error alloc buffers 0x%08x", CLASSNAME, __func__, omx_err);

  LOGTIME(8);

  omx_err = m_omx_mixer.SetStateForComponent(OMX_StateExecuting);
  if (omx_err != OMX_ErrorNone)
    CLog::Log(LOGERROR, "%s:%s - m_omx_mixer OMX_StateExecuting omx_err(0x%08x)", CLASSNAME, __func__, omx_err);

  LOGTIME(9);

  m_Initialized = true;

  return true;
}


static void copy_planes(uint8_t **dst_buffer, int d_pitch, int d_planes, int d_samplesize, int offset, uint8_t *src_buffer, int src_samples, int planesize)
{
  for (int i=0; i < d_planes; i++)
    memcpy(dst_buffer[i] + offset * d_pitch, src_buffer + i * planesize, src_samples * d_samplesize / d_planes);
}

int CActiveAEResamplePi::Resample(uint8_t **dst_buffer, int dst_samples, uint8_t **src_buffer, int src_samples, double ratio)
{
  #ifdef DEBUG_VERBOSE
  CLog::Log(LOGINFO, "%s::%s samples:%d->%d (%.2f)", CLASSNAME, __func__, src_samples, dst_samples, ratio);
  #endif
  if (!m_Initialized)
    return 0;
  OMX_ERRORTYPE omx_err   = OMX_ErrorNone;

  if (m_ratio != 0.0 && ratio != m_ratio)
  {
    OMX_PARAM_U32TYPE scaleType;
    OMX_INIT_STRUCTURE(scaleType);

    scaleType.nPortIndex            = m_omx_mixer.GetInputPort();
    scaleType.nU32 = (1 << 16) / ratio;
    omx_err = m_omx_mixer.SetConfig(OMX_IndexParamBrcmTimeScale, &scaleType);
    if (omx_err != OMX_ErrorNone)
      CLog::Log(LOGERROR, "%s::%s - error m_omx_mixer Failed to set OMX_IndexParamBrcmTimeScale omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
    m_ratio = ratio;
  }

  const int s_planes = m_src_fmt >= AV_SAMPLE_FMT_U8P ? m_src_channels : 1;
  const int d_planes = m_dst_fmt >= AV_SAMPLE_FMT_U8P ? m_dst_channels : 1;
  const int s_chans  = m_src_fmt >= AV_SAMPLE_FMT_U8P ? 1 : m_src_channels;
  const int d_chans  = m_dst_fmt >= AV_SAMPLE_FMT_U8P ? 1 : m_dst_channels;
  const int s_pitch = s_chans * m_src_pitch;
  const int d_pitch = d_chans * m_dst_pitch;

  const int s_samplesize = m_src_channels * m_src_pitch;
  const int d_samplesize = m_dst_channels * m_dst_pitch;
  const int max_src_samples = BUFFERSIZE / s_samplesize;
  const int max_dst_samples = (long long)(BUFFERSIZE / d_samplesize) * m_src_rate / (m_dst_rate + m_src_rate-1);

  int sent = 0;
  int received = 0;

  while (1)
  {
    if (m_encoded_buffer && m_encoded_buffer->nFilledLen)
    {
      int samples_available = m_encoded_buffer->nFilledLen / d_samplesize - m_offset;
      int samples = std::min(samples_available, dst_samples - received);
      copy_planes(dst_buffer, d_pitch, d_planes, d_samplesize, received, (uint8_t *)m_encoded_buffer->pBuffer + m_offset * d_pitch, samples, m_encoded_buffer->nFilledLen / d_planes);
      received += samples;
      m_offset += samples;
      if (m_offset == m_encoded_buffer->nFilledLen / d_samplesize)
      {
        m_offset = 0;
        m_encoded_buffer = NULL;
      }
      else if (m_offset > m_encoded_buffer->nFilledLen / d_samplesize) assert(0);
      else assert(sent == src_samples);
    }

    if (sent >= src_samples)
      break;

    OMX_BUFFERHEADERTYPE *omx_buffer = m_omx_mixer.GetInputBuffer(1000);
    if (omx_buffer == NULL)
    {
      CLog::Log(LOGERROR, "%s::%s m_omx_mixer.GetInputBuffer failed to get buffer", CLASSNAME, __func__);
      return false;
    }
    int send = std::min(std::min(max_dst_samples, max_src_samples), src_samples - sent);

    omx_buffer->nOffset = 0;
    omx_buffer->nFlags = OMX_BUFFERFLAG_EOS;
    omx_buffer->nFilledLen = send * s_samplesize;

    assert(omx_buffer->nFilledLen > 0 && omx_buffer->nFilledLen <= omx_buffer->nAllocLen);

    if (omx_buffer->nFilledLen)
    {
      int planesize = omx_buffer->nFilledLen / s_planes;
      for (int i=0; i < s_planes; i++)
        memcpy((uint8_t *)omx_buffer->pBuffer + i * planesize, src_buffer[i] + sent * s_pitch, planesize);
      sent += send;
    }

    omx_err = m_omx_mixer.EmptyThisBuffer(omx_buffer);
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s OMX_EmptyThisBuffer() failed with result(0x%x)", CLASSNAME, __func__, omx_err);
      m_omx_mixer.DecoderEmptyBufferDone(m_omx_mixer.GetComponent(), omx_buffer);
      return false;
    }

    m_encoded_buffer = m_omx_mixer.GetOutputBuffer();

    if (!m_encoded_buffer)
    {
      CLog::Log(LOGERROR, "%s::%s no output buffer", CLASSNAME, __func__);
      return false;
    }
    omx_err = m_omx_mixer.FillThisBuffer(m_encoded_buffer);
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s m_omx_mixer.FillThisBuffer result(0x%x)", CLASSNAME, __func__, omx_err);
      m_omx_mixer.DecoderFillBufferDone(m_omx_mixer.GetComponent(), m_encoded_buffer);
      return false;
    }
    omx_err = m_omx_mixer.WaitForOutputDone(1000);
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s m_omx_mixer.WaitForOutputDone result(0x%x)", CLASSNAME, __func__, omx_err);
      return false;
    }
    assert(m_encoded_buffer->nFilledLen > 0 && m_encoded_buffer->nFilledLen <= m_encoded_buffer->nAllocLen);

    if (m_omx_mixer.BadState())
    {
      CLog::Log(LOGERROR, "%s::%s m_omx_mixer.BadState", CLASSNAME, __func__);
      return false;
    }
    if (sent < src_samples)
      CLog::Log(LOGERROR, "%s::%s More data to send %d/%d", CLASSNAME, __func__, sent, src_samples);
  }
  #ifdef DEBUG_VERBOSE
  CLog::Log(LOGINFO, "%s::%s format:%d->%d rate:%d->%d chan:%d->%d samples %d->%d (%f) %d", CLASSNAME, __func__,
    (int)m_src_fmt, (int)m_dst_fmt, m_src_rate, m_dst_rate, m_src_channels, m_dst_channels, src_samples, dst_samples, ratio, received);
  #endif
  assert(received <= dst_samples);
  return received;
}

int64_t CActiveAEResamplePi::GetDelay(int64_t base)
{
  int64_t ret = av_rescale_rnd(GetBufferedSamples(), m_dst_rate, base, AV_ROUND_UP);

  #ifdef DEBUG_VERBOSE
  CLog::Log(LOGINFO, "%s::%s = %" PRId64, CLASSNAME, __func__, ret);
  #endif
  return ret;
}

int CActiveAEResamplePi::GetBufferedSamples()
{
  int samples = 0;
  if (m_encoded_buffer)
  {
    const int d_samplesize = m_dst_channels * m_src_pitch;
    samples = m_encoded_buffer->nFilledLen / d_samplesize - m_offset;
  }
  #ifdef DEBUG_VERBOSE
  CLog::Log(LOGINFO, "%s::%s = %d", CLASSNAME, __func__, samples);
  #endif
  return samples;
}

int CActiveAEResamplePi::CalcDstSampleCount(int src_samples, int dst_rate, int src_rate)
{
  int ret = av_rescale_rnd(src_samples, dst_rate, src_rate, AV_ROUND_UP);
  #ifdef DEBUG_VERBOSE
  CLog::Log(LOGINFO, "%s::%s = %d", CLASSNAME, __func__, ret);
  #endif
  return ret;
}

int CActiveAEResamplePi::GetSrcBufferSize(int samples)
{
  int ret = av_samples_get_buffer_size(NULL, m_src_channels, samples, m_src_fmt, 1);
  #ifdef DEBUG_VERBOSE
  CLog::Log(LOGINFO, "%s::%s = %d", CLASSNAME, __func__, ret);
  #endif
  return ret;
}

int CActiveAEResamplePi::GetDstBufferSize(int samples)
{
  int ret = av_samples_get_buffer_size(NULL, m_dst_channels, samples, m_dst_fmt, 1);
  #ifdef DEBUG_VERBOSE
  CLog::Log(LOGINFO, "%s::%s = %d", CLASSNAME, __func__, ret);
  #endif
  return ret;
}
