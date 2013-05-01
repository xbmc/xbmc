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
  float x, y, z;            // screen coords
  float u1, v1;             // texture coords (required for texturing)
  float u2, v2;             // difuse texture cords (required for diffuse texturing)
  unsigned char r, g, b, a; // per-vertex colors (only necessary when diffuse color varies by vertex)
};
typedef std::vector<PackedVertex> PackedVertices;

/*! \brief A collection of data required to perform a single render call.
     This can include many vertices as long as they can be batched into a
     single draw call
 */
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
  /*! \brief Queue a new batch in the scene. It will not be visible until
      the graph is drawn
   */
  void Add(const BatchDraw &batch);

  /*! \brief Discard all current batches in the scene. If the scene was not
       drawn already, the current batches will never be seen.
   */
  void Clear();

  /*! \brief Quick way to add a rectangle to the scene, with optional texture.
      If texture coords are missing, the entire texture will be drawn.
   */
  void DrawQuad(const CRect &rect, color_t color, CBaseTexture *texture=NULL, const CRect *texCoords=NULL);

// Only use these on a COPY of the Scene Graph. Forced double-buffering helps
// to avoid complicated Locking.

  /*! \brief helper function for iterating through batches*/
  const_iterator begin() const;
 
 /*! \brief helper function for iterating through batches*/
  const_iterator end() const;

  /*! \brief Perform quick optimizations on the scene based on similar
       attributes. Contiguous batches can be merged under some conditions which
       results in less draw calls
  */
  void MergeSimilar();

private:
  void RectToVertices(const CRect &rect, PackedVertices &packedvertices);
  std::vector<BatchDraw> m_batches;
  CCriticalSection m_critSection;
};
