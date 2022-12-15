/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdio.h>
#include <string>
#include <vector>

#define FLAG_BOLD 0
#define FLAG_ITALIC 1
#define FLAG_UNDERLINE 2
#define FLAG_STRIKETHROUGH 3
#define FLAG_COLOR 4
#define FLAG_LANGUAGE 5

class CDVDSubtitleStream;
class CRegExp;

class CDVDSubtitleTagSami
{
public:
  CDVDSubtitleTagSami()
  {
    m_tags = NULL;
    m_tagOptions = NULL;
    m_flag[FLAG_BOLD] = false;
    m_flag[FLAG_ITALIC] = false;
    m_flag[FLAG_UNDERLINE] = false;
    m_flag[FLAG_STRIKETHROUGH] = false;
    m_flag[FLAG_COLOR] = false;
    m_flag[FLAG_LANGUAGE] = false; //set to true when classID != lang
  }
  virtual ~CDVDSubtitleTagSami();
  bool Init();
  /*!
   \brief Convert a subtitle text line.
   \param line The text line
   \param len The line length
   \param langClassID The SAMI Class ID language (keep the language lines with this ID and ignore all others)
  */
  void ConvertLine(std::string& strUTF8, const char* langClassID = NULL);
  void CloseTag(std::string& text);
  void LoadHead(CDVDSubtitleStream* samiStream);

  typedef struct
  {
    std::string ID;
    std::string Name;
    std::string Lang;
    std::string SAMIType;
  } SLangclass;

  std::vector<SLangclass> m_Langclass;

private:
  CRegExp* m_tags;
  CRegExp* m_tagOptions;
  bool m_flag[6];
};
