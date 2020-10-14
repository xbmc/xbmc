/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUITexture.h"
#include "utils/Color.h"

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

class CRenderSystemGLES;

class CGUITextureGLES : public CGUITexture
{
public:
  CGUITextureGLES(float posX, float posY, float width, float height, const CTextureInfo& texture);
  ~CGUITextureGLES() override = default;

  CGUITextureGLES* Clone() const override;

protected:
  void Begin(UTILS::Color color) override;
  void Draw(float* x, float* y, float* z, const CRect& texture, const CRect& diffuse, int orientation) override;
  void End() override;

private:
  CGUITextureGLES(const CGUITextureGLES& texture) = default;

  std::array<GLubyte, 4> m_col;

  PackedVertices m_packedVertices;
  std::vector<GLushort> m_idx;
  CRenderSystemGLES *m_renderSystem;
};

