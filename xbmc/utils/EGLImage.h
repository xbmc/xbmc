/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "system_egl.h"

#include <array>

#include <EGL/eglext.h>
#include <drm_fourcc.h>

#include "system_gl.h"

class CEGLImage
{
public:
  static const int MAX_NUM_PLANES{3};

  struct EglPlane
  {
    int fd{0};
    int offset{0};
    int pitch{0};
    uint64_t modifier{DRM_FORMAT_MOD_INVALID};
  };

  struct EglAttrs
  {
    int width{0};
    int height{0};
    uint32_t format{0};
    int colorSpace{0};
    int colorRange{0};
    std::array<EglPlane, MAX_NUM_PLANES> planes;
  };

  explicit CEGLImage(EGLDisplay display);

  CEGLImage(CEGLImage const& other) = delete;
  CEGLImage& operator=(CEGLImage const& other) = delete;

  bool CreateImage(EglAttrs imageAttrs);
  void UploadImage(GLenum textureTarget);
  void DestroyImage();

#if defined(EGL_EXT_image_dma_buf_import_modifiers)
  bool SupportsFormatAndModifier(uint32_t format, uint64_t modifier);

private:
  bool SupportsFormat(uint32_t format);
#else
private:
#endif
  EGLDisplay m_display{nullptr};
  EGLImageKHR m_image{nullptr};

  PFNEGLCREATEIMAGEKHRPROC m_eglCreateImageKHR{nullptr};
  PFNEGLDESTROYIMAGEKHRPROC m_eglDestroyImageKHR{nullptr};
  PFNGLEGLIMAGETARGETTEXTURE2DOESPROC m_glEGLImageTargetTexture2DOES{nullptr};
};
