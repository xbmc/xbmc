/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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

