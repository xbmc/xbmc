/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
