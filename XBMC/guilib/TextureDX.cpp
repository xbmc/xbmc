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
#include "TextureDX.h"
#include "../xbmc/FileSystem/File.h"
#include "../xbmc/FileSystem/Directory.h"
#include "../xbmc/FileSystem/SpecialProtocol.h"

#ifdef HAS_DX

DWORD PadPow2(DWORD x) 
{
  return x;
}

/************************************************************************/
/*    CDXTexture                                                       */
/************************************************************************/
CDXTexture::CDXTexture(unsigned int width, unsigned int height, unsigned int BPP)
: CBaseTexture(width, height, BPP)
{
  Allocate(m_imageWidth, m_imageHeight, m_nBPP);
}

CDXTexture::~CDXTexture()
{
  Delete();
}

CDXTexture::CDXTexture(CBaseTexture& texture)
{
  *this = texture;
}

CBaseTexture& CDXTexture::operator = (const CBaseTexture &rhs)
{
  if (this != &rhs) 
  {
    if(rhs.GetTextureObject() == NULL)
      return *this;

    m_pTexture = rhs.GetTextureObject();
    m_imageWidth = rhs.GetWidth();
    m_imageHeight = rhs.GetHeight();

    m_pTexture->AddRef();
  }

  return *this;
}

void CDXTexture::Allocate(unsigned int width, unsigned int height, unsigned int BPP)
{
  D3DFORMAT format;

  if(BPP == 8)
    format = D3DFMT_LIN_A8;
  else if(BPP == 32)
    format = D3DFMT_LIN_X8R8G8B8;
  else
    return;

  if (D3D_OK != D3DXCreateTexture(g_RenderSystem.GetD3DDevice(), width, height, 1, 0, format, D3DPOOL_MANAGED , &m_pTexture))
  {
    CLog::Log(LOGDEBUG, "CDXTexture::CDXTexture: Error creating new texture for size %d x %d", width, height);
  }

  m_imageWidth = width;
  m_imageHeight = height;
}

void CDXTexture::Delete()
{
  if(m_pTexture)
  {
    m_pTexture->Release();
    m_pTexture = NULL;
  }

  m_imageWidth = 0;
  m_imageHeight = 0;
}

bool CDXTexture::LoadFromFile(const CStdString& texturePath)
{
  D3DXIMAGE_INFO info;

  if ( D3DXCreateTextureFromFileEx(g_RenderSystem.GetD3DDevice(), _P(texturePath).c_str(),
    D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED,
    D3DX_FILTER_NONE , D3DX_FILTER_NONE, 0, &info, NULL, &m_pTexture) != D3D_OK)
  {
    return FALSE;
  }

  m_imageWidth = info.Width;
  m_imageHeight = info.Height;

  return TRUE;
}


bool CDXTexture::bool LoadFromMemory(unsigned int width, unsigned int height, unsigned int pitch, unsigned int BPP, unsigned char* pPixels)
{
  return TRUE;
}



/*
CDXTexture::CDXTexture(unsigned int w, unsigned int h, unsigned int BPP)
: CBaseTexture(w, h, BPP)
{
  D3DFORMAT format;

  if(BPP == 8)
    format = D3DFMT_LIN_A8;
  else
    format = D3DFMT_LIN_X8R8G8B8;

  if (D3D_OK != D3DXCreateTexture(g_RenderSystem.GetD3DDevice(), w, h, 1, 0, D3DFMT_LIN_A8, format, &m_pTexture))
  {
    CLog::Log(LOGDEBUG, "CDXTexture::CDXTexture: Error creating new texture for size %f", h);
  }
}

CDXTexture::CDXTexture(CTexture* surface, bool load, bool freeSurface) 
: CBaseTexture()
{
  Update(surface, load, freeSurface);
}

CDXTexture::~CDXTexture()
{
  if(m_pTexture)
  {
    DELETE_TEXTURE(m_pTexture);
    m_pTexture = NULL;
  }
}

bool CDXTexture::Load(const CStdString& texturePath)
{
  D3DXIMAGE_INFO info;

  if ( D3DXCreateTextureFromFileEx(g_RenderSystem.GetD3DDevice(), _P(texturePath).c_str(),
    D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED,
    D3DX_FILTER_NONE , D3DX_FILTER_NONE, 0, &info, NULL, &m_pTexture) != D3D_OK)
  {
    return FALSE;
  }

  imageWidth = info.Width;
  imageHeight = info.Height;

  textureWidth = info.Width;
  textureHeight = info.Height;

  return TRUE;
}

void CDXTexture::Update(CTexture* surface, bool loadToGPU, bool freeSurface)
{
  if(surface == NULL)
    return;

  m_pTexture = surface->GetTextureObject();

  LPDIRECT3DSURFACE9 textureSurface;
  m_pTexture->GetSurfaceLevel(0, &textureSurface);

  D3DSURFACE_DESC desc;
  textureSurface->GetDesc(&desc);

  imageWidth = desc.Width;
  imageHeight = desc.Height;

  textureWidth = desc.Width;;
  textureHeight = desc.Height;
 
}

void CDXTexture::Update(int w, int h, int pitch, const unsigned char *pixels, bool loadToGPU)
{
  imageWidth = w;
  imageHeight = h;

  if (!m_bRequiresPower2Textures)
  {
    textureWidth = imageWidth;
    textureHeight = imageHeight;
  }
  else
  {
    textureWidth = PadPow2(imageWidth);
    textureHeight = PadPow2(imageHeight);
  }

  if (D3DXCreateTexture(g_RenderSystem.GetD3DDevice(), textureWidth, textureHeight, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED, &m_pTexture) == D3D_OK)
  {
    D3DLOCKED_RECT lr;
    RECT rc = { 0, 0, textureWidth, textureHeight };
    if ( D3D_OK == m_pTexture->LockRect( 0, &lr, &rc, 0 ))
    {
      for (int y = 0; y < textureHeight; y++)
      {
        BYTE *dest = (BYTE *)lr.pBits + y * lr.Pitch;
        BYTE *source = (BYTE *)pixels + y * pitch;
        memcpy(dest, source, pitch);
      }
      m_pTexture->UnlockRect( 0 );
    }
  }
}

void CDXTexture::LoadToGPU()
{
  
}
*/

#endif
