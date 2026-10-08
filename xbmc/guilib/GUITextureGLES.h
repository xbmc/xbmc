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
  bool DrawQuads(const std::vector<Quad>& quads, unsigned int version) override;

private:
  CGUITextureGLES(const CGUITextureGLES& texture);

  // See m_attrsnap and m_attrgrad0/1 in gles_shader.vert.
  struct QuadVertex
  {
    float x, y;
    float oppositeX, oppositeY, push;
    float u1, v1;
    float u2, v2;
    float du1dx, dv1dx, du1dy, dv1dy;
    float du2dx, dv2dx, du2dy, dv2dy;
  };

  std::array<GLubyte, 4> m_col;

  KODI::UTILS::GL::CGLBufferObject m_quadBuffer{GL_ARRAY_BUFFER};
  unsigned int m_quadBufferVersion{0};
  std::size_t m_quadCount{0};

  CRenderSystemGLES *m_renderSystem;
  bool m_isGLES20{true};
};

