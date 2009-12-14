#ifndef _DSRENDERER_H
#define _DSRENDERER_H
#pragma once

#include <streams.h>
#pragma warning(disable : 4995)

#include <vector>

using namespace std;

#include <d3d9.h>

#include "IDsRenderer.h"
#include "DsSyncRenderer.h"

#include <vector>
#include "event.h"
#include "utils/CriticalSection.h"
#define DS_NBR_3D_SURFACE 3
#define DS_MAX_3D_SURFACE 10
[uuid("0403C469-E53E-4eda-8C1E-883CF2D760C7")]
class CDsRenderer  : public IDsRenderer,
                     public CUnknown,
                     public CCritSec
                    
{
public:
  CDsRenderer();
  virtual ~CDsRenderer();
  DECLARE_IUNKNOWN;
  // IDSRenderer
  STDMETHODIMP CreateRenderer(IUnknown** ppRenderer) { return E_NOTIMPL; };
  STDMETHODIMP RenderPresent(IDirect3DTexture9* videoTexture,IDirect3DSurface9* videoSurface,REFERENCE_TIME pTimeStamp);

protected:
  
  virtual bool SetDevice();
  virtual HRESULT CreateSurfaces(D3DFORMAT Format = D3DFMT_X8R8G8B8);
  virtual void DeleteSurfaces();
  virtual void StartThreads();
  virtual void StopThreads();

  CDsSyncRenderer                         m_SyncRenderer;
  
  CCritSec                                m_RenderLock;
  
  //d3d stuff
  CComPtr<IDirect3D9>                     m_D3D;
  CComPtr<IDirect3DDevice9>               m_D3DDev;
  CComPtr<IDirect3DSurface9>              m_pVideoSurface[DS_MAX_3D_SURFACE];
  CComPtr<IDirect3DTexture9>              m_pVideoTexture[DS_MAX_3D_SURFACE];
  
  int                                     m_nCurSurface;// Surface currently displayed

  D3DFORMAT                               m_SurfaceType;// Surface type
  D3DFORMAT                               m_BackbufferType;// backbuffer type
  
  //Current video information
  int                                     m_iVideoWidth;
  int                                     m_iVideoHeight;

  
};

#endif // _DSRENDERER_H
