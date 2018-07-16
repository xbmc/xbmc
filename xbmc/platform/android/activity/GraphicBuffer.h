/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 *
 *  The Original Code is Mozilla Corporation code.
 *
 *  The Initial Developer of the Original Code is Mozilla Foundation.
 *  Portions created by the Initial Developer are Copyright (C) 2009
 *  the Initial Developer. All Rights Reserved.
 *
 *  Contributor(s):
 *    James Willcox <jwillcox@mozilla.com>
 */

#pragma once

#include <stdint.h>
#include "guilib/XBTF.h"

class DllGraphicBuffer;

/* Copied from Android's gralloc.h */
enum gfxImageUsage {
    /* buffer is never read in software */
    GRALLOC_USAGE_SW_READ_NEVER   = 0x00000000,
    /* buffer is rarely read in software */
    GRALLOC_USAGE_SW_READ_RARELY  = 0x00000002,
    /* buffer is often read in software */
    GRALLOC_USAGE_SW_READ_OFTEN   = 0x00000003,
    /* mask for the software read values */
    GRALLOC_USAGE_SW_READ_MASK    = 0x0000000F,

    /* buffer is never written in software */
    GRALLOC_USAGE_SW_WRITE_NEVER  = 0x00000000,
    /* buffer is never written in software */
    GRALLOC_USAGE_SW_WRITE_RARELY = 0x00000020,
    /* buffer is never written in software */
    GRALLOC_USAGE_SW_WRITE_OFTEN  = 0x00000030,
    /* mask for the software write values */
    GRALLOC_USAGE_SW_WRITE_MASK   = 0x000000F0,

    /* buffer will be used as an OpenGL ES texture */
    GRALLOC_USAGE_HW_TEXTURE      = 0x00000100,
    /* buffer will be used as an OpenGL ES render target */
    GRALLOC_USAGE_HW_RENDER       = 0x00000200,
    /* buffer will be used by the 2D hardware blitter */
    GRALLOC_USAGE_HW_2D           = 0x00000400,
    /* buffer will be used with the framebuffer device */
    GRALLOC_USAGE_HW_FB           = 0x00001000,
    /* mask for the software usage bit-mask */
    GRALLOC_USAGE_HW_MASK         = 0x00001F00,

    /* implementation-specific private usage flags */
    GRALLOC_USAGE_PRIVATE_0       = 0x10000000,
    GRALLOC_USAGE_PRIVATE_1       = 0x20000000,
    GRALLOC_USAGE_PRIVATE_2       = 0x40000000,
    GRALLOC_USAGE_PRIVATE_3       = 0x80000000,
    GRALLOC_USAGE_PRIVATE_MASK    = 0xF0000000,
};

class CGraphicBuffer
{
public:
  CGraphicBuffer(uint32_t width, uint32_t height, uint32_t format, gfxImageUsage usage);
  virtual       ~CGraphicBuffer();

  bool          Lock(gfxImageUsage usage, void **addr);
  bool          Unlock();
  uint32_t      GetNativeBuffer();

private:
  uint32_t      GetAndroidFormat(uint32_t format);

  void          *m_handle;
  static        DllGraphicBuffer *m_dll;
};
