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

#include "DVDSubtitleTagMicroDVD.h"
#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "utils/StringUtils.h"

void CDVDSubtitleTagMicroDVD::ConvertLine(CDVDOverlayText* pOverlay, const char* line, int len)
{
  std::string strUTF8;
  strUTF8.assign(line, len);

  m_flag[FLAG_BOLD] = 0;
  m_flag[FLAG_ITALIC] = 0;
  m_flag[FLAG_COLOR] = 0;

  int machine_status = 1;
  size_t pos = 0;

  while (machine_status > 0)
  {
    if (machine_status == 1)
    {
      if (strUTF8[pos] == '{')
      {
        size_t pos2, pos3;
        if (((pos2 = strUTF8.find(':', pos)) != std::string::npos) && \
           ((pos3 = strUTF8.find('}', pos2)) != std::string::npos))
        {
          std::string tagName = strUTF8.substr(pos + 1, pos2 - pos - 1);
          std::string tagValue = strUTF8.substr(pos2 + 1, pos3 - pos2 - 1);
          StringUtils::ToLower(tagValue);
          strUTF8.erase(pos, pos3 - pos + 1);
          if ((tagName == "Y") || (tagName == "y"))
          {
            if ((tagValue == "b") && (m_flag[FLAG_BOLD] == 0))
            {
              m_flag[FLAG_BOLD] = (tagName == "Y")?TAG_ALL_LINE:TAG_ONE_LINE;
              strUTF8.insert(pos, "[B]");
              pos += 3;
            }
            else if ((tagValue == "i") && (m_flag[FLAG_ITALIC] == 0))
            {
              m_flag[FLAG_ITALIC] = (tagName == "Y")?TAG_ALL_LINE:TAG_ONE_LINE;
              strUTF8.insert(pos, "[I]");
              pos += 3;
            }
          }
          else if ((tagName == "C") || (tagName == "c"))
          {
            if ((tagValue[0] == '$') && (tagValue.size() == 7))
            {
              bool bHex = true;
              for( int i=1 ; i<7 ; i++ )
              {
                char temp = tagValue[i];
                if( !(('0' <= temp && temp <= '9') ||
                  ('a' <= temp && temp <= 'f') ||
                  ('A' <= temp && temp <= 'F') ))
                {
                  bHex = false;
                  break;
                }
              }
              if( bHex && (m_flag[FLAG_COLOR] == 0))
              {
                std::string tempColorTag = "[COLOR ";
                tempColorTag += "FF";
                tempColorTag += tagValue.substr(1, 6);
                tempColorTag += "]";
                m_flag[FLAG_COLOR] = (tagName == "C")?TAG_ALL_LINE:TAG_ONE_LINE;
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
          strUTF8.replace(pos, 1, "[I]");
          pos += 3;
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
      if ((pos4= strUTF8.find('|', pos)) != std::string::npos)
      {
        pos = pos4;
        if (m_flag[FLAG_BOLD] == TAG_ONE_LINE)
        {
          m_flag[FLAG_BOLD] = 0;
          strUTF8.insert(pos, "[/B]");
          pos += 4;
        }
        if (m_flag[FLAG_ITALIC] == TAG_ONE_LINE)
        {
          m_flag[FLAG_ITALIC] = 0;
          strUTF8.insert(pos, "[/I]");
          pos += 4;
        }
        if (m_flag[FLAG_COLOR] == TAG_ONE_LINE)
        {
          m_flag[FLAG_COLOR] = 0;
          strUTF8.insert(pos, "[/COLOR]");
          pos += 8;
        }
        strUTF8.replace(pos, 1, "[CR]");
        pos += 4;
        machine_status = 1;
      }
      else
      {
        if (m_flag[FLAG_BOLD] != 0)
          strUTF8.append("[/B]");
        if (m_flag[FLAG_ITALIC] != 0)
          strUTF8.append("[/I]");
        if (m_flag[FLAG_COLOR] != 0)
          strUTF8.append("[/COLOR]");
        machine_status = 0;
      }
    }
  }
    
  if (strUTF8.empty())
    return;

  if( strUTF8[strUTF8.size()-1] == '\n' )
    strUTF8.erase(strUTF8.size()-1);

  // add a new text element to our container
  pOverlay->AddElement(new CDVDOverlayText::CElementText(strUTF8.c_str()));
}

