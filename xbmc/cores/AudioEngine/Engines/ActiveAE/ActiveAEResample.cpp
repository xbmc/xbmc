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

#include "ActiveAEResample.h"

using namespace ActiveAE;

CActiveAEResample::CActiveAEResample()
{
  m_pContext = NULL;
}

CActiveAEResample::~CActiveAEResample()
{
  if (m_pContext)
    m_dllSwResample.swr_free(&m_pContext);

  m_dllAvUtil.Unload();
  m_dllSwResample.Unload();
}

bool CActiveAEResample::Init(uint64_t dst_chan_layout, int dst_channels, int dst_rate, AVSampleFormat dst_fmt, uint64_t src_chan_layout, int src_channels, int src_rate, AVSampleFormat src_fmt, CAEChannelInfo *remapLayout)
{
  if (!m_dllAvUtil.Load() || !m_dllSwResample.Load())
    return false;

  m_dst_chan_layout = dst_chan_layout;
  m_dst_channels = dst_channels;
  m_dst_rate = dst_rate;
  m_dst_fmt = dst_fmt;
  m_src_chan_layout = src_chan_layout;
  m_src_channels = src_channels;
  m_src_rate = src_rate;
  m_src_fmt = src_fmt;

  if (m_dst_chan_layout == 0)
    m_dst_chan_layout = m_dllAvUtil.av_get_default_channel_layout(m_dst_channels);
  if (m_src_chan_layout == 0)
    m_src_chan_layout = m_dllAvUtil.av_get_default_channel_layout(m_src_channels);

  m_pContext = m_dllSwResample.swr_alloc_set_opts(NULL, m_dst_chan_layout, m_dst_fmt, m_dst_rate,
                                                        m_src_chan_layout, m_src_fmt, m_src_rate,
                                                        0, NULL);

  if(!m_pContext)
  {
    CLog::Log(LOGERROR, "CActiveAEResample::Init - create context failed");
    return false;
  }
  if (remapLayout)
  {
    // one-to-one mapping of channels
    // remapLayout is the layout of the sink, if the channel is in our src layout
    // the channel is mapped by setting coef 1.0
    memset(m_rematrix, 0, sizeof(m_rematrix));
    for (unsigned int out=0; out<remapLayout->Count(); out++)
    {
      int idx = GetAVChannelIndex((*remapLayout)[out], m_src_chan_layout);
      if (idx >= 0)
      {
        m_rematrix[out][idx] = 1.0;
      }
    }

    if (m_dllSwResample.swr_set_matrix(m_pContext, (const double*)m_rematrix, AE_CH_MAX) < 0)
    {
      CLog::Log(LOGERROR, "CActiveAEResample::Init - setting channel matrix failed");
      return false;
    }
  }
  if(m_dllSwResample.swr_init(m_pContext) < 0)
  {
    CLog::Log(LOGERROR, "CActiveAEResample::Init - init resampler failed");
    return false;
  }
  return true;
}

int CActiveAEResample::Resample(uint8_t **dst_buffer, int dst_samples, uint8_t **src_buffer, int src_samples)
{
  int ret = m_dllSwResample.swr_convert(m_pContext, dst_buffer, dst_samples, (const uint8_t**)src_buffer, src_samples);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CActiveAEResample::Resample - resample failed");
    return 0;
  }
  return ret;
}

int64_t CActiveAEResample::GetDelay(int64_t base)
{
  return m_dllSwResample.swr_get_delay(m_pContext, base);
}

int CActiveAEResample::GetBufferedSamples()
{
  return m_dllAvUtil.av_rescale_rnd(m_dllSwResample.swr_get_delay(m_pContext, m_src_rate),
                                    m_dst_rate, m_src_rate, AV_ROUND_UP);
}

int CActiveAEResample::CalcDstSampleCount(int src_samples, int dst_rate, int src_rate)
{
  return m_dllAvUtil.av_rescale_rnd(src_samples, dst_rate, src_rate, AV_ROUND_UP);
}

int CActiveAEResample::GetSrcBufferSize(int samples)
{
  return m_dllAvUtil.av_samples_get_buffer_size(NULL, m_src_channels, samples, m_src_fmt, 1);
}

int CActiveAEResample::GetDstBufferSize(int samples)
{
  return m_dllAvUtil.av_samples_get_buffer_size(NULL, m_dst_channels, samples, m_dst_fmt, 1);
}

uint64_t CActiveAEResample::GetAVChannelLayout(CAEChannelInfo &info)
{
  uint64_t channelLayout = 0;
  if (info.HasChannel(AE_CH_FL))   channelLayout |= AV_CH_FRONT_LEFT;
  if (info.HasChannel(AE_CH_FR))   channelLayout |= AV_CH_FRONT_RIGHT;
  if (info.HasChannel(AE_CH_FC))   channelLayout |= AV_CH_FRONT_CENTER;
  if (info.HasChannel(AE_CH_LFE))  channelLayout |= AV_CH_LOW_FREQUENCY;
  if (info.HasChannel(AE_CH_BL))   channelLayout |= AV_CH_BACK_LEFT;
  if (info.HasChannel(AE_CH_BR))   channelLayout |= AV_CH_BACK_RIGHT;
  if (info.HasChannel(AE_CH_FLOC)) channelLayout |= AV_CH_FRONT_LEFT_OF_CENTER;
  if (info.HasChannel(AE_CH_FROC)) channelLayout |= AV_CH_FRONT_RIGHT_OF_CENTER;
  if (info.HasChannel(AE_CH_BC))   channelLayout |= AV_CH_BACK_CENTER;
  if (info.HasChannel(AE_CH_SL))   channelLayout |= AV_CH_SIDE_LEFT;
  if (info.HasChannel(AE_CH_SR))   channelLayout |= AV_CH_SIDE_RIGHT;
  if (info.HasChannel(AE_CH_TC))   channelLayout |= AV_CH_TOP_CENTER;
  if (info.HasChannel(AE_CH_TFL))  channelLayout |= AV_CH_TOP_FRONT_LEFT;
  if (info.HasChannel(AE_CH_TFC))  channelLayout |= AV_CH_TOP_FRONT_CENTER;
  if (info.HasChannel(AE_CH_TFR))  channelLayout |= AV_CH_TOP_FRONT_RIGHT;
  if (info.HasChannel(AE_CH_TBL))   channelLayout |= AV_CH_TOP_BACK_LEFT;
  if (info.HasChannel(AE_CH_TBC))   channelLayout |= AV_CH_TOP_BACK_CENTER;
  if (info.HasChannel(AE_CH_TBR))   channelLayout |= AV_CH_TOP_BACK_RIGHT;

  return channelLayout;
}

//CAEChannelInfo CActiveAEResample::GetAEChannelLayout(uint64_t layout)
//{
//  CAEChannelInfo channelLayout;
//  channelLayout.Reset();
//
//  if (layout & AV_CH_FRONT_LEFT           ) channelLayout += AE_CH_FL  ;
//  if (layout & AV_CH_FRONT_RIGHT          ) channelLayout += AE_CH_FR  ;
//  if (layout & AV_CH_FRONT_CENTER         ) channelLayout += AE_CH_FC  ;
//  if (layout & AV_CH_LOW_FREQUENCY        ) channelLayout += AE_CH_LFE ;
//  if (layout & AV_CH_BACK_LEFT            ) channelLayout += AE_CH_BL  ;
//  if (layout & AV_CH_BACK_RIGHT           ) channelLayout += AE_CH_BR  ;
//  if (layout & AV_CH_FRONT_LEFT_OF_CENTER ) channelLayout += AE_CH_FLOC;
//  if (layout & AV_CH_FRONT_RIGHT_OF_CENTER) channelLayout += AE_CH_FROC;
//  if (layout & AV_CH_BACK_CENTER          ) channelLayout += AE_CH_BC  ;
//  if (layout & AV_CH_SIDE_LEFT            ) channelLayout += AE_CH_SL  ;
//  if (layout & AV_CH_SIDE_RIGHT           ) channelLayout += AE_CH_SR  ;
//  if (layout & AV_CH_TOP_CENTER           ) channelLayout += AE_CH_TC  ;
//  if (layout & AV_CH_TOP_FRONT_LEFT       ) channelLayout += AE_CH_TFL ;
//  if (layout & AV_CH_TOP_FRONT_CENTER     ) channelLayout += AE_CH_TFC ;
//  if (layout & AV_CH_TOP_FRONT_RIGHT      ) channelLayout += AE_CH_TFR ;
//  if (layout & AV_CH_TOP_BACK_LEFT        ) channelLayout += AE_CH_BL  ;
//  if (layout & AV_CH_TOP_BACK_CENTER      ) channelLayout += AE_CH_BC  ;
//  if (layout & AV_CH_TOP_BACK_RIGHT       ) channelLayout += AE_CH_BR  ;
//
//  return channelLayout;
//}

AVSampleFormat CActiveAEResample::GetAVSampleFormat(AEDataFormat format)
{
  if      (format == AE_FMT_U8)     return AV_SAMPLE_FMT_U8;
  else if (format == AE_FMT_S16NE)  return AV_SAMPLE_FMT_S16;
  else if (format == AE_FMT_S32NE)  return AV_SAMPLE_FMT_S32;
  else if (format == AE_FMT_FLOAT)  return AV_SAMPLE_FMT_FLT;
  else if (format == AE_FMT_DOUBLE) return AV_SAMPLE_FMT_DBL;

  else if (format == AE_FMT_U8P)     return AV_SAMPLE_FMT_U8P;
  else if (format == AE_FMT_S16NEP)  return AV_SAMPLE_FMT_S16P;
  else if (format == AE_FMT_S32NEP)  return AV_SAMPLE_FMT_S32P;
  else if (format == AE_FMT_FLOATP)  return AV_SAMPLE_FMT_FLTP;
  else if (format == AE_FMT_DOUBLEP) return AV_SAMPLE_FMT_DBLP;

  return AV_SAMPLE_FMT_FLT;
}

AEDataFormat CActiveAEResample::GetAESampleFormat(AVSampleFormat format)
{
  if      (format == AV_SAMPLE_FMT_U8)   return AE_FMT_U8;
  else if (format == AV_SAMPLE_FMT_S16)  return AE_FMT_S16NE;
  else if (format == AV_SAMPLE_FMT_S32)  return AE_FMT_S32NE;
  else if (format == AV_SAMPLE_FMT_FLT)  return AE_FMT_FLOAT;
  else if (format == AV_SAMPLE_FMT_DBL)  return AE_FMT_DOUBLE;

  else if (format == AV_SAMPLE_FMT_U8P)   return AE_FMT_U8P;
  else if (format == AV_SAMPLE_FMT_S16P)  return AE_FMT_S16NEP;
  else if (format == AV_SAMPLE_FMT_S32P)  return AE_FMT_S32NEP;
  else if (format == AV_SAMPLE_FMT_FLTP)  return AE_FMT_FLOATP;
  else if (format == AV_SAMPLE_FMT_DBLP)  return AE_FMT_DOUBLEP;

  CLog::Log(LOGERROR, "CActiveAEResample::GetAESampleFormat - format not supported");
  return AE_FMT_INVALID;
}

uint64_t CActiveAEResample::GetAVChannel(enum AEChannel aechannel)
{
  switch (aechannel)
  {
  case AE_CH_FL:   return AV_CH_FRONT_LEFT;
  case AE_CH_FR:   return AV_CH_FRONT_RIGHT;
  case AE_CH_FC:   return AV_CH_FRONT_CENTER;
  case AE_CH_LFE:  return AV_CH_LOW_FREQUENCY;
  case AE_CH_BL:   return AV_CH_BACK_LEFT;
  case AE_CH_BR:   return AV_CH_BACK_RIGHT;
  case AE_CH_FLOC: return AV_CH_FRONT_LEFT_OF_CENTER;
  case AE_CH_FROC: return AV_CH_FRONT_RIGHT_OF_CENTER;
  case AE_CH_BC:   return AV_CH_BACK_CENTER;
  case AE_CH_SL:   return AV_CH_SIDE_LEFT;
  case AE_CH_SR:   return AV_CH_SIDE_RIGHT;
  case AE_CH_TC:   return AV_CH_TOP_CENTER;
  case AE_CH_TFL:  return AV_CH_TOP_FRONT_LEFT;
  case AE_CH_TFC:  return AV_CH_TOP_FRONT_CENTER;
  case AE_CH_TFR:  return AV_CH_TOP_FRONT_RIGHT;
  case AE_CH_TBL:  return AV_CH_TOP_BACK_LEFT;
  case AE_CH_TBC:  return AV_CH_TOP_BACK_CENTER;
  case AE_CH_TBR:  return AV_CH_TOP_BACK_RIGHT;
  default:
    return 0;
  }
}

int CActiveAEResample::GetAVChannelIndex(enum AEChannel aechannel, uint64_t layout)
{
  return m_dllAvUtil.av_get_channel_layout_channel_index(layout, GetAVChannel(aechannel));
}
