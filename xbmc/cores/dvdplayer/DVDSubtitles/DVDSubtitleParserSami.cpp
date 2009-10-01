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

#include "DVDSubtitleParserSami.h"
#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "DVDClock.h"
#include "utils/RegExp.h"
#include "DVDStreamInfo.h"
#include "StdString.h"
#include "SamiTagConvertor.h"

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

  SamiTagConvertor TagConv;
  if (!TagConv.Init())
    return false;

  CDVDOverlayText* pOverlay = NULL;
  while (m_stringstream.getline(line, sizeof(line)))
  {
    int pos = reg.RegFind(line);
    const char* text = line;
    if (pos > -1)
    {
      CStdString start = reg.GetMatch(1);
      if(pOverlay)
      {
        TagConv.ConvertLine(pOverlay, text, pos);
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
      TagConv.ConvertLine(pOverlay, text, strlen(text));
  }

  return true;
}

