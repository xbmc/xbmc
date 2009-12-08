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

#include "IDsRenderer.h"
#define VMRBITMAP_UPDATE            0x80000000
class CDSGraph;

[uuid("A2636B41-5E3C-4426-B6BC-CD8616600912")]
class CDX9AllocatorPresenter  : public  IDsRenderer,
                                        IVMRSurfaceAllocator9,
                                        IVMRImagePresenter9
                    
{
public:
  CCritSec        m_VMR9AlphaBitmapLock;
  void          UpdateAlphaBitmap();
  CDX9AllocatorPresenter(HRESULT& hr, CStdString &_Error);
  virtual ~CDX9AllocatorPresenter();

  // IVMRSurfaceAllocator9
  virtual HRESULT STDMETHODCALLTYPE InitializeDevice(/*[in]*/ DWORD_PTR dwUserID,/*[in]*/ VMR9AllocationInfo *lpAllocInfo,/* [out][in] */ DWORD *lpNumBuffers);
  virtual HRESULT STDMETHODCALLTYPE TerminateDevice(/*[in]*/ DWORD_PTR dwID);  
  virtual HRESULT STDMETHODCALLTYPE GetSurface(/*[in]*/ DWORD_PTR dwUserID,/*[in]*/ DWORD SurfaceIndex,/*[in]*/ DWORD SurfaceFlags,/* [out] */ IDirect3DSurface9 **lplpSurface);
  virtual HRESULT STDMETHODCALLTYPE AdviseNotify(/*[in]*/ IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify);

  // IVMRImagePresenter9
  virtual HRESULT STDMETHODCALLTYPE StartPresenting(/*[in]*/ DWORD_PTR dwUserID);
  virtual HRESULT STDMETHODCALLTYPE StopPresenting(/*[in]*/ DWORD_PTR dwUserID);
  virtual HRESULT STDMETHODCALLTYPE PresentImage(/*[in]*/ DWORD_PTR dwUserID,/*[in]*/ VMR9PresentationInfo *lpPresInfo);

  // IUnknown
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void** ppvObject);
  virtual ULONG STDMETHODCALLTYPE AddRef();
  virtual ULONG STDMETHODCALLTYPE Release();

  // IDSRenderer
  STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);
  bool CheckDevice();
  
  UINT GetAdapter(IDirect3D9 *pD3D);
  STDMETHODIMP_(void) SetPosition(RECT w, RECT v) {} ;
  STDMETHODIMP_(bool) Paint(bool fAll) { return true; } ;
  STDMETHODIMP_(void) SetTime(REFERENCE_TIME rtNow) {};
  
protected:
  void DeleteSurfaces();
  HRESULT AllocVideoSurface(D3DFORMAT Format = D3DFMT_A8R8G8B8);
  D3DFORMAT            m_SurfaceType;
  D3DFORMAT            m_BackbufferType;
  D3DFORMAT            m_DisplayType;
  VMR9AlphaBitmap      m_VMR9AlphaBitmap;
  CAutoVectorPtr<BYTE>  m_VMR9AlphaBitmapData;
  tagRECT          m_VMR9AlphaBitmapRect;
  int            m_VMR9AlphaBitmapWidthBytes;
  bool m_fUseInternalTimer;
  bool m_pRequireResetDevice;
  
private:
  CCritSec    m_ObjectLock;
  HWND        m_window;
  long        m_refCount;
  int   m_iVideoWidth, m_iVideoHeight;
  REFERENCE_TIME m_pPrevEndFrame;
  float m_fps;
  CComPtr<IDirect3D9>                     m_D3D;
  CComPtr<IDirect3DDevice9>               m_D3DDev;
  CComPtr<IVMRSurfaceAllocatorNotify9>    m_lpIVMRSurfAllocNotify;
  vector<CComPtr<IDirect3DSurface9> >     m_surfaces;
  D3DDEVICE_CREATION_PARAMETERS           m_currentD3DParam;
  CComPtr<IDirect3DSurface9>              m_renderTarget;
  CComPtr<IDirect3DTexture9>              m_pVideoTexture;
  CComPtr<IDirect3DSurface9>              m_pVideoSurface;
};

#endif // _DXALLOCATORPRESENTER_H
