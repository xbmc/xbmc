/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDDemuxCC.h"

#include "DVDDemuxUtils.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstring>

namespace
{
void FreeAndClearQueue(std::queue<DemuxPacket*>& queue)
{
  while (!queue.empty())
  {
    CDVDDemuxUtils::FreeDemuxPacket(queue.front());
    queue.pop();
  }
}
} // namespace

CDVDDemuxCC::CDVDDemuxCC(AVCodecID codec, const uint8_t* extradata, int extrasize)
{
  m_ceaCodec = (codec == AV_CODEC_ID_H264) ? CEA_CODEC_H264 : CEA_CODEC_MPEG2;

  // Detect AVCC packaging: extradata starts with version byte 0x01
  m_ceaPkg = CEA_PACKAGING_ANNEX_B;
  if (codec == AV_CODEC_ID_H264 && extrasize >= 7 && extradata && extradata[0] == 1)
    m_ceaPkg = CEA_PACKAGING_AVCC;

  if (extradata && extrasize > 0)
    m_extradata.assign(extradata, extradata + extrasize);

  InitLibcea();
  StartVideoPacketFeedThread();
}

void CDVDDemuxCC::StartVideoPacketFeedThread()
{
  m_stopVideoPacketFeedThread = false;
  m_videoPacketFeedThread = std::thread(&CDVDDemuxCC::RunVideoPacketFeedThread, this);
}

void CDVDDemuxCC::StopVideoPacketFeedThread()
{
  m_stopVideoPacketFeedThread = true;
  m_videoPacketFeedEvent.Set();
  if (m_videoPacketFeedThread.joinable())
    m_videoPacketFeedThread.join();
}

bool CDVDDemuxCC::InitLibcea()
{
  m_ceaCtx.reset(cea_init_default());
  if (!m_ceaCtx)
  {
    CLog::Log(LOGERROR, "CDVDDemuxCC: failed to initialize libcea");
    return false;
  }

  cea_set_log_callback(m_ceaCtx.get(), LogCallback, nullptr, CEA_LOG_INFO);

  if (cea_set_demuxer(m_ceaCtx.get(), m_ceaCodec, m_ceaPkg, nullptr, 0) < 0)
  {
    CLog::Log(LOGERROR, "CDVDDemuxCC: failed to configure demuxer");
    m_ceaCtx.reset();
    return false;
  }

  cea_set_caption_callback(m_ceaCtx.get(), CaptionCallback, this);
  return true;
}

CDVDDemuxCC::~CDVDDemuxCC()
{
  // Thread already joined, no lock needed for the queues below.
  StopVideoPacketFeedThread();
  FreeAndClearQueue(m_videoPacketFeedQueue);
  FreeAndClearQueue(m_captionQueue);
}

void CDVDDemuxCC::Flush()
{
  StopVideoPacketFeedThread();

  {
    std::lock_guard<CCriticalSection> lock(m_videoPacketFeedSection);
    FreeAndClearQueue(m_videoPacketFeedQueue);
  }

  // Drain any queued packets from before the seek
  {
    std::lock_guard<CCriticalSection> lock(m_captionSection);
    FreeAndClearQueue(m_captionQueue);
  }

  // Reinitialize libcea to clear its internal reorder buffer and decoder state.
  // Streams are intentionally kept so that VideoPlayer's selection stream list
  // remains consistent and the user's subtitle choice is preserved after seek.
  InitLibcea();
  StartVideoPacketFeedThread();
}

CDemuxStream* CDVDDemuxCC::GetStream(int iStreamId) const
{
  std::lock_guard<CCriticalSection> lock(m_captionSection);
  for (const auto& stream : m_streams)
  {
    if (stream.uniqueId == iStreamId)
      return const_cast<CDemuxStreamSubtitle*>(&stream);
  }
  return nullptr;
}

std::vector<CDemuxStream*> CDVDDemuxCC::GetStreams() const
{
  std::lock_guard<CCriticalSection> lock(m_captionSection);
  std::vector<CDemuxStream*> streams;
  streams.reserve(m_streams.size());
  for (const auto& stream : m_streams)
    streams.push_back(const_cast<CDemuxStreamSubtitle*>(&stream));
  return streams;
}

int CDVDDemuxCC::GetNrOfStreams() const
{
  std::lock_guard<CCriticalSection> lock(m_captionSection);
  return static_cast<int>(m_streams.size());
}

std::vector<DemuxPacket*> CDVDDemuxCC::Read(DemuxPacket* pSrcPacket)
{
  if (!m_ceaCtx || !pSrcPacket)
    return {};

  if (pSrcPacket->pts == DVD_NOPTS_VALUE && pSrcPacket->dts == DVD_NOPTS_VALUE)
    return {};

  // Called from ProcessVideoData after CheckContinuity, so pkt->pts reflects the
  // final corrected timestamp including any discontinuity adjustment. This is what
  // libcea needs: a smoothly increasing pts_ms across DTS discontinuities.
  double pts_dvd = (pSrcPacket->pts != DVD_NOPTS_VALUE) ? pSrcPacket->pts : pSrcPacket->dts;

  // Copy: pSrcPacket's buffer is only valid for the duration of this call.
  DemuxPacket* feedPkt = CDVDDemuxUtils::AllocateDemuxPacket(pSrcPacket->iSize);
  feedPkt->iSize = pSrcPacket->iSize;
  std::memcpy(feedPkt->pData, pSrcPacket->pData, pSrcPacket->iSize);
  feedPkt->pts = pts_dvd;

  {
    std::lock_guard<CCriticalSection> lock(m_videoPacketFeedSection);
    m_videoPacketFeedQueue.push(feedPkt);
  }
  m_videoPacketFeedEvent.Set();

  // Drain whatever caption packets the feed thread has produced so far
  std::lock_guard<CCriticalSection> lock(m_captionSection);
  std::vector<DemuxPacket*> packets;
  packets.reserve(m_captionQueue.size());
  while (!m_captionQueue.empty())
  {
    packets.push_back(m_captionQueue.front());
    m_captionQueue.pop();
  }
  return packets;
}

void CDVDDemuxCC::RunVideoPacketFeedThread()
{
  while (!m_stopVideoPacketFeedThread)
  {
    m_videoPacketFeedEvent.Wait();
    if (m_stopVideoPacketFeedThread)
      break;

    while (true)
    {
      DemuxPacket* pkt = nullptr;
      {
        std::lock_guard<CCriticalSection> lock(m_videoPacketFeedSection);
        if (m_videoPacketFeedQueue.empty())
          break;
        pkt = m_videoPacketFeedQueue.front();
        m_videoPacketFeedQueue.pop();
      }

      const int64_t ptsMs = DVD_TIME_TO_MSEC(pkt->pts);
      cea_feed_packet(m_ceaCtx.get(), pkt->pData, pkt->iSize, ptsMs);
      CDVDDemuxUtils::FreeDemuxPacket(pkt);
    }
  }
}

void CDVDDemuxCC::CaptionCallback(const cea_caption* cap, void* userdata)
{
  CDVDDemuxCC* self = static_cast<CDVDDemuxCC*>(userdata);

  // Called synchronously from cea_feed_packet() on the feed thread; guards
  // m_streams/m_captionQueue against GetStream/GetStreams/GetNrOfStreams/Read.
  std::lock_guard<CCriticalSection> lock(self->m_captionSection);

  self->EnsureStream(cap->field, cap->channel);
  // In rollup modes (RU2/RU3/RU4), skip CLEAR events. The next SHOW will finalize
  // the previous subtitle at its own PTS via DVDOverlayCodecCCText's "different text"
  // path, producing a zero-gap transition. Emitting a CLEAR here would set the previous
  // subtitle's stop time to end_ms and then the next SHOW would start at start_ms —
  // any difference between those two timestamps causes a visible blank frame (flash).
  const bool isRollup = CEA_IS_ROLLUP(cap->mode);
  if (!cap->text && isRollup)
    return;

  const std::string text = cap->text ? cap->text : "";
  const double pts = DVD_MSEC_TO_TIME(static_cast<double>(cap->pts_ms));
  CLog::Log(LOGDEBUG, "CDVDDemuxCC: cap field={} mode={:#x} pts_ms={} text={}", cap->field,
            static_cast<int>(cap->mode), cap->pts_ms, text.empty() ? "<clear>" : text);

  DemuxPacket* pkt = CDVDDemuxUtils::AllocateDemuxPacket(static_cast<int>(text.size()));
  pkt->iSize = static_cast<int>(text.size());
  if (!text.empty())
    std::copy(text.begin(), text.end(), pkt->pData);
  pkt->iStreamId = (cap->field - 1) * 2 + (cap->channel - 1);
  pkt->pts = pts;
  pkt->duration = 0;

  self->m_captionQueue.push(pkt);
}

void CDVDDemuxCC::LogCallback(cea_log_level level, const char* msg, void* /*userdata*/)
{
  int kodiLevel = LOGINFO;
  switch (level)
  {
    case CEA_LOG_DEBUG:
      kodiLevel = LOGDEBUG;
      break;
    case CEA_LOG_WARNING:
      kodiLevel = LOGWARNING;
      break;
    case CEA_LOG_ERROR:
      kodiLevel = LOGERROR;
      break;
    case CEA_LOG_FATAL:
      kodiLevel = LOGFATAL;
      break;
    default:
      break;
  }
  CLog::Log(kodiLevel, "libcea: {}", msg);
}

bool CDVDDemuxCC::HasStream(int uniqueId) const
{
  return std::any_of(m_streams.begin(), m_streams.end(),
                      [uniqueId](const auto& s) { return s.uniqueId == uniqueId; });
}

void CDVDDemuxCC::EnsureStream(int field, int channel)
{
  const int uniqueId = (field - 1) * 2 + (channel - 1);
  if (HasStream(uniqueId))
    return;

  if (field == 3) // CEA-708: services are discovered and appended lazily, one at a time
  {
    m_streams.push_back(CreateStream(field, channel));
    return;
  }

  // EIA-608: a packet for CC N implies CC1..CC(N-1) are also present in the stream
  // even if not yet observed, so backfill any lower channels that are missing. This
  // avoids the subtitle dialog's CC list growing/reordering piecemeal as
  // lower-numbered channels are separately discovered later.
  for (int uid = 0; uid <= uniqueId; ++uid)
  {
    if (!HasStream(uid))
      m_streams.push_back(CreateStream(uid / 2 + 1, uid % 2 + 1));
  }
}

CDemuxStreamSubtitle CDVDDemuxCC::CreateStream(int field, int channel) const
{
  CDemuxStreamSubtitle stream;
  stream.source = STREAM_SOURCE_VIDEOMUX;
  stream.name = BuildStreamName(field, channel);
  stream.language = "und";

  auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (settings->GetBool(CSettings::SETTING_SUBTITLES_CAPTIONSIMPAIRED))
    stream.flags = FLAG_HEARING_IMPAIRED;

  stream.codec = AV_CODEC_ID_TEXT;
  stream.uniqueId = (field - 1) * 2 + (channel - 1);
  return stream;
}

std::string CDVDDemuxCC::BuildStreamName(int field, int channel)
{
  const std::string ccLabel =
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(39206); // "CC"

  if (field == 3) // CEA-708 service
    return StringUtils::Format("{} CEA 708 Service {}", ccLabel, channel);

  // EIA-608: field 1/channel 1-2 -> CC1/CC2, field 2/channel 1-2 -> CC3/CC4
  const int ccNumber = (field - 1) * 2 + channel;
  return StringUtils::Format("{}{} (CEA 608)", ccLabel, ccNumber);
}
