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

  int            m_iVideoWidth;
  int            m_iVideoHeight;
  double         m_fps;

  volatile bool m_bStop;
  //Clock stuff
  LONGLONG       m_FlipTimeStamp;
  LONGLONG       m_llSampleTime;     // Time of current sample
  LONGLONG       m_llLastSampleTime; // Time of last sample
  LONGLONG       m_ptstarget; //position of sample
  REFERENCE_TIME m_rtTimePerFrame; // Time per frame of video in 100ns units. As extracted from video header or stream
  double         m_dFrameCycle; // Same as above but in ms units
  double         m_dCycleDifference; // Difference in video and display cycle time relative to the video cycle time
                             // calcul is easy as 1000.0 / display frequency
  double         m_dOptimumDisplayCycle; // The display cycle that is closest to the frame rate. A multiple of the actual display cycle
  double         m_dD3DRefreshCycle; // Display refresh cycle ms
  double         m_dEstRefreshCycle; // As estimated from scan lines

  bool           m_bSnapToVSync;
  bool           m_bNeedCheckSample;
  bool           m_bInterlaced;
  REFERENCE_TIME m_lastFrameArrivedTime;
  bool           m_lastFramePainted;  


  D3DFORMAT      m_SurfaceType;
  D3DFORMAT      m_BackbufferType;
  D3DFORMAT      m_DisplayType;


  CRITICAL_SECTION m_critPrensent;

  

  HRESULT AllocSurface(D3DFORMAT Format = D3DFMT_A8R8G8B8);
};




#endif // _DSRENDERER_H
