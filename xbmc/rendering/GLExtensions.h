/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Map.h"

#include <string_view>

struct CGLExtensions
{
  enum class Extension
  {
    APPLE_texture_format_BGRA8888,
    ARB_multitexture,
    ARB_pixel_buffer_object,
    ARB_texture_float,
    ARB_texture_swizzle,
    EXT_color_buffer_float,
    EXT_framebuffer_object,
    EXT_texture_filter_anisotropic,
    EXT_texture_format_BGRA8888,
    EXT_texture_swizzle,
    EXT_unpack_subimage,
    IMG_texture_format_BGRA8888,
    KHR_debug,
    NVX_gpu_memory_info,
    NV_vdpau_interop,
    OES_EGL_image_external,
    EXTENSION_MAX
  };
  using enum Extension;

  static constexpr auto stringMap = make_map<Extension, std::string_view>({
      {APPLE_texture_format_BGRA8888, "GL_APPLE_texture_format_BGRA8888"},
      {ARB_multitexture, "GL_ARB_multitexture"},
      {ARB_pixel_buffer_object, "GL_ARB_pixel_buffer_object"},
      {ARB_texture_float, "GL_ARB_texture_float"},
      {ARB_texture_swizzle, "GL_ARB_texture_swizzle"},
      {EXT_color_buffer_float, "GL_EXT_color_buffer_float"},
      {EXT_framebuffer_object, "GL_EXT_framebuffer_object"},
      {EXT_texture_filter_anisotropic, "GL_EXT_texture_filter_anisotropic"},
      {EXT_texture_format_BGRA8888, "GL_EXT_texture_format_BGRA8888"},
      {EXT_texture_swizzle, "GL_EXT_texture_swizzle"},
      {EXT_unpack_subimage, "GL_EXT_unpack_subimage"},
      {IMG_texture_format_BGRA8888, "GL_IMG_texture_format_BGRA8888"},
      {KHR_debug, "GL_KHR_debug"},
      {NVX_gpu_memory_info, "GL_NVX_gpu_memory_info"},
      {NV_vdpau_interop, "GL_NV_vdpau_interop"},
      {OES_EGL_image_external, "GL_OES_EGL_image_external"},
  });

  static_assert(static_cast<size_t>(EXTENSION_MAX) == stringMap.size(),
                "Mismatch between enum and stringmap!");

  static bool IsExtensionSupported(Extension extension);
};
