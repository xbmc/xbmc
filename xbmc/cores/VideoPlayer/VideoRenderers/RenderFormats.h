#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <cstddef>
#include <vector>

enum ERenderFormat {
  RENDER_FMT_NONE = 0,
  RENDER_FMT_YUV420P,
  RENDER_FMT_YUV420P10,
  RENDER_FMT_YUV420P16,
  RENDER_FMT_VDPAU,
  RENDER_FMT_VDPAU_420,
  RENDER_FMT_NV12,
  RENDER_FMT_UYVY422,
  RENDER_FMT_YUYV422,
  RENDER_FMT_DXVA,
  RENDER_FMT_VAAPI,
  RENDER_FMT_VAAPINV12,
  RENDER_FMT_OMXEGL,
  RENDER_FMT_CVBREF,
  RENDER_FMT_BYPASS,
  RENDER_FMT_MEDIACODEC,
  RENDER_FMT_MEDIACODECSURFACE,
  RENDER_FMT_IMXMAP,
  RENDER_FMT_MMAL,
  RENDER_FMT_AML,
};

struct CRenderInfo
{
  CRenderInfo()
  {
    optimal_buffer_size = 0;
    max_buffer_size = 0;
    opaque_pointer = NULL;
  }
  unsigned int optimal_buffer_size;
  unsigned int max_buffer_size;
  // Supported pixel formats, can be called before configure
  std::vector<ERenderFormat> formats;
  // Can be used for initialising video codec with information from renderer (e.g. a shared image pool)
  void *opaque_pointer;
};

