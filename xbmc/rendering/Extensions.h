/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Map.h"

#include <string_view>

struct GLEXTENSIONS
{
  enum EXTENSION
  {
    APPLE_texture_format_BGRA8888,
    ARB_multitexture,
    ARB_pixel_buffer_object,
    ARB_texture_float,
    EXT_color_buffer_float,
    EXT_framebuffer_object,
    EXT_texture_format_BGRA8888,
    EXT_unpack_subimage,
    IMG_texture_format_BGRA8888,
    KHR_debug,
    NV_vdpau_interop,
    NVX_gpu_memory_info,
    OES_EGL_image_external,
    EXTENSION_MAX
  };

  static constexpr auto stringMap = make_map<EXTENSION, std::string_view>({
      {APPLE_texture_format_BGRA8888, "GL_APPLE_texture_format_BGRA8888"},
      {ARB_multitexture, "GL_ARB_multitexture"},
      {ARB_pixel_buffer_object, "GL_ARB_pixel_buffer_object"},
      {ARB_texture_float, "GL_ARB_texture_float"},
      {EXT_color_buffer_float, "GL_EXT_color_buffer_float"},
      {EXT_framebuffer_object, "GL_EXT_framebuffer_object"},
      {EXT_texture_format_BGRA8888, "GL_EXT_texture_format_BGRA8888"},
      {EXT_unpack_subimage, "GL_EXT_unpack_subimage"},
      {IMG_texture_format_BGRA8888, "GL_IMG_texture_format_BGRA8888"},
      {KHR_debug, "GL_KHR_debug"},
      {NV_vdpau_interop, "GL_NV_vdpau_interop"},
      {NVX_gpu_memory_info, "GL_NVX_gpu_memory_info"},
      {OES_EGL_image_external, "GL_OES_EGL_image_external"},
  });

  static_assert(static_cast<size_t>(EXTENSION_MAX) == stringMap.size(),
                "Mismatch between enum and stringmap!");
};
