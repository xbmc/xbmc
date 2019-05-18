/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class SSE4
{
public:
  static void copy_frame(void* pSrc,
                         void* pDest,
                         void* pCacheBlock,
                         unsigned int width,
                         unsigned int height,
                         unsigned int pitch);
};
