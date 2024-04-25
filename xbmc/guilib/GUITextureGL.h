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

#include <array>

#include "system_gl.h"

class CRenderSystemGL;

class CGUITextureGL : public CGUITexture
{
public:
  static void Register();
  static CGUITexture* CreateTexture(
      float posX, float posY, float width, float height, const CTextureInfo& texture);

  static void DrawQuad(const CRect& coords,
                       UTILS::COLOR::Color color,
                       CTexture* texture = nullptr,
                       const CRect* texCoords = nullptr,
                       const float depth = 1.0,
                       const bool blending = true);

  CGUITextureGL(float posX, float posY, float width, float height, const CTextureInfo& texture);
  ~CGUITextureGL() override = default;

  CGUITextureGL* Clone() const override;

protected:
  void Begin(UTILS::COLOR::Color color) override;
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

