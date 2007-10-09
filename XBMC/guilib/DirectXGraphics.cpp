#include "include.h"
#include "DirectXGraphics.h"

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

#ifndef HAS_SDL
HRESULT XGWriteSurfaceToFile(LPDIRECT3DSURFACE8 pSurface, const char *fileName)
#else
HRESULT XGWriteSurfaceToFile(void* pixels, int width, int height, const char *fileName)
#endif
{
#ifndef HAS_SDL
  D3DLOCKED_RECT lr;
  D3DSURFACE_DESC desc;
  pSurface->GetDesc(&desc);
  if (S_OK == pSurface->LockRect(&lr, NULL, 0))
  {
#endif
    FILE *file = fopen(fileName, "wb");
    if (file)
    {
      // create a 24bit BMP header
      BMPHEAD bh;
      memset((char *)&bh,0,sizeof(BMPHEAD));
      memcpy(bh.id,"BM",2);
      bh.headersize = 54L;
      bh.infoSize = 0x28L;
#ifndef HAS_SDL
      bh.width = desc.Width;
      bh.height = desc.Height;
#else
      bh.width = width;
      bh.height = height;
#endif
      bh.biPlanes = 1;
      bh.bits = 24;
      bh.biCompression = 0L;

      //number of bytes per line in a BMP is divisible by 4
      long bytesPerLine = bh.width * 3;
      if (bytesPerLine & 0x0003)
      {
        bytesPerLine |= 0x0003;
        ++bytesPerLine;
      }
      // filesize = headersize + bytesPerLine * number of lines
      bh.filesize = bh.headersize + bytesPerLine * bh.height;
        
      fwrite(&bh.id, 1, sizeof(bh) - 2*sizeof(char), file);

      BYTE *lineBuf = new BYTE[bytesPerLine];
      memset(lineBuf, 0, bytesPerLine);
      // lines are stored in BMPs upside down
#ifndef HAS_SDL
      for (UINT y = bh.height; y; --y)
#else
      for (UINT y = 1 ; y <= (UINT) bh.height ; ++y) //compensate for gl's inverted Y axis
#endif
      {
#ifndef HAS_SDL
        BYTE *s = (BYTE *)lr.pBits + (y - 1) * lr.Pitch;
        BYTE *d = lineBuf;
#else
        BYTE *s = (BYTE *)pixels + (y - 1) * 4 * width;
        BYTE *d = lineBuf;
#endif
        for (UINT x = 0; x < (UINT) bh.width; x++)
        {
          *d++ = *(s + x * 4);
          *d++ = *(s + x * 4 + 1);
          *d++ = *(s + x * 4 + 2);
        }
        fwrite(lineBuf, 1, bytesPerLine, file);
      }
      delete[] lineBuf;
      fclose(file);
    }
#ifndef HAS_SDL
    pSurface->UnlockRect();
  }
#endif
  return S_OK;
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

#ifndef HAS_SDL
void GetTextureFromData(D3DTexture *pTex, void *texData, LPDIRECT3DTEXTURE8 *ppTexture)
#else
void GetTextureFromData(D3DTexture *pTex, void *texData, SDL_Surface* *ppTexture)
#endif
{
  XB_D3DFORMAT fmt;
  DWORD width, height, pitch, offset;
  ParseTextureHeader(pTex, fmt, width, height, pitch, offset);

#ifndef HAS_SDL
  D3DXCreateTexture(g_graphicsContext.Get3DDevice(), width, height, 1, 0, GetD3DFormat(fmt), D3DPOOL_MANAGED, ppTexture);
  D3DLOCKED_RECT lr;
  if (D3D_OK == (*ppTexture)->LockRect(0, &lr, NULL, 0))
#else
  *ppTexture = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
  if (SDL_LockSurface(*ppTexture) == 0)      	
#endif
  {
    BYTE *texDataStart = (BYTE *)texData;
    DWORD *color = (DWORD *)texData;
    texDataStart += offset;
#ifndef HAS_SDL
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
#else
    DWORD destPitch = (*ppTexture)->pitch;
    if (fmt == XB_D3DFMT_DXT1)
    {
      BYTE *decoded = new BYTE[destPitch * height];
      ConvertDXT1(texDataStart, width, height, decoded);
      texDataStart = decoded;
      pitch = destPitch;
    }
    else if (fmt == XB_D3DFMT_DXT2 || fmt == XB_D3DFMT_DXT4)
    {
      BYTE *decoded = new BYTE[destPitch * height];
      ConvertDXT4(texDataStart, width, height, decoded);
      texDataStart = decoded;
      pitch = destPitch;
    }
#endif
    if (IsSwizzledFormat(fmt))
    { // first we unswizzle
      BYTE *unswizzled = new BYTE[pitch * height];
      Unswizzle(texDataStart, BytesPerPixelFromFormat(fmt), width, height, unswizzled);
      texDataStart = unswizzled;
    }

#ifndef HAS_SDL
    BYTE *dstPixels = (BYTE *)lr.pBits;
#else
    BYTE *dstPixels = (BYTE *)(*ppTexture)->pixels;
#endif

    if (IsPalettedFormat(fmt))
    {
      for (unsigned int y = 0; y < height; y++)
      {
        BYTE *src = texDataStart + y * pitch;
        DWORD *dest = (DWORD *)(dstPixels + y * destPitch);
        for (unsigned int x = 0; x < width; x++)
          *dest++ = color[*src++];
      }
    }
    else
    {
      for (unsigned int y = 0; y < height; y++)
      {
        BYTE *src = texDataStart + y * pitch;
        BYTE *dest = dstPixels + y * destPitch;
        memcpy(dest, src, min((unsigned int)pitch, (unsigned int)destPitch));
      }
    }
#ifdef HAS_SDL
    if (IsSwizzledFormat(fmt) || fmt == XB_D3DFMT_DXT1 || fmt == XB_D3DFMT_DXT2 || fmt == XB_D3DFMT_DXT4)
#else
    if (IsSwizzledFormat(fmt))
#endif
    {
      delete[] texDataStart;
    }

#ifndef HAS_SDL
    (*ppTexture)->UnlockRect(0);
#else
    SDL_UnlockSurface(*ppTexture);
#endif
  }
}

#ifndef HAS_SDL
CXBPackedResource::CXBPackedResource()
{
  m_buffer = NULL;
}

CXBPackedResource::~CXBPackedResource()
{
  if (m_buffer)
    delete[] m_buffer;
  m_buffer = NULL;
}

HRESULT CXBPackedResource::Create(const char *fileName, int unused, void *unusedVoid)
{
  // load the file
  FILE *file = fopen(fileName, "rb");
  if (!file)
    return -1;

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  m_buffer = new BYTE[size];
  fread(m_buffer, 1, size, file);

  // check our header - we should really use this for the info instead of filesize
  fclose(file);
  return S_OK;
}

LPDIRECT3DTEXTURE8 CXBPackedResource::GetTexture(UINT unused)
{
  // now here's where the fun starts...
  LPDIRECT3DTEXTURE8 pTexture = NULL;

  D3DTexture *pTex = (D3DTexture *)(m_buffer + sizeof(XPR_HEADER));

  XPR_HEADER *hdr = (XPR_HEADER *)m_buffer;
  GetTextureFromData(pTex, m_buffer + hdr->dwHeaderSize, &pTexture);

  return pTexture;
}
#endif
