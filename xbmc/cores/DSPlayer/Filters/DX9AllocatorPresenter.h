//------------------------------------------------------------------------------
// File: DX9AllocatorPresenter.h
//
// Desc: DirectShow sample code - interface for the CDX9AllocatorPresenter class
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#if !defined(AFX_ALLOCATOR_H__F675D766_1E57_4269_A4B9_C33FB672B856__INCLUDED_)
#define AFX_ALLOCATOR_H__F675D766_1E57_4269_A4B9_C33FB672B856__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vmr9.h>
#include <wxdebug.h>
#include <Wxutil.h>
#pragma warning(push, 2)

// C4995'function': name was marked as #pragma deprecated
//
// The version of vector which shipped with Visual Studio .NET 2003 
// indirectly uses some deprecated functions.  Warning C4995 is disabled 
// because the file cannot be changed and we do not want to 
// display warnings which the user cannot fix.
#pragma warning(disable : 4995)

#include <vector>
#pragma warning(pop)
using namespace std;

//#include "PlaneScene.h"
#include <d3d9.h>
// Common files
//#include "helpers/CComPtr.h"
#include "subpic/isubpic.h"
DEFINE_GUID(CLSID_VMR9AllocatorPresenter,
0x4e4834fa, 0x22c2, 0x40e2, 0x94, 0x46, 0xf7, 0x7d, 0xd0, 0x5d, 0x24, 0x5e);
#define VMRBITMAP_UPDATE            0x80000000
class CDSGraph;

class CDX9AllocatorPresenter  : public  ISubPicAllocatorPresenterImpl,
                                        IVMRSurfaceAllocator9,
                                        IVMRImagePresenter9
                    
{
public:
  CCritSec        m_VMR9AlphaBitmapLock;
  void          UpdateAlphaBitmap();
  CDX9AllocatorPresenter(HRESULT& hr, HWND wnd, CStdString &_Error,IDirect3D9* d3d = NULL, IDirect3DDevice9* d3dd = NULL);
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

  // ISubPicAllocatorPresenter
  STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);
  UINT GetAdapter(IDirect3D9 *pD3D);
  STDMETHODIMP_(void) SetPosition(RECT w, RECT v);
  STDMETHODIMP_(bool) Paint(bool fAll);
  STDMETHODIMP_(void) SetTime(REFERENCE_TIME rtNow);


// IBasicVideo2
  STDMETHODIMP get_AvgTimePerFrame(REFTIME* pAvgTimePerFrame) {return E_NOTIMPL;}
  STDMETHODIMP get_BitRate(long* pBitRate) {return E_NOTIMPL;}
  STDMETHODIMP get_BitErrorRate(long* pBitErrorRate) {return E_NOTIMPL;}
  STDMETHODIMP get_VideoWidth(long* pVideoWidth) {return E_NOTIMPL;}
  STDMETHODIMP get_VideoHeight(long* pVideoHeight) {return E_NOTIMPL;}
  STDMETHODIMP put_SourceLeft(long SourceLeft) {return E_NOTIMPL;}
  STDMETHODIMP get_SourceLeft(long* pSourceLeft) {return E_NOTIMPL;}
  STDMETHODIMP put_SourceWidth(long SourceWidth) {return E_NOTIMPL;}
  STDMETHODIMP get_SourceWidth(long* pSourceWidth) {return E_NOTIMPL;}
  STDMETHODIMP put_SourceTop(long SourceTop) {return E_NOTIMPL;}
  STDMETHODIMP get_SourceTop(long* pSourceTop) {return E_NOTIMPL;}
  STDMETHODIMP put_SourceHeight(long SourceHeight) {return E_NOTIMPL;}
  STDMETHODIMP get_SourceHeight(long* pSourceHeight) {return E_NOTIMPL;}
  STDMETHODIMP put_DestinationLeft(long DestinationLeft) {return E_NOTIMPL;}
  STDMETHODIMP get_DestinationLeft(long* pDestinationLeft) {return E_NOTIMPL;}
  STDMETHODIMP put_DestinationWidth(long DestinationWidth) {return E_NOTIMPL;}
  STDMETHODIMP get_DestinationWidth(long* pDestinationWidth) {return E_NOTIMPL;}
  STDMETHODIMP put_DestinationTop(long DestinationTop) {return E_NOTIMPL;}
  STDMETHODIMP get_DestinationTop(long* pDestinationTop) {return E_NOTIMPL;}
  STDMETHODIMP put_DestinationHeight(long DestinationHeight) {return E_NOTIMPL;}
  STDMETHODIMP get_DestinationHeight(long* pDestinationHeight) {return E_NOTIMPL;}
  STDMETHODIMP SetSourcePosition(long Left, long Top, long Width, long Height) {return E_NOTIMPL;}
  STDMETHODIMP GetSourcePosition(long* pLeft, long* pTop, long* pWidth, long* pHeight){return E_NOTIMPL;}
  STDMETHODIMP SetDefaultSourcePosition() {return E_NOTIMPL;}
  STDMETHODIMP SetDestinationPosition(long Left, long Top, long Width, long Height) {return E_NOTIMPL;}
  STDMETHODIMP GetDestinationPosition(long* pLeft, long* pTop, long* pWidth, long* pHeight){return E_NOTIMPL;}
  STDMETHODIMP SetDefaultDestinationPosition() {return E_NOTIMPL;}
  STDMETHODIMP GetVideoSize(long* pWidth, long* pHeight) {return E_NOTIMPL;}

//IDispatch
  
  
protected:
  HRESULT CreateDevice();
  void DeleteSurfaces();
  HRESULT AllocVideoSurface(D3DFORMAT Format = D3DFMT_A8R8G8B8);
  D3DFORMAT            m_SurfaceType;
  D3DFORMAT            m_BackbufferType;
  D3DFORMAT            m_DisplayType;
  bool m_ReadyToPaint;
  VMR9AlphaBitmap      m_VMR9AlphaBitmap;
  CAutoVectorPtr<BYTE>  m_VMR9AlphaBitmapData;
  CRect          m_VMR9AlphaBitmapRect;
  int            m_VMR9AlphaBitmapWidthBytes;
  bool m_fUseInternalTimer;
private:
  
    // needed to make this a thread safe object
    CCritSec    m_ObjectLock;
    HWND        m_window;
    long        m_refCount;

  int   m_iVideoWidth, m_iVideoHeight;
  int   m_iARX, m_iARY;
  REFERENCE_TIME previousEndFrame;
  double m_fps;
    CComPtr<IDirect3D9>                     m_D3D;
    CComPtr<IDirect3DDevice9>               m_D3DDev;
    CComPtr<IVMRSurfaceAllocatorNotify9>    m_lpIVMRSurfAllocNotify;
    vector<CComPtr<IDirect3DSurface9> >     m_surfaces;
    CComPtr<IDirect3DSurface9>              m_renderTarget;
    CComPtr<IDirect3DTexture9>              m_privateTexture;
  //from mediaportal
  CComPtr<IDirect3DSurface9> m_pVideoSurface;
  //from planescene
  
  //CUSTOMVERTEX                     m_vertices[4];
  //CComPtr<IDirect3DVertexBuffer9> m_vertexBuffer;
    //CPlaneScene                             m_scene;
};

#endif // !defined(AFX_ALLOCATOR_H__F675D766_1E57_4269_A4B9_C33FB672B856__INCLUDED_)
