/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(HAS_NEON) && !defined(__LP64__)
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
