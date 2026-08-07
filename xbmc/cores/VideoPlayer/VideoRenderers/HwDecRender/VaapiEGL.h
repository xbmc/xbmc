/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Buffers/DmaBufIdentityCache.h"
#include "utils/Geometry.h"

#include "platform/posix/utils/FileHandle.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include <va/va.h>

#include "system_egl.h"
#include "system_gl.h"

#include <EGL/eglext.h>

namespace VAAPI
{

class CCapabilities;
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
  virtual bool Import(CVaapiRenderPicture* pic) = 0;
  virtual void Reset() = 0;

  virtual GLuint GetTextureY() = 0;
  virtual GLuint GetTextureVU() = 0;
  virtual CSizeInt GetTextureSize() = 0;
};

class CVaapi2Texture : public CVaapiTexture
{
public:
  bool Import(CVaapiRenderPicture* pic) override;
  void Reset() override;
  void Init(InteropInfo &interop) override;

  GLuint GetTextureY() override;
  GLuint GetTextureVU() override;
  CSizeInt GetTextureSize() override;

  // Probe every importable VA fourcc the renderer cares about and Add()
  // each successful one to caps.
  static void TestInteropFormats(VADisplay vaDpy, EGLDisplay eglDisplay, CCapabilities& caps);

private:
  static bool TestEsh(VADisplay vaDpy, EGLDisplay eglDisplay, std::uint32_t rtFormat, std::int32_t pixelFormat);

  struct MappedTexture
  {
    EGLImageKHR eglImage{EGL_NO_IMAGE_KHR};
    GLuint glTexture{};
  };

  InteropInfo m_interop;
  bool m_hasPlaneModifiers{false};
  bool m_imported{false};
  std::array<KODI::UTILS::POSIX::CFileHandle, 4> m_drmFDs;
  MappedTexture m_y, m_vu;
  CSizeInt m_textureSize;
};

//! \brief Textures cached per dma-buf identity; the render slot's picture reference pins content.
class CVaapiTexturePool
{
public:
  // must exceed the largest VAAPI surface pool (AV1: 27) or cycling evicts every frame
  static constexpr size_t MAX_ENTRIES = 32;

  ~CVaapiTexturePool() { ReleaseAll(); }

  void Init(InteropInfo& interop);
  //! \brief Imported texture for the picture's surface dma-buf; nullptr on export or import failure.
  CVaapi2Texture* Get(CVaapiRenderPicture* pic);
  //! \brief Reset and drop every cached texture; needs the GL context current.
  void ReleaseAll();

private:
  InteropInfo m_interop{};
  DRMPRIME::CDmaBufIdentityCache m_cache{MAX_ENTRIES};
  // cache handle = entry index + 1; freed slots are recycled
  std::vector<std::unique_ptr<CVaapi2Texture>> m_entries;
  std::vector<size_t> m_free;
};
}

