/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "system_gl.h"

#include "GUITexture.h"
#include "utils/Color.h"

class CRenderSystemGL;

class CGUITextureGL : public CGUITextureBase
{
public:
  CGUITextureGL(float posX, float posY, float width, float height, const CTextureInfo& texture);
  static void DrawQuad(const CRect &coords, UTILS::Color color, CBaseTexture *texture = NULL, const CRect *texCoords = NULL);

protected:
  void Begin(UTILS::Color color) override;
  void Draw(float *x, float *y, float *z, const CRect &texture, const CRect &diffuse, int orientation) override;
  void End() override;

private:
  GLubyte m_col[4];

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

