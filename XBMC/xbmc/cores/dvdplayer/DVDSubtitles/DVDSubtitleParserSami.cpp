/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
 
#include "stdafx.h"
#include "DVDSubtitleParserSami.h"
#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "DVDClock.h"
#include "utils/RegExp.h"
#include "DVDStreamInfo.h"

using namespace std;

CDVDSubtitleParserSami::CDVDSubtitleParserSami(CDVDSubtitleStream* stream, const string& filename)
    : CDVDSubtitleParser(stream, filename)
{

}

CDVDSubtitleParserSami::~CDVDSubtitleParserSami()
{
  Dispose();
}

bool CDVDSubtitleParserSami::Open(CDVDStreamInfo &hints)
{
  if (!m_pStream->Open(m_strFileName))
    return false;

  char line[1024];
  char text[1024];

  CRegExp reg(true);
  if (!reg.RegComp("<SYNC START=([0-9]+)>"))
    return false;

  bool reuse=false;
  while (reuse || m_pStream->ReadLine(line, sizeof(line)))
  {
    if (reg.RegFind(line) > -1)
    {
      char* startFrame = reg.GetReplaceString("\\1");
      m_pStream->ReadLine(text,sizeof(text));
      m_pStream->ReadLine(line,sizeof(line));
      if (reg.RegFind(line) > -1)
      {
        char* endFrame   = reg.GetReplaceString("\\1");
      
        CDVDOverlayText* pOverlay = new CDVDOverlayText();
        pOverlay->Acquire(); // increase ref count with one so that we can hold a handle to this overlay

        pOverlay->iPTSStartTime = atoi(startFrame)*DVD_TIME_BASE/1000; 
        pOverlay->iPTSStopTime  = atoi(endFrame)*DVD_TIME_BASE/1000;

        free(endFrame);

        CStdStringW strUTF16;
        CStdStringA strUTF8;
        g_charsetConverter.subtitleCharsetToW(text, strUTF16);
        g_charsetConverter.wToUTF8(strUTF16, strUTF8);
        if (strUTF8.IsEmpty())
          continue;
        // add a new text element to our container
        pOverlay->AddElement(new CDVDOverlayText::CElementText(strUTF8.c_str()));
        reuse = true;
      
        m_collection.Add(pOverlay);
      }
      free(startFrame);
    }
    else
      reuse = false;
  }

  return true;
}

void CDVDSubtitleParserSami::Dispose()
{
  m_collection.Clear();
}

void CDVDSubtitleParserSami::Reset()
{
  m_collection.Reset();
}

// parse exactly one subtitle
CDVDOverlay* CDVDSubtitleParserSami::Parse(double iPts)
{
  return  m_collection.Get(iPts);;
}

