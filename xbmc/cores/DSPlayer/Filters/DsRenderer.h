#ifndef _DSRENDERER_H
#define _DSRENDERER_H
#pragma once

#include <streams.h>
#pragma warning(disable : 4995)

#include <vector>
#pragma warning(pop)
using namespace std;

#include <d3d9.h>

#include "IDsRenderer.h"
#include "DSVideoClock.h"

#include "event.h"
#include "utils/CriticalSection.h"




[uuid("0403C469-E53E-4eda-8C1E-883CF2D760C7")]
class CDsRenderer  : public IDsRenderer,
                     public CUnknown
                    
{
public:
  CDsRenderer();
  virtual ~CDsRenderer();
  DECLARE_IUNKNOWN;
  // IDSRenderer
  STDMETHODIMP CreateRenderer(IUnknown** ppRenderer) { return E_NOTIMPL; };
  STDMETHODIMP RenderPresent(IDirect3DTexture9* videoTexture,IDirect3DSurface9* videoSurface);

  


private:
  CCriticalSection m_critsection;
  
protected:
  CComPtr<IDirect3D9>                     m_D3D;
  CComPtr<IDirect3DDevice9>               m_D3DDev;
  CComPtr<IDirect3DTexture9>              m_pVideoTexture;
  CComPtr<IDirect3DSurface9>              m_pVideoSurface;
  CDSVideoClock                           m_VideoClock;
  void InitClock();

  D3DFORMAT            m_SurfaceType;
  D3DFORMAT            m_BackbufferType;
  D3DFORMAT            m_DisplayType;


  CRITICAL_SECTION m_critPrensent;

  int            m_iVideoWidth;
  int            m_iVideoHeight;
  float          m_fps;

  HRESULT AllocSurface(D3DFORMAT Format = D3DFMT_A8R8G8B8);
};


#endif // _DSRENDERER_H
