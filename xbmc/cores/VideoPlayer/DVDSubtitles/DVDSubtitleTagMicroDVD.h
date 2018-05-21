#pragma once

/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <string.h>

#define FLAG_BOLD   0
#define FLAG_ITALIC 1
#define FLAG_COLOR  2

#define TAG_ONE_LINE 1
#define TAG_ALL_LINE 2

class CDVDOverlayText;

class CDVDSubtitleTagMicroDVD
{
public:
  CDVDSubtitleTagMicroDVD()
  {
    memset(&m_flag, 0, sizeof(m_flag));
  }
  void ConvertLine(CDVDOverlayText* pOverlay, const char* line, int len);

private:
  int m_flag[3];
};

