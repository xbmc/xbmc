/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CScreenshotAML
{
  public:
    // Captures the current visible video framebuffer and blends it into
    // the passed overlay. The buffer format is BGRA (4 byte)
    static void CaptureVideoFrame(unsigned char *buffer, int iWidth, int iHeight, bool bBlendToBuffer = true);
};
