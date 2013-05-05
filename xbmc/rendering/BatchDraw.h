/*
*      Copyright (C) 2005-2013 Team XBMC
*      http://www.xbmc.org
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
#include <vector>
#include <stdint.h>
typedef uint32_t color_t;
struct PackedVertex
{
  float x, y, z;            // screen coords
  float u1, v1;             // texture coords (required for texturing)
  float u2, v2;             // difuse texture cords (required for diffuse texturing)
  unsigned char r, g, b, a; // per-vertex colors (only necessary when diffuse color varies by vertex)
};
typedef std::vector<PackedVertex> PackedVertices;
class CBaseTexture;
/*! \brief A collection of data required to perform a single render call.
     This can include many vertices as long as they can be batched into a
     single draw call
 */
class BatchDraw
{
friend class CSceneGraph;
public:
  BatchDraw();
  void Reset();
  void SetTexture(const CBaseTexture *texture);
  void SetDiffuseTexture(const CBaseTexture *diffuseTexture);
  void SetDirty(bool dirty);
  void SetColor(uint32_t color);
  void AddVertices(const PackedVertices &vertices);
  void AddVertices(const PackedVertex *vertex, int count);
private:
  CBaseTexture *m_texture;
  CBaseTexture *m_diffuseTexture;
  bool m_dirty;
  uint32_t m_color;
  PackedVertices m_vertices;
};

