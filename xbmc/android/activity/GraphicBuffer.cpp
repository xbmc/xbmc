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
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Corporation code.
 *
 * The Initial Developer of the Original Code is Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   James Willcox <jwillcox@mozilla.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "DllGraphicBuffer.h"
#include "GraphicBuffer.h"
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
  m_width(width), m_height(height), m_usage(usage), m_format(format), m_handle(0)
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