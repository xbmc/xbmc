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
#include "Texture.h"
#include "Picture.h"
#include "WindowingFactory.h"

#define MAX_PICTURE_WIDTH  2048
#define MAX_PICTURE_HEIGHT 2048

/************************************************************************/
/*                                                                      */
/************************************************************************/
CBaseTexture::CBaseTexture(unsigned int width, unsigned int height, unsigned int BPP)
{
  m_imageWidth = width;
  m_imageHeight = height;
  m_pTexture = NULL;
  m_pPixels = NULL;
  m_nBPP = BPP;
  m_loadedToGPU = false;
}

CBaseTexture::~CBaseTexture()
{
  
}

CBaseTexture::CBaseTexture(const CBaseTexture& texture)
{
 
}

void CBaseTexture::Allocate(unsigned int width, unsigned int height, unsigned int BPP)
{
  if (BPP != 0)
    m_nBPP = BPP;

  m_imageWidth = width;
  m_imageHeight = height;

  if (g_Windowing.NeedPower2Texture())
  {
    m_nTextureWidth = PadPow2(m_imageWidth);
    m_nTextureHeight = PadPow2(m_imageHeight);
  }
  else
  {
    m_nTextureWidth = m_imageWidth;
    m_nTextureHeight = m_imageHeight;
  }

  delete[] m_pPixels;
  m_pPixels = new unsigned char[m_nTextureWidth * m_nTextureHeight * m_nBPP / 8];
}

void CBaseTexture::Update(int w, int h, int pitch, const unsigned char *pixels, bool loadToGPU) 
{
  if (pixels == NULL)
    return;

  delete[] m_pPixels;

  Allocate(w, h, 0);

  // Resize texture to POT if needed
  const unsigned char *src = pixels;
  unsigned char* resized = m_pPixels;

  for (int y = 0; y < h; y++)
  {
    memcpy(resized, src, pitch); // make sure pitch is not bigger than our width
    src += pitch;

    // repeat last column to simulate clamp_to_edge
    for(unsigned int i = pitch; i < m_nTextureWidth*4; i+=4)
      memcpy(resized+i, src-4, 4);

    resized += (m_nTextureWidth * 4);
  }

  // repeat last row to simulate clamp_to_edge
  for(unsigned int y = h; y < m_nTextureHeight; y++) 
  {
    memcpy(resized, src - pitch, pitch);

    // repeat last column to simulate clamp_to_edge
    for(unsigned int i = pitch; i < m_nTextureWidth*4; i+=4) 
      memcpy(resized+i, src-4, 4);

    resized += (m_nTextureWidth * 4);
  }

  if (loadToGPU)
    LoadToGPU();
}

bool CBaseTexture::LoadFromFile(const CStdString& texturePath)
{
  CPicture pic;

  CBaseTexture* original = new CTexture();
    
  if(!pic.Load(texturePath, original, MAX_PICTURE_WIDTH, MAX_PICTURE_HEIGHT))
  {
    CLog::Log(LOGERROR, "Texture manager unable to load file: %s", texturePath.c_str());
    return 0;
  }
  m_imageWidth = original->GetWidth();
  m_imageHeight = original->GetHeight();
  m_nBPP = original->GetBPP();

  Update(original->GetWidth(), original->GetHeight(), original->GetPitch(), original->GetPixels(), false);

  delete original;

  return true;
}

bool CBaseTexture::LoadFromMemory(unsigned int width, unsigned int height, unsigned int pitch, unsigned int BPP, unsigned char* pPixels)
{
  m_imageWidth = width;
  m_imageHeight = height;
  m_nBPP = BPP;
  Update(width, height, pitch, pPixels, false);

  return true;
}

DWORD CBaseTexture::PadPow2(DWORD x) 
{
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return ++x;
}
