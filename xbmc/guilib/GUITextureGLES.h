#pragma once

/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "GUITexture.h"

#include "system_gl.h"
#include <vector>
#include "utils/Color.h"

struct PackedVertex
{
  float x, y, z;
  float u1, v1;
  float u2, v2;
};
typedef std::vector<PackedVertex> PackedVertices;

class CRenderSystemGLES;

class CGUITextureGLES : public CGUITextureBase
{
public:
  CGUITextureGLES(float posX, float posY, float width, float height, const CTextureInfo& texture);
  static void DrawQuad(const CRect &coords, UTILS::Color color, CBaseTexture *texture = NULL, const CRect *texCoords = NULL);
protected:
  void Begin(UTILS::Color color);
  void Draw(float *x, float *y, float *z, const CRect &texture, const CRect &diffuse, int orientation);
  void End();

  GLubyte m_col[4];

  PackedVertices m_packedVertices;
  std::vector<GLushort> m_idx;
  CRenderSystemGLES *m_renderSystem;
};

