#pragma once

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
#include <stdio.h>
#include "StdString.h"

#define FLAG_BOLD   0
#define FLAG_ITALIC 1
#define FLAG_COLOR  2
#define FLAG_LANGUAGE   3

class CDVDOverlayText;
class CDVDSubtitleStream;
class CRegExp;

class SamiTagConvertor
{
public:
  SamiTagConvertor()
  {
    m_tags = NULL;
    m_tagOptions = NULL;
    tag_flag[FLAG_BOLD] = false;
    tag_flag[FLAG_ITALIC] = false;
    tag_flag[FLAG_COLOR] = false;
    tag_flag[FLAG_LANGUAGE] = false; //set to true when classID != lang
  }
  virtual ~SamiTagConvertor();
  bool Init();
  void ConvertLine(CDVDOverlayText* pOverlay, const char* line, int len, const char* lang = NULL);
  void CloseTag(CDVDOverlayText* pOverlay);
  void LoadHead(CDVDSubtitleStream* samiStream);

  typedef struct
  {
    CStdString ID;
    CStdString Name;
    CStdString Lang;
    CStdString SAMIType;
  } SLangclass;

  std::vector<SLangclass> m_Langclass;

private:
  CRegExp *m_tags;
  CRegExp *m_tagOptions;
  bool tag_flag[4];
};

