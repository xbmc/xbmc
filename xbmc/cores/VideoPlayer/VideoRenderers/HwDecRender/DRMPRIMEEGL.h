/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Buffers/VideoBufferDRMPRIME.h"
#include "utils/EGLImage.h"
#include "utils/Geometry.h"

#include <array>

#include "system_gl.h"

class CDRMPRIMETexture
{
public:
  CDRMPRIMETexture() = default;
  ~CDRMPRIMETexture();

  bool Map(CVideoBufferDRMPRIME* buffer);
  void Unmap();
  void Init(EGLDisplay eglDisplay);

  GLuint GetTexture() { return m_texture; }
  CSizeInt GetTextureSize() { return {m_texWidth, m_texHeight}; }

protected:
  CVideoBufferDRMPRIME* m_primebuffer{nullptr};
  std::unique_ptr<CEGLImage> m_eglImage;

  const GLenum m_textureTarget{GL_TEXTURE_EXTERNAL_OES};
  GLuint m_texture{0};
  int m_texWidth{0};
  int m_texHeight{0};
};

/*
 * CDRMPRIMETextureYUV: import a DRMPRIME dma-buf as separate per-plane
 * GL_TEXTURE_2D textures, the same way VAAPIGLES handles VA-API surfaces
 * (see CVaapi2Texture::Map). Used by the limited-range render path on
 * CRendererDRMPRIMEGLES so the YUV->RGB matrix can be applied in a
 * standard BaseYUV2RGBGLSLShader instead of being baked in by the OES
 * external sampler's full-range-only Mesa matrix.
 *
 * Supports a fixed set of source DRM fourccs (NV12 / P010 / P012 / P016 /
 * YUV420). For other formats, SupportsFormat returns false and the
 * caller is expected to fall back to the OES path.
 */
class CDRMPRIMETextureYUV
{
public:
  static constexpr int MAX_PLANES = 3;

  CDRMPRIMETextureYUV() = default;
  ~CDRMPRIMETextureYUV();

  CDRMPRIMETextureYUV(const CDRMPRIMETextureYUV&) = delete;
  CDRMPRIMETextureYUV& operator=(const CDRMPRIMETextureYUV&) = delete;

  void Init(EGLDisplay eglDisplay);
  bool Map(CVideoBufferDRMPRIME* buffer);
  void Unmap();

  GLuint GetTexture(int plane) const { return m_textures[plane]; }
  int GetNumPlanes() const { return m_numPlanes; }
  CSizeInt GetTextureSize() const { return {m_texWidth, m_texHeight}; }
  uint32_t GetSourceFormat() const { return m_sourceFormat; }

  // Returns true if the given DRM fourcc can be imported by this class.
  static bool SupportsFormat(uint32_t fourcc);

protected:
  CVideoBufferDRMPRIME* m_primebuffer{nullptr};
  EGLDisplay m_eglDisplay{nullptr};
  std::array<std::unique_ptr<CEGLImage>, MAX_PLANES> m_eglImages;
  std::array<GLuint, MAX_PLANES> m_textures{{0, 0, 0}};
  int m_numPlanes{0};
  int m_texWidth{0};
  int m_texHeight{0};
  uint32_t m_sourceFormat{0};
};
