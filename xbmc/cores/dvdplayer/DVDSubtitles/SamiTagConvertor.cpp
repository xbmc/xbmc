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

#include "SamiTagConvertor.h"
#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "utils/RegExp.h"

SamiTagConvertor::~SamiTagConvertor()
{
  delete m_tags;
  delete m_tagOptions;
}

bool SamiTagConvertor::Init()
{
  m_tags = new CRegExp(true);
  if (!m_tags->RegComp("(<[^>]*>)"))
    return false;

  m_tagOptions = new CRegExp(true);
  if (!m_tagOptions->RegComp("([a-z]+)[ \t]*=[ \t]*(?:[\"'])?([^\"'> ]+)(?:[\"'])?(?:>)?"))
    return false;

  return true;
}

void SamiTagConvertor::ConvertLine(CDVDOverlayText* pOverlay, const char* line, int len)
{
  CStdStringA strUTF8;
  strUTF8.assign(line, len);
  strUTF8.Replace("\n", "");

  int pos = 0;
  while ((pos=m_tags->RegFind(strUTF8.c_str(), pos)) >= 0)
  {
    // Parse Tags
    CStdString fullTag = m_tags->GetMatch(0);
    fullTag.ToLower();
    strUTF8.erase(pos, fullTag.length());
    if (fullTag == "<b>")
    {
      tag_flag[0] = true;
      strUTF8.insert(pos, "[B]");
      pos += 3;
    }
    else if (fullTag == "</b>" && tag_flag[0])
    {
      tag_flag[0] = false;
      strUTF8.insert(pos, "[/B]");
      pos += 4;
    }
    else if (fullTag == "<i>")
    {
      tag_flag[1] = true;
      strUTF8.insert(pos, "[I]");
      pos += 3;
    }
    else if (fullTag == "</i>" && tag_flag[1])
    {
      tag_flag[1] = false;
      strUTF8.insert(pos, "[/I]");
      pos += 4;
    }
    else if (fullTag == "</font>" && tag_flag[2])
    {
      tag_flag[2] = false;
      strUTF8.insert(pos, "[/COLOR]");
      pos += 8;
    }
    else if (fullTag.Left(5) == "<font")
    {
      int pos2 = 5;
      while ((pos2 = m_tagOptions->RegFind(fullTag.c_str(), pos2)) >= 0)
      {
        CStdString tagOptionName = m_tagOptions->GetMatch(1);
        CStdString tagOptionValue = m_tagOptions->GetMatch(2);
        pos2 += tagOptionName.length() + tagOptionValue.length();
        if (tagOptionName == "color")
        {
          tag_flag[2] = true;
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

  if (strUTF8.IsEmpty())
    return;
  // add a new text element to our container
  pOverlay->AddElement(new CDVDOverlayText::CElementText(strUTF8.c_str()));
}

void SamiTagConvertor::CloseTag(CDVDOverlayText* pOverlay)
{
  if (tag_flag[0])
  {
    pOverlay->AddElement(new CDVDOverlayText::CElementText("[/B]"));
    tag_flag[0] = false;
  }
  if (tag_flag[1])
  {
    pOverlay->AddElement(new CDVDOverlayText::CElementText("[/I]"));
    tag_flag[1] = false;
  }
  if (tag_flag[2])
  {
    pOverlay->AddElement(new CDVDOverlayText::CElementText("[/COLOR]"));
    tag_flag[2] = false;
  }
}
