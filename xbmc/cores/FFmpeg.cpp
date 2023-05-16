/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/FFmpeg.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

extern "C"
{
#include <libavcodec/bsf.h>
}

#include <map>
#include <mutex>

static thread_local CFFmpegLog* CFFmpegLogTls;

namespace FFMPEG_HELP_TOOLS
{

std::string FFMpegErrorToString(int err)
{
  std::string text;
  text.resize(AV_ERROR_MAX_STRING_SIZE);
  av_strerror(err, text.data(), AV_ERROR_MAX_STRING_SIZE);
  return text;
}

} // namespace FFMPEG_HELP_TOOLS

void CFFmpegLog::SetLogLevel(int level)
{
  CFFmpegLog::ClearLogLevel();
  CFFmpegLog *log = new CFFmpegLog();
  log->level = level;
  CFFmpegLogTls = log;
}

int CFFmpegLog::GetLogLevel()
{
  CFFmpegLog* log = CFFmpegLogTls;
  if (!log)
    return -1;
  return log->level;
}

void CFFmpegLog::ClearLogLevel()
{
  CFFmpegLog* log = CFFmpegLogTls;
  CFFmpegLogTls = nullptr;
  if (log)
    delete log;
}

static CCriticalSection m_logSection;
std::map<const CThread*, std::string> g_logbuffer;

void ff_flush_avutil_log_buffers(void)
{
  std::unique_lock<CCriticalSection> lock(m_logSection);
  /* Loop through the logbuffer list and remove any blank buffers
     If the thread using the buffer is still active, it will just
     add a new buffer next time it writes to the log */
  std::map<const CThread*, std::string>::iterator it;
  for (it = g_logbuffer.begin(); it != g_logbuffer.end(); )
    if ((*it).second.empty())
      g_logbuffer.erase(it++);
    else
      ++it;
}

void ff_avutil_log(void* ptr, int level, const char* format, va_list va)
{
  std::unique_lock<CCriticalSection> lock(m_logSection);
  const CThread* threadId = CThread::GetCurrentThread();
  std::string &buffer = g_logbuffer[threadId];

  AVClass* avc= ptr ? *(AVClass**)ptr : NULL;

  int maxLevel = AV_LOG_WARNING;
  if (CFFmpegLog::GetLogLevel() > 0)
    maxLevel = AV_LOG_INFO;

  if (level > maxLevel && !CServiceBroker::GetLogging().CanLogComponent(LOGFFMPEG))
    return;
  else if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel <= LOG_LEVEL_NORMAL)
    return;

  int type;
  switch (level)
  {
    case AV_LOG_INFO:
      type = LOGINFO;
      break;

    case AV_LOG_ERROR:
      type = LOGERROR;
      break;

    case AV_LOG_DEBUG:
    default:
      type = LOGDEBUG;
      break;
  }

  std::string message = StringUtils::FormatV(format, va);
  std::string prefix = StringUtils::Format("ffmpeg[{}]: ", fmt::ptr(threadId));
  if (avc)
  {
    if (avc->item_name)
      prefix += std::string("[") + avc->item_name(ptr) + "] ";
    else if (avc->class_name)
      prefix += std::string("[") + avc->class_name + "] ";
  }

  buffer += message;
  int pos, start = 0;
  while ((pos = buffer.find_first_of('\n', start)) >= 0)
  {
    if (pos > start)
      CLog::Log(type, "{}{}", prefix, buffer.substr(start, pos - start));
    start = pos+1;
  }
  buffer.erase(0, start);
}

FFmpegExtraData::FFmpegExtraData(size_t size)
  : m_data(reinterpret_cast<uint8_t*>(av_mallocz(size + AV_INPUT_BUFFER_PADDING_SIZE))),
    m_size(size)
{
  // using av_mallocz because some APIs require at least the padding to be zeroed, e.g. AVCodecParameters
  if (!m_data)
    throw std::bad_alloc();
}

FFmpegExtraData::FFmpegExtraData(const uint8_t* data, size_t size) : FFmpegExtraData(size)
{
  if (size > 0)
    std::memcpy(m_data, data, size);
}

FFmpegExtraData::~FFmpegExtraData()
{
  av_free(m_data);
}

FFmpegExtraData::FFmpegExtraData(const FFmpegExtraData& e) : FFmpegExtraData(e.m_size)
{
  if (m_size > 0)
    std::memcpy(m_data, e.m_data, m_size);
}

FFmpegExtraData::FFmpegExtraData(FFmpegExtraData&& other) noexcept : FFmpegExtraData()
{
  std::swap(m_data, other.m_data);
  std::swap(m_size, other.m_size);
}

FFmpegExtraData& FFmpegExtraData::operator=(const FFmpegExtraData& other)
{
  if (this != &other)
  {
    if (m_size >= other.m_size && other.m_size > 0) // reuse current buffer if large enough
    {
      std::memcpy(m_data, other.m_data, other.m_size);
      m_size = other.m_size;
    }
    else
    {
      FFmpegExtraData extraData(other);
      *this = std::move(extraData);
    }
  }
  return *this;
}

FFmpegExtraData& FFmpegExtraData::operator=(FFmpegExtraData&& other) noexcept
{
  if (this != &other)
  {
    std::swap(m_data, other.m_data);
    std::swap(m_size, other.m_size);
  }
  return *this;
}

bool FFmpegExtraData::operator==(const FFmpegExtraData& other) const
{
  return m_size == other.m_size && (m_size == 0 || std::memcmp(m_data, other.m_data, m_size) == 0);
}

bool FFmpegExtraData::operator!=(const FFmpegExtraData& other) const
{
  return !(*this == other);
}

uint8_t* FFmpegExtraData::TakeData()
{
  auto tmp = m_data;
  m_data = nullptr;
  m_size = 0;
  return tmp;
}

FFmpegExtraData GetPacketExtradata(const AVPacket* pkt, const AVCodecParameters* codecPar)
{
  constexpr int FF_MAX_EXTRADATA_SIZE = ((1 << 28) - AV_INPUT_BUFFER_PADDING_SIZE);

  if (!pkt)
    return {};

  /* extract_extradata bitstream filter is implemented only
   * for certain codecs, as noted in discussion of PR#21248
   */

  AVCodecID codecId = codecPar->codec_id;

  // clang-format off
  if (
    codecId != AV_CODEC_ID_MPEG1VIDEO &&
    codecId != AV_CODEC_ID_MPEG2VIDEO &&
    codecId != AV_CODEC_ID_H264 &&
    codecId != AV_CODEC_ID_HEVC &&
    codecId != AV_CODEC_ID_MPEG4 &&
    codecId != AV_CODEC_ID_VC1 &&
    codecId != AV_CODEC_ID_AV1 &&
    codecId != AV_CODEC_ID_AVS2 &&
    codecId != AV_CODEC_ID_AVS3 &&
    codecId != AV_CODEC_ID_CAVS
  )
    // clang-format on
    return {};

  const AVBitStreamFilter* f = av_bsf_get_by_name("extract_extradata");
  if (!f)
    return {};

  AVBSFContext* bsf = nullptr;
  int ret = av_bsf_alloc(f, &bsf);
  if (ret < 0)
    return {};

  ret = avcodec_parameters_copy(bsf->par_in, codecPar);
  if (ret < 0)
  {
    av_bsf_free(&bsf);
    return {};
  }

  ret = av_bsf_init(bsf);
  if (ret < 0)
  {
    av_bsf_free(&bsf);
    return {};
  }

  AVPacket* dstPkt = av_packet_alloc();
  if (!dstPkt)
  {
    CLog::LogF(LOGERROR, "failed to allocate packet");

    av_bsf_free(&bsf);
    return {};
  }
  AVPacket* pktRef = dstPkt;

  ret = av_packet_ref(pktRef, pkt);
  if (ret < 0)
  {
    av_bsf_free(&bsf);
    av_packet_free(&dstPkt);
    return {};
  }

  ret = av_bsf_send_packet(bsf, pktRef);
  if (ret < 0)
  {
    av_packet_unref(pktRef);
    av_bsf_free(&bsf);
    av_packet_free(&dstPkt);
    return {};
  }

  FFmpegExtraData extraData;
  ret = 0;
  while (ret >= 0)
  {
    ret = av_bsf_receive_packet(bsf, pktRef);
    if (ret < 0)
    {
      if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
        break;

      continue;
    }

    size_t retExtraDataSize = 0;
    uint8_t* retExtraData =
        av_packet_get_side_data(pktRef, AV_PKT_DATA_NEW_EXTRADATA, &retExtraDataSize);
    if (retExtraData && retExtraDataSize > 0 && retExtraDataSize < FF_MAX_EXTRADATA_SIZE)
    {
      try
      {
        extraData = FFmpegExtraData(retExtraData, retExtraDataSize);
      }
      catch (const std::bad_alloc&)
      {
        CLog::LogF(LOGERROR, "failed to allocate {} bytes for extradata", retExtraDataSize);

        av_packet_unref(pktRef);
        av_bsf_free(&bsf);
        av_packet_free(&dstPkt);
        return {};
      }

      CLog::LogF(LOGDEBUG, "fetching extradata, extradata_size({})", retExtraDataSize);

      av_packet_unref(pktRef);
      break;
    }

    av_packet_unref(pktRef);
  }

  av_bsf_free(&bsf);
  av_packet_free(&dstPkt);

  return extraData;
}
