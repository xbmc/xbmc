/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDSubtitleParserMPL2.h"

#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "DVDStreamInfo.h"
#include "DVDSubtitleTagMicroDVD.h"
#include "cores/VideoPlayer/Interface/Addon/TimingConstants.h"
#include "utils/RegExp.h"

CDVDSubtitleParserMPL2::CDVDSubtitleParserMPL2(std::unique_ptr<CDVDSubtitleStream> && stream, const std::string& filename)
    : CDVDSubtitleParserText(std::move(stream), filename), m_framerate(DVD_TIME_BASE / 10.0)
{

}

CDVDSubtitleParserMPL2::~CDVDSubtitleParserMPL2()
{
  Dispose();
}

bool CDVDSubtitleParserMPL2::Open(CDVDStreamInfo &hints)
{
  if (!CDVDSubtitleParserText::Open())
    return false;

  // MPL2 is time-based, with 0.1s accuracy
  m_framerate = DVD_TIME_BASE / 10.0;

  char line[1024];

  CRegExp reg;
  if (!reg.RegComp("\\[([0-9]+)\\]\\[([0-9]+)\\]"))
    return false;
  CDVDSubtitleTagMicroDVD TagConv;

  while (m_pStream->ReadLine(line, sizeof(line)))
  {
    if ((strlen(line) > 0) && (line[strlen(line) - 1] == '\r'))
      line[strlen(line) - 1] = 0;

    int pos = reg.RegFind(line);
    if (pos > -1)
    {
      const char* text = line + pos + reg.GetFindLen();
      std::string startFrame(reg.GetMatch(1));
      std::string endFrame  (reg.GetMatch(2));
      CDVDOverlayText* pOverlay = new CDVDOverlayText();
      pOverlay->Acquire(); // increase ref count with one so that we can hold a handle to this overlay

      pOverlay->iPTSStartTime = m_framerate * atoi(startFrame.c_str());
      pOverlay->iPTSStopTime  = m_framerate * atoi(endFrame.c_str());

      TagConv.ConvertLine(pOverlay, text, strlen(text));
      m_collection.Add(pOverlay);
    }
  }

  return true;
}

