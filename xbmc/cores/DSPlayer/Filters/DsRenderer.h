/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#ifndef _DSRENDERER_H
#define _DSRENDERER_H
#pragma once

#include <streams.h>
#pragma warning(disable : 4995)

#include <vector>

using namespace std;

#include <d3d9.h>

#include "IDsRenderer.h"


#include <vector>
#include "event.h"
#include "D3DResource.h"
#include "utils/CriticalSection.h"


#define DS_NBR_3D_SURFACE 3
#define DS_MAX_3D_SURFACE 10
[uuid("0403C469-E53E-4eda-8C1E-883CF2D760C7")]
class CDsRenderer  : public IDsRenderer,
                     public CUnknown,
                     public ID3DResource,
                     public CCritSec
                    
{
public:
  CDsRenderer();
  virtual ~CDsRenderer();
  DECLARE_IUNKNOWN;
  // IDSRenderer
  STDMETHODIMP CreateRenderer(IUnknown** ppRenderer) { return E_NOTIMPL; };
  STDMETHODIMP RenderPresent(IDirect3DTexture9* videoTexture,IDirect3DSurface9* videoSurface);  
protected:
  HRESULT CreateSurfaces(D3DFORMAT Format = D3DFMT_X8R8G8B8);
  void DeleteSurfaces();
  UINT    GetAdapter(IDirect3D9 *pD3D);
  CCritSec m_RenderLock;
  //d3d stuff
  Com::SmartPtr<IDirect3DTexture9>                      m_pVideoTexture[DS_MAX_3D_SURFACE];
  Com::SmartPtr<IDirect3DSurface9>                      m_pVideoSurface[DS_MAX_3D_SURFACE];
  
  int                                     m_nCurSurface;// Surface currently displayed

  D3DFORMAT                               m_SurfaceType;// Surface type
  D3DFORMAT                               m_BackbufferType;// backbuffer type
  
  //Current video information
  int                                     m_iVideoWidth;
  int                                     m_iVideoHeight;
};

#endif // _DSRENDERER_H
