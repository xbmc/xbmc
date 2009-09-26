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

#include "DVDSubtitleParserSubrip.h"
#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "DVDClock.h"
#include "StdString.h"
#include "utils/RegExp.h"

using namespace std;

CDVDSubtitleParserSubrip::CDVDSubtitleParserSubrip(CDVDSubtitleStream* pStream, const string& strFile)
    : CDVDSubtitleParserText(pStream, strFile)
{
}

CDVDSubtitleParserSubrip::~CDVDSubtitleParserSubrip()
{
  Dispose();
}

bool CDVDSubtitleParserSubrip::Open(CDVDStreamInfo &hints)
{
  if (!CDVDSubtitleParserText::Open())
    return false;

  CRegExp tags(true);
  if (!tags.RegComp("(<[^>]*>)"))
    return false;

  CRegExp tagOptions(true);
  if (!tagOptions.RegComp("([a-z]+)[ \t]*=[ \t]*(?:[\"'])?([^\"'> ]+)(?:[\"'])?(?:>)?"))
    return false;

  char line[1024];
  char* pLineStart;

  while (m_stringstream.getline(line, sizeof(line)))
  {
    pLineStart = line;

    // trim
    while (pLineStart[0] == ' ') pLineStart++;

    if (strlen(pLineStart) > 0)
    {
      char sep;
      int hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2;
      int c = sscanf(line, "%d%c%d%c%d%c%d --> %d%c%d%c%d%c%d\n",
                     &hh1, &sep, &mm1, &sep, &ss1, &sep, &ms1,
                     &hh2, &sep, &mm2, &sep, &ss2, &sep, &ms2);

      if (c == 1)
      {
        // numbering, skip it
      }
      else if (c == 14) // time info
      {
        CDVDOverlayText* pOverlay = new CDVDOverlayText();
        pOverlay->Acquire(); // increase ref count with one so that we can hold a handle to this overlay

        pOverlay->iPTSStartTime = ((double)(((hh1 * 60 + mm1) * 60) + ss1) * 1000 + ms1) * (DVD_TIME_BASE / 1000);
        pOverlay->iPTSStopTime  = ((double)(((hh2 * 60 + mm2) * 60) + ss2) * 1000 + ms2) * (DVD_TIME_BASE / 1000);

        bool tag_b = false;
        bool tag_i = false;
        bool tag_color = false;
        while (m_stringstream.getline(line, sizeof(line)))
        {

          if ((strlen(line) > 0) && (line[strlen(line) - 1] == '\r'))
            line[strlen(line) - 1] = 0;

          pLineStart = line;
          // trim
          while (pLineStart[0] == ' ') pLineStart++;

          // empty line, next subtitle is about to start
          if (strlen(pLineStart) <= 0) break;

          int pos = 0;
          CStdStringA strUTF8;
          strUTF8.assign(line, strlen(line));
          while ((pos=tags.RegFind(strUTF8.c_str(), pos)) >= 0)
          {
            // Parse Tags
            CStdString fullTag = tags.GetMatch(0);
			fullTag.ToLower();
            strUTF8.erase(pos, fullTag.length());
            if (fullTag == "<b>")
            {
              tag_b = true;
              strUTF8.insert(pos, "[B]");
              pos += 3;
            }
            else if (fullTag == "</b>" && tag_b)
            {
              tag_b = false;
              strUTF8.insert(pos, "[/B]");
              pos += 4;
            }
            else if (fullTag == "<i>")
            {
              tag_i = true;
              strUTF8.insert(pos, "[I]");
              pos += 3;
            }
            else if (fullTag == "</i>" && tag_i)
            {
              tag_i = false;
              strUTF8.insert(pos, "[/I]");
              pos += 4;
            }
            else if (fullTag == "</font>" && tag_color)
            {
              tag_color = false;
              strUTF8.insert(pos, "[/COLOR]");
              pos += 8;
            }
            else if (fullTag.Left(5) == "<font")
            {
              int pos2 = 5;
              while ((pos2 = tagOptions.RegFind(fullTag.c_str(), pos2)) >= 0)
              {
                CStdString tagOptionName = tagOptions.GetMatch(1);
                CStdString tagOptionValue = tagOptions.GetMatch(2);
                pos2 += tagOptionName.length() + tagOptionValue.length();
                if (tagOptionName == "color")
                {
                  tag_color = true;
                  CStdString tempColorTag = "[COLOR ";
                  if (tagOptionValue[0] == '#')
                  {
                    tagOptionValue.erase(0, 1);
                    tempColorTag += "FF";
                  }
                  tempColorTag += tagOptionValue;
                  tempColorTag += "]";
                  strUTF8.insert(pos, tempColorTag);
                  pos += tempColorTag.length();
                }
              }
            }
          }
          // add a new text element to our container
          pOverlay->AddElement(new CDVDOverlayText::CElementText(strUTF8.c_str()));
        }
        if (tag_color)
          pOverlay->AddElement(new CDVDOverlayText::CElementText("[/COLOR]"));
        if (tag_b)
          pOverlay->AddElement(new CDVDOverlayText::CElementText("[/B]"));
        if (tag_i)
          pOverlay->AddElement(new CDVDOverlayText::CElementText("[/I]"));
        m_collection.Add(pOverlay);
      }
    }
  }

  return true;
}

