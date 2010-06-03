/*
 *      Copyright (C) 2004-2009 Team XBMC
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

#include "xbox.h"
#include "Surface.h"
#include "EndianSwap.h"

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

bool  IsPowerOf2(UINT number)
{
  return (number & (~number+1)) == number;
}

int GetLog2(UINT Number)
{
  int power = 0;
  while (Number)
  {
    Number >>= 1;
    power++;
  }
  return power-1;
}

BOOL IsPaletted(XB_D3DFORMAT format)
{
  switch (format)
  {
  case XB_D3DFMT_P8:
    return true;
  default:
    return false;
  }
}

void SetTextureHeader(UINT Width, UINT Height, UINT Levels, UINT Usage, XB_D3DFORMAT Format, D3DTexture *pTexture, UINT Data, UINT Pitch)
{
  // TODO: No idea what most of this is.
  // Byte swapping to convert the header on big-endian system
  memset(pTexture, 0, sizeof(D3DTexture));
  pTexture->Common = Endian_SwapLE32(D3DCOMMON_TYPE_TEXTURE + 1); // what does the 1 give??
  pTexture->Format |= (Format & 0xFF) << 8;
  pTexture->Format |= 0x10029; // no idea why
  pTexture->Data = Endian_SwapLE32(Data);    // offset of texture data
  if (IsPowerOf2(Width) && IsPowerOf2(Height))
  {
    pTexture->Format |= (GetLog2(Width) & 0xF) << 20;
    pTexture->Format |= (GetLog2(Height) & 0xF) << 24;
  }
  else
  {
    pTexture->Size |= (Width - 1) & 0xfff;
    pTexture->Size |= ((Height - 1) & 0xfff) << 12;
    pTexture->Size |= (((Pitch >> 6) & 0xff) - 1) << 24;
  }
  pTexture->Format = Endian_SwapLE32(pTexture->Format);
  pTexture->Size = Endian_SwapLE32(pTexture->Size);
}

BOOL IsSwizzledFormat(XB_D3DFORMAT format)
{
  switch (format)
  {
  case XB_D3DFMT_A8R8G8B8:
  case XB_D3DFMT_P8:
    return true;
  default:
    return false;
  }
}

DWORD BytesPerPixelFromFormat(XB_D3DFORMAT format)
{
  switch (format)
  {
  case XB_D3DFMT_A8R8G8B8:
  case XB_D3DFMT_LIN_A8R8G8B8:
  case XB_D3DFMT_DXT1:
  case XB_D3DFMT_DXT4:
    return 4;
  case XB_D3DFMT_P8:
    return 1;
  default:
    break;
  }
  return 0;
}

// Swizzle.
// Format is:

// 00 01 04 05
// 02 03 06 07
// 08 09 12 13
// 10 11 14 15 ...

// Currently only works for 32bit and 8bit textures, with power of 2 width and height
void Swizzle(const void *src, unsigned int depth, unsigned int width, unsigned int height, void *dest)
{
  for (UINT y = 0; y < height; y++)
  {
    UINT sy = 0;
    if (y < width)
    {
      for (int bit = 0; bit < 16; bit++)
        sy |= ((y >> bit) & 1) << (2*bit);
      sy <<= 1; // y counts twice
    }
    else
    {
      UINT y_mask = y % width;
      for (int bit = 0; bit < 16; bit++)
        sy |= ((y_mask >> bit) & 1) << (2*bit);
      sy <<= 1; // y counts twice
      sy += (y / width) * width * width;
    }
    BYTE *s = (BYTE *)src + y * width * depth;
    for (UINT x = 0; x < width; x++)
    {
      UINT sx = 0;
      if (x < height * 2)
      {
        for (int bit = 0; bit < 16; bit++)
          sx |= ((x >> bit) & 1) << (2*bit);
      }
      else
      {
        int x_mask = x % (2*height);
        for (int bit = 0; bit < 16; bit++)
          sx |= ((x_mask >> bit) & 1) << (2*bit);
        sx += (x / (2 * height)) * 2 * height * height;
      }
      BYTE *d = (BYTE *)dest + (sx + sy)*depth;
      for (unsigned int i = 0; i < depth; ++i)
        *d++ = *s++;
    }
  }
}


VOID SwizzleRect(LPCVOID pSource, DWORD Pitch, LPVOID pDest, DWORD Width, DWORD Height, DWORD BytesPerPixel)
{
  // knows nothing about Pitch
  Swizzle(pSource, BytesPerPixel, Width, Height, pDest);
}
