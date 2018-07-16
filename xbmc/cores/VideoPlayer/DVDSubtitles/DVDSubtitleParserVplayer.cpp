/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDSubtitleParserVplayer.h"
#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "cores/VideoPlayer/Interface/Addon/TimingConstants.h"
#include "utils/RegExp.h"

CDVDSubtitleParserVplayer::CDVDSubtitleParserVplayer(std::unique_ptr<CDVDSubtitleStream> && pStream, const std::string& strFile)
    : CDVDSubtitleParserText(std::move(pStream), strFile), m_framerate(DVD_TIME_BASE)
{
}

CDVDSubtitleParserVplayer::~CDVDSubtitleParserVplayer()
{
  Dispose();
}

bool CDVDSubtitleParserVplayer::Open(CDVDStreamInfo &hints)
{
  if (!CDVDSubtitleParserText::Open())
    return false;

  // Vplayer subtitles have 1-second resolution
  m_framerate = DVD_TIME_BASE;

  // Vplayer subtitles don't have StopTime, so we use following subtitle's StartTime
  // for that, unless gap was more than 4 seconds. Then we set subtitle duration
  // for 4 seconds, to not have text hanging around in silent scenes...
  int iDefaultDuration = 4 * (int)m_framerate;

  char line[1024];

  CRegExp reg;
  if (!reg.RegComp("([0-9]+):([0-9]+):([0-9]+):([^|]*?)(\\|([^|]*?))?$"))
    return false;

  CDVDOverlayText* pPrevOverlay = NULL;

  while (m_pStream->ReadLine(line, sizeof(line)))
  {
    if (reg.RegFind(line) > -1)
    {
      std::string hour(reg.GetMatch(1));
      std::string min (reg.GetMatch(2));
      std::string sec (reg.GetMatch(3));
      std::string lines[3];
      lines[0] = reg.GetMatch(4);
      lines[1] = reg.GetMatch(6);
      lines[2] = reg.GetMatch(8);

      CDVDOverlayText* pOverlay = new CDVDOverlayText();
      pOverlay->Acquire(); // increase ref count with one so that we can hold a handle to this overlay

      pOverlay->iPTSStartTime = m_framerate * (3600*atoi(hour.c_str()) + 60*atoi(min.c_str()) + atoi(sec.c_str()));

      // set StopTime for previous overlay
      if (pPrevOverlay)
      {
        if ( (pOverlay->iPTSStartTime - pPrevOverlay->iPTSStartTime) < iDefaultDuration)
          pPrevOverlay->iPTSStopTime = pOverlay->iPTSStartTime;
        else
          pPrevOverlay->iPTSStopTime = pPrevOverlay->iPTSStartTime + iDefaultDuration;
      }
      pPrevOverlay = pOverlay;
      for (int i = 0; i < 3 && !lines[i].empty(); i++)
          pOverlay->AddElement(new CDVDOverlayText::CElementText(lines[i].c_str()));

      m_collection.Add(pOverlay);
    }

    // set StopTime for the last subtitle
    if (pPrevOverlay)
      pPrevOverlay->iPTSStopTime = pPrevOverlay->iPTSStartTime + iDefaultDuration;
  }

  return true;
}

