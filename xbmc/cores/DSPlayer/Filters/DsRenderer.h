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
#include "dvdplayer/DVDPlayer.h"
#include "dvdplayer/DVDPlayerSubtitle.h"


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
  STDMETHODIMP RenderPresent(CD3DTexture* videoTexture,IDirect3DSurface9* videoSurface);
  void AddSubtitleStream();
  bool AddSubtitleFile(const std::string& filename);
  
protected:
  HRESULT CreateSurfaces(D3DFORMAT Format = D3DFMT_X8R8G8B8);
  void DeleteSurfaces();
  UINT    GetAdapter(IDirect3D9 *pD3D);
  CCritSec                                m_RenderLock;
  //d3d stuff
  CD3DTexture*                            m_pVideoTexture[DS_MAX_3D_SURFACE];
  IDirect3DSurface9*                      m_pVideoSurface[DS_MAX_3D_SURFACE];
  
  int                                     m_nCurSurface;// Surface currently displayed

  D3DFORMAT                               m_SurfaceType;// Surface type
  D3DFORMAT                               m_BackbufferType;// backbuffer type
  
  //Current video information
  int                                     m_iVideoWidth;
  int                                     m_iVideoHeight;

  CDVDPlayerSubtitle m_dsPlayerSubtitle; // subtitle part
  CDVDOverlayContainer m_overlayContainer;
  CCurrentStream m_CurrentSubtitle;
  CSelectionStreams m_SelectionStreams;
  CDVDDemux* m_pSubtitleDemuxer;
};

#endif // _DSRENDERER_H
