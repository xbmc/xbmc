/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDSubtitleParserVplayer.h"
#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "DVDClock.h"
#include "utils/RegExp.h"
#include "utils/StdString.h"

using namespace std;

CDVDSubtitleParserVplayer::CDVDSubtitleParserVplayer(CDVDSubtitleStream* pStream, const string& strFile)
    : CDVDSubtitleParserText(pStream, strFile), m_framerate(DVD_TIME_BASE)
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
      std::string hour = reg.GetReplaceString("\\1");
      std::string min  = reg.GetReplaceString("\\2");
      std::string sec  = reg.GetReplaceString("\\3");
      std::string lines[3];
      lines[0] = reg.GetReplaceString("\\4");
      lines[1] = reg.GetReplaceString("\\6");
      lines[2] = reg.GetReplaceString("\\8");

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

