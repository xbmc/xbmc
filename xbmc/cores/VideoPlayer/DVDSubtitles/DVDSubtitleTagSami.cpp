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

#include "DVDSubtitleTagSami.h"
#include "DVDSubtitleStream.h"
#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"

CDVDSubtitleTagSami::~CDVDSubtitleTagSami()
{
  delete m_tags;
  delete m_tagOptions;
}

bool CDVDSubtitleTagSami::Init()
{
  delete m_tags;
  delete m_tagOptions;
  m_tags = new CRegExp(true);
  if (!m_tags->RegComp("(<[^>]*>|\\{[^\\}]*\\})"))
    return false;

  m_tagOptions = new CRegExp(true);
  if (!m_tagOptions->RegComp("([a-z]+)[ \t]*=[ \t]*(?:[\"'])?([^\"'> ]+)(?:[\"'])?(?:>)?"))
    return false;

  return true;
}

void CDVDSubtitleTagSami::ConvertLine(CDVDOverlayText* pOverlay, const char* line, int len, const char* lang)
{
  std::string strUTF8;
  strUTF8.assign(line, len);
  StringUtils::Trim(strUTF8);

  int pos = 0;
  int del_start = 0;
  while ((pos=m_tags->RegFind(strUTF8.c_str(), pos)) >= 0)
  {
    // Parse Tags
    std::string fullTag = m_tags->GetMatch(0);
    StringUtils::ToLower(fullTag);
    strUTF8.erase(pos, fullTag.length());
    if (fullTag == "<b>" || fullTag == "{\\b1}")
    {
      m_flag[FLAG_BOLD] = true;
      strUTF8.insert(pos, "[B]");
      pos += 3;
    }
    else if ((fullTag == "</b>" || fullTag == "{\\b0}") && m_flag[FLAG_BOLD])
    {
      m_flag[FLAG_BOLD] = false;
      strUTF8.insert(pos, "[/B]");
      pos += 4;
    }
    else if (fullTag == "<i>" || fullTag == "{\\i1}")
    {
      m_flag[FLAG_ITALIC] = true;
      strUTF8.insert(pos, "[I]");
      pos += 3;
    }
    else if ((fullTag == "</i>" || fullTag == "{\\i0}") && m_flag[FLAG_ITALIC])
    {
      m_flag[FLAG_ITALIC] = false;
      strUTF8.insert(pos, "[/I]");
      pos += 4;
    }
    else if ((fullTag == "</font>" || fullTag == "{\\c}") && m_flag[FLAG_COLOR])
    {
      m_flag[FLAG_COLOR] = false;
      strUTF8.insert(pos, "[/COLOR]");
      pos += 8;
    }
    else if (StringUtils::StartsWith(fullTag, "{\\c&h") ||
             StringUtils::StartsWith(fullTag, "{\\1c&h"))
    {
      m_flag[FLAG_COLOR] = true;
      std::string tempColorTag = "[COLOR FF";
      std::string tagOptionValue;
      if (StringUtils::StartsWith(fullTag, "{\\c&h"))
         tagOptionValue = fullTag.substr(5,6);
      else
         tagOptionValue = fullTag.substr(6,6);
      tempColorTag += tagOptionValue.substr(4,2);
      tempColorTag += tagOptionValue.substr(2,2);
      tempColorTag += tagOptionValue.substr(0,2);
      tempColorTag += "]";
      strUTF8.insert(pos, tempColorTag);
      pos += tempColorTag.length();
    }
    else if (StringUtils::StartsWith(fullTag, "<font"))
    {
      int pos2 = 5;
      while ((pos2 = m_tagOptions->RegFind(fullTag.c_str(), pos2)) >= 0)
      {
        std::string tagOptionName = m_tagOptions->GetMatch(1);
        std::string tagOptionValue = m_tagOptions->GetMatch(2);
        pos2 += tagOptionName.length() + tagOptionValue.length();
        if (tagOptionName == "color")
        {
          m_flag[FLAG_COLOR] = true;
          std::string tempColorTag = "[COLOR ";
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
    else if (lang && (StringUtils::StartsWith(fullTag, "<p ")))
    {
      int pos2 = 3;
      while ((pos2 = m_tagOptions->RegFind(fullTag.c_str(), pos2)) >= 0)
      {
        std::string tagOptionName = m_tagOptions->GetMatch(1);
        std::string tagOptionValue = m_tagOptions->GetMatch(2);
        pos2 += tagOptionName.length() + tagOptionValue.length();
        if (tagOptionName == "class")
        {
          if (m_flag[FLAG_LANGUAGE])
          {
            strUTF8.erase(del_start, pos - del_start);
            pos = del_start;
          }
          if (!tagOptionValue.compare(lang))
          {
            m_flag[FLAG_LANGUAGE] = false;
          }
          else
          {
            m_flag[FLAG_LANGUAGE] = true;
            del_start = pos;
          }
          break;
        }
      }
    }
    else if (fullTag == "</p>" && m_flag[FLAG_LANGUAGE])
    {
      strUTF8.erase(del_start, pos - del_start);
      pos = del_start;
      m_flag[FLAG_LANGUAGE] = false;
    }
    else if (fullTag == "<br>" && !strUTF8.empty())
    {
      strUTF8.insert(pos, "\n");
      pos += 1;
    }
  }

  if(m_flag[FLAG_LANGUAGE])
    strUTF8.erase(del_start);

  if (strUTF8.empty())
    return;

  if( strUTF8[strUTF8.size()-1] == '\n' )
    strUTF8.erase(strUTF8.size()-1);

  // add a new text element to our container
  pOverlay->AddElement(new CDVDOverlayText::CElementText(strUTF8.c_str()));
}

void CDVDSubtitleTagSami::CloseTag(CDVDOverlayText* pOverlay)
{
  if (m_flag[FLAG_BOLD])
  {
    pOverlay->AddElement(new CDVDOverlayText::CElementText("[/B]"));
    m_flag[FLAG_BOLD] = false;
  }
  if (m_flag[FLAG_ITALIC])
  {
    pOverlay->AddElement(new CDVDOverlayText::CElementText("[/I]"));
    m_flag[FLAG_ITALIC] = false;
  }
  if (m_flag[FLAG_COLOR])
  {
    pOverlay->AddElement(new CDVDOverlayText::CElementText("[/COLOR]"));
    m_flag[FLAG_COLOR] = false;
  }
  m_flag[FLAG_LANGUAGE] = false;
}

void CDVDSubtitleTagSami::LoadHead(CDVDSubtitleStream* samiStream)
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
          StringUtils::Trim(lc.Name);
          StringUtils::Trim(lc.Lang);
          StringUtils::Trim(lc.SAMIType);
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

