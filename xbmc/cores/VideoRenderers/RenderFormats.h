#ifndef _RENDER_FORMATS_H_
#define _RENDER_FORMATS_H_
/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

enum ERenderFormat {
  RENDER_FMT_NONE = 0,
  RENDER_FMT_YUV420P,
  RENDER_FMT_YUV420P10,
  RENDER_FMT_YUV420P16,
  RENDER_FMT_VDPAU,
  RENDER_FMT_NV12,
  RENDER_FMT_UYVY422,
  RENDER_FMT_YUYV422,
  RENDER_FMT_DXVA,
  RENDER_FMT_VAAPI,
  RENDER_FMT_OMXEGL,
  RENDER_FMT_CVBREF,
  RENDER_FMT_BYPASS,

  /*
   * Generic OpenGL planar formats.
   *
   * This depends on the following extensions:
   * - EXT_texture_rg
   * - OES_texture_npot (GLESv2), or ARB_texture_non_power_of_two (GL)
   *
   * The naming convention separates planes by '_' and within each
   * plane, the order of R, G, B, A, Y, U and V indicates how those
   * components map to the rgba value returned by the sampler. X
   * indicates that the corresponding component in the rgba value is
   * not used.
   *
   * For example, RENDER_FMT_Y_UV means:
   * - texture[0].r = 'Y' component
   * - texture[1].r = 'U' component
   * - texture[1].g = 'V' component
   */
  RENDER_FMT_Y_UV,      // 1 plane for Y and 1 plane for UV
  RENDER_FMT_Y_U_V      // 3 planes for Y U V
};

#endif
