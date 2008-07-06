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

// TODO: Override SetFileName() to not FreeResources() immediately.  Instead, keep the current image around until we
//       actually have a new image in AllocResources().

CGUILargeImage::CGUILargeImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& texture)
: CGUIImage(dwParentID, dwControlId, posX, posY, width, height, texture, 0), m_fallbackImage(dwParentID, dwControlId, posX, posY, width, height, texture)
{
  m_fallbackImage.SetFileName(texture.file.GetFallback(), true);  // true to set it constant
  ControlType = GUICONTROL_LARGE_IMAGE;
  m_usingBundledTexture = false;
}

CGUILargeImage::~CGUILargeImage(void)
{

}

void CGUILargeImage::AllocateOnDemand()
{
  // if we're hidden, we can free our resources and return
  if (!IsVisible() && m_visible != DELAYED)
  {
    if (m_bDynamicResourceAlloc && IsAllocated())
      FreeResources();
    return;
  }

  // either visible or delayed - we need the resources allocated in either case
  if (!m_texturesAllocated || !m_vecTextures.size())
    AllocResources();
}

void CGUILargeImage::PreAllocResources()
{
  FreeResources();
  if (!m_image.diffuse.IsEmpty())
    g_TextureManager.PreLoad(m_image.diffuse);
}

void CGUILargeImage::AllocResources()
{
  if (m_strFileName.IsEmpty())
    return;
  if (m_vecTextures.size())
    FreeTextures();
  // don't call CGUIControl::AllocResources(), as this resets m_hasRendered, which we don't want
  SetInvalid();
  m_bAllocated = true;

  // first check our textureManager for bundled files
  int iImages = g_TextureManager.Load(m_strFileName, m_dwColorKey, true);
  if (iImages)
  {
    m_texturesAllocated = true;
    for (int i = 0; i < iImages; i++)
    {
#ifndef HAS_SDL
      LPDIRECT3DTEXTURE8 texture;
#elif defined(HAS_SDL_2D)
      SDL_Surface * texture;
#else
      CGLTexture * texture;
#endif
      texture = g_TextureManager.GetTexture(m_strFileName, i, m_iTextureWidth, m_iTextureHeight, m_pPalette, m_linearTexture);
      m_vecTextures.push_back(texture);
    }
    m_usingBundledTexture = true;
  }
  else
  { // use our large image background loader
#ifndef HAS_SDL
    LPDIRECT3DTEXTURE8 texture;
#elif defined(HAS_SDL_2D)
    SDL_Surface * texture;
#else
    CGLTexture * texture;
#endif
    texture = g_largeTextureManager.GetImage(m_strFileName, m_iTextureWidth, m_iTextureHeight, m_orientation, !m_texturesAllocated);
    m_texturesAllocated = true;

    if (!texture)
      return;

    m_usingBundledTexture = false;
    m_vecTextures.push_back(texture);
  }

  m_linearTexture = false;

  CalculateSize();

  LoadDiffuseImage();
}

void CGUILargeImage::FreeResources()
{
  m_fallbackImage.FreeResources();
  CGUIImage::FreeResources();
}

void CGUILargeImage::FreeTextures()
{
  if (m_texturesAllocated)
  {
    if (m_usingBundledTexture)
      g_TextureManager.ReleaseTexture(m_strFileName);
    else
      g_largeTextureManager.ReleaseImage(m_strFileName);
  }

  if (m_diffuseTexture)
    g_TextureManager.ReleaseTexture(m_image.diffuse);
  m_diffuseTexture = NULL;
  m_diffusePalette = NULL;

  m_vecTextures.erase(m_vecTextures.begin(), m_vecTextures.end());
  m_iCurrentImage = 0;
  m_iCurrentLoop = 0;
  m_iImageWidth = 0;
  m_iImageHeight = 0;
  m_texturesAllocated = false;
}

int CGUILargeImage::GetOrientation() const
{
  if (m_usingBundledTexture) return m_image.orientation;
  // otherwise multiply our orientations
  static char orient_table[] = { 0, 1, 2, 3, 4, 5, 6, 7,
                                 1, 0, 3, 2, 5, 4, 7, 6,
                                 2, 3, 0, 1, 6, 7, 4, 5,
                                 3, 2, 1, 0, 7, 6, 5, 4,
                                 4, 7, 6, 5, 0, 3, 2, 1,
                                 5, 6, 7, 4, 1, 2, 3, 0,
                                 6, 5, 4, 7, 2, 1, 0, 3,
                                 7, 4, 5, 6, 3, 0, 1, 2 };
  return (int)orient_table[8 * m_image.orientation + m_orientation];
}

void CGUILargeImage::SetFileName(const CStdString& strFileName, bool setConstant)
{
  if (setConstant)
    m_image.file.SetLabel(strFileName, "");
  // no fallback is required - it's handled at rendertime
  if (m_strFileName.Equals(strFileName)) return;
  // Don't completely free resources here - we may be just changing
  // filenames mid-animation
  FreeTextures();  // TODO: perhaps freetextures might be done better after a fade or something?
  m_strFileName = strFileName;
  // Don't allocate resources here as this is done at render time
}

void CGUILargeImage::Render()
{
  if (!m_vecTextures.size())
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

