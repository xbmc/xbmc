#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include <stdio.h>

#define FLAG_BOLD   0
#define FLAG_ITALIC 1
#define FLAG_COLOR  2

#define TAG_ONE_LINE 1
#define TAG_ALL_LINE 2

class CDVDOverlayText;

class CDVDSubtitleTagMicroDVD
{
public:
  void ConvertLine(CDVDOverlayText* pOverlay, const char* line, int len);

private:
  int m_flag[3];
};

