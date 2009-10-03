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

#include "Texture.h"
#include "Picture.h"
#include "WindowingFactory.h"
#include "utils/log.h"

#define MAX_PICTURE_WIDTH  2048
#define MAX_PICTURE_HEIGHT 2048

/************************************************************************/
/*                                                                      */
/************************************************************************/
CBaseTexture::CBaseTexture(unsigned int width, unsigned int height, unsigned int format)
{
  m_texture = NULL;
  m_pixels = NULL;
  m_loadedToGPU = false;
  Allocate(width, height, format);
}

CBaseTexture::~CBaseTexture()
{
  delete[] m_pixels;
}

void CBaseTexture::Allocate(unsigned int width, unsigned int height, unsigned int format)
{
  m_imageWidth = width;
  m_imageHeight = height;
  m_format = format;

  if (!g_Windowing.SupportsNPOT((m_format & XB_FMT_DXT_MASK) != 0))
  {
    m_textureWidth = PadPow2(m_imageWidth);
    m_textureHeight = PadPow2(m_imageHeight);
  }
  else
  {
    m_textureWidth = m_imageWidth;
    m_textureHeight = m_imageHeight;
  }
  if (m_format & XB_FMT_DXT_MASK)
  { // DXT textures must be a multiple of 4 in width and height
    m_textureWidth = ((m_textureWidth + 3) / 4) * 4;
    m_textureHeight = ((m_textureHeight + 3) / 4) * 4;
  }

  delete[] m_pixels;
  m_pixels = new unsigned char[GetPitch() * GetRows()];
}

void CBaseTexture::Update(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, const unsigned char *pixels, bool loadToGPU) 
{
  if (pixels == NULL)
    return;

  Allocate(width, height, format);

  unsigned int srcPitch = pitch ? pitch : GetPitch(width);
  unsigned int srcRows = GetRows(height);
  unsigned int dstPitch = GetPitch(m_textureWidth);
  unsigned int dstRows = GetRows(m_textureHeight);

  assert(srcPitch <= dstPitch && srcRows <= dstRows);

  if (srcPitch == dstPitch)
    memcpy(m_pixels, pixels, srcPitch * srcRows);
  else
  {
    const unsigned char *src = pixels;
    unsigned char* dst = m_pixels;
    for (unsigned int y = 0; y < srcRows; y++)
    {
      memcpy(dst, src, srcPitch);
      src += srcPitch;
      dst += dstPitch;
    }
  }
  ClampToEdge();

  if (loadToGPU)
    LoadToGPU();
}

void CBaseTexture::ClampToEdge()
{
  unsigned int imagePitch = GetPitch(m_imageWidth);
  unsigned int imageRows = GetRows(m_imageHeight);
  unsigned int texturePitch = GetPitch(m_textureWidth);
  unsigned int textureRows = GetRows(m_textureHeight);
  if (imagePitch < texturePitch)
  {
    unsigned int blockSize = GetBlockSize();
    unsigned char *src = m_pixels + imagePitch - blockSize;
    unsigned char *dst = m_pixels;
    for (unsigned int y = 0; y < imageRows; y++)
    {
      for (unsigned int x = imagePitch; x < texturePitch; x += blockSize)
        memcpy(dst + x, src, blockSize);
      dst += texturePitch;
    }
  }

  if (imageRows < textureRows)
  {
    unsigned char *dst = m_pixels + imageRows * texturePitch;
    for (unsigned int y = imageRows; y < textureRows; y++)
    {
      memcpy(dst, dst - texturePitch, texturePitch);
      dst += texturePitch;
    }
  }
}

bool CBaseTexture::LoadFromFile(const CStdString& texturePath)
{
  CPicture pic;
  if(!pic.Load(texturePath, this, MAX_PICTURE_WIDTH, MAX_PICTURE_HEIGHT))
  {
    CLog::Log(LOGERROR, "Texture manager unable to load file: %s", texturePath.c_str());
    return false;
  }
  ClampToEdge();
  return true;
}

bool CBaseTexture::LoadFromMemory(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, unsigned char* pixels)
{
  m_imageWidth = width;
  m_imageHeight = height;
  m_format = format;
  Update(width, height, pitch, format, pixels, false);
  return true;
}

bool CBaseTexture::LoadPaletted(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, const unsigned char *pixels, const COLOR *palette)
{
  if (pixels == NULL || palette == NULL)
    return false;

  Allocate(width, height, format);

  for (unsigned int y = 0; y < height; y++)
  {
    unsigned char *dest = m_pixels + y * GetPitch();
    const unsigned char *src = pixels + y * pitch;
    for (unsigned int x = 0; x < width; x++)
    {
      COLOR col = palette[*src++];
      *dest++ = col.b;
      *dest++ = col.g;
      *dest++ = col.r;
      *dest++ = col.x;
    }
  }
  ClampToEdge();
  return true;
}

unsigned int CBaseTexture::PadPow2(unsigned int x) 
{
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return ++x;
}

unsigned int CBaseTexture::GetPitch(unsigned int width) const
{
  switch (m_format)
  {
  case XB_FMT_DXT1:
    return ((width + 3) / 4) * 8;
  case XB_FMT_DXT3:
  case XB_FMT_DXT5:
  case XB_FMT_DXT5_YCoCg:
    return ((width + 3) / 4) * 16;
  case XB_FMT_A8:
    return width;
  case XB_FMT_A8R8G8B8:
  default:
    return width*4;
  }
}

unsigned int CBaseTexture::GetRows(unsigned int height) const
{
  switch (m_format)
  {
  case XB_FMT_DXT1:
    return (height + 3) / 4;
  case XB_FMT_DXT3:
  case XB_FMT_DXT5:
  case XB_FMT_DXT5_YCoCg:
    return (height + 3) / 4;
  default:
    return height;
  }
}

unsigned int CBaseTexture::GetBlockSize() const
{
  switch (m_format)
  {
  case XB_FMT_DXT1:
    return 8;
  case XB_FMT_DXT3:
  case XB_FMT_DXT5:
  case XB_FMT_DXT5_YCoCg:
    return 16;
  case XB_FMT_A8:
    return 1;
  default:
    return 4;
  }
}
