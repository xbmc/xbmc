/*
 *      Copyright (C) 2005-2017 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "VideoBackgroundDecoder.h"
#include "utils/log.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
}

CVideoBackgroundDecoder::CVideoBackgroundDecoder()
  : m_fmtCtx(nullptr)
  , m_codecCtx(nullptr)
  , m_swsCtx(nullptr)
  , m_avFrame(nullptr)
  , m_rgbFrame(nullptr)
  , m_rgbBuffer(nullptr)
  , m_videoStream(-1)
  , m_width(0)
  , m_height(0)
  , m_timeBase(0.0)
  , m_nextFrameMs(0)
  , m_isOpen(false)
{
}

CVideoBackgroundDecoder::~CVideoBackgroundDecoder()
{
  Close();
}

bool CVideoBackgroundDecoder::Open(const std::string& filename)
{
  Close();

  if (avformat_open_input(&m_fmtCtx, filename.c_str(), nullptr, nullptr) < 0)
  {
    CLog::Log(LOGERROR, "CVideoBackgroundDecoder: Could not open file %s", filename.c_str());
    return false;
  }

  if (avformat_find_stream_info(m_fmtCtx, nullptr) < 0)
  {
    CLog::Log(LOGERROR, "CVideoBackgroundDecoder: Could not find stream info for %s", filename.c_str());
    avformat_close_input(&m_fmtCtx);
    return false;
  }

  m_videoStream = -1;
  for (unsigned int i = 0; i < m_fmtCtx->nb_streams; i++)
  {
    if (m_fmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      m_videoStream = (int)i;
      break;
    }
  }

  if (m_videoStream < 0)
  {
    CLog::Log(LOGERROR, "CVideoBackgroundDecoder: No video stream found in %s", filename.c_str());
    avformat_close_input(&m_fmtCtx);
    return false;
  }

  m_codecCtx = m_fmtCtx->streams[m_videoStream]->codec;
  AVCodec* codec = avcodec_find_decoder(m_codecCtx->codec_id);
  if (!codec)
  {
    CLog::Log(LOGERROR, "CVideoBackgroundDecoder: Could not find decoder for codec %d", m_codecCtx->codec_id);
    avformat_close_input(&m_fmtCtx);
    m_codecCtx = nullptr;
    return false;
  }

  if (avcodec_open2(m_codecCtx, codec, nullptr) < 0)
  {
    CLog::Log(LOGERROR, "CVideoBackgroundDecoder: Could not open codec");
    avformat_close_input(&m_fmtCtx);
    m_codecCtx = nullptr;
    return false;
  }

  m_width = m_codecCtx->width;
  m_height = m_codecCtx->height;

  m_avFrame = av_frame_alloc();
  m_rgbFrame = av_frame_alloc();
  if (!m_avFrame || !m_rgbFrame)
  {
    CLog::Log(LOGERROR, "CVideoBackgroundDecoder: Could not allocate frames");
    Close();
    return false;
  }

  int bufSize = avpicture_get_size(AV_PIX_FMT_BGRA, m_width, m_height);
  m_rgbBuffer = (uint8_t*)av_malloc(bufSize);
  if (!m_rgbBuffer)
  {
    CLog::Log(LOGERROR, "CVideoBackgroundDecoder: Could not allocate RGB buffer");
    Close();
    return false;
  }

  avpicture_fill((AVPicture*)m_rgbFrame, m_rgbBuffer, AV_PIX_FMT_BGRA, m_width, m_height);

  m_swsCtx = sws_getContext(m_width, m_height, m_codecCtx->pix_fmt,
                            m_width, m_height, AV_PIX_FMT_BGRA,
                            SWS_BILINEAR, nullptr, nullptr, nullptr);
  if (!m_swsCtx)
  {
    CLog::Log(LOGERROR, "CVideoBackgroundDecoder: Could not create sws context");
    Close();
    return false;
  }

  m_timeBase = av_q2d(m_fmtCtx->streams[m_videoStream]->time_base);
  m_nextFrameMs = 0;
  m_isOpen = true;

  CLog::Log(LOGDEBUG, "CVideoBackgroundDecoder: Opened %s (%dx%d)", filename.c_str(), m_width, m_height);
  return true;
}

void CVideoBackgroundDecoder::Close()
{
  if (m_swsCtx)
  {
    sws_freeContext(m_swsCtx);
    m_swsCtx = nullptr;
  }

  if (m_avFrame)
  {
    av_frame_free(&m_avFrame);
    m_avFrame = nullptr;
  }

  if (m_rgbFrame)
  {
    av_frame_free(&m_rgbFrame);
    m_rgbFrame = nullptr;
  }

  if (m_rgbBuffer)
  {
    av_free(m_rgbBuffer);
    m_rgbBuffer = nullptr;
  }

  if (m_codecCtx)
  {
    avcodec_close(m_codecCtx);
    m_codecCtx = nullptr;
  }

  if (m_fmtCtx)
  {
    avformat_close_input(&m_fmtCtx);
    m_fmtCtx = nullptr;
  }

  m_videoStream = -1;
  m_width = 0;
  m_height = 0;
  m_timeBase = 0.0;
  m_nextFrameMs = 0;
  m_isOpen = false;
}

bool CVideoBackgroundDecoder::IsOpen() const
{
  return m_isOpen;
}

bool CVideoBackgroundDecoder::DecodeNextFrame()
{
  bool didSeek = false;
  AVPacket pkt;
  av_init_packet(&pkt);
  pkt.data = nullptr;
  pkt.size = 0;

  while (true)
  {
    int ret = av_read_frame(m_fmtCtx, &pkt);
    if (ret == AVERROR_EOF)
    {
      if (didSeek)
        return false;  // already tried looping — give up (corrupt/empty file)
      SeekToStart();
      didSeek = true;
      continue;
    }
    if (ret < 0)
      return false;

    if (pkt.stream_index != m_videoStream)
    {
      av_free_packet(&pkt);
      continue;
    }

    int gotFrame = 0;
    avcodec_decode_video2(m_codecCtx, m_avFrame, &gotFrame, &pkt);
    av_free_packet(&pkt);

    if (gotFrame)
    {
      sws_scale(m_swsCtx, m_avFrame->data, m_avFrame->linesize, 0, m_height,
                m_rgbFrame->data, m_rgbFrame->linesize);

      double frameDurationMs = av_frame_get_pkt_duration(m_avFrame) * m_timeBase * 1000.0;
      if (frameDurationMs <= 0.0)
        frameDurationMs = 33.0;

      m_nextFrameMs += (unsigned int)frameDurationMs;
      return true;
    }
  }
}

void CVideoBackgroundDecoder::SeekToStart()
{
  av_seek_frame(m_fmtCtx, m_videoStream, 0, AVSEEK_FLAG_BACKWARD);
  avcodec_flush_buffers(m_codecCtx);
  m_nextFrameMs = 0;
}

bool CVideoBackgroundDecoder::Update(unsigned int currentTimeMs)
{
  if (!m_isOpen)
    return false;

  if (currentTimeMs >= m_nextFrameMs)
    return DecodeNextFrame();

  return false;
}

const uint8_t* CVideoBackgroundDecoder::GetCurrentFrame(int& width, int& height) const
{
  if (!m_isOpen)
    return nullptr;

  width = m_width;
  height = m_height;
  return m_rgbFrame->data[0];
}
