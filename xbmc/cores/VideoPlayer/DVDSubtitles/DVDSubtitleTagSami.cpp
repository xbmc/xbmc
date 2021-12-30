/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDSubtitleTagSami.h"

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
  if (!m_tags->RegComp("(<[^>]*>|\\[nh])"))
    return false;

  m_tagOptions = new CRegExp(true);
  if (!m_tagOptions->RegComp("([a-z]+)[ \t]*=[ \t]*(?:[\"'])?([^\"'> ]+)(?:[\"'])?(?:>)?"))
    return false;

  return true;
}

void CDVDSubtitleTagSami::ConvertLine(std::string& strUTF8, const char* langClassID)
{
  StringUtils::Trim(strUTF8);

  int pos = 0;
  int del_start = 0;
  while ((pos = m_tags->RegFind(strUTF8.c_str(), pos)) >= 0)
  {
    // Parser for SubRip/SAMI Tags
    std::string fullTag = m_tags->GetMatch(0);
    StringUtils::ToLower(fullTag);
    strUTF8.erase(pos, fullTag.length());
    if (fullTag == "<b>")
    {
      m_flag[FLAG_BOLD] = true;
      strUTF8.insert(pos, "{\\b1}");
      pos += 5;
    }
    else if ((fullTag == "</b>") && m_flag[FLAG_BOLD])
    {
      m_flag[FLAG_BOLD] = false;
      strUTF8.insert(pos, "{\\b0}");
      pos += 5;
    }
    else if (fullTag == "<i>")
    {
      m_flag[FLAG_ITALIC] = true;
      strUTF8.insert(pos, "{\\i1}");
      pos += 5;
    }
    else if ((fullTag == "</i>") && m_flag[FLAG_ITALIC])
    {
      m_flag[FLAG_ITALIC] = false;
      strUTF8.insert(pos, "{\\i0}");
      pos += 5;
    }
    else if (fullTag == "<u>")
    {
      m_flag[FLAG_UNDERLINE] = true;
      strUTF8.insert(pos, "{\\u1}");
      pos += 5;
    }
    else if ((fullTag == "</u>") && m_flag[FLAG_UNDERLINE])
    {
      m_flag[FLAG_UNDERLINE] = false;
      strUTF8.insert(pos, "{\\u0}");
      pos += 5;
    }
    else if (fullTag == "<s>")
    {
      m_flag[FLAG_STRIKETHROUGH] = true;
      strUTF8.insert(pos, "{\\s1}");
      pos += 5;
    }
    else if ((fullTag == "</s>") && m_flag[FLAG_STRIKETHROUGH])
    {
      m_flag[FLAG_STRIKETHROUGH] = false;
      strUTF8.insert(pos, "{\\s0}");
      pos += 5;
    }
    else if ((fullTag == "</font>") && m_flag[FLAG_COLOR])
    {
      m_flag[FLAG_COLOR] = false;
      strUTF8.insert(pos, "{\\c}");
      pos += 4;
    }
    else if (StringUtils::StartsWith(fullTag, "<font"))
    {
      int pos2 = 5;
      while ((pos2 = m_tagOptions->RegFind(fullTag.c_str(), pos2)) >= 0)
      {
        std::string tagOptionName = m_tagOptions->GetMatch(1);
        std::string tagOptionValue = m_tagOptions->GetMatch(2);
        std::string colorHex = "FFFFFF";
        pos2 += static_cast<int>(tagOptionName.length() + tagOptionValue.length());
        if (tagOptionName == "color")
        {
          m_flag[FLAG_COLOR] = true;
          if (tagOptionValue[0] == '#' && tagOptionValue.size() >= 7)
          {
            tagOptionValue.erase(0, 1);
            tagOptionValue.erase(6, tagOptionValue.size());
            colorHex = tagOptionValue;
          }
          else if (tagOptionValue.size() == 6)
          {
            bool bHex = true;
            for (int i = 0; i < 6; i++)
            {
              char temp = tagOptionValue[i];
              if (!(('0' <= temp && temp <= '9') || ('a' <= temp && temp <= 'f') ||
                    ('A' <= temp && temp <= 'F')))
              {
                bHex = false;
                break;
              }
            }
            if (bHex)
              colorHex = tagOptionValue;
          }
          colorHex =
              colorHex.substr(4, 2) + tagOptionValue.substr(2, 2) + tagOptionValue.substr(0, 2);
          std::string tempColorTag = "{\\c&H" + colorHex + "&}";
          strUTF8.insert(pos, tempColorTag);
          pos += static_cast<int>(tempColorTag.length());
        }
      }
    }
    // Parse specific SAMI Tags (all below)
    else if (langClassID && (StringUtils::StartsWith(fullTag, "<p ")))
    {
      int pos2 = 3;
      while ((pos2 = m_tagOptions->RegFind(fullTag.c_str(), pos2)) >= 0)
      {
        std::string tagOptionName = m_tagOptions->GetMatch(1);
        std::string tagOptionValue = StringUtils::ToLower(m_tagOptions->GetMatch(2));
        pos2 += static_cast<int>(tagOptionName.length() + tagOptionValue.length());
        if (tagOptionName == "class")
        {
          if (m_flag[FLAG_LANGUAGE])
          {
            strUTF8.erase(del_start, pos - del_start);
            pos = del_start;
          }
          if (!tagOptionValue.compare(langClassID))
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
    else if ((fullTag == "</p>") && m_flag[FLAG_LANGUAGE])
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

  if (m_flag[FLAG_LANGUAGE])
    strUTF8.erase(del_start);

  if (strUTF8.empty())
    return;
  if (strUTF8 == "&nbsp;") // SAMI specific blank paragraph parameter
  {
    strUTF8.clear();
    return;
  }

  std::wstring wStrHtml, wStr;
  g_charsetConverter.utf8ToW(strUTF8, wStrHtml, false);
  HTML::CHTMLUtil::ConvertHTMLToW(wStrHtml, wStr);
  g_charsetConverter.wToUTF8(wStr, strUTF8);
}

void CDVDSubtitleTagSami::CloseTag(std::string& text)
{
  if (m_flag[FLAG_BOLD])
  {
    m_flag[FLAG_BOLD] = false;
    text += "{\\b0}";
  }
  if (m_flag[FLAG_ITALIC])
  {
    m_flag[FLAG_ITALIC] = false;
    text += "{\\i0}";
  }
  if (m_flag[FLAG_UNDERLINE])
  {
    m_flag[FLAG_UNDERLINE] = false;
    text += "{\\u0}";
  }
  if (m_flag[FLAG_STRIKETHROUGH])
  {
    m_flag[FLAG_STRIKETHROUGH] = false;
    text += "{\\s0}";
  }
  if (m_flag[FLAG_COLOR])
  {
    m_flag[FLAG_COLOR] = false;
    text += "{\\c}";
  }
  m_flag[FLAG_LANGUAGE] = false;
}

void CDVDSubtitleTagSami::LoadHead(CDVDSubtitleStream* samiStream)
{
  bool inSTYLE = false;
  CRegExp reg(true);
  if (!reg.RegComp("\\.([a-z]+)[ \t]*\\{[ \t]*name:([^;]*?);[ \t]*lang:([^;]*?);[ "
                   "\t]*SAMIType:([^;]*?);[ \t]*\\}"))
    return;

  std::string line;
  while (samiStream->ReadLine(line))
  {
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
