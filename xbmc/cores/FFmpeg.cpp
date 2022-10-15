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

std::tuple<uint8_t*, int> GetPacketExtradata(const AVPacket* pkt,
                                             const AVCodecParserContext* parserCtx,
                                             AVCodecContext* codecCtx)
{
  constexpr int FF_MAX_EXTRADATA_SIZE = ((1 << 28) - AV_INPUT_BUFFER_PADDING_SIZE);

  if (!pkt)
    return std::make_tuple(nullptr, 0);

  uint8_t* extraData = nullptr;
  int extraDataSize = 0;

#if LIBAVFORMAT_BUILD >= AV_VERSION_INT(59, 0, 100)
  /* extract_extradata bitstream filter is implemented only
   * for certain codecs, as noted in discussion of PR#21248
   */

  AVCodecID codecId = codecCtx->codec_id;

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
    return std::make_tuple(nullptr, 0);

  const AVBitStreamFilter* f = av_bsf_get_by_name("extract_extradata");
  if (!f)
    return std::make_tuple(nullptr, 0);

  AVBSFContext* bsf = nullptr;
  int ret = av_bsf_alloc(f, &bsf);
  if (ret < 0)
    return std::make_tuple(nullptr, 0);

  bsf->par_in->codec_id = codecId;

  ret = av_bsf_init(bsf);
  if (ret < 0)
  {
    av_bsf_free(&bsf);
    return std::make_tuple(nullptr, 0);
  }

  AVPacket* dstPkt = av_packet_alloc();
  if (!dstPkt)
  {
    CLog::LogF(LOGERROR, "failed to allocate packet");

    av_bsf_free(&bsf);
    return std::make_tuple(nullptr, 0);
  }
  AVPacket* pktRef = dstPkt;

  ret = av_packet_ref(pktRef, pkt);
  if (ret < 0)
  {
    av_bsf_free(&bsf);
    av_packet_free(&dstPkt);
    return std::make_tuple(nullptr, 0);
  }

  ret = av_bsf_send_packet(bsf, pktRef);
  if (ret < 0)
  {
    av_packet_unref(pktRef);
    av_bsf_free(&bsf);
    av_packet_free(&dstPkt);
    return std::make_tuple(nullptr, 0);
  }

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
      extraData = static_cast<uint8_t*>(av_malloc(retExtraDataSize + AV_INPUT_BUFFER_PADDING_SIZE));
      if (!extraData)
      {
        CLog::LogF(LOGERROR, "failed to allocate {} bytes for extradata", retExtraDataSize);

        av_packet_unref(pktRef);
        av_bsf_free(&bsf);
        av_packet_free(&dstPkt);
        return std::make_tuple(nullptr, 0);
      }

      CLog::LogF(LOGDEBUG, "fetching extradata, extradata_size({})", retExtraDataSize);

      memcpy(extraData, retExtraData, retExtraDataSize);
      memset(extraData + retExtraDataSize, 0, AV_INPUT_BUFFER_PADDING_SIZE);
      extraDataSize = retExtraDataSize;

      av_packet_unref(pktRef);
      break;
    }

    av_packet_unref(pktRef);
  }

  av_bsf_free(&bsf);
  av_packet_free(&dstPkt);
#else
  if (codecCtx && parserCtx && parserCtx->parser && parserCtx->parser->split)
    extraDataSize = parserCtx->parser->split(codecCtx, pkt->data, pkt->size);

  if (extraDataSize <= 0 || extraDataSize >= FF_MAX_EXTRADATA_SIZE)
  {
    CLog::LogF(LOGDEBUG, "fetched extradata of weird size {}", extraDataSize);
    return std::make_tuple(nullptr, 0);
  }

  extraData = static_cast<uint8_t*>(av_malloc(extraDataSize + AV_INPUT_BUFFER_PADDING_SIZE));
  if (!extraData)
  {
    CLog::LogF(LOGERROR, "failed to allocate {} bytes for extradata", extraDataSize);
    return std::make_tuple(nullptr, 0);
  }

  CLog::LogF(LOGDEBUG, "fetching extradata, extradata_size({})", extraDataSize);

  memcpy(extraData, pkt->data, extraDataSize);
  memset(extraData + extraDataSize, 0, AV_INPUT_BUFFER_PADDING_SIZE);
#endif

  return std::make_tuple(extraData, extraDataSize);
}
