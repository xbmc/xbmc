/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDSubtitleTagSami.h"

#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "DVDSubtitleStream.h"
#include "utils/CharsetConverter.h"
#include "utils/HTMLUtil.h"
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
  if (!m_tags->RegComp("(<[^>]*>|\\{[^\\}]*\\})|\\[nh]"))
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
    else if ((fullTag == "\\n") || (StringUtils::StartsWith(fullTag, "<br") && !strUTF8.empty()))
    {
      strUTF8.insert(pos, "\n");
      pos += 1;
    }
    // SubRip (.srt) hard space
    else if (fullTag == "\\h")
    {
      // Unicode no-break space
      strUTF8.insert(pos, "\xC2\xA0");
      pos += 2;
    }
  }

  if(m_flag[FLAG_LANGUAGE])
    strUTF8.erase(del_start);

  if (strUTF8.empty())
    return;

  if( strUTF8[strUTF8.size()-1] == '\n' )
    strUTF8.erase(strUTF8.size()-1);

  std::wstring wStrHtml, wStr;
  g_charsetConverter.utf8ToW(strUTF8, wStrHtml, false);
  HTML::CHTMLUtil::ConvertHTMLToW(wStrHtml, wStr);
  g_charsetConverter.wToUTF8(wStr, strUTF8);

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
  char cLine[1024];
  bool inSTYLE = false;
  CRegExp reg(true);
  if (!reg.RegComp("\\.([a-z]+)[ \t]*\\{[ \t]*name:([^;]*?);[ \t]*lang:([^;]*?);[ \t]*SAMIType:([^;]*?);[ \t]*\\}"))
    return;

  while (samiStream->ReadLine(cLine, sizeof(cLine)))
  {
    std::string line = cLine;
    StringUtils::Trim(line);

   if (StringUtils::EqualsNoCase(line, "<BODY>"))
      break;
    if (inSTYLE)
    {
      if (StringUtils::EqualsNoCase(line, "</STYLE>"))
        break;
      else
      {
        if (reg.RegFind(line.c_str()) > -1)
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
      if (StringUtils::EqualsNoCase(line, "<STYLE TYPE=\"text/css\">"))
        inSTYLE = true;
    }
  }
}
