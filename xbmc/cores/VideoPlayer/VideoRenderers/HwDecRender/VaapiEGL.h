/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Geometry.h"

#include "platform/posix/utils/FileHandle.h"

#include <array>
#include <cstdint>

#include <va/va.h>

#include "system_egl.h"
#include "system_gl.h"

#include <EGL/eglext.h>

namespace VAAPI
{

class CVaapiRenderPicture;

struct InteropInfo
{
  EGLDisplay eglDisplay = nullptr;
  PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
  PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
  PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;
  GLenum textureTarget;
};

class CVaapiTexture
{
public:
  CVaapiTexture() = default;
  virtual ~CVaapiTexture() = default;

  virtual void Init(InteropInfo &interop) = 0;
  virtual bool Map(CVaapiRenderPicture *pic) = 0;
  virtual void Unmap() = 0;

  virtual GLuint GetTextureY() = 0;
  virtual GLuint GetTextureVU() = 0;
  virtual CSizeInt GetTextureSize() = 0;
};

class CVaapi1Texture : public CVaapiTexture
{
public:
  CVaapi1Texture() = default;

  bool Map(CVaapiRenderPicture *pic) override;
  void Unmap() override;
  void Init(InteropInfo &interop) override;

  GLuint GetTextureY() override;
  GLuint GetTextureVU() override;
  CSizeInt GetTextureSize() override;

  static void TestInterop(VADisplay vaDpy, EGLDisplay eglDisplay, bool &general, bool &deepColor);

  GLuint m_texture = 0;
  GLuint m_textureY = 0;
  GLuint m_textureVU = 0;
  int m_texWidth = 0;
  int m_texHeight = 0;

protected:
  static bool TestInteropDeepColor(VADisplay vaDpy, EGLDisplay eglDisplay);

  InteropInfo m_interop;
  CVaapiRenderPicture *m_vaapiPic = nullptr;
  struct GLSurface
  {
    VAImage vaImage;
    VABufferInfo vBufInfo;
    EGLImageKHR eglImage;
    EGLImageKHR eglImageY, eglImageVU;
  } m_glSurface;
};

class CVaapi2Texture : public CVaapiTexture
{
public:
  bool Map(CVaapiRenderPicture *pic) override;
  void Unmap() override;
  void Init(InteropInfo &interop) override;

  GLuint GetTextureY() override;
  GLuint GetTextureVU() override;
  CSizeInt GetTextureSize() override;

  static void TestInterop(VADisplay vaDpy, EGLDisplay eglDisplay, bool &general, bool &deepColor);
  static bool TestInteropGeneral(VADisplay vaDpy, EGLDisplay eglDisplay);

private:
  static bool TestEsh(VADisplay vaDpy, EGLDisplay eglDisplay, std::uint32_t rtFormat, std::int32_t pixelFormat);

  struct MappedTexture
  {
    EGLImageKHR eglImage{EGL_NO_IMAGE_KHR};
    GLuint glTexture{};
  };

  InteropInfo m_interop;
  CVaapiRenderPicture* m_vaapiPic{};
  bool m_hasPlaneModifiers{false};
  std::array<KODI::UTILS::POSIX::CFileHandle, 4> m_drmFDs;
  MappedTexture m_y, m_vu;
  CSizeInt m_textureSize;
};

}

