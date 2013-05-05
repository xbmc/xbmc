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

CBatchDraw::CBatchDraw()
: m_texture(NULL), m_diffuseTexture(NULL), m_dirty(true), m_color(0)
{
}

void CBatchDraw::Reset()
{
  *this = CBatchDraw();
}

void CBatchDraw::SetTexture(const CBaseTexture *texture)
{
  m_texture = (CBaseTexture*) texture;
}

void CBatchDraw::SetDiffuseTexture(const CBaseTexture *diffuseTexture)
{
  m_diffuseTexture = (CBaseTexture*) diffuseTexture;
}

void CBatchDraw::SetDirty(bool dirty)
{
  m_dirty = dirty;
}

void CBatchDraw::SetColor(uint32_t color)
{
  m_color = color;
}

void CBatchDraw::AddVertices(const PackedVertices &vertices)
{
  m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
}

void CBatchDraw::AddVertices(const PackedVertex *vertex, int count)
{
  m_vertices.insert(m_vertices.end(), vertex, vertex+count);
}

const CBaseTexture* CBatchDraw::GetTexture() const
{
  return m_texture;
}

const CBaseTexture* CBatchDraw::GetDiffuseTexture() const
{
  return m_diffuseTexture;
}

bool CBatchDraw::IsDirty() const
{
  return m_dirty;
}

uint32_t CBatchDraw::GetColor() const
{
  return m_color;
}
const PackedVertices* CBatchDraw::GetVertices() const
{
  return &m_vertices;
}
