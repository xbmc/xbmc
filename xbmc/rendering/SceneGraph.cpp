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

#include "SceneGraph.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "guilib/GraphicContext.h"
CSceneGraph::CSceneGraph()
{
}

CSceneGraph::CSceneGraph( const CSceneGraph& other )
{
  CSingleLock lock(m_critSection);
  m_batches = other.m_batches;
}

CSceneGraph::const_iterator CSceneGraph::begin() const
{
  return m_batches.size() ? &*m_batches.begin() : NULL;
}

CSceneGraph::const_iterator CSceneGraph::end() const
{
  return m_batches.size() ? &*m_batches.end() : NULL;
}

void CSceneGraph::Add(const BatchDraw &batch)
{
  CSingleLock lock(m_critSection);
  m_batches.push_back(batch);
}

void CSceneGraph::Clear()
{
  CSingleLock lock(m_critSection);
  m_batches.clear();
}

void CSceneGraph::DrawQuad(const CRect &rect, color_t color, CBaseTexture *texture, const CRect *texCoords)
{
  BatchDraw quad;
  PackedVertices packedvertices;
  RectToVertices(rect, packedvertices);

  if (texture)
  {
    CRect coords = texCoords ? *texCoords : CRect(0.0f, 0.0f, 1.0f, 1.0f);
    packedvertices[0].u1 = coords.x1;
    packedvertices[0].v1 = coords.y1;
    packedvertices[1].u1 = coords.x2;
    packedvertices[1].v1 = coords.y1;
    packedvertices[2].u1 = coords.x2;
    packedvertices[2].v1 = coords.y2;
    packedvertices[3].u1 = coords.x1;
    packedvertices[3].v1 = coords.y2;
  }

  quad.SetColor(color);
  quad.SetTexture(texture);
  quad.AddVertices(packedvertices);
  Add(quad);
}

void CSceneGraph::RectToVertices(const CRect &rect, PackedVertices &packedvertices)
{
#define ROUND_TO_PIXEL(x) (float)(MathUtils::round_int(x))
  PackedVertex vertex;
  vertex.x = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalXCoord(rect.x1, rect.y1));
  vertex.y = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalYCoord(rect.x1, rect.y1));
  vertex.z = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalZCoord(rect.x1, rect.y1));
  packedvertices.push_back(vertex);

  vertex.x = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalXCoord(rect.x2, rect.y1));
  vertex.y = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalYCoord(rect.x2, rect.y1));
  vertex.z = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalZCoord(rect.x2, rect.y1));
  packedvertices.push_back(vertex);

  vertex.x = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalXCoord(rect.x2, rect.y2));
  vertex.y = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalYCoord(rect.x2, rect.y2));
  vertex.z = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalZCoord(rect.x2, rect.y2));
  packedvertices.push_back(vertex);

  vertex.x = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalXCoord(rect.x1, rect.y2));
  vertex.y = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalYCoord(rect.x1, rect.y2));
  vertex.z = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalZCoord(rect.x1, rect.y2));
  packedvertices.push_back(vertex);
}

void CSceneGraph::MergeSimilar()
{
  if (m_batches.size() < 2) return;
  std::vector<BatchDraw>::iterator batchesEnd = m_batches.end();
  for (std::vector<BatchDraw>::iterator i =  m_batches.begin(); i != batchesEnd -1;)
  {
    if( i->m_texture == (i+1)->m_texture && \
        i->m_diffuseTexture == (i+1)->m_diffuseTexture && \
        i->m_vertices.size() >= 4 && \
        i->m_color == (i+1)->m_color && \
        (i+1)->m_vertices.size() >= 4)

    {
      i->m_vertices.insert(i->m_vertices.end(), (i+1)->m_vertices.begin(), (i+1)->m_vertices.end());
      i = m_batches.erase(i+1);
      batchesEnd = m_batches.end();
      if (i == batchesEnd)
        break;
    }
    else
      i++;
  }
}
