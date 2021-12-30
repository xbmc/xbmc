/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDSubtitleTagMicroDVD.h"

#include "utils/StringUtils.h"

void CDVDSubtitleTagMicroDVD::ConvertLine(std::string& strUTF8)
{
  m_flag[FLAG_BOLD] = 0;
  m_flag[FLAG_ITALIC] = 0;
  m_flag[FLAG_UNDERLINE] = 0;
  m_flag[FLAG_STRIKETHROUGH] = 0;
  m_flag[FLAG_COLOR] = 0;

  int machine_status = 1;
  size_t pos = 0;

  while (machine_status > 0)
  {
    if (machine_status == 1)
    {
      if (strUTF8[pos] == '{')
      {
        size_t pos2 = strUTF8.find(':', pos);
        size_t pos3 = strUTF8.find('}', pos2);

        if ((pos2 != std::string::npos) && (pos3 != std::string::npos))
        {
          std::string tagName = strUTF8.substr(pos + 1, pos2 - pos - 1);
          std::string tagValue = strUTF8.substr(pos2 + 1, pos3 - pos2 - 1);
          StringUtils::ToLower(tagValue);
          strUTF8.erase(pos, pos3 - pos + 1);
          if ((tagName == "Y") || (tagName == "y"))
          {
            if ((tagValue == "b") && (m_flag[FLAG_BOLD] == 0))
            {
              m_flag[FLAG_BOLD] = (tagName == "Y") ? TAG_ALL_LINE : TAG_ONE_LINE;
              strUTF8.insert(pos, "{\\b1}");
              pos += 5;
            }
            else if ((tagValue == "i") && (m_flag[FLAG_ITALIC] == 0))
            {
              m_flag[FLAG_ITALIC] = (tagName == "Y") ? TAG_ALL_LINE : TAG_ONE_LINE;
              strUTF8.insert(pos, "{\\i1}");
              pos += 5;
            }
            else if ((tagValue == "u") && (m_flag[FLAG_UNDERLINE] == 0))
            {
              m_flag[FLAG_UNDERLINE] = (tagName == "U") ? TAG_ALL_LINE : TAG_ONE_LINE;
              strUTF8.insert(pos, "{\\u1}");
              pos += 5;
            }
            else if ((tagValue == "s") && (m_flag[FLAG_STRIKETHROUGH] == 0))
            {
              m_flag[FLAG_STRIKETHROUGH] = (tagName == "S") ? TAG_ALL_LINE : TAG_ONE_LINE;
              strUTF8.insert(pos, "{\\s1}");
              pos += 5;
            }
          }
          else if ((tagName == "C") || (tagName == "c"))
          {
            if ((tagValue[0] == '$') && (tagValue.size() == 7))
            {
              bool bHex = true;
              for (int i = 1; i < 7; i++)
              {
                char temp = tagValue[i];
                if (!(('0' <= temp && temp <= '9') || ('a' <= temp && temp <= 'f') ||
                      ('A' <= temp && temp <= 'F')))
                {
                  bHex = false;
                  break;
                }
              }

              if (bHex && (m_flag[FLAG_COLOR] == 0))
              {
                m_flag[FLAG_COLOR] = (tagName == "C") ? TAG_ALL_LINE : TAG_ONE_LINE;
                std::string tempColorTag = "{\\c&H" + tagValue.substr(1, 6) + "&}";
                strUTF8.insert(pos, tempColorTag);
                pos += tempColorTag.length();
              }
            }
          }
        }
        else
          machine_status = 2;
      }
      else if (strUTF8[pos] == '/')
      {
        if (m_flag[FLAG_ITALIC] == 0)
        {
          m_flag[FLAG_ITALIC] = TAG_ONE_LINE;
          strUTF8.insert(pos, "{\\i1}");
          pos += 5;
        }
        else
          strUTF8.erase(pos, 1);
      }
      else
        machine_status = 2;
    }
    else if (machine_status == 2)
    {
      size_t pos4;
      if ((pos4 = strUTF8.find('|', pos)) != std::string::npos)
      {
        pos = pos4;
        if (m_flag[FLAG_BOLD] == TAG_ONE_LINE)
        {
          m_flag[FLAG_BOLD] = 0;
          strUTF8.insert(pos, "{\\b0}");
          pos += 5;
        }
        if (m_flag[FLAG_ITALIC] == TAG_ONE_LINE)
        {
          m_flag[FLAG_ITALIC] = 0;
          strUTF8.insert(pos, "{\\i0}");
          pos += 5;
        }
        if (m_flag[FLAG_UNDERLINE] == TAG_ONE_LINE)
        {
          m_flag[FLAG_UNDERLINE] = 0;
          strUTF8.insert(pos, "{\\u0}");
          pos += 5;
        }
        if (m_flag[FLAG_STRIKETHROUGH] == TAG_ONE_LINE)
        {
          m_flag[FLAG_STRIKETHROUGH] = 0;
          strUTF8.insert(pos, "{\\s0}");
          pos += 5;
        }
        if (m_flag[FLAG_COLOR] == TAG_ONE_LINE)
        {
          m_flag[FLAG_COLOR] = 0;
          strUTF8.insert(pos, "{\\c}");
          pos += 4;
        }
        strUTF8.replace(pos, 1, "\n");
        pos += 1;
        machine_status = 1;
      }
      else
      {
        if (m_flag[FLAG_BOLD] != 0)
          strUTF8.append("{\\b0}");
        if (m_flag[FLAG_ITALIC] != 0)
          strUTF8.append("{\\i0}");
        if (m_flag[FLAG_UNDERLINE] != 0)
          strUTF8.append("{\\u0}");
        if (m_flag[FLAG_STRIKETHROUGH] != 0)
          strUTF8.append("{\\s0}");
        if (m_flag[FLAG_COLOR] != 0)
          strUTF8.append("{\\c}");
        machine_status = 0;
      }
    }
  }
}
