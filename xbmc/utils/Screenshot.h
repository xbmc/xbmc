/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CScreenshotSurface
{

public:
  int            m_width;
  int            m_height;
  int            m_stride;
  unsigned char* m_buffer;

  CScreenshotSurface(void);
  ~CScreenshotSurface();
  bool capture( void );
};

class CScreenShot
{

public:
  static void TakeScreenshot();
  static void TakeScreenshot(const std::string &filename, bool sync);
};
