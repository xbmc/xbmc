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

#include <cstring>
#include <memory>
#include <sstream>

COverlayCodecWebVTT::COverlayCodecWebVTT() : CDVDOverlayCodec("WebVTT Subtitle Decoder")
{
  m_pOverlay = nullptr;
}

COverlayCodecWebVTT::~COverlayCodecWebVTT()
{
  Dispose();
}

bool COverlayCodecWebVTT::Open(CDVDStreamInfo& hints, CDVDCodecOptions& options)
{
  Dispose();

  if (!Initialize())
    return false;

  if (!m_webvttHandler.Initialize())
    return false;

  // "fmp4" extradata is provided by InputStream Adaptive to identify
  // WebVTT in MP4 encapsulated subtitles (ISO/IEC 14496-30:2014)
  if (hints.extradata && std::strncmp(static_cast<const char*>(hints.extradata), "fmp4", 4) == 0)
  {
    m_isISOFormat = true;
  }

  return true;
}

void COverlayCodecWebVTT::Dispose()
{
  if (m_pOverlay)
  {
    m_pOverlay->Release();
    m_pOverlay = nullptr;
  }
}

OverlayMessage COverlayCodecWebVTT::Decode(DemuxPacket* pPacket)
{
  if (!pPacket)
    return OverlayMessage::OC_ERROR;

  char* data = reinterpret_cast<char*>(pPacket->pData);
  std::vector<subtitleData> subtitleList;

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

    std::string strData(data, pPacket->iSize);
    std::istringstream streamData(strData);

    std::string line;
    while (std::getline(streamData, line))
    {
      m_webvttHandler.DecodeLine(line, &subtitleList);
    }

    // It is mandatory to decode an empty line to mark the end
    // of the last WebVTT Cue in case it is missing
    m_webvttHandler.DecodeLine("", &subtitleList);
  }

  for (auto& subData : subtitleList)
  {
    KODI::SUBTITLES::subtitleOpts opts;
    opts.useMargins = subData.useMargins;
    opts.marginLeft = subData.marginLeft;
    opts.marginRight = subData.marginRight;
    opts.marginVertical = subData.marginVertical;

    int subId = AddSubtitle(subData.text.c_str(), subData.startTime, subData.stopTime, &opts);

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
  if (m_isISOFormat)
  {
    m_previousSubIds.clear();
    FlushSubtitles();
  }
}

CDVDOverlay* COverlayCodecWebVTT::GetOverlay()
{
  if (m_pOverlay)
    return nullptr;
  m_pOverlay = CreateOverlay();
  m_pOverlay->SetOverlayContainerFlushable(false);
  return m_pOverlay->Acquire();
}
