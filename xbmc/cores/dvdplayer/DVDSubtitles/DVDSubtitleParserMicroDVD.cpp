/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#ifndef DVDSUBTITLES_DVDSUBTITLEPARSERMICRODVD_H_INCLUDED
#define DVDSUBTITLES_DVDSUBTITLEPARSERMICRODVD_H_INCLUDED
#include "DVDSubtitleParserMicroDVD.h"
#endif

#ifndef DVDSUBTITLES_DVDCODECS_OVERLAY_DVDOVERLAYTEXT_H_INCLUDED
#define DVDSUBTITLES_DVDCODECS_OVERLAY_DVDOVERLAYTEXT_H_INCLUDED
#include "DVDCodecs/Overlay/DVDOverlayText.h"
#endif

#ifndef DVDSUBTITLES_DVDCLOCK_H_INCLUDED
#define DVDSUBTITLES_DVDCLOCK_H_INCLUDED
#include "DVDClock.h"
#endif

#ifndef DVDSUBTITLES_UTILS_REGEXP_H_INCLUDED
#define DVDSUBTITLES_UTILS_REGEXP_H_INCLUDED
#include "utils/RegExp.h"
#endif

#ifndef DVDSUBTITLES_DVDSTREAMINFO_H_INCLUDED
#define DVDSUBTITLES_DVDSTREAMINFO_H_INCLUDED
#include "DVDStreamInfo.h"
#endif

#ifndef DVDSUBTITLES_UTILS_STDSTRING_H_INCLUDED
#define DVDSUBTITLES_UTILS_STDSTRING_H_INCLUDED
#include "utils/StdString.h"
#endif

#ifndef DVDSUBTITLES_UTILS_LOG_H_INCLUDED
#define DVDSUBTITLES_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif

#ifndef DVDSUBTITLES_DVDSUBTITLETAGMICRODVD_H_INCLUDED
#define DVDSUBTITLES_DVDSUBTITLETAGMICRODVD_H_INCLUDED
#include "DVDSubtitleTagMicroDVD.h"
#endif


using namespace std;

CDVDSubtitleParserMicroDVD::CDVDSubtitleParserMicroDVD(CDVDSubtitleStream* stream, const string& filename)
    : CDVDSubtitleParserText(stream, filename), m_framerate( DVD_TIME_BASE / 25.0 )
{

}

CDVDSubtitleParserMicroDVD::~CDVDSubtitleParserMicroDVD()
{
  Dispose();
}

bool CDVDSubtitleParserMicroDVD::Open(CDVDStreamInfo &hints)
{
  if (!CDVDSubtitleParserText::Open())
    return false;

  CLog::Log(LOGDEBUG, "%s - framerate %d:%d", __FUNCTION__, hints.fpsrate, hints.fpsscale);
  if (hints.fpsscale > 0 && hints.fpsrate > 0)
  {
    m_framerate = (double)hints.fpsscale / (double)hints.fpsrate;
    m_framerate *= DVD_TIME_BASE;
  }
  else
    m_framerate = DVD_TIME_BASE / 25.0;

  char line[1024];

  CRegExp reg;
  if (!reg.RegComp("\\{([0-9]+)\\}\\{([0-9]+)\\}"))
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

