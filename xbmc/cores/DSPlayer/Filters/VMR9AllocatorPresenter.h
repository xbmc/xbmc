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

class CDSGraph;
enum VMR_STATE
{
  RENDER_STATE_NEEDDEVICE = 0,
  RENDER_STATE_RUNNING,
  RENDER_STATE_DEVICELOST,
  
};

[uuid("A2636B41-5E3C-4426-B6BC-CD8616600912")]
class CVMR9AllocatorPresenter  : public CDsRenderer,
                                        IVMRSurfaceAllocator9,
                                        IVMRImagePresenter9
                    
{
public:
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

  STDMETHODIMP_(void) SetPosition(RECT w, RECT v) {} ;
  STDMETHODIMP_(bool) Paint(bool fAll) { return true; } ;
  STDMETHODIMP_(void) SetTime(REFERENCE_TIME rtNow) {};
  
  void ChangeD3dDev();
  virtual void OnLostDevice();
  virtual void OnDestroyDevice();
  virtual void OnCreateDevice();
protected:
  void DeleteVmrSurfaces();
  void GetCurrentVideoSize();

  
  D3DFORMAT            m_DisplayType;
  
  bool m_fUseInternalTimer;
  //Clock stuff
  REFERENCE_TIME m_rtTimePerFrame;
  REFERENCE_TIME m_pPrevEndFrame;
  

  float          m_fps;
  int            m_fFrameRate;
  bool           m_bRenderCreated;
private:
  long        m_refCount;
  CComPtr<IVMRSurfaceAllocatorNotify9>    m_pIVMRSurfAllocNotify;
  vector<CComPtr<IDirect3DSurface9> >     m_pSurfaces;
  int                                     m_pNbrSurface;
  int                                     m_pCurSurface;
  VMR_STATE                               m_vmrState;
};

#endif // _DXALLOCATORPRESENTER_H
