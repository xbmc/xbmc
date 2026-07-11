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
#include "utils/log.h"

#include <algorithm>

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

  // Don't pass extradata to cea_set_demuxer: the extradata SPS may report a
  // different max_num_reorder_frames than the actual in-stream SPS, and libcea
  // never updates the window once it's set from extradata. Passing NULL lets
  // libcea default to window=4 and auto-detect the correct value from the first
  // in-stream SPS NAL. The packaging type (AVCC/Annex B) is set above from our
  // own extradata check, which is all cea_set_demuxer actually needs from us.
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
  while (!m_captionQueue.empty())
  {
    CDVDDemuxUtils::FreeDemuxPacket(m_captionQueue.front());
    m_captionQueue.pop();
  }
}

void CDVDDemuxCC::Flush()
{
  // Drain any queued packets from before the seek
  while (!m_captionQueue.empty())
  {
    CDVDDemuxUtils::FreeDemuxPacket(m_captionQueue.front());
    m_captionQueue.pop();
  }

  // Reinitialize libcea to clear its internal reorder buffer and decoder state.
  // Streams are intentionally kept so that VideoPlayer's selection stream list
  // remains consistent and the user's subtitle choice is preserved after seek.
  InitLibcea();
}

CDemuxStream* CDVDDemuxCC::GetStream(int iStreamId) const
{
  for (const auto& stream : m_streams)
  {
    if (stream.uniqueId == iStreamId)
      return const_cast<CDemuxStreamSubtitle*>(&stream);
  }
  return nullptr;
}

std::vector<CDemuxStream*> CDVDDemuxCC::GetStreams() const
{
  std::vector<CDemuxStream*> streams;
  streams.reserve(m_streams.size());
  for (const auto& stream : m_streams)
    streams.push_back(const_cast<CDemuxStreamSubtitle*>(&stream));
  return streams;
}

int CDVDDemuxCC::GetNrOfStreams() const
{
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

  const int64_t ptsMs = DVD_TIME_TO_MSEC(pts_dvd);
  CLog::Log(LOGDEBUG, "CDVDDemuxCC: feed pts_ms={} size={}", ptsMs, pSrcPacket->iSize);
  cea_feed_packet(m_ceaCtx.get(), pSrcPacket->pData, pSrcPacket->iSize, ptsMs);

  if (m_captionQueue.empty())
    return {};

  std::vector<DemuxPacket*> packets;
  packets.reserve(m_captionQueue.size());
  while (!m_captionQueue.empty())
  {
    packets.push_back(m_captionQueue.front());
    m_captionQueue.pop();
  }
  return packets;
}

void CDVDDemuxCC::CaptionCallback(const cea_caption* cap, void* userdata)
{
  CDVDDemuxCC* self = static_cast<CDVDDemuxCC*>(userdata);
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
  CLog::Log(LOGDEBUG, "CDVDDemuxCC: cap field={} mode={:#x} pts_ms={} text={}",
            cap->field, static_cast<int>(cap->mode), cap->pts_ms, text.empty() ? "<clear>" : text);

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

void CDVDDemuxCC::EnsureStream(int field, int channel)
{
  const int uniqueId = (field - 1) * 2 + (channel - 1);
  for (const auto& stream : m_streams)
  {
    if (stream.uniqueId == uniqueId)
      return;
  }

  CDemuxStreamSubtitle stream;
  stream.source = STREAM_SOURCE_VIDEOMUX;
  stream.name =
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(39206); // "CC"
  stream.language = "und";

  auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (settings->GetBool(CSettings::SETTING_SUBTITLES_CAPTIONSIMPAIRED))
    stream.flags = FLAG_HEARING_IMPAIRED;

  stream.codec = AV_CODEC_ID_TEXT;
  stream.uniqueId = uniqueId;
  auto it = std::lower_bound(m_streams.begin(), m_streams.end(), uniqueId,
                             [](const CDemuxStreamSubtitle& s, int id) { return s.uniqueId < id; });
  m_streams.insert(it, std::move(stream));
}
