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
#include "threads/CriticalSection.h"
#include "guilib/Geometry.h"

typedef uint32_t color_t;
class CBaseTexture;

struct PackedVertex
{
  float x, y, z;
  float u1, v1;
  float u2, v2;
  unsigned char r, g, b, a;
};
typedef std::vector<PackedVertex> PackedVertices;

struct BatchDraw
{
  BatchDraw() : texture(NULL), diffuseTexture(NULL), dirty(true), color(0) { vertices.reserve(4); };
  CBaseTexture *texture;
  CBaseTexture *diffuseTexture;
  bool dirty;
  uint32_t color;

  PackedVertices vertices;
};

class CSceneGraph
{
public:
  typedef const BatchDraw *const_iterator;
  CSceneGraph();
  CSceneGraph(const CSceneGraph& other);
  void Add(const BatchDraw &batch);
  void Clear();
  void RectToVertices(const CRect &rect, PackedVertices &packedvertices);
  void DrawQuad(const CRect &rect, color_t color, CBaseTexture *texture=NULL, const CRect *texCoords=NULL);

// Only use these on a COPY of the Scene Graph. Forced double-buffering helps
// to avoid complicated Locking.

  const_iterator begin() const;
  const_iterator end() const;
  void MergeSimilar();

private:
  std::vector<BatchDraw> m_batches;
  CCriticalSection m_critSection;
};
