#include "stdafx.h"

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

DWORD SetTextureHeader(UINT Width, UINT Height, UINT Levels, UINT Usage, XB_D3DFORMAT Format, D3DPOOL Pool, D3DTexture *pTexture, UINT Data, UINT Pitch)
{
#ifdef USE_XDK
  return XGSetTextureHeader(Width, Height, Levels, Usage, (D3DFORMAT)Format, Pool, (LPDIRECT3DTEXTURE8)pTexture, Data, Pitch);
#else
  // TODO: No idea what most of this is.
  memset(pTexture, 0, sizeof(D3DTexture));
  pTexture->Common = D3DCOMMON_TYPE_TEXTURE + 1; // what does the 1 give??
  pTexture->Format |= (Format & 0xFF) << 8;
  pTexture->Format |= 0x10029; // no idea why
  pTexture->Data = Data;    // offset of texture data
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
  static int count=0;
  count++;
#endif
  return D3D_OK;
}

BOOL IsSwizzledFormat(XB_D3DFORMAT format)
{
#ifdef USE_XDK
  return XGIsSwizzledFormat((D3DFORMAT)format);
#else
  switch (format)
  {
  case XB_D3DFMT_A8R8G8B8:
  case XB_D3DFMT_P8:
    return true;
  default:
    return false;
  }
#endif
}

DWORD BytesPerPixelFromFormat(XB_D3DFORMAT format)
{
#ifdef USE_XDK
  return XGBytesPerPixelFromFormat((D3DFORMAT)format);
#else
  switch (format)
  {
  case XB_D3DFMT_A8R8G8B8:
  case XB_D3DFMT_LIN_A8R8G8B8:
  case XB_D3DFMT_DXT1:
  case XB_D3DFMT_DXT4:
    return 4;
  case XB_D3DFMT_P8:
    return 1;
  }
  return 0;
#endif
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


VOID SwizzleRect(LPCVOID pSource, DWORD Pitch, LPCRECT pRect, LPVOID pDest, DWORD Width, DWORD Height, CONST LPPOINT pPoint, DWORD BytesPerPixel)
{
#ifdef USE_XDK
  XGSwizzleRect(pSource, Pitch, pRect, pDest, Width, Height, pPoint, BytesPerPixel);
#else
  // Note: Doesn't listen to pRect or pPoint
  //       Also, knows nothing about Pitch
  Swizzle(pSource, BytesPerPixel, Width, Height, pDest);
#endif
}

extern LPDIRECT3DDEVICE8 pD3DDevice;
// Round a number to the nearest power of 2 rounding up
// runs pretty quickly - the only expensive op is the bsr
// alternive would be to dec the source, round down and double the result
// which is slightly faster but rounds 1 to 2

D3DFORMAT D3DFormat(XB_D3DFORMAT format)
{
  switch (format)
  {
  case XB_D3DFMT_LIN_A8R8G8B8:
    return D3DFMT_A8R8G8B8;
  case XB_D3DFMT_DXT1:
    return D3DFMT_DXT1;
  case XB_D3DFMT_DXT3:
    return D3DFMT_DXT3;
  case XB_D3DFMT_DXT5:
    return D3DFMT_DXT5;
  default:
    return (D3DFORMAT)format;
  }
}

HRESULT CompressRect(LPVOID pDestBuf, XB_D3DFORMAT DestFormat, DWORD DestPitch, DWORD Width, DWORD Height, LPVOID pSrcData, XB_D3DFORMAT SrcFormat, DWORD SrcPitch, FLOAT AlphaRef, DWORD Flags)
{
#ifdef USE_XDK
  return XGCompressRect(pDestBuf, (D3DFORMAT)DestFormat, DestPitch, Width, Height, pSrcData, (D3DFORMAT)SrcFormat, SrcPitch, AlphaRef, Flags);
#else
  // Note: The below doesn't do anything with AlphaRef or Flags
  LPDIRECT3DSURFACE8 pSrcSurf, pDestSurf;
  HRESULT hr = pD3DDevice->CreateImageSurface(Width, Height, D3DFormat(SrcFormat), &pSrcSurf);
  if (FAILED(hr)) return hr;

  D3DLOCKED_RECT rect;
  pSrcSurf->LockRect(&rect, NULL, 0);
  for (UINT y = 0; y < Height; y++)
  {
    BYTE *src = (BYTE *)pSrcData + y * SrcPitch;
    BYTE *dest = (BYTE *)rect.pBits + y * rect.Pitch;
    memcpy(dest, src, min((UINT)rect.Pitch, SrcPitch));
  }
  pSrcSurf->UnlockRect();

  int padWidth = PadPow2(Width), padHeight = PadPow2(Height);
  hr = pD3DDevice->CreateImageSurface(padWidth, padHeight, D3DFormat(DestFormat), &pDestSurf);
  if (FAILED(hr)) return hr;

	hr = D3DXLoadSurfaceFromSurface(pDestSurf, NULL, NULL, pSrcSurf, NULL, NULL, D3DX_FILTER_NONE, 0);
  if (FAILED(hr)) return hr;

  // copy into dest buffer
  pDestSurf->LockRect(&rect, NULL, 0);
  for (UINT y = 0; y < Height / 4; y++)
  {
    BYTE *src = (BYTE *)rect.pBits + y * rect.Pitch;
    BYTE *dest = (BYTE *)pDestBuf + y * DestPitch;
    memcpy(dest, src, min((UINT)rect.Pitch, DestPitch));
  }
  pDestSurf->UnlockRect();

  pSrcSurf->Release();
  pDestSurf->Release();

  return hr;
#endif
}
