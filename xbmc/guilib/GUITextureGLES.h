/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUITexture.h"
#include "utils/ColorUtils.h"
#include "utils/GLBufferObject.h"

#include <array>
#include <cstddef>
#include <vector>

#include "system_gl.h"

class CGUIQuadDrawerGLES;
class CRenderSystemGLES;

class CGUITextureGLES : public CGUITexture
{
public:
  static void Register(CGUIQuadDrawerGLES& quadDrawer);
  static CGUITexture* CreateTexture(
      float posX, float posY, float width, float height, const CTextureInfo& texture);

  CGUITextureGLES(float posX, float posY, float width, float height, const CTextureInfo& texture);
  ~CGUITextureGLES() override = default;

  CGUITextureGLES* Clone() const override;

protected:
  void Free() override;
  void Begin(KODI::UTILS::COLOR::Color color) override;
  void Draw(float* x, float* y, float* z, const CRect& texture, const CRect& diffuse, int orientation) override;
  void End() override;
  bool DrawQuads(const std::vector<Quad>& quads, unsigned int version, const CRect& rect) override;

private:
  CGUITextureGLES(const CGUITextureGLES& texture);

  // One corner of a quad and its opposite corner; see m_attrpos and m_attrsnap in gles_shader.vert.
  struct QuadVertex
  {
    std::array<float, 4> corner; // anchor x, anchor y, offset x, offset y
    std::array<float, 4> opposite;
    std::array<float, 4> texture; // u, v at this corner and at the opposite one
    std::array<float, 4> diffuse;
  };

  std::array<GLubyte, 4> m_col;

  KODI::UTILS::GL::CGLBufferObject m_quadBuffer{GL_ARRAY_BUFFER};
  unsigned int m_quadBufferVersion{0};
  std::size_t m_quadCount{0};

  CRenderSystemGLES *m_renderSystem;
  bool m_isGLES20{true};
};

