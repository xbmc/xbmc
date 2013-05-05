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
#include "BatchDraw.h"

class CBaseTexture;

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
  void Reset();

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
