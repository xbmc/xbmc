/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <vector>
#include "cores/IPlayer.h"

extern "C" {
#include <libavutil/pixfmt.h>
}

struct CRenderInfo
{
  CRenderInfo()
  {
    Reset();
  }
  void Reset()
  {
    max_buffer_size = 0;
    opaque_pointer = nullptr;
    m_deintMethods.clear();
    formats.clear();
  }
  unsigned int max_buffer_size;
  // Supported pixel formats, can be called before configure
  std::vector<AVPixelFormat> formats;
  std::vector<EINTERLACEMETHOD> m_deintMethods;
  // Can be used for initialising video codec with information from renderer (e.g. a shared image pool)
  void *opaque_pointer;
};
