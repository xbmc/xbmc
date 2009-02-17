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
#include "GUILargeImage.h"
#include "TextureManager.h"
#include "GUILargeTextureManager.h"

CGUILargeImage::CGUILargeImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& texture)
: CGUIImage(dwParentID, dwControlId, posX, posY, width, height, texture, true),
  m_fallbackImage(0, 0, posX, posY, width, height, texture)
{
  m_fallbackImage.SetFileName(texture.file.GetFallback(), true);  // true to specify that it's constant
  ControlType = GUICONTROL_LARGE_IMAGE;
}

CGUILargeImage::~CGUILargeImage(void)
{

}

void CGUILargeImage::PreAllocResources()
{
  CGUIImage::PreAllocResources();
  m_fallbackImage.PreAllocResources();
}

void CGUILargeImage::FreeResources()
{
  CGUIImage::FreeResources();
  m_fallbackImage.FreeResources();
}

void CGUILargeImage::Render()
{
  if (!m_texture.IsAllocated())
    m_fallbackImage.Render();
  else
    m_fallbackImage.FreeResources();
  CGUIImage::Render();
}

void CGUILargeImage::SetAspectRatio(const CAspectRatio &aspect)
{
  CGUIImage::SetAspectRatio(aspect);
  m_fallbackImage.SetAspectRatio(aspect);
}

