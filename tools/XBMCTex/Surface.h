// class CSurface - wraps the various interfaces (SDL/DirectX)
#pragma once

#ifdef USE_SDL
#include "SurfaceSDL.h"
#else

#define WIN32_LEAN_AND_MEAN   // Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT 0x500    // Win 2k/XP REQUIRED!
#include <windows.h>

#include "D3D8.h"
#include "D3DX8.h"

class CSurfaceRect
{
public:
  CSurfaceRect()
  {
    pBits = NULL;
    Pitch = 0;
  };
  BYTE *pBits;
  DWORD Pitch;
};

class CSurface
{
public:
  CSurface();
  ~CSurface();

  enum FORMAT { FMT_ARGB, FMT_LIN_ARGB, FMT_PALETTED };
  struct ImageInfo
  {
    unsigned int width;
    unsigned int height;
    FORMAT format;
  };

  bool CreateFromFile(const char *Filename, FORMAT format);

  bool Create(unsigned int width, unsigned int height, FORMAT format);

  bool Lock(CSurfaceRect *rect);
  bool Unlock();

  unsigned int Width() const { return m_width; };
  unsigned int Height() const { return m_height; };
  unsigned int BPP() const { return m_bpp; };
  unsigned int Pitch() const { return m_width * m_bpp; };

  const ImageInfo &Info() const { return m_info; };
private:
  friend class CGraphicsDevice;
  void Clear();
  LPDIRECT3DSURFACE8 m_surface;
  unsigned int m_width;
  unsigned int m_height;
  unsigned int m_bpp;
  ImageInfo m_info;
};

class CGraphicsDevice
{
public:
  CGraphicsDevice();
  ~CGraphicsDevice();
  bool Create();
  bool CreateSurface(unsigned int width, unsigned int height, CSurface::FORMAT format, CSurface *surface);
private:
  LPDIRECT3D8 m_pD3D;
  LPDIRECT3DDEVICE8 m_pD3DDevice;
};

extern CGraphicsDevice g_device;
#endif