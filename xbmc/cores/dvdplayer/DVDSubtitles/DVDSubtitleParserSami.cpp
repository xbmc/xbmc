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

#ifndef DVDSUBTITLES_DVDSUBTITLEPARSERSAMI_H_INCLUDED
#define DVDSUBTITLES_DVDSUBTITLEPARSERSAMI_H_INCLUDED
#include "DVDSubtitleParserSami.h"
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

#ifndef DVDSUBTITLES_UTILS_STRINGUTILS_H_INCLUDED
#define DVDSUBTITLES_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif

#ifndef DVDSUBTITLES_UTILS_URIUTILS_H_INCLUDED
#define DVDSUBTITLES_UTILS_URIUTILS_H_INCLUDED
#include "utils/URIUtils.h"
#endif

#ifndef DVDSUBTITLES_DVDSUBTITLETAGSAMI_H_INCLUDED
#define DVDSUBTITLES_DVDSUBTITLETAGSAMI_H_INCLUDED
#include "DVDSubtitleTagSami.h"
#endif


using namespace std;

CDVDSubtitleParserSami::CDVDSubtitleParserSami(CDVDSubtitleStream* pStream, const string& filename)
    : CDVDSubtitleParserText(pStream, filename)
{

}

CDVDSubtitleParserSami::~CDVDSubtitleParserSami()
{
  Dispose();
}

bool CDVDSubtitleParserSami::Open(CDVDStreamInfo &hints)
{
  if (!CDVDSubtitleParserText::Open())
    return false;

  char line[1024];

  CRegExp reg(true);
  if (!reg.RegComp("<SYNC START=([0-9]+)>"))
    return false;

  CStdString strFileName;
  CStdString strClassID;
  strFileName = URIUtils::GetFileName(m_filename);

  CDVDSubtitleTagSami TagConv;
  if (!TagConv.Init())
    return false;
  TagConv.LoadHead(m_pStream);
  if (TagConv.m_Langclass.size() >= 2)
  {
    for (unsigned int i = 0; i < TagConv.m_Langclass.size(); i++)
    {
      if (strFileName.find(TagConv.m_Langclass[i].Name, 9) == 9)
      {
        strClassID = TagConv.m_Langclass[i].ID;
        StringUtils::ToLower(strClassID);
        break;
      }
    }
  }
  const char *lang = NULL;
  if (!strClassID.empty())
    lang = strClassID.c_str();

  CDVDOverlayText* pOverlay = NULL;
  while (m_pStream->ReadLine(line, sizeof(line)))
  {
    if ((strlen(line) > 0) && (line[strlen(line) - 1] == '\r'))
      line[strlen(line) - 1] = 0;

    int pos = reg.RegFind(line);
    const char* text = line;
    if (pos > -1)
    {
      CStdString start = reg.GetMatch(1);
      if(pOverlay)
      {
        TagConv.ConvertLine(pOverlay, text, pos, lang);
        pOverlay->iPTSStopTime  = (double)atoi(start.c_str()) * DVD_TIME_BASE / 1000;
        pOverlay->Release();
        TagConv.CloseTag(pOverlay);
      }

      pOverlay = new CDVDOverlayText();
      pOverlay->Acquire(); // increase ref count with one so that we can hold a handle to this overlay

      pOverlay->iPTSStartTime = (double)atoi(start.c_str()) * DVD_TIME_BASE / 1000;
      pOverlay->iPTSStopTime  = DVD_NOPTS_VALUE;
      m_collection.Add(pOverlay);
      text += pos + reg.GetFindLen();
    }
    if(pOverlay)
      TagConv.ConvertLine(pOverlay, text, strlen(text), lang);
  }
  m_collection.Sort();
  return true;
}

