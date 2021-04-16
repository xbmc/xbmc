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

#include "system_gl.h"

class CRenderSystemGL;

class CGUITextureGL : public CGUITexture
{
public:
  CGUITextureGL(float posX, float posY, float width, float height, const CTextureInfo& texture);
  ~CGUITextureGL() override = default;

  CGUITextureGL* Clone() const override;

protected:
  void Begin(UTILS::Color color) override;
  void Draw(float *x, float *y, float *z, const CRect &texture, const CRect &diffuse, int orientation) override;
  void End() override;

private:
  CGUITextureGL(const CGUITextureGL& texture) = default;

  std::array<GLubyte, 4> m_col;

  struct PackedVertex
  {
    float x, y, z;
    float u1, v1;
    float u2, v2;
  };

  std::vector<PackedVertex> m_packedVertices;
  std::vector<GLushort> m_idx;
  CRenderSystemGL *m_renderSystem;
};

