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
#include <vector>

#include "system_gl.h"

struct PackedVertex
{
  float x, y, z;
  float u1, v1;
  float u2, v2;
};
typedef std::vector<PackedVertex> PackedVertices;

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
  void Begin(KODI::UTILS::COLOR::Color color) override;
  void Draw(float* x, float* y, float* z, const CRect& texture, const CRect& diffuse, int orientation) override;
  void End() override;

  // Call while the GL context is still valid: on app exit, CGUIWindowManager::DestroyWindows() (and
  // this object's destructor) runs only after CRenderSystemGLES::DestroyRenderSystem() tears it down.
  void Free() override;

private:
  // Doesn't copy m_VBO/m_IBO -- each clone must own and (re)populate its own GL buffers rather
  // than share/double-delete the source's.
  CGUITextureGLES(const CGUITextureGLES& texture);

  std::array<GLubyte, 4> m_col;

  PackedVertices m_packedVertices;
  std::vector<GLushort> m_idx;
  CRenderSystemGLES *m_renderSystem;
  bool m_isGLES20{true};

  KODI::UTILS::GL::CGLBufferObject m_VBO{GL_ARRAY_BUFFER};
  KODI::UTILS::GL::CGLBufferObject m_IBO{GL_ELEMENT_ARRAY_BUFFER};
};

