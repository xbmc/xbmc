#ifndef _EVRALLOCATORPRESENTER_H
#define _EVRALLOCATORPRESENTER_H
#pragma once



#include <evr.h>
#include <dxva2api.h>
#include <atlcomcli.h>
#include <streams.h>
#include <d3d9.h>
#include "idsrenderer.h"
#include <avrt.h>
// dxva.dll
typedef HRESULT (__stdcall *PTR_DXVA2CreateDirect3DDeviceManager9)(UINT* pResetToken, IDirect3DDeviceManager9** ppDeviceManager);

// mf.dll
typedef HRESULT (__stdcall *PTR_MFCreatePresentationClock)(IMFPresentationClock** ppPresentationClock);

// evr.dll
typedef HRESULT (__stdcall *PTR_MFCreateDXSurfaceBuffer)(REFIID riid, IUnknown* punkSurface, BOOL fBottomUpWhenLinear, IMFMediaBuffer** ppBuffer);
typedef HRESULT (__stdcall *PTR_MFCreateVideoSampleFromSurface)(IUnknown* pUnkSurface, IMFSample** ppSample);
typedef HRESULT (__stdcall *PTR_MFCreateVideoMediaType)(const MFVIDEOFORMAT* pVideoFormat, IMFVideoMediaType** ppIVideoMediaType);

// AVRT.dll
typedef HANDLE  (__stdcall *PTR_AvSetMmThreadCharacteristicsW)(LPCWSTR TaskName, LPDWORD TaskIndex);
typedef BOOL	(__stdcall *PTR_AvSetMmThreadPriority)(HANDLE AvrtHandle, AVRT_PRIORITY Priority);
typedef BOOL	(__stdcall *PTR_AvRevertMmThreadCharacteristics)(HANDLE AvrtHandle);

class COuterEVR;
[uuid("7612B889-0929-4363-9BA3-580D735AA0F6")]
class CEVRAllocatorPresenter : public IDsRenderer,
                               public IMFVideoDeviceID,
                               public IMFVideoPresenter,
                               public IDirect3DDeviceManager9,
                               public IMFGetService,
                               public IMFTopologyServiceLookupClient,
                               public IMFVideoDisplayControl,
                               public IEVRTrustedVideoPlugin,
							   public IQualProp
{
public:
  CEVRAllocatorPresenter(HRESULT& hr, HWND wnd, CStdString &_Error,IDirect3D9* d3d = NULL, IDirect3DDevice9* d3dd = NULL);
  virtual ~CEVRAllocatorPresenter();

  STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);

  //IBaseFilter delegate
  bool GetState( DWORD dwMilliSecsTimeout, FILTER_STATE *State, HRESULT &_ReturnValue);

  // IUnknown
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void** ppvObject);
  virtual ULONG STDMETHODCALLTYPE AddRef();
  virtual ULONG STDMETHODCALLTYPE Release();
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

  //IQualProp
  virtual HRESULT STDMETHODCALLTYPE get_FramesDroppedInRenderer(int *pcFrames){return E_NOTIMPL;};
  virtual HRESULT STDMETHODCALLTYPE get_FramesDrawn(int *pcFramesDrawn){return E_NOTIMPL;};
  virtual HRESULT STDMETHODCALLTYPE get_AvgFrameRate(int *piAvgFrameRate){return E_NOTIMPL;};
  virtual HRESULT STDMETHODCALLTYPE get_Jitter(int *iJitter){return E_NOTIMPL;};
  virtual HRESULT STDMETHODCALLTYPE get_AvgSyncOffset(int *piAvg){return E_NOTIMPL;};
  virtual HRESULT STDMETHODCALLTYPE get_DevSyncOffset(int *piDev){return E_NOTIMPL;};
  //IMFVideoPresenter
  STDMETHODIMP ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam);
  STDMETHODIMP GetCurrentMediaType(__deref_out  IMFVideoMediaType **ppMediaType);

	// IMFTopologyServiceLookupClient        
	STDMETHODIMP	InitServicePointers(/* [in] */ __in  IMFTopologyServiceLookup *pLookup);
	STDMETHODIMP	ReleaseServicePointers();

	// IMFVideoDeviceID
	STDMETHODIMP	GetDeviceID(/* [out] */	__out  IID *pDeviceID);

	// IMFGetService
	STDMETHODIMP	GetService (/* [in] */ __RPC__in REFGUID guidService,
								/* [in] */ __RPC__in REFIID riid,
								/* [iid_is][out] */ __RPC__deref_out_opt LPVOID *ppvObject);
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
typedef enum
{
  Started = State_Running,
  Stopped = State_Stopped,
  Paused = State_Paused,
  Shutdown = State_Running + 1
} RENDER_STATE;

  long m_refCount;
  bool m_fUseInternalTimer;
  int                                       m_nStepCount;
  HANDLE                                    m_hEvtQuit;			// Stop rendering thread event
  bool                                      m_bEvtQuit;
  HANDLE                                    m_hEvtFlush;		// Discard all buffers
  bool                                      m_bEvtFlush;
  HANDLE									m_hThread;
  HANDLE									m_hGetMixerThread;
  RENDER_STATE							m_nRenderState;

  void									RenderThread();
  static DWORD WINAPI						PresentThread(LPVOID lpParam);

  void									GetMixerThread();
  static DWORD WINAPI						GetMixerThreadStatic(LPVOID lpParam);
  bool                                      m_bPendingRenegotiate;
  bool                                      m_bPendingMediaFinished;
  bool                                      m_bPendingResetDevice;


  COuterEVR *m_pOuterEVR;
// === Media type negociation functions
  HRESULT									RenegotiateMediaType();
  HRESULT									IsMediaTypeSupported(IMFMediaType* pMixerType);
  HRESULT									CreateProposedOutputType(IMFMediaType* pMixerType, IMFMediaType** pType);
  HRESULT									SetMediaType(IMFMediaType* pType);

// === Functions pointers on Vista / .Net3 specifics library
  PTR_DXVA2CreateDirect3DDeviceManager9	pfDXVA2CreateDirect3DDeviceManager9;
  PTR_MFCreateDXSurfaceBuffer				pfMFCreateDXSurfaceBuffer;
  PTR_MFCreateVideoSampleFromSurface		pfMFCreateVideoSampleFromSurface;
  PTR_MFCreateVideoMediaType				pfMFCreateVideoMediaType;
//vista specific										
  PTR_AvSetMmThreadCharacteristicsW		pfAvSetMmThreadCharacteristicsW;
  PTR_AvSetMmThreadPriority				pfAvSetMmThreadPriority;
  PTR_AvRevertMmThreadCharacteristics		pfAvRevertMmThreadCharacteristics;
protected:
  CComPtr<IDirect3D9> m_D3D;
  CComPtr<IDirect3DDevice9> m_D3DDev;
  CComPtr<IDirect3DDeviceManager9> m_pDeviceManager;
  CComPtr<IMFMediaType> m_pMediaType;
  CComPtr<IMFTransform> m_pMixer;
  CComPtr<IMediaEventSink> m_pSink;
  CComPtr<IMFClock> m_pClock;
  UINT m_iResetToken;

  //msg handlings
  void StartWorkerThreads();
  void StopWorkerThreads(){};
  HRESULT TRACE_EVR(const char* strTrace);
};

#endif