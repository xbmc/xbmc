/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDOverlayCodecText.h"

#include "DVDCodecs/DVDCodecs.h"
#include "DVDOverlayText.h"
#include "DVDStreamInfo.h"
#include "cores/VideoPlayer/DVDSubtitles/DVDSubtitleTagSami.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "utils/log.h"

#include <memory>

CDVDOverlayCodecText::CDVDOverlayCodecText() : CDVDOverlayCodec("Text Subtitle Decoder")
{
  m_pOverlay = nullptr;
}

CDVDOverlayCodecText::~CDVDOverlayCodecText()
{
  Dispose();
}

bool CDVDOverlayCodecText::Open(CDVDStreamInfo& hints, CDVDCodecOptions& options)
{
  if (hints.codec != AV_CODEC_ID_TEXT && hints.codec != AV_CODEC_ID_SSA &&
      hints.codec != AV_CODEC_ID_SUBRIP)
    return false;

  m_codecId = hints.codec;

  Dispose();

  return Initialize();
}

void CDVDOverlayCodecText::Dispose()
{
  if (m_pOverlay)
  {
    m_pOverlay->Release();
    m_pOverlay = nullptr;
  }
}

OverlayMessage CDVDOverlayCodecText::Decode(DemuxPacket* pPacket)
{
  if (!pPacket)
    return OverlayMessage::OC_ERROR;

  uint8_t* data = pPacket->pData;
  char* start = (char*)data;
  char* end = (char*)(data + pPacket->iSize);

  if (m_codecId == AV_CODEC_ID_SSA)
  {
    // currently just skip the prefixed ssa fields (8 fields)
    int nFieldCount = 8;
    while (nFieldCount > 0 && start < end)
    {
      if (*start == ',')
        nFieldCount--;

      start++;
    }
  }

  std::string text(start, end);
  double PTSStartTime = 0;
  double PTSStopTime = 0;

  CDVDOverlayCodec::GetAbsoluteTimes(PTSStartTime, PTSStopTime, pPacket);
  CDVDSubtitleTagSami TagConv;

  if (TagConv.Init())
  {
    TagConv.ConvertLine(text);
    TagConv.CloseTag(text);
    AddSubtitle(text, PTSStartTime, PTSStopTime);
  }
  else
    CLog::Log(LOGERROR, "{} - Failed to initialize tag converter", __FUNCTION__);

  return m_pOverlay ? OverlayMessage::OC_DONE : OverlayMessage::OC_OVERLAY;
}

void CDVDOverlayCodecText::PostProcess(std::string& text)
{
  // The data that come from InputStream could contains \r chars
  // we have to remove them all because it causes to display empty box "tofu"
  StringUtils::Replace(text, "\r", "");
  CSubtitlesAdapter::PostProcess(text);
}

void CDVDOverlayCodecText::Reset()
{
  Dispose();
  Flush();
}

void CDVDOverlayCodecText::Flush()
{
  if (m_pOverlay)
  {
    m_pOverlay->Release();
    m_pOverlay = nullptr;
  }

  FlushSubtitles();
}

CDVDOverlay* CDVDOverlayCodecText::GetOverlay()
{
  if (m_pOverlay)
    return nullptr;
  m_pOverlay = CreateOverlay();
  return m_pOverlay->Acquire();
}
