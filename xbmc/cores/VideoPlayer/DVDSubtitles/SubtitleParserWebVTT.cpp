/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SubtitleParserWebVTT.h"

#include "DVDCodecs/Overlay/DVDOverlay.h"
#include "SubtitlesStyle.h"
#include "cores/VideoPlayer/DVDSubtitles/webvtt/WebVTTHandler.h"
#include "utils/CharArrayParser.h"

#include <vector>

using namespace KODI;

CSubtitleParserWebVTT::CSubtitleParserWebVTT(std::unique_ptr<CDVDSubtitleStream>&& pStream,
                                             const std::string& strFile)
  : CDVDSubtitleParserText(std::move(pStream), strFile, "WebVTT Subtitle Parser")
{
}

bool CSubtitleParserWebVTT::Open(CDVDStreamInfo& hints)
{
  if (!CDVDSubtitleParserText::Open())
    return false;

  if (!Initialize())
    return false;

  CWebVTTHandler m_webvttHandler;

  if (!m_webvttHandler.Initialize())
    return false;

  std::string line;

  // WebVTT signature is according to spec length-variable and stops
  // with two consecutive line terminators.
  // Get the first line to check WebVTT signature.
  if(!(m_pStream->ReadLine(line) && m_webvttHandler.CheckSignature(line)))
    return false;

  // Start decoding all lines
  std::vector<subtitleData> subtitleList;

  while (m_pStream->ReadLine(line))
  {
    m_webvttHandler.DecodeLine(line, &subtitleList);
  }

  // We send an empty line to mark the end of the last Cue
  m_webvttHandler.DecodeLine("", &subtitleList);

  // Send decoded lines to the renderer
  for (auto& subData : subtitleList)
  {
    SUBTITLES::STYLE::subtitleOpts opts;
    opts.useMargins = subData.useMargins;
    opts.marginLeft = subData.marginLeft;
    opts.marginRight = subData.marginRight;
    opts.marginVertical = subData.marginVertical;

    AddSubtitle(subData.text, subData.startTime, subData.stopTime, &opts);
  }

  std::shared_ptr<CDVDOverlay> overlay = CreateOverlay();
  overlay->SetForcedMargins(m_webvttHandler.IsForcedMargins());
  m_collection.Add(overlay);

  return true;
}
