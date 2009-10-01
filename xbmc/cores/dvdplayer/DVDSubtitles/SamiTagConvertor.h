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

class CDVDOverlayText;
class CRegExp;

class SamiTagConvertor
{
public:
  SamiTagConvertor()
  {
    m_tags = NULL;
    m_tagOptions = NULL;
    tag_flag[0] = false;
    tag_flag[1] = false;
    tag_flag[2] = false;
  }
  virtual ~SamiTagConvertor();
  bool Init();
  void ConvertLine(CDVDOverlayText* pOverlay, const char* line, int len);
  void CloseTag(CDVDOverlayText* pOverlay);

private:
  CRegExp *m_tags;
  CRegExp *m_tagOptions;
  bool tag_flag[3];
};
