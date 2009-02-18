/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "include.h"
#include "GUIBorderedImage.h"

CGUIBorderedImage::CGUIBorderedImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CTextureInfo& texture, const CTextureInfo& borderTexture, const FRECT &borderSize)
   : CGUIImage(dwParentID, dwControlId, posX + borderSize.left, posY + borderSize.top, width - borderSize.left - borderSize.right, height - borderSize.top - borderSize.bottom, texture),
     m_borderImage(posX, posY, width, height, borderTexture)
{
  memcpy(&m_borderSize, &borderSize, sizeof(FRECT));
  ControlType = GUICONTROL_BORDEREDIMAGE;
}

CGUIBorderedImage::CGUIBorderedImage(const CGUIBorderedImage &right)
: CGUIImage(right), m_borderImage(right.m_borderImage)
{
  memcpy(&m_borderSize, &right.m_borderSize, sizeof(FRECT));
  ControlType = GUICONTROL_BORDEREDIMAGE;
}

CGUIBorderedImage::~CGUIBorderedImage(void)
{
}

void CGUIBorderedImage::Render()
{
  if (!m_borderImage.GetFileName().IsEmpty() && m_texture.IsAllocated())
  {
    if (m_bInvalidated)
    {
      const CRect &rect = m_texture.GetRenderRect();
      m_borderImage.SetPosition(rect.x1 - m_borderSize.left, rect.y1 - m_borderSize.top);
      m_borderImage.SetWidth(rect.Width() + m_borderSize.left + m_borderSize.right);
      m_borderImage.SetHeight(rect.Height() + m_borderSize.top + m_borderSize.bottom);
    }
    m_borderImage.Render();
  }
  CGUIImage::Render();
}

void CGUIBorderedImage::PreAllocResources()
{
  m_borderImage.PreAllocResources();
  CGUIImage::PreAllocResources();
}

void CGUIBorderedImage::AllocResources()
{
  m_borderImage.AllocResources();
  CGUIImage::AllocResources();
}

void CGUIBorderedImage::FreeResources()
{
  m_borderImage.FreeResources();
  CGUIImage::FreeResources();
}

void CGUIBorderedImage::DynamicResourceAlloc(bool bOnOff)
{
  m_borderImage.DynamicResourceAlloc(bOnOff);
  CGUIImage::DynamicResourceAlloc(bOnOff);
}

bool CGUIBorderedImage::IsAllocated() const
{
  return m_borderImage.IsAllocated() && CGUIImage::IsAllocated();
}
