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
#include "BatchDraw.h"

BatchDraw::BatchDraw()
: m_texture(NULL), m_diffuseTexture(NULL), m_dirty(true), m_color(0)
{
}

void BatchDraw::Reset()
{
  *this = BatchDraw();
}

void BatchDraw::SetTexture(const CBaseTexture *texture)
{
  m_texture = (CBaseTexture*) texture;
}

void BatchDraw::SetDiffuseTexture(const CBaseTexture *diffuseTexture)
{
  m_diffuseTexture = (CBaseTexture*) diffuseTexture;
}

void BatchDraw::SetDirty(bool dirty)
{
  m_dirty = dirty;
}

void BatchDraw::SetColor(uint32_t color)
{
  m_color = color;
}

void BatchDraw::AddVertices(const PackedVertices &vertices)
{
  m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
}

void BatchDraw::AddVertices(const PackedVertex *vertex, int count)
{
  m_vertices.insert(m_vertices.end(), vertex, vertex+count);
}
