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


#include <d3d9.h>

#include "DsRenderer.h"
#include "geometry.h"

class CMacrovisionKicker;
class COuterVMR9;

[uuid("A2636B41-5E3C-4426-B6BC-CD8616600912")]
class CVMR9AllocatorPresenter  : public CDsRenderer,
                                        IVMRSurfaceAllocator9,
                                        IVMRImagePresenter9
                    
{
public:
  CVMR9AllocatorPresenter(HRESULT& hr, CStdString &_Error);

  DECLARE_IUNKNOWN
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

  // IVMRSurfaceAllocator9
  virtual STDMETHODIMP InitializeDevice(DWORD_PTR dwUserID, VMR9AllocationInfo *lpAllocInfo, DWORD *lpNumBuffers);
  virtual STDMETHODIMP TerminateDevice(DWORD_PTR dwID);  
  virtual STDMETHODIMP GetSurface(DWORD_PTR dwUserID, DWORD SurfaceIndex, DWORD SurfaceFlags, IDirect3DSurface9 **lplpSurface);
  virtual STDMETHODIMP AdviseNotify(IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify);

  // IVMRImagePresenter9
  virtual STDMETHODIMP StartPresenting( DWORD_PTR dwUserID);
  virtual STDMETHODIMP StopPresenting( DWORD_PTR dwUserID);
  virtual STDMETHODIMP PresentImage( DWORD_PTR dwUserID, VMR9PresentationInfo *lpPresInfo);

  // IDSRenderer
  STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);

  virtual void OnLostDevice();
  virtual void OnDestroyDevice();
  virtual void OnCreateDevice();
protected:
  void DeleteVmrSurfaces();
  void GetCurrentVideoSize();
  HRESULT ChangeD3dDev();

  D3DFORMAT m_DisplayType;
  
  bool m_fUseInternalTimer;
  //Clock stuff
  REFERENCE_TIME m_rtTimePerFrame;
  float          m_fps;
  int            m_fFrameRate;
  bool           m_bRenderCreated;
  bool           m_bNeedNewDevice;
private:
  long        m_refCount;
  IVMRSurfaceAllocatorNotify9*        m_pIVMRSurfAllocNotify;
  std::vector<IDirect3DSurface9*>     m_pSurfaces;
  int                                 m_pNbrSurface;
  int                                 m_pCurSurface;
  bool          m_bNeedCheckSample;

  CD3DTexture* m_pTexture;
  
};

#endif // _DXALLOCATORPRESENTER_H
