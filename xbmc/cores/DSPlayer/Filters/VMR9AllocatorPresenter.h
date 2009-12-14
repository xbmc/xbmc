#ifndef _DXALLOCATORPRESENTER_H
#define _DXALLOCATORPRESNETER_H
#pragma once

#include <vmr9.h>
#include <wxdebug.h>
#include <Wxutil.h>

#pragma warning(push, 2)

#pragma warning(disable : 4995)

#include <vector>
#pragma warning(pop)
using namespace std;

#include <d3d9.h>

#include "DsRenderer.h"
#include "geometry.h"

#include "VideoReferenceClock.h"
#define VMRBITMAP_UPDATE            0x80000000
class CDSGraph;

[uuid("A2636B41-5E3C-4426-B6BC-CD8616600912")]
class CVMR9AllocatorPresenter  : public CDsRenderer,
                                        IVMRSurfaceAllocator9,
                                        IVMRImagePresenter9
                    
{
public:
  CCritSec        m_VMR9AlphaBitmapLock;
  void          UpdateAlphaBitmap();
  CVMR9AllocatorPresenter(HRESULT& hr, CStdString &_Error);
  virtual ~CVMR9AllocatorPresenter();

  // IVMRSurfaceAllocator9
  virtual STDMETHODIMP InitializeDevice(DWORD_PTR dwUserID, VMR9AllocationInfo *lpAllocInfo, DWORD *lpNumBuffers);
  virtual STDMETHODIMP TerminateDevice(DWORD_PTR dwID);  
  virtual STDMETHODIMP GetSurface(DWORD_PTR dwUserID, DWORD SurfaceIndex, DWORD SurfaceFlags, IDirect3DSurface9 **lplpSurface);
  virtual STDMETHODIMP AdviseNotify(IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify);

  // IVMRImagePresenter9
  virtual STDMETHODIMP StartPresenting( DWORD_PTR dwUserID);
  virtual STDMETHODIMP StopPresenting( DWORD_PTR dwUserID);
  virtual STDMETHODIMP PresentImage( DWORD_PTR dwUserID, VMR9PresentationInfo *lpPresInfo);

  // IUnknown
  virtual STDMETHODIMP QueryInterface(REFIID riid,void** ppvObject);
  virtual ULONG STDMETHODCALLTYPE AddRef();
  virtual ULONG STDMETHODCALLTYPE Release();

  // IDSRenderer
  STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);

  // ID3DResource
  void OnLostDevice();
  //void OnResetDevice();
  //void OnDestroyDevice();
  void OnCreateDevice();

  UINT GetAdapter(IDirect3D9 *pD3D);
  STDMETHODIMP_(void) SetPosition(RECT w, RECT v) {} ;
  STDMETHODIMP_(bool) Paint(bool fAll) { return true; } ;
  STDMETHODIMP_(void) SetTime(REFERENCE_TIME rtNow) {};
  
  
protected:
  void DeleteSurfaces();
  HRESULT AllocVideoSurface(D3DFORMAT Format = D3DFMT_A8R8G8B8);
  void GetCurrentVideoSize();

  D3DFORMAT            m_SurfaceType;
  D3DFORMAT            m_BackbufferType;
  D3DFORMAT            m_DisplayType;

  VMR9AlphaBitmap      m_VMR9AlphaBitmap;
  CAutoVectorPtr<BYTE>  m_VMR9AlphaBitmapData;
  tagRECT          m_VMR9AlphaBitmapRect;
  int            m_VMR9AlphaBitmapWidthBytes;
  bool m_fUseInternalTimer;
  //Clock stuff
  REFERENCE_TIME m_rtTimePerFrame;
  REFERENCE_TIME m_pPrevEndFrame;
  int            m_iVideoWidth;
  int            m_iVideoHeight;

  float          m_fps;
  int            m_fFrameRate;
  bool           m_renderingOk;
private:
  long        m_refCount;
  CRITICAL_SECTION m_critPrensent;
  
  CComPtr<IDirect3D9>                     m_D3D;
  CComPtr<IDirect3DDevice9>               m_D3DDev;
  CComPtr<IVMRSurfaceAllocatorNotify9>    m_pIVMRSurfAllocNotify;

  vector<CComPtr<IDirect3DSurface9> >     m_pSurfaces;
  int                                     m_pNbrSurface;
  int                                     m_pCurSurface;
  CComPtr<IDirect3DTexture9>              m_pVideoTexture;
  CComPtr<IDirect3DSurface9>              m_pVideoSurface;
};

#endif // _DXALLOCATORPRESENTER_H
