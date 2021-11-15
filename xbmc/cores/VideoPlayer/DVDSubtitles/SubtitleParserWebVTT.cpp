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
#include "utils/StringUtils.h"

#include <vector>

CSubtitleParserWebVTT::CSubtitleParserWebVTT(std::unique_ptr<CDVDSubtitleStream>&& pStream,
                                             const std::string& strFile)
  : CDVDSubtitleParserText(std::move(pStream), strFile, "WebVTT Subtitle Parser")
{
}

CSubtitleParserWebVTT::~CSubtitleParserWebVTT()
{
  Dispose();
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

  char line[1024];

  // Get the first chars to check WebVTT signature
  if (m_pStream->Read(line, 10) > 0 && !m_webvttHandler.CheckSignature(line))
    return false;
  m_pStream->Seek(0, SEEK_SET);

  // Start decoding all lines
  std::string strLine;
  std::vector<subtitleData> subtitleList;

  while (m_pStream->ReadLine(line, sizeof(line)))
  {
    strLine.assign(line);
    m_webvttHandler.DecodeLine(strLine, &subtitleList);
  }

  // "ReadLine" ignore the last empty line of the file and could also be missing,
  // it is mandatory to send it to mark the end of the last Cue
  strLine.clear();
  m_webvttHandler.DecodeLine(strLine, &subtitleList);

  // Send decoded lines to the renderer
  for (auto& subData : subtitleList)
  {
    KODI::SUBTITLES::subtitleOpts opts;
    opts.useMargins = subData.useMargins;
    opts.marginLeft = subData.marginLeft;
    opts.marginRight = subData.marginRight;
    opts.marginVertical = subData.marginVertical;

    AddSubtitle(subData.text.c_str(), subData.startTime, subData.stopTime, &opts);
  }

  CDVDOverlay* overlay = CreateOverlay();
  overlay->SetForcedMargins(true);
  m_collection.Add(overlay);

  return true;
}

void CSubtitleParserWebVTT::Dispose()
{
  CDVDSubtitleParserCollection::Dispose();
}
