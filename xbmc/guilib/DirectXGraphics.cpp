/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "DirectXGraphics.h"
#include "Texture.h"
#include "XBTF.h"

LPVOID XPhysicalAlloc(SIZE_T s, DWORD ulPhysicalAddress, DWORD ulAlignment, DWORD flProtect)
{
  return malloc(s);
}

void XPhysicalFree(LPVOID lpAddress)
{
  free(lpAddress);
}

D3DFORMAT GetD3DFormat(XB_D3DFORMAT format)
{
  switch (format)
  {
  case XB_D3DFMT_A8R8G8B8:
  case XB_D3DFMT_LIN_A8R8G8B8:
    return D3DFMT_LIN_A8R8G8B8;
  case XB_D3DFMT_DXT1:
    return D3DFMT_DXT1;
  case XB_D3DFMT_DXT2:
    return D3DFMT_DXT2;
  case XB_D3DFMT_DXT4:
    return D3DFMT_DXT4;
  case XB_D3DFMT_P8:
    return D3DFMT_LIN_A8R8G8B8;
  default:
    return D3DFMT_UNKNOWN;
  }
}

DWORD BytesPerPixelFromFormat(XB_D3DFORMAT format)
{
  switch (format)
  {
  case XB_D3DFMT_A8R8G8B8:
  case XB_D3DFMT_LIN_A8R8G8B8:
  case XB_D3DFMT_DXT4:
    return 4;
  case XB_D3DFMT_P8:
  case XB_D3DFMT_DXT1:
  case XB_D3DFMT_DXT2:
    return 1;
  default:
    return 0;
  }
}

bool IsPalettedFormat(XB_D3DFORMAT format)
{
  if (format == XB_D3DFMT_P8)
    return true;
  return false;
}

void ParseTextureHeader(D3DTexture *tex, XB_D3DFORMAT &fmt, DWORD &width, DWORD &height, DWORD &pitch, DWORD &offset)
{
  fmt = (XB_D3DFORMAT)((tex->Format & 0xff00) >> 8);
  offset = tex->Data;
  if (tex->Size)
  {
    width = (tex->Size & 0x00000fff) + 1;
    height = ((tex->Size & 0x00fff000) >> 12) + 1;
    pitch = (((tex->Size & 0xff000000) >> 24) + 1) << 6;
  }
  else
  {
    width = 1 << ((tex->Format & 0x00f00000) >> 20);
    height = 1 << ((tex->Format & 0x0f000000) >> 24);
    pitch = width * BytesPerPixelFromFormat(fmt);
  }
}

bool IsSwizzledFormat(XB_D3DFORMAT format)
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

// Unswizzle.
// Format is:

// 00 01 04 05
// 02 03 06 07
// 08 09 12 13
// 10 11 14 15 ...

// Currently only works for 32bit and 8bit textures, with power of 2 width and height
void Unswizzle(const void *src, unsigned int depth, unsigned int width, unsigned int height, void *dest)
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
    BYTE *d = (BYTE *)dest + y * width * depth;
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
      BYTE *s = (BYTE *)src + (sx + sy)*depth;
      for (unsigned int i = 0; i < depth; ++i)
        *d++ = *s++;
    }
  }
}

void DXT1toARGB(const void *src, void *dest, unsigned int destWidth)
{
  const BYTE *b = (const BYTE *)src;
  // colour is in R5G6B5 format, convert to R8G8B8
  DWORD colour[4];
  BYTE red[4];
  BYTE green[4];
  BYTE blue[4];
  for (int i = 0; i < 2; i++)
  {
    red[i] = b[2*i+1] & 0xf8;
    green[i] = ((b[2*i+1] & 0x7) << 5) | ((b[2*i] & 0xe0) >> 3);
    blue[i] = (b[2*i] & 0x1f) << 3;
    colour[i] = (red[i] << 16) | (green[i] << 8) | blue[i];
  }
  if (colour[0] > colour[1])
  {
    red[2] = (2 * red[0] + red[1] + 1) / 3;
    green[2] = (2 * green[0] + green[1] + 1) / 3;
    blue[2] = (2 * blue[0] + blue[1] + 1) / 3;
    red[3] = (red[0] + 2 * red[1] + 1) / 3;
    green[3] = (green[0] + 2 * green[1] + 1) / 3;
    blue[3] = (blue[0] + 2 * blue[1] + 1) / 3;
    for (int i = 0; i < 4; i++)
      colour[i] = (red[i] << 16) | (green[i] << 8) | blue[i] | 0xFF000000;
  }
  else
  {
    red[2] = (red[0] + red[1]) / 2;
    green[2] = (green[0] + green[1]) / 2;
    blue[2] = (blue[0] + blue[1]) / 2;
    for (int i = 0; i < 3; i++)
      colour[i] = (red[i] << 16) | (green[i] << 8) | blue[i] | 0xFF000000;
    colour[3] = 0;  // transparent
  }
  // ok, now grab the bits
  for (int y = 0; y < 4; y++)
  {
    DWORD *d = (DWORD *)dest + destWidth * y;
    *d++ = colour[(b[4 + y] & 0x03)];
    *d++ = colour[(b[4 + y] & 0x0c) >> 2];
    *d++ = colour[(b[4 + y] & 0x30) >> 4];
    *d++ = colour[(b[4 + y] & 0xc0) >> 6];
  }
}

void DXT4toARGB(const void *src, void *dest, unsigned int destWidth)
{
  const BYTE *b = (const BYTE *)src;
  BYTE alpha[8];
  alpha[0] = b[0];
  alpha[1] = b[1];
  if (alpha[0] > alpha[1])
  {
    alpha[2] = (6 * alpha[0] + 1 * alpha[1]+ 3) / 7;
    alpha[3] = (5 * alpha[0] + 2 * alpha[1] + 3) / 7;    // bit code 011
    alpha[4] = (4 * alpha[0] + 3 * alpha[1] + 3) / 7;    // bit code 100
    alpha[5] = (3 * alpha[0] + 4 * alpha[1] + 3) / 7;    // bit code 101
    alpha[6] = (2 * alpha[0] + 5 * alpha[1] + 3) / 7;    // bit code 110
    alpha[7] = (1 * alpha[0] + 6 * alpha[1] + 3) / 7;    // bit code 111
  }
  else
  {
    alpha[2] = (4 * alpha[0] + 1 * alpha[1] + 2) / 5;    // Bit code 010
    alpha[3] = (3 * alpha[0] + 2 * alpha[1] + 2) / 5;    // Bit code 011
    alpha[4] = (2 * alpha[0] + 3 * alpha[1] + 2) / 5;    // Bit code 100
    alpha[5] = (1 * alpha[0] + 4 * alpha[1] + 2) / 5;    // Bit code 101
    alpha[6] = 0;                                      // Bit code 110
    alpha[7] = 255;                                    // Bit code 111
  }
  // ok, now grab the bits
  BYTE a[4][4];
  a[0][0] = alpha[(b[2] & 0xe0) >> 5];
  a[0][1] = alpha[(b[2] & 0x1c) >> 2];
  a[0][2] = alpha[((b[2] & 0x03) << 1) | ((b[3] & 0x80) >> 7)];
  a[0][3] = alpha[(b[3] & 0x70) >> 4];
  a[1][0] = alpha[(b[3] & 0x0e) >> 1];
  a[1][1] = alpha[((b[3] & 0x01) << 2) | ((b[4] & 0xc0) >> 6)];
  a[1][2] = alpha[(b[4] & 0x38) >> 3];
  a[1][3] = alpha[(b[4] & 0x07)];
  a[2][0] = alpha[(b[5] & 0xe0) >> 5];
  a[2][1] = alpha[(b[5] & 0x1c) >> 2];
  a[2][2] = alpha[((b[5] & 0x03) << 1) | ((b[6] & 0x80) >> 7)];
  a[2][3] = alpha[(b[6] & 0x70) >> 4];
  a[3][0] = alpha[(b[6] & 0x0e) >> 1];
  a[3][1] = alpha[((b[6] & 0x01) << 2) | ((b[7] & 0xc0) >> 6)];
  a[3][2] = alpha[(b[7] & 0x38) >> 3];
  a[3][3] = alpha[(b[7] & 0x07)];

  b = (BYTE *)src + 8;
  // colour is in R5G6B5 format, convert to R8G8B8
  DWORD colour[4];
  BYTE red[4];
  BYTE green[4];
  BYTE blue[4];
  for (int i = 0; i < 2; i++)
  {
    red[i] = b[2*i+1] & 0xf8;
    green[i] = ((b[2*i+1] & 0x7) << 5) | ((b[2*i] & 0xe0) >> 3);
    blue[i] = (b[2*i] & 0x1f) << 3;
  }
  red[2] = (2 * red[0] + red[1] + 1) / 3;
  green[2] = (2 * green[0] + green[1] + 1) / 3;
  blue[2] = (2 * blue[0] + blue[1] + 1) / 3;
  red[3] = (red[0] + 2 * red[1] + 1) / 3;
  green[3] = (green[0] + 2 * green[1] + 1) / 3;
  blue[3] = (blue[0] + 2 * blue[1] + 1) / 3;
  for (int i = 0; i < 4; i++)
    colour[i] = (red[i] << 16) | (green[i] << 8) | blue[i];
  // and assign them to our texture
  for (int y = 0; y < 4; y++)
  {
    DWORD *d = (DWORD *)dest + destWidth * y;
    *d++ = colour[(b[4 + y] & 0x03)] | (a[y][0] << 24);
    *d++ = colour[(b[4 + y] & 0x0e) >> 2] | (a[y][1] << 24);
    *d++ = colour[(b[4 + y] & 0x30) >> 4] | (a[y][2] << 24);
    *d++ = colour[(b[4 + y] & 0xe0) >> 6] | (a[y][3] << 24);
  }

}

void ConvertDXT1(const void *src, unsigned int width, unsigned int height, void *dest)
{
  for (unsigned int y = 0; y < height; y += 4)
  {
    for (unsigned int x = 0; x < width; x += 4)
    {
      const BYTE *s = (const BYTE *)src + y * width / 2 + x * 2;
      DWORD *d = (DWORD *)dest + y * width + x;
      DXT1toARGB(s, d, width);
    }
  }
}

void ConvertDXT4(const void *src, unsigned int width, unsigned int height, void *dest)
{
  // [4 4 4 4][4 4 4 4]
  //
  //
  //
  for (unsigned int y = 0; y < height; y += 4)
  {
    for (unsigned int x = 0; x < width; x += 4)
    {
      const BYTE *s = (const BYTE *)src + y * width + x * 4;
      DWORD *d = (DWORD *)dest + y * width + x;
      DXT4toARGB(s, d, width);
    }
  }
}

void GetTextureFromData(D3DTexture *pTex, void *texData, CBaseTexture **ppTexture)
{
  XB_D3DFORMAT fmt;
  DWORD width, height, pitch, offset;
  ParseTextureHeader(pTex, fmt, width, height, pitch, offset);

  *ppTexture = new CTexture(width, height, XB_FMT_A8R8G8B8);

  if (*ppTexture)
  {
    BYTE *texDataStart = (BYTE *)texData;
    COLOR *color = (COLOR *)texData;
    texDataStart += offset;
/* DXMERGE - We should really support DXT1,DXT2 and DXT4 in both renderers
             Perhaps we should extend CTexture::Update() to support a bunch of different texture types
             Rather than assuming linear 32bits
             We could just override, as at least then all the loading code from various texture formats
             will be in one place

    BYTE *dstPixels = (BYTE *)lr.pBits;
    DWORD destPitch = lr.Pitch;
    if (fmt == XB_D3DFMT_DXT1)  // Not sure if these are 100% correct, but they seem to work :P
    {
      pitch /= 2;
      destPitch /= 4;
    }
    else if (fmt == XB_D3DFMT_DXT2)
    {
      destPitch /= 4;
    }
    else if (fmt == XB_D3DFMT_DXT4)
    {
      pitch /= 4;
      destPitch /= 4;
    }
*/
    if (fmt == XB_D3DFMT_DXT1)
    {
      pitch = width * 4;
      BYTE *decoded = new BYTE[pitch * height];
      ConvertDXT1(texDataStart, width, height, decoded);
      texDataStart = decoded;
    }
    else if (fmt == XB_D3DFMT_DXT2 || fmt == XB_D3DFMT_DXT4)
    {
      pitch = width * 4;
      BYTE *decoded = new BYTE[pitch * height];
      ConvertDXT4(texDataStart, width, height, decoded);
      texDataStart = decoded;
    }
    if (IsSwizzledFormat(fmt))
    { // first we unswizzle
      BYTE *unswizzled = new BYTE[pitch * height];
      Unswizzle(texDataStart, BytesPerPixelFromFormat(fmt), width, height, unswizzled);
      texDataStart = unswizzled;
    }

    if (IsPalettedFormat(fmt))
      (*ppTexture)->LoadPaletted(width, height, pitch, XB_FMT_A8R8G8B8, texDataStart, color);
    else
      (*ppTexture)->LoadFromMemory(width, height, pitch, XB_FMT_A8R8G8B8, true, texDataStart);

    if (IsSwizzledFormat(fmt) || fmt == XB_D3DFMT_DXT1 || fmt == XB_D3DFMT_DXT2 || fmt == XB_D3DFMT_DXT4)
    {
      delete[] texDataStart;
    }
  }
}
