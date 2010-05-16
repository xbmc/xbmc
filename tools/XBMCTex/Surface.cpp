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

// class CSurface
#include "Surface.h"

#ifndef HAS_SDL

#include <stdio.h>
#include "xbox.h"

CGraphicsDevice g_device;

CSurface::CSurface()
{
  m_surface = NULL;
  m_width = 0;
  m_height = 0;
  m_bpp = 0;
}

CSurface::~CSurface()
{
  if (m_surface)
    m_surface->Release();
}

bool CSurface::Create(unsigned int width, unsigned int height, CSurface::FORMAT format)
{
  if (m_surface)
  {
    m_surface->Release();
    m_surface = NULL;
  }

  m_info.width = width;
  m_info.height = height;
  m_info.format = format;

  if (format == FMT_LIN_ARGB)
  { // round width to multiple of 64 pixels
    m_width = (width + 63) & ~63;
    m_height = height;
  }
  else
  { // round up to nearest power of 2
    m_width = PadPow2(width);
    m_height = PadPow2(height);
  }
  m_bpp = (format == FMT_PALETTED) ? 1 : 4;

  if (!g_device.CreateSurface(m_width, m_height, format, this))
    return false;

  Clear();
  return true;
}

void CSurface::Clear()
{
  CSurfaceRect rect;
  if (Lock(&rect))
  {
    BYTE *pixels = rect.pBits;
    for (unsigned int i = 0; i < m_height; i++)
    {
      memset(pixels, 0, rect.Pitch);
      pixels += rect.Pitch;
    }
    Unlock();
  }
}

bool CSurface::CreateFromFile(const char *Filename, FORMAT format)
{
  // get the info from the file
	D3DXIMAGE_INFO info;
	if (S_OK != D3DXGetImageInfoFromFile(Filename, &info))
    return false;

  if (!Create(info.Width, info.Height, format))
    return false;

	bool success = (S_OK == D3DXLoadSurfaceFromFile(m_surface, NULL, NULL, Filename, NULL, D3DX_FILTER_NONE, 0, NULL));
  ClampToEdge();
  return success;
}


void CSurface::ClampToEdge()
{
  // fix up the last row and column to simulate clamp_to_edge
  if (!m_info.width || !m_info.height == 0)
    return; // invalid texture
  CSurfaceRect rect;
  if (Lock(&rect))
  {
    for (unsigned int y = 0; y < m_info.height; y++)
    {
      BYTE *src = rect.pBits + y * rect.Pitch;
      for (unsigned int x = m_info.width; x < m_width; x++)
        memcpy(src + x*m_bpp, src + (m_info.width - 1)*m_bpp, m_bpp);
    }
    BYTE *src = rect.pBits + (m_info.height - 1) * rect.Pitch;
    for (unsigned int y = m_info.height; y < m_height; y++)
    {
      BYTE *dest = rect.pBits + y * rect.Pitch;
      memcpy(dest, src, rect.Pitch);
    }
    Unlock();
  }
}

bool CSurface::Lock(CSurfaceRect *rect)
{
  D3DLOCKED_RECT lr;
  if (m_surface && rect)
  {
    m_surface->LockRect(&lr, NULL, 0);
    rect->pBits = (BYTE *)lr.pBits;
    rect->Pitch = lr.Pitch;
    return true;
  }
  return false;
}

bool CSurface::Unlock()
{
  if (m_surface)
  {
    m_surface->UnlockRect();
    return true;
  }
  return false;
}

CGraphicsDevice::CGraphicsDevice()
{
  m_pD3D = NULL;
  m_pD3DDevice = NULL;
}

CGraphicsDevice::~CGraphicsDevice()
{
  if (m_pD3DDevice)
    m_pD3DDevice->Release();
  if (m_pD3D)
    m_pD3D->Release();
}

bool CGraphicsDevice::Create()
{
	m_pD3D = Direct3DCreate8(D3D_SDK_VERSION);
	if (!m_pD3D)
	{
		printf("Cannot init D3D\n");
		return false;
	}

	HRESULT hr;
	D3DDISPLAYMODE dispMode;
	D3DPRESENT_PARAMETERS presentParams;

	m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dispMode);

	ZeroMemory(&presentParams, sizeof(presentParams));
	presentParams.Windowed = TRUE;
	presentParams.hDeviceWindow = GetConsoleWindow();
	presentParams.SwapEffect = D3DSWAPEFFECT_COPY;
	presentParams.BackBufferWidth = 8;
	presentParams.BackBufferHeight = 8;
	presentParams.BackBufferFormat = dispMode.Format;

	hr = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentParams, &m_pD3DDevice);
	if (FAILED(hr))
	{
		printf("Cannot init D3D device: %08x\n", hr);
		m_pD3D->Release();
    return false;
	}
  return true;
}

bool CGraphicsDevice::CreateSurface(unsigned int width, unsigned int height, enum CSurface::FORMAT format, CSurface *surface)
{
  if (m_pD3DDevice)
    return (S_OK == m_pD3DDevice->CreateImageSurface(width, height, (format == CSurface::FMT_PALETTED) ? D3DFMT_P8 : D3DFMT_A8R8G8B8, &surface->m_surface));
  return false;
}

#endif