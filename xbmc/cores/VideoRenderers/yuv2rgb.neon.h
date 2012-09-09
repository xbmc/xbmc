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
 */
#ifdef __cplusplus
extern "C" {
#endif

#if defined(__ARM_NEON__)
  void yuv420_2_rgb8888_neon
  (
    uint8_t *dst_ptr,
    const uint8_t *y_ptr,
    const uint8_t *u_ptr,
    const uint8_t *v_ptr,
    int width,
    int height,
    int y_pitch,
    int uv_pitch,
    int rgb_pitch
  );

  void yuv422_2_rgb8888_neon
  (
    uint8_t *dst_ptr,
    const uint8_t *y_ptr,
    const uint8_t *u_ptr,
    const uint8_t *v_ptr,
    int width,
    int height,
    int y_pitch,
    int uv_pitch,
    int rgb_pitch
  );
#endif

#ifdef __cplusplus
}
#endif
