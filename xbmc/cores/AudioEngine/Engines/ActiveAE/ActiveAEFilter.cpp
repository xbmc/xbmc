/*
 *      Copyright (C) 2010-2016 Team Kodi
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

#include "ActiveAEFilter.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include <algorithm>

extern "C" {
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libswresample/swresample.h"
}

using namespace ActiveAE;

CActiveAEFilter::CActiveAEFilter()
{
  m_pFilterGraph = nullptr;
  m_pFilterCtxIn = nullptr;
  m_pFilterCtxOut = nullptr;
  m_pOutFrame = nullptr;
  m_pConvertCtx = nullptr;
  m_pConvertFrame = nullptr;
  m_needConvert = false;
}

CActiveAEFilter::~CActiveAEFilter()
{
  CloseFilter();
}

void CActiveAEFilter::Init(AVSampleFormat fmt, int sampleRate, uint64_t channelLayout)
{
  m_sampleFormat = fmt;
  m_sampleRate = sampleRate;
  m_channelLayout = channelLayout;
  m_tempo = 1.0;
  m_bufferedSamples = 0;
}

bool CActiveAEFilter::SetTempo(float tempo)
{
  m_tempo = tempo;
  if (m_tempo == 1.0)
  {
    CloseFilter();
    return true;
  }

  if (!CreateFilterGraph())
    return false;

  if (!CreateAtempoFilter())
  {
    CloseFilter();
    return false;
  }

  m_bufferedSamples = 0;
  return true;
}

bool CActiveAEFilter::CreateFilterGraph()
{
  CloseFilter();

  m_pFilterGraph = avfilter_graph_alloc();
  if (!m_pFilterGraph)
  {
    CLog::Log(LOGERROR, "CActiveAEFilter::CreateFilterGraph - unable to alloc filter graph");
    return false;
  }

  AVFilter* srcFilter = avfilter_get_by_name("abuffer");
  AVFilter* outFilter = avfilter_get_by_name("abuffersink");

  std::string args = StringUtils::Format("time_base=1/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
                                         m_sampleRate,
                                         m_sampleRate,
                                         av_get_sample_fmt_name(m_sampleFormat),
                                         m_channelLayout);

  int ret = avfilter_graph_create_filter(&m_pFilterCtxIn, srcFilter, "in", args.c_str(), NULL, m_pFilterGraph);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CActiveAEFilter::CreateFilterGraph - avfilter_graph_create_filter: src");
    return false;
  }

  ret = avfilter_graph_create_filter(&m_pFilterCtxOut, outFilter, "out", NULL, NULL, m_pFilterGraph);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CActiveAEFilter::CreateFilterGraph - avfilter_graph_create_filter: out");
    return false;
  }

  m_pOutFrame = av_frame_alloc();

  return true;
}

bool CActiveAEFilter::CreateAtempoFilter()
{
  AVFilter *atempo;

  atempo = avfilter_get_by_name("atempo");
  m_pFilterCtxAtempo = avfilter_graph_alloc_filter(m_pFilterGraph, atempo, "atempo");
  std::string args =  StringUtils::Format("tempo=%f", m_tempo);
  int ret = avfilter_init_str(m_pFilterCtxAtempo, args.c_str());

  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CActiveAEFilter::CreateAtempoFilter - avfilter_init_str failed");
    return false;
  }

  ret = avfilter_link(m_pFilterCtxIn, 0, m_pFilterCtxAtempo, 0);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CActiveAEFilter::CreateAtempoFilter - avfilter_link failed for in filter");
    return false;
  }

  ret = avfilter_link(m_pFilterCtxAtempo, 0, m_pFilterCtxOut, 0);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CActiveAEFilter::CreateAtempoFilter - avfilter_link failed for out filter");
    return false;
  }

  ret = avfilter_graph_config(m_pFilterGraph, NULL);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CActiveAEFilter::CreateAtempoFilter - avfilter_graph_config failed");
    return false;
  }

  m_needConvert = false;
  if (m_pFilterCtxAtempo->outputs[0]->format != m_sampleFormat)
  {
    m_needConvert = true;
    m_pConvertCtx = swr_alloc();
    m_pConvertFrame = av_frame_alloc();
  }

  m_hasData = false;
  m_needData = true;
  m_filterEof = false;

  return true;
}

void CActiveAEFilter::CloseFilter()
{
  if (m_pFilterGraph)
  {
    avfilter_graph_free(&m_pFilterGraph);

    m_pFilterCtxIn = nullptr;
    m_pFilterCtxOut = nullptr;
  }

  if (m_pOutFrame)
    av_frame_free(&m_pOutFrame);

  if (m_pConvertFrame)
    av_frame_free(&m_pConvertFrame);

  if (m_pConvertCtx)
    swr_free(&m_pConvertCtx);

  m_bufferedSamples = 0;
}

int CActiveAEFilter::ProcessFilter(uint8_t **dst_buffer, int dst_samples, uint8_t **src_buffer, int src_samples, int src_bufsize)
{
  int result;

  if (src_samples)
  {
    m_bufferedSamples += src_samples;

    AVFrame *frame = av_frame_alloc();
    if (!frame)
      return -1;

    int channels = av_get_channel_layout_nb_channels(m_channelLayout);

    av_frame_set_channel_layout(frame, m_channelLayout);
    av_frame_set_channels(frame, channels);
    av_frame_set_sample_rate(frame, m_sampleRate);
    frame->nb_samples = src_samples;
    frame->format = m_sampleFormat;

    result = avcodec_fill_audio_frame(frame, channels, m_sampleFormat,
                             src_buffer[0], src_bufsize, 16);
    if (result < 0)
    {
      CLog::Log(LOGERROR, "CActiveAEFilter::ProcessFilter - avcodec_fill_audio_frame failed");
      return -1;
    }

    result = av_buffersrc_write_frame(m_pFilterCtxIn, frame);
    av_frame_free(&frame);
    if (result < 0)
    {
      CLog::Log(LOGERROR, "CActiveAEFilter::ProcessFilter - av_buffersrc_add_frame failed");
      return -1;
    }
  }
  else if (!m_filterEof && m_needData)
  {
    result = av_buffersrc_write_frame(m_pFilterCtxIn, nullptr);
    if (result < 0)
    {
      CLog::Log(LOGERROR, "CActiveAEFilter::ProcessFilter - av_buffersrc_add_frame");
      return -1;
    }
  }

  if (!m_hasData)
  {
    m_needData = false;
    AVFrame *outFrame = m_needConvert ? m_pConvertFrame : m_pOutFrame;

    result = av_buffersink_get_frame(m_pFilterCtxOut, outFrame);

    if (result  == AVERROR(EAGAIN))
    {
      m_needData = true;
      return 0;
    }
    else if (result == AVERROR_EOF)
    {
      result = av_buffersink_get_frame(m_pFilterCtxOut, outFrame);
      m_filterEof = true;
      if (result < 0)
        return 0;
    }
    else if (result < 0)
    {
      CLog::Log(LOGERROR, "CActiveAEFilter::ProcessFilter - av_buffersink_get_frame");
      return -1;
    }

    if (m_needConvert)
    {
      av_frame_unref(m_pOutFrame);
      m_pOutFrame->format = m_sampleFormat;
      av_frame_set_channel_layout(m_pOutFrame, m_channelLayout);
      av_frame_set_sample_rate(m_pOutFrame, m_sampleRate);
      result = swr_convert_frame(m_pConvertCtx, m_pOutFrame, m_pConvertFrame);
      if (result < 0)
      {
        CLog::Log(LOGERROR, "CActiveAEFilter::ProcessFilter - swr_convert_frame failed");
        return -1;
      }
    }

    m_hasData = true;
    m_sampleOffset = 0;
  }

  if (m_hasData)
  {
    int channels = av_get_channel_layout_nb_channels(m_channelLayout);
    int planes = av_sample_fmt_is_planar(m_sampleFormat) ? channels : 1;
    int samples = std::min(dst_samples, m_pOutFrame->nb_samples - m_sampleOffset);
    int bytes = samples * av_get_bytes_per_sample(m_sampleFormat) * channels / planes;
    int bytesOffset = m_sampleOffset * av_get_bytes_per_sample(m_sampleFormat) * channels / planes;
    for (int i=0; i<planes; i++)
    {
      memcpy(dst_buffer[i], m_pOutFrame->extended_data[i] + bytesOffset, bytes);
    }
    m_sampleOffset += samples;

    if (m_sampleOffset >= m_pOutFrame->nb_samples)
    {
      av_frame_unref(m_pOutFrame);
      m_hasData = false;
    }

    m_bufferedSamples -= samples * m_tempo;
    if (m_bufferedSamples < 0)
      m_bufferedSamples = 0;
    return samples;
  }

  return 0;
}

bool CActiveAEFilter::IsEof()
{
  return m_filterEof;
}

bool CActiveAEFilter::NeedData()
{
  return m_needData;
}

bool CActiveAEFilter::IsActive()
{
  if (m_pFilterGraph)
    return true;
  else
    return false;
}

int CActiveAEFilter::GetBufferedSamples()
{
  return m_bufferedSamples;
}
