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
#include "DVDSubtitleStream.h"
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

void SamiTagConvertor::ConvertLine(CDVDOverlayText* pOverlay, const char* line, int len, const char* lang)
{
  CStdStringA strUTF8;
  strUTF8.assign(line, len);

  int pos = 0;
  int del_start = 0;
  while ((pos=m_tags->RegFind(strUTF8.c_str(), pos)) >= 0)
  {
    // Parse Tags
    CStdString fullTag = m_tags->GetMatch(0);
    fullTag.ToLower();
    strUTF8.erase(pos, fullTag.length());
    if (fullTag == "<b>")
    {
      tag_flag[FLAG_BOLD] = true;
      strUTF8.insert(pos, "[B]");
      pos += 3;
    }
    else if (fullTag == "</b>" && tag_flag[FLAG_BOLD])
    {
      tag_flag[FLAG_BOLD] = false;
      strUTF8.insert(pos, "[/B]");
      pos += 4;
    }
    else if (fullTag == "<i>")
    {
      tag_flag[FLAG_ITALIC] = true;
      strUTF8.insert(pos, "[I]");
      pos += 3;
    }
    else if (fullTag == "</i>" && tag_flag[FLAG_ITALIC])
    {
      tag_flag[FLAG_ITALIC] = false;
      strUTF8.insert(pos, "[/I]");
      pos += 4;
    }
    else if (fullTag == "</font>" && tag_flag[FLAG_COLOR])
    {
      tag_flag[FLAG_COLOR] = false;
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
          tag_flag[FLAG_COLOR] = true;
          CStdString tempColorTag = "[COLOR ";
          if (tagOptionValue[0] == '#')
          {
            tagOptionValue.erase(0, 1);
            tempColorTag += "FF";
          }
          else if( tagOptionValue.size() == 6 )
          {
            bool bHex = true;
            for( int i=0 ; i<6 ; i++ )
            {
              char temp = tagOptionValue[i];
              if( !(('0' <= temp && temp <= '9') ||
                ('a' <= temp && temp <= 'f') ||
                ('A' <= temp && temp <= 'F') ))
              {
                bHex = false;
                break;
              }
            }
            if( bHex ) tempColorTag += "FF";
          }
          tempColorTag += tagOptionValue;
          tempColorTag += "]";
          strUTF8.insert(pos, tempColorTag);
          pos += tempColorTag.length();
        }
      }
    }
    else if (lang && (fullTag.Left(3) == "<p "))
    {
      int pos2 = 3;
      while ((pos2 = m_tagOptions->RegFind(fullTag.c_str(), pos2)) >= 0)
      {
        CStdString tagOptionName = m_tagOptions->GetMatch(1);
        CStdString tagOptionValue = m_tagOptions->GetMatch(2);
        pos2 += tagOptionName.length() + tagOptionValue.length();
        if (tagOptionName == "class")
        {
          if (tag_flag[FLAG_LANGUAGE])
          {
            strUTF8.erase(del_start, pos - del_start);
            pos = del_start;
          }
          if (!tagOptionValue.Compare(lang))
          {
            tag_flag[FLAG_LANGUAGE] = false;
          }
          else
          {
            tag_flag[FLAG_LANGUAGE] = true;
            del_start = pos;
          }
          break;
        }
      }
    }
    else if (fullTag == "</p>" && tag_flag[FLAG_LANGUAGE])
    {
      strUTF8.erase(del_start, pos - del_start);
      pos = del_start;
      tag_flag[FLAG_LANGUAGE] = false;
    }
    else if (fullTag == "<br>" && !strUTF8.IsEmpty())
    {
      strUTF8.Insert(pos, "\n");
      pos += 1;
    }
  }

  if(tag_flag[FLAG_LANGUAGE])
    strUTF8.erase(del_start);

  if (strUTF8.IsEmpty())
    return;

  if( strUTF8[strUTF8.size()-1] == '\n' )
    strUTF8.Delete(strUTF8.size()-1);

  // add a new text element to our container
  pOverlay->AddElement(new CDVDOverlayText::CElementText(strUTF8.c_str()));
}

void SamiTagConvertor::CloseTag(CDVDOverlayText* pOverlay)
{
  if (tag_flag[FLAG_BOLD])
  {
    pOverlay->AddElement(new CDVDOverlayText::CElementText("[/B]"));
    tag_flag[FLAG_BOLD] = false;
  }
  if (tag_flag[FLAG_ITALIC])
  {
    pOverlay->AddElement(new CDVDOverlayText::CElementText("[/I]"));
    tag_flag[FLAG_ITALIC] = false;
  }
  if (tag_flag[FLAG_COLOR])
  {
    pOverlay->AddElement(new CDVDOverlayText::CElementText("[/COLOR]"));
    tag_flag[FLAG_COLOR] = false;
  }
  tag_flag[FLAG_LANGUAGE] = false;
}

void SamiTagConvertor::LoadHead(CDVDSubtitleStream* samiStream)
{
  char line[1024];
  bool inSTYLE = false;
  CRegExp reg(true);
  if (!reg.RegComp("\\.([a-z]+)[ \t]*\\{[ \t]*name:([^;]*?);[ \t]*lang:([^;]*?);[ \t]*SAMIType:([^;]*?);[ \t]*\\}"))
    return;

  while (samiStream->ReadLine(line, sizeof(line)))
  {
    if (!strnicmp(line, "<BODY>", 6))
      break;
    if (inSTYLE)
    {
      if (!strnicmp(line, "</STYLE>", 8))
        break;
      else
      {
        if (reg.RegFind(line) > -1)
        {
          SLangclass lc;
          lc.ID = reg.GetMatch(1);
          lc.Name = reg.GetMatch(2);
          lc.Lang = reg.GetMatch(3);
          lc.SAMIType = reg.GetMatch(4);
          lc.Name.Trim();
          lc.Lang.Trim();
          lc.SAMIType.Trim();
          m_Langclass.push_back(lc);
        }
      }
    }
    else
    {
      if (!strnicmp(line, "<STYLE TYPE=\"text/css\">", 23))
        inSTYLE = true;
    }
  }
}

