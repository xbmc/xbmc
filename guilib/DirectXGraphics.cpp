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

HRESULT XGWriteSurfaceToFile(LPDIRECT3DSURFACE8 pSurface, const char *fileName)
{
  D3DLOCKED_RECT lr;
  D3DSURFACE_DESC desc;
  pSurface->GetDesc(&desc);
  if (S_OK == pSurface->LockRect(&lr, NULL, 0))
  {
    FILE *file = fopen(fileName, "wb");
    if (file)
    {
      // create a 24bit BMP header
      BMPHEAD bh;
      memset((char *)&bh,0,sizeof(BMPHEAD));
      memcpy(bh.id,"BM",2);
      bh.headersize = 54L;
      bh.infoSize = 0x28L;
      bh.width = desc.Width;
      bh.height = desc.Height;
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
      for (UINT y = desc.Height; y; --y)
      {
        BYTE *s = (BYTE *)lr.pBits + (y - 1) * lr.Pitch;
        BYTE *d = lineBuf;
        for (UINT x = 0; x < desc.Width; x++)
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
    pSurface->UnlockRect();
  }
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

void GetTextureFromData(D3DTexture *pTex, void *texData, LPDIRECT3DTEXTURE8 *ppTexture)
{
  XB_D3DFORMAT fmt;
  DWORD width, height, pitch, offset;
  ParseTextureHeader(pTex, fmt, width, height, pitch, offset);
  D3DXCreateTexture(g_graphicsContext.Get3DDevice(), width, height, 1, 0, GetD3DFormat(fmt), D3DPOOL_MANAGED, ppTexture);
  D3DLOCKED_RECT lr;
  if (D3D_OK == (*ppTexture)->LockRect(0, &lr, NULL, 0))
  {
    BYTE *texDataStart = (BYTE *)texData;
    DWORD *color = (DWORD *)texData;
    texDataStart += offset;
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
    if (IsSwizzledFormat(fmt))
    { // first we unswizzle
      BYTE *unswizzled = new BYTE[pitch * height];
      Unswizzle(texDataStart, BytesPerPixelFromFormat(fmt), width, height, unswizzled);
      texDataStart = unswizzled;
    }
    if (IsPalettedFormat(fmt))
    {
      for (unsigned int y = 0; y < height; y++)
      {
        BYTE *src = texDataStart + y * pitch;
        DWORD *dest = (DWORD *)((BYTE *)lr.pBits + y * destPitch);
        for (unsigned int x = 0; x < width; x++)
          *dest++ = color[*src++];
      }
    }
    else
    {
      for (unsigned int y = 0; y < height; y++)
      {
        BYTE *src = texDataStart + y * pitch;
        BYTE *dest = (BYTE *)lr.pBits + y * destPitch;
        memcpy(dest, src, min(pitch, (unsigned int)destPitch));
      }
    }
    if (IsSwizzledFormat(fmt))
    {
      delete[] texDataStart;
    }
    (*ppTexture)->UnlockRect(0);
  }
}

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