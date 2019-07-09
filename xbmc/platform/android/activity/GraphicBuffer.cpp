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

#include "GraphicBuffer.h"

#include "DllGraphicBuffer.h"
#include "XBMCApp.h"
#include "utils/log.h"

DllGraphicBuffer *CGraphicBuffer::m_dll = NULL;

/* Yanked from Android's hardware.h */
enum
{
    HAL_PIXEL_FORMAT_RGBA_8888          = 1,
    HAL_PIXEL_FORMAT_RGBX_8888          = 2,
    HAL_PIXEL_FORMAT_RGB_888            = 3,
    HAL_PIXEL_FORMAT_RGB_565            = 4,
    HAL_PIXEL_FORMAT_BGRA_8888          = 5,
    HAL_PIXEL_FORMAT_RGBA_5551          = 6,
    HAL_PIXEL_FORMAT_RGBA_4444          = 7,

    /*
     * Android YUV format:
     *
     * This format is exposed outside of the HAL to software
     * decoders and applications.
     * EGLImageKHR must support it in conjunction with the
     * OES_EGL_image_external extension.
     *
     * YV12 is 4:2:0 YCrCb planar format comprised of a WxH Y plane followed
     * by (W/2) x (H/2) Cr and Cb planes.
     *
     * This format assumes
     * - an even width
     * - an even height
     * - a horizontal stride multiple of 16 pixels
     * - a vertical stride equal to the height
     *
     *   y_size = stride * height
     *   c_size = ALIGN(stride/2, 16) * height/2
     *   size = y_size + c_size * 2
     *   cr_offset = y_size
     *   cb_offset = y_size + c_size
     *
     */
    HAL_PIXEL_FORMAT_YV12   = 0x32315659, // YCrCb 4:2:0 Planar
};

CGraphicBuffer::CGraphicBuffer(uint32_t width, uint32_t height, uint32_t format, gfxImageUsage usage):
  m_handle(0)
{
  CLog::Log(LOGDEBUG, "CGraphicBuffer::CGraphicBuffer");
  if (!m_dll)
  {
    // m_dll is static, there can be only one.
    m_dll = new DllGraphicBuffer;
    m_dll->Load();
    m_dll->EnableDelayedUnload(false);
  }

  m_handle = malloc(4096 * 4);
  if (m_dll)
    m_dll->GraphicBufferCtor(m_handle, width, height, GetAndroidFormat(format), usage);
}

CGraphicBuffer::~CGraphicBuffer()
{
  if (m_handle)
  {
    if (m_dll)
      m_dll->GraphicBufferDtor(m_handle);
    free(m_handle);
  }
}

bool CGraphicBuffer::Lock(gfxImageUsage aUsage, void **addr)
{
  if (m_dll)
    return m_dll->GraphicBufferLock(m_handle, aUsage, addr) == 0;
  return false;
}

uint32_t CGraphicBuffer::GetNativeBuffer()
{
  if (m_dll)
    return m_dll->GraphicBufferGetNativeBuffer(m_handle);
  return false;
}

bool CGraphicBuffer::Unlock()
{
  if (m_dll)
    return m_dll->GraphicBufferUnlock(m_handle) == 0;
  return false;
}

uint32_t CGraphicBuffer::GetAndroidFormat(uint32_t format)
{
  switch (format)
  {
    case XB_FMT_A8R8G8B8:
      return HAL_PIXEL_FORMAT_BGRA_8888;
    case XB_FMT_RGBA8:
      return HAL_PIXEL_FORMAT_RGBA_8888;
    case XB_FMT_RGB8:
      return HAL_PIXEL_FORMAT_RGB_888;
  }
  return 0;
}
