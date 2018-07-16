/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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

