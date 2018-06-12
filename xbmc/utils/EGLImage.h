/*
 *      Copyright (C) 2017 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <drm_fourcc.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "system_gl.h"

#include <array>

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
    std::array<EglPlane, MAX_NUM_PLANES> planes;
  };

  explicit CEGLImage(EGLDisplay display);

  CEGLImage(CEGLImage const& other) = delete;
  CEGLImage& operator=(CEGLImage const& other) = delete;

  bool CreateImage(EglAttrs imageAttrs);
  void UploadImage(GLenum textureTarget);
  void DestroyImage();

private:
  EGLDisplay m_display{nullptr};
  EGLImageKHR m_image{nullptr};

  PFNEGLCREATEIMAGEKHRPROC m_eglCreateImageKHR{nullptr};
  PFNEGLDESTROYIMAGEKHRPROC m_eglDestroyImageKHR{nullptr};
  PFNGLEGLIMAGETARGETTEXTURE2DOESPROC m_glEGLImageTargetTexture2DOES{nullptr};
};
