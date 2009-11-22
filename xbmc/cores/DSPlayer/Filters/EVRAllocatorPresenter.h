#ifndef _EVRALLOCATORPRESENTER_H
#define _EVRALLOCATORPRESENTER_H
#pragma once



#include <evr.h>
#include <dxva2api.h>
#include <atlcomcli.h>
#include <d3d9.h>
#include "idsrenderer.h"

class COuterEVR;
[uuid("7612B889-0929-4363-9BA3-580D735AA0F6")]
class CEVRAllocatorPresenter : public IDsRenderer,
                               public IMFVideoDeviceID,
                               public IMFVideoPresenter,
                               public IDirect3DDeviceManager9,
                               public IMFGetService,
                               public IMFTopologyServiceLookupClient,
                               public IMFVideoDisplayControl,
                               public IEVRTrustedVideoPlugin
{
public:
  CEVRAllocatorPresenter(HRESULT& hr, HWND wnd, CStdString &_Error,IDirect3D9* d3d = NULL, IDirect3DDevice9* d3dd = NULL);
  virtual ~CEVRAllocatorPresenter();

  STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);

// IBaseFilter delegate
  bool GetState( DWORD dwMilliSecsTimeout, FILTER_STATE *State, HRESULT &_ReturnValue);
  // IUnknown
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void** ppvObject);
  virtual ULONG STDMETHODCALLTYPE AddRef();
  virtual ULONG STDMETHODCALLTYPE Release();
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
  //IMFVideoPresenter
	// IMFVideoPresenter
	STDMETHODIMP	ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam);
	STDMETHODIMP	GetCurrentMediaType(__deref_out  IMFVideoMediaType **ppMediaType);

	// IMFTopologyServiceLookupClient        
	STDMETHODIMP	InitServicePointers(/* [in] */ __in  IMFTopologyServiceLookup *pLookup){return E_NOTIMPL;};
	STDMETHODIMP	ReleaseServicePointers(){return E_NOTIMPL;};

	// IMFVideoDeviceID
	STDMETHODIMP	GetDeviceID(/* [out] */	__out  IID *pDeviceID){return E_NOTIMPL;};

	// IMFGetService
	STDMETHODIMP	GetService (/* [in] */ __RPC__in REFGUID guidService,
								/* [in] */ __RPC__in REFIID riid,
								/* [iid_is][out] */ __RPC__deref_out_opt LPVOID *ppvObject){return E_NOTIMPL;};
  // IMFClockStateSink
	STDMETHODIMP	OnClockStart(/* [in] */ MFTIME hnsSystemTime, /* [in] */ LONGLONG llClockStartOffset){return E_NOTIMPL;};        
	STDMETHODIMP	STDMETHODCALLTYPE OnClockStop(/* [in] */ MFTIME hnsSystemTime){return E_NOTIMPL;};
	STDMETHODIMP	STDMETHODCALLTYPE OnClockPause(/* [in] */ MFTIME hnsSystemTime){return E_NOTIMPL;};
	STDMETHODIMP	STDMETHODCALLTYPE OnClockRestart(/* [in] */ MFTIME hnsSystemTime){return E_NOTIMPL;};
	STDMETHODIMP	STDMETHODCALLTYPE OnClockSetRate(/* [in] */ MFTIME hnsSystemTime, /* [in] */ float flRate){return E_NOTIMPL;};

	// IMFVideoDisplayControl
    STDMETHODIMP GetNativeVideoSize(SIZE *pszVideo, SIZE *pszARVideo){return E_NOTIMPL;};    
    STDMETHODIMP GetIdealVideoSize(SIZE *pszMin, SIZE *pszMax){return E_NOTIMPL;};
    STDMETHODIMP SetVideoPosition(const MFVideoNormalizedRect *pnrcSource, const LPRECT prcDest){return E_NOTIMPL;};
    STDMETHODIMP GetVideoPosition(MFVideoNormalizedRect *pnrcSource, LPRECT prcDest){return E_NOTIMPL;};
    STDMETHODIMP SetAspectRatioMode(DWORD dwAspectRatioMode){return E_NOTIMPL;};
    STDMETHODIMP GetAspectRatioMode(DWORD *pdwAspectRatioMode){return E_NOTIMPL;};
    STDMETHODIMP SetVideoWindow(HWND hwndVideo){return E_NOTIMPL;};
    STDMETHODIMP GetVideoWindow(HWND *phwndVideo){return E_NOTIMPL;};
    STDMETHODIMP RepaintVideo( void){return E_NOTIMPL;};
    STDMETHODIMP GetCurrentImage(BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, LONGLONG *pTimeStamp){return E_NOTIMPL;};
    STDMETHODIMP SetBorderColor(COLORREF Clr){return E_NOTIMPL;};
    STDMETHODIMP GetBorderColor(COLORREF *pClr){return E_NOTIMPL;};
    STDMETHODIMP SetRenderingPrefs(DWORD dwRenderFlags){return E_NOTIMPL;};
    STDMETHODIMP GetRenderingPrefs(DWORD *pdwRenderFlags){return E_NOTIMPL;};
    STDMETHODIMP SetFullscreen(BOOL fFullscreen){return E_NOTIMPL;};
    STDMETHODIMP GetFullscreen(BOOL *pfFullscreen){return E_NOTIMPL;};

		// IEVRTrustedVideoPlugin
    STDMETHODIMP IsInTrustedVideoMode(BOOL *pYes){return E_NOTIMPL;};
    STDMETHODIMP CanConstrict(BOOL *pYes){return E_NOTIMPL;};
    STDMETHODIMP SetConstriction(DWORD dwKPix){return E_NOTIMPL;};
    STDMETHODIMP DisableImageExport(BOOL bDisable){return E_NOTIMPL;};

	// IDirect3DDeviceManager9
	STDMETHODIMP	ResetDevice(IDirect3DDevice9 *pDevice,UINT resetToken){return E_NOTIMPL;};
	STDMETHODIMP	OpenDeviceHandle(HANDLE *phDevice){return E_NOTIMPL;};
	STDMETHODIMP	CloseDeviceHandle(HANDLE hDevice){return E_NOTIMPL;}; 
    STDMETHODIMP	TestDevice(HANDLE hDevice){return E_NOTIMPL;};
	STDMETHODIMP	LockDevice(HANDLE hDevice, IDirect3DDevice9 **ppDevice, BOOL fBlock){return E_NOTIMPL;};
	STDMETHODIMP	UnlockDevice(HANDLE hDevice, BOOL fSaveState){return E_NOTIMPL;};
	STDMETHODIMP	GetVideoService(HANDLE hDevice, REFIID riid, void **ppService){return E_NOTIMPL;};
private:

  long m_refCount;
  COuterEVR *m_pOuterEVR;


protected:
  CComPtr<IDirect3D9> m_D3D;
  CComPtr<IDirect3DDevice9> m_D3DDev;
  CComPtr<IDirect3DDeviceManager9> m_pDeviceManager;
  CComPtr<IMFMediaType> m_pMediaType;

  UINT m_iResetToken;
};

#endif