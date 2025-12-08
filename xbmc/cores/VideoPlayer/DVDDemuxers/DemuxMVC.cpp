/*
*      Copyright (C) 2005-2013 Team XBMC
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

#include "DemuxMVC.h"
#include "DVDDemuxUtils.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "cores/FFmpeg.h"
#include "utils/log.h"

extern "C" {
#include "libavutil/opt.h"
};

#define MVC_SEEK_TIME_WINDOW 75000 // experimental value depends on seeking accurate

static int mvc_file_read(void *h, uint8_t* buf, int size)
{
  CDVDInputStream* pInputStream = static_cast<CDemuxMVC*>(h)->m_pInput;
  int s = pInputStream->Read(buf, size);

  if (pInputStream->IsEOF()) {
	  return AVERROR_EOF;
  }

  return s;
}

static int64_t mvc_file_seek(void *h, int64_t pos, int whence)
{
  CDVDInputStream* pInputStream = static_cast<CDemuxMVC*>(h)->m_pInput;
  if (whence == AVSEEK_SIZE)
    return pInputStream->GetLength();
  else
    return pInputStream->Seek(pos, whence & ~AVSEEK_FORCE);
}

CDemuxMVC::CDemuxMVC()
{
  m_ioContext = nullptr;
  m_pFormatContext = nullptr;
  m_pInput = nullptr;
  m_nStreamIndex = -1;
}

CDemuxMVC::~CDemuxMVC()
{
  Dispose();
}

bool CDemuxMVC::Open(CDVDInputStream* pInput)
{
  int ret;

  if (!pInput)
    return false;
  m_pInput = pInput;

  int bufferSize = 4096;
  int blockSize = m_pInput->GetBlockSize();
  if (blockSize > 1)
    bufferSize = blockSize;
  auto buffer = (unsigned char*)av_malloc(bufferSize);
  m_ioContext = avio_alloc_context(buffer, bufferSize, 0, this, mvc_file_read, nullptr, mvc_file_seek);

  m_pFormatContext = avformat_alloc_context();
  m_pFormatContext->pb = m_ioContext;

  const AVInputFormat *format = av_find_input_format("mpegts");
  ret = avformat_open_input(&m_pFormatContext, m_pInput->GetFileName().c_str(), format, nullptr);
  if (ret < 0)
  {
    CLog::Log(LOGDEBUG, "{}: Opening MVC demuxing context failed ({})", __FUNCTION__, ret);
    Dispose();
    return false;
  }

  av_opt_set_int(m_pFormatContext, "analyzeduration", 500000, 0);
  av_opt_set_int(m_pFormatContext, "correct_ts_overflow", 0, 0);

  // Find the streams
  ret = avformat_find_stream_info(m_pFormatContext, nullptr);
  //it always returns -1 so just ignore it
  //if (ret < 0)
  //{
  //  CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::OpenMVCDemuxer(): avformat_find_stream_info failed ({})", ret);
  //  Dispose();
  //  return false;
  //}

  // print some extra information
  av_dump_format(m_pFormatContext, 0, m_pInput->GetFileName().c_str(), 0);

  // Find and select our MVC stream
  CLog::Log(LOGDEBUG, "{}: MVC m2ts has {} streams", __FUNCTION__, m_pFormatContext->nb_streams);
  for (unsigned i = 0; i < m_pFormatContext->nb_streams; i++)
  {
    if (m_pFormatContext->streams[i]->codecpar->codec_id == AV_CODEC_ID_H264_MVC
      && m_pFormatContext->streams[i]->codecpar->extradata_size > 0)
    {
      m_nStreamIndex = i;
      break;
    }
    else
      m_pFormatContext->streams[i]->discard = AVDISCARD_ALL;
  }

  if (m_nStreamIndex < 0)
  {
    CLog::Log(LOGDEBUG, "{}: MVC Stream not found", __FUNCTION__);
    Dispose();
    return false;
  }

  return true;
}

bool CDemuxMVC::Reset()
{
  CDVDInputStream* pInput = m_pInput;
  Dispose();
  Open(pInput);
  return true;
}

void CDemuxMVC::Abort()
{
}

void CDemuxMVC::Flush()
{
  if (m_pFormatContext)
  {
    if (m_pFormatContext->pb)
      avio_flush(m_pFormatContext->pb);
    avformat_flush(m_pFormatContext);
  }
}

DemuxPacket* CDemuxMVC::Read()
{
  int ret;
  DemuxPacket* newPkt = nullptr;
  AVPacket* pkt = av_packet_alloc();
  if (!pkt)
  {
    CLog::Log(LOGERROR, "CDemuxMVC::{} - av_packet_alloc failed: {}", __FUNCTION__,
              strerror(errno));
    return newPkt;
  }

  while (true)
  {
    ret = av_read_frame(m_pFormatContext, pkt);

    if (ret == AVERROR(EINTR) || ret == AVERROR(EAGAIN))
      continue;
    else if (ret == AVERROR_EOF)
      break;
    else if (pkt->size <= 0 || pkt->stream_index != m_nStreamIndex)
    {
      av_packet_unref(pkt);
      continue;
    }
    else
    {
      AVStream *stream = m_pFormatContext->streams[pkt->stream_index];
      newPkt = CDVDDemuxUtils::AllocateDemuxPacket(pkt->size);
      if (pkt->data)
        memcpy(newPkt->pData, pkt->data, pkt->size);
      newPkt->iSize = pkt->size;
      newPkt->iStreamId = stream->id;
      newPkt->dts =
        ConvertTimestamp(pkt->dts, stream->time_base.den, stream->time_base.num);
      newPkt->pts =
        ConvertTimestamp(pkt->pts, stream->time_base.den, stream->time_base.num);
      newPkt->duration = DVD_SEC_TO_TIME((double)pkt->duration * stream->time_base.num /
                                          stream->time_base.den);
      break;
    }
  }

  av_packet_free(&pkt);
  return newPkt;
}

bool CDemuxMVC::SeekTime(double time, bool backwards, double* startpts)
{
  if (!m_pInput)
    return false;

  AVRational time_base = m_pFormatContext->streams[m_nStreamIndex]->time_base;
  int64_t seek_pts = av_rescale(DVD_MSEC_TO_TIME(time), time_base.den, (int64_t)time_base.num * AV_TIME_BASE);
  int64_t starttime = 0;

  if (m_pFormatContext->start_time != (int64_t)AV_NOPTS_VALUE)
    starttime = av_rescale(m_pFormatContext->start_time, time_base.den, (int64_t)time_base.num * AV_TIME_BASE);
  if (starttime != 0)
    seek_pts += starttime;
  if (seek_pts < MVC_SEEK_TIME_WINDOW)
    seek_pts = 0;
  else
    seek_pts -= MVC_SEEK_TIME_WINDOW;

  av_seek_frame(m_pFormatContext, m_nStreamIndex, seek_pts, backwards ? AVSEEK_FLAG_BACKWARD : 0);

  return true;
}

std::string CDemuxMVC::GetFileName()
{
  return m_pInput->GetFileName();
}

AVStream* CDemuxMVC::GetAVStream() const {
  return m_pFormatContext ? m_pFormatContext->streams[m_nStreamIndex] : nullptr;
}

void CDemuxMVC::Dispose()
{
  if (m_pFormatContext)
    avformat_close_input(&m_pFormatContext);

  if (m_ioContext)
  {
    av_free(m_ioContext->buffer);
    av_free(m_ioContext);
  }

  m_ioContext = nullptr;
  m_pFormatContext = nullptr;
  m_pInput = nullptr;
  m_nStreamIndex = -1;
}

double CDemuxMVC::ConvertTimestamp(int64_t pts, int den, int num) const {
  if (pts == AV_NOPTS_VALUE)
    return DVD_NOPTS_VALUE;

  // do calculations in floats as they can easily overflow otherwise
  // we don't care for having a completly exact timestamp anyway
  double timestamp = (double)pts * num / den;
  double starttime = 0.0;

  if (m_menu_type != MenuType::NATIVE &&
      m_pFormatContext->start_time != static_cast<int64_t>(AV_NOPTS_VALUE))
  {
    starttime = static_cast<double>(m_start_time) / AV_TIME_BASE;
  }

  if (timestamp > starttime)
    timestamp -= starttime;
  // allow for largest possible difference in pts and dts for a single packet
  else if (timestamp + 0.5 > starttime)
    timestamp = 0;

  return timestamp * DVD_TIME_BASE;
}

std::vector<CDemuxStream*> CDemuxMVC::GetStreams() const
{
  std::vector<CDemuxStream*> streams;
  return streams;
}
