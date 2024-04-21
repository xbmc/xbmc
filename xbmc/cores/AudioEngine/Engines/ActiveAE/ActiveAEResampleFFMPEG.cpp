/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Utils/AEUtil.h"
#include "ActiveAEResampleFFMPEG.h"
#include "utils/log.h"

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

using namespace ActiveAE;

CActiveAEResampleFFMPEG::CActiveAEResampleFFMPEG()
{
  m_pContext = NULL;
  m_doesResample = false;
}

CActiveAEResampleFFMPEG::~CActiveAEResampleFFMPEG()
{
  swr_free(&m_pContext);
}

bool CActiveAEResampleFFMPEG::Init(SampleConfig dstConfig, SampleConfig srcConfig, bool upmix, bool normalize, double centerMix,
                                   CAEChannelInfo *remapLayout, AEQuality quality, bool force_resample)
{
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

  if (m_src_rate != m_dst_rate)
    m_doesResample = true;

  if (m_dst_chan_layout == 0)
  {
    AVChannelLayout layout = {};
    av_channel_layout_default(&layout, m_dst_channels);
    m_dst_chan_layout = layout.u.mask;
    av_channel_layout_uninit(&layout);
  }
  if (m_src_chan_layout == 0)
  {
    AVChannelLayout layout = {};
    av_channel_layout_default(&layout, m_src_channels);
    m_src_chan_layout = layout.u.mask;
    av_channel_layout_uninit(&layout);
  }

  AVChannelLayout dstChLayout = {};
  AVChannelLayout srcChLayout = {};

  av_channel_layout_from_mask(&dstChLayout, m_dst_chan_layout);
  av_channel_layout_from_mask(&srcChLayout, m_src_chan_layout);

  int ret = swr_alloc_set_opts2(&m_pContext, &dstChLayout, m_dst_fmt, m_dst_rate, &srcChLayout,
                                m_src_fmt, m_src_rate, 0, NULL);

  if (ret)
  {
    CLog::Log(LOGERROR, "CActiveAEResampleFFMPEG::Init - create context failed");
    return false;
  }

  if(quality == AE_QUALITY_HIGH)
  {
    av_opt_set_double(m_pContext, "cutoff", 1.0, 0);
    av_opt_set_int(m_pContext,"filter_size", 256, 0);
  }
  else if(quality == AE_QUALITY_MID)
  {
    // 0.97 is default cutoff so use (1.0 - 0.97) / 2.0 + 0.97
    av_opt_set_double(m_pContext, "cutoff", 0.985, 0);
    av_opt_set_int(m_pContext,"filter_size", 64, 0);
  }
  else if(quality == AE_QUALITY_LOW)
  {
    av_opt_set_double(m_pContext, "cutoff", 0.97, 0);
    av_opt_set_int(m_pContext,"filter_size", 32, 0);
  }

  if (m_dst_fmt == AV_SAMPLE_FMT_S32 || m_dst_fmt == AV_SAMPLE_FMT_S32P)
  {
    av_opt_set_int(m_pContext, "output_sample_bits", m_dst_bits, 0);
  }

  // tell resampler to clamp float values
  // not required for sink stage (remapLayout == true)
  if ((m_dst_fmt == AV_SAMPLE_FMT_FLT || m_dst_fmt == AV_SAMPLE_FMT_FLTP) &&
      (m_src_fmt == AV_SAMPLE_FMT_FLT || m_src_fmt == AV_SAMPLE_FMT_FLTP) &&
      !remapLayout && normalize)
  {
     av_opt_set_double(m_pContext, "rematrix_maxval", 1.0, 0);
  }

  av_opt_set_double(m_pContext, "center_mix_level", centerMix, 0);

  if (remapLayout)
  {
    // one-to-one mapping of channels
    // remapLayout is the layout of the sink, if the channel is in our src layout
    // the channel is mapped by setting coef 1.0
    memset(m_rematrix, 0, sizeof(m_rematrix));
    m_dst_chan_layout = 0;
    for (unsigned int out=0; out<remapLayout->Count(); out++)
    {
      m_dst_chan_layout += ((uint64_t)1) << out;
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
      CLog::Log(LOGERROR, "CActiveAEResampleFFMPEG::Init - setting channel matrix failed");
      return false;
    }
  }
  // stereo upmix
  else if (upmix && m_src_channels == 2 && m_dst_channels > 2)
  {
    memset(m_rematrix, 0, sizeof(m_rematrix));
    av_channel_layout_uninit(&dstChLayout);
    av_channel_layout_from_mask(&dstChLayout, m_dst_chan_layout);
    for (int out=0; out<m_dst_channels; out++)
    {
      AVChannel outChan = av_channel_layout_channel_from_index(&dstChLayout, out);
      switch (outChan)
      {
        case AV_CHAN_FRONT_LEFT:
        case AV_CHAN_BACK_LEFT:
        case AV_CHAN_SIDE_LEFT:
          m_rematrix[out][0] = 1.0;
          break;
        case AV_CHAN_FRONT_RIGHT:
        case AV_CHAN_BACK_RIGHT:
        case AV_CHAN_SIDE_RIGHT:
          m_rematrix[out][1] = 1.0;
          break;
        case AV_CHAN_FRONT_CENTER:
          m_rematrix[out][0] = 0.5;
          m_rematrix[out][1] = 0.5;
          break;
        case AV_CHAN_LOW_FREQUENCY:
          m_rematrix[out][0] = 0.5;
          m_rematrix[out][1] = 0.5;
          break;
        default:
          break;
      }
    }

    av_channel_layout_uninit(&dstChLayout);

    if (swr_set_matrix(m_pContext, (const double*)m_rematrix, AE_CH_MAX) < 0)
    {
      CLog::Log(LOGERROR, "CActiveAEResampleFFMPEG::Init - setting channel matrix failed");
      return false;
    }
  }

  if(swr_init(m_pContext) < 0)
  {
    CLog::Log(LOGERROR, "CActiveAEResampleFFMPEG::Init - init resampler failed");
    return false;
  }
  return true;
}

int CActiveAEResampleFFMPEG::Resample(uint8_t **dst_buffer, int dst_samples, uint8_t **src_buffer, int src_samples, double ratio)
{
  int delta = 0;
  int distance = 0;
  if (ratio != 1.0)
  {
    delta = (src_samples*ratio-src_samples)*m_dst_rate/m_src_rate;
    distance = src_samples*m_dst_rate/m_src_rate;
    m_doesResample = true;
  }

  if (m_doesResample)
  {
    if (swr_set_compensation(m_pContext, delta, distance) < 0)
    {
      CLog::Log(LOGERROR, "CActiveAEResampleFFMPEG::Resample - set compensation failed");
      return -1;
    }
  }

  //! @bug libavresample isn't const correct
  int ret = swr_convert(m_pContext, dst_buffer, dst_samples, const_cast<const uint8_t**>(src_buffer), src_samples);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CActiveAEResampleFFMPEG::Resample - resample failed");
    return -1;
  }

  // special handling for S24 formats which are carried in S32
  if (m_dst_fmt == AV_SAMPLE_FMT_S32 || m_dst_fmt == AV_SAMPLE_FMT_S32P)
  {
    // S24NE3
    if (m_dst_bits == 24 && m_dst_dither_bits == -8)
    {
      int planes = av_sample_fmt_is_planar(m_dst_fmt) ? m_dst_channels : 1;
      int samples = ret * m_dst_channels / planes;
      uint8_t *src, *dst;
      for (int i=0; i<planes; i++)
      {
        src = dst = dst_buffer[i];
        for (int j=0; j<samples; j++)
        {
#ifndef WORDS_BIGENDIAN
          src++;
#endif
          *dst++ = *src++;
          *dst++ = *src++;
          *dst++ = *src++;
#ifdef WORDS_BIGENDIAN
          src++;
#endif
        }
      }
    }
    // shift bits if destination format requires it, swr_resamples aligns to the left
    // Example:
    // ALSA uses SNE24NE that means 24 bit load in 32 bit package and 0 dither bits
    // WASAPI uses SNE24NEMSB which is 24 bit load in 32 bit package and 8 dither bits
    // dither bits are always assumed from the right
    // FFmpeg internally calculates with S24NEMSB which means, that we need to shift the
    // data 8 bits to the right in order to get the correct alignment of 0 dither bits
    // if we want to use ALSA as output. For WASAPI nothing had to be done.
    // SNE24NEMSB 1 1 1 0 >> 8 = 0 1 1 1 = SNE24NE
    else if (m_dst_bits != 32 && (m_dst_dither_bits + m_dst_bits) != 32)
    {
      int planes = av_sample_fmt_is_planar(m_dst_fmt) ? m_dst_channels : 1;
      int samples = ret * m_dst_channels / planes;
      for (int i=0; i<planes; i++)
      {
        uint32_t* buf = (uint32_t*)dst_buffer[i];
        for (int j=0; j<samples; j++)
        {
          *buf = *buf >> (32 - m_dst_bits - m_dst_dither_bits);
          buf++;
        }
      }
    }
  }
  return ret;
}

int64_t CActiveAEResampleFFMPEG::GetDelay(int64_t base)
{
  return swr_get_delay(m_pContext, base);
}

int CActiveAEResampleFFMPEG::GetBufferedSamples()
{
  return av_rescale_rnd(swr_get_delay(m_pContext, m_src_rate),
                                    m_dst_rate, m_src_rate, AV_ROUND_UP);
}

int CActiveAEResampleFFMPEG::CalcDstSampleCount(int src_samples, int dst_rate, int src_rate)
{
  return av_rescale_rnd(src_samples, dst_rate, src_rate, AV_ROUND_UP);
}

int CActiveAEResampleFFMPEG::GetSrcBufferSize(int samples)
{
  return av_samples_get_buffer_size(NULL, m_src_channels, samples, m_src_fmt, 1);
}

int CActiveAEResampleFFMPEG::GetDstBufferSize(int samples)
{
  return av_samples_get_buffer_size(NULL, m_dst_channels, samples, m_dst_fmt, 1);
}
