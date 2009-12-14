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
#include "D3DResource.h"

#include "event.h"
#include "utils/CriticalSection.h"

[uuid("0403C469-E53E-4eda-8C1E-883CF2D760C7")]
class CDsRenderer  : public IDsRenderer,
                     public ID3DResource,
                     public CUnknown
                    
{
public:
  CDsRenderer();
  virtual ~CDsRenderer();
  DECLARE_IUNKNOWN;
  // IDSRenderer
  STDMETHODIMP CreateRenderer(IUnknown** ppRenderer) { return E_NOTIMPL; };
  STDMETHODIMP RenderPresent(IDirect3DTexture9* videoTexture,IDirect3DSurface9* videoSurface);
  // ID3DResource
  void OnLostDevice();
  void OnResetDevice();
  void OnDestroyDevice();
  void OnCreateDevice();

private:
  bool m_devicevalid;
  bool m_deviceused;

  CCriticalSection m_critsection;
  CEvent           m_createevent;
  CEvent           m_releaseevent;
  
  CComPtr<IDirect3D9>                     m_D3D;
  CComPtr<IDirect3DDevice9>               m_D3DDev;
};

#endif // _DSRENDERER_H
