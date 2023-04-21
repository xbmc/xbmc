/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OverlayCodecWebVTT.h"

#include "DVDCodecs/DVDCodecs.h"
#include "DVDOverlayText.h"
#include "DVDStreamInfo.h"
#include "cores/VideoPlayer/DVDSubtitles/SubtitlesStyle.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "utils/CharArrayParser.h"

#include <cstring>
#include <memory>
#include <string>

using namespace KODI;

COverlayCodecWebVTT::COverlayCodecWebVTT()
  : CDVDOverlayCodec("WebVTT Subtitle Decoder"), m_pOverlay(nullptr)
{
}

bool COverlayCodecWebVTT::Open(CDVDStreamInfo& hints, CDVDCodecOptions& options)
{
  m_pOverlay.reset();

  if (!Initialize())
    return false;

  if (!m_webvttHandler.Initialize())
    return false;

  // Extradata can be provided by Inputstream addons (e.g. inputstream.adaptive)
  if (hints.extradata)
  {
    std::string extradata{reinterpret_cast<char*>(hints.extradata.GetData()),
                          hints.extradata.GetSize()};
    if (extradata == "file")
    {
      // WebVTT data like single file are sent one time only,
      // then we have to prevent the flush performed by video seek
      m_allowFlush = false;
    }
    else if (extradata == "fmp4")
    {
      // WebVTT in MP4 encapsulated subtitles (ISO/IEC 14496-30:2014)
      m_isISOFormat = true;
    }
  }

  return true;
}

OverlayMessage COverlayCodecWebVTT::Decode(DemuxPacket* pPacket)
{
  if (!pPacket)
    return OverlayMessage::OC_ERROR;

  const char* data = reinterpret_cast<const char*>(pPacket->pData);
  std::vector<subtitleData> subtitleList;

  m_webvttHandler.Reset();

  // WebVTT subtitles has no relation with packet PTS then if
  // a period/chapter change happens (e.g. HLS streaming) VP can detect a discontinuity
  // and adjust the packet PTS by substracting the pts offset correction value,
  // so here we have to adjust WebVTT subtitles PTS by substracting it at same way
  m_webvttHandler.SetPeriodStart(pPacket->m_ptsOffsetCorrection * -1);

  if (m_isISOFormat)
  {
    double prevSubStopTime = 0.0;

    m_webvttHandler.DecodeStream(data, pPacket->iSize, pPacket->dts, &subtitleList,
                                 prevSubStopTime);

    // Set stop time to all previously added subtitles
    if (prevSubStopTime > 0)
    {
      for (auto& subId : m_previousSubIds)
      {
        ChangeSubtitleStopTime(subId, prevSubStopTime);
      }
      m_previousSubIds.clear();
    }
  }
  else
  {
    if (!m_webvttHandler.CheckSignature(data))
      return OverlayMessage::OC_ERROR;

    CCharArrayParser charArrayParser;
    charArrayParser.Reset(data, pPacket->iSize);
    std::string line;

    while (charArrayParser.ReadNextLine(line))
    {
      m_webvttHandler.DecodeLine(line, &subtitleList);
    }

    // We send an empty line to mark the end of the last Cue
    m_webvttHandler.DecodeLine("", &subtitleList);
  }

  for (auto& subData : subtitleList)
  {
    SUBTITLES::STYLE::subtitleOpts opts;
    opts.useMargins = subData.useMargins;
    opts.marginLeft = subData.marginLeft;
    opts.marginRight = subData.marginRight;
    opts.marginVertical = subData.marginVertical;

    int subId = AddSubtitle(subData.text, subData.startTime, subData.stopTime, &opts);

    if (m_isISOFormat)
      m_previousSubIds.emplace_back(subId);
  }

  return m_pOverlay ? OverlayMessage::OC_DONE : OverlayMessage::OC_OVERLAY;
}

void COverlayCodecWebVTT::Reset()
{
  Flush();
}

void COverlayCodecWebVTT::Flush()
{
  if (m_allowFlush)
  {
    m_pOverlay.reset();
    m_previousSubIds.clear();
    FlushSubtitles();
  }
}

std::shared_ptr<CDVDOverlay> COverlayCodecWebVTT::GetOverlay()
{
  if (m_pOverlay)
    return nullptr;
  m_pOverlay = CreateOverlay();
  m_pOverlay->SetOverlayContainerFlushable(m_allowFlush);
  m_pOverlay->SetForcedMargins(m_webvttHandler.IsForcedMargins());
  return m_pOverlay;
}
