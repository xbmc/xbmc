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

// Guid to tag IMFSample with DirectX surface index
static const GUID GUID_SURFACE_INDEX = { 0x30c8e9f6, 0x415, 0x4b81, { 0xa3, 0x15, 0x1, 0xa, 0xc6, 0xa9, 0xda, 0x19 } };

class COuterEVR;
[uuid("7612B889-0929-4363-9BA3-580D735AA0F6")]
class CEVRAllocatorPresenter : public IDsRenderer,
                               public IMFVideoDeviceID,
                               public IMFVideoPresenter,
                               public IDirect3DDeviceManager9,
                               public IMFGetService,
							   public IMFAsyncCallback,
                               public IMFTopologyServiceLookupClient,
                               public IMFVideoDisplayControl,
                               public IEVRTrustedVideoPlugin,
							   public IQualProp,
                               public CCritSec
{
public:
  CEVRAllocatorPresenter(HRESULT& hr, HWND wnd, CStdString &_Error,IDirect3D9* d3d = NULL, IDirect3DDevice9* d3dd = NULL);
  virtual ~CEVRAllocatorPresenter();

  STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);
  STDMETHODIMP	InitializeDevice(AM_MEDIA_TYPE*	pMediaType);
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

  //IMFAsyncCallback
  virtual HRESULT STDMETHODCALLTYPE GetParameters(DWORD *pdwFlags ,DWORD *pdwQueue){return E_NOTIMPL;};
  virtual HRESULT STDMETHODCALLTYPE Invoke(IMFAsyncResult *pAsyncResult){return E_NOTIMPL;};

  //IMFVideoPresenter
  STDMETHODIMP ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam);
  STDMETHODIMP GetCurrentMediaType(__deref_out  IMFVideoMediaType **ppMediaType);

  //IMFTopologyServiceLookupClient
  STDMETHODIMP	InitServicePointers(/* [in] */ __in  IMFTopologyServiceLookup *pLookup);
  STDMETHODIMP	ReleaseServicePointers();

  //IMFVideoDeviceID
  STDMETHODIMP  GetDeviceID(/* [out] */ __out  IID *pDeviceID);

  //IMFGetService
  STDMETHODIMP  GetService (/* [in] */ __RPC__in REFGUID guidService,
                            /* [in] */ __RPC__in REFIID riid,
                            /* [iid_is][out] */ __RPC__deref_out_opt LPVOID *ppvObject);
  //IMFClockStateSink
  STDMETHODIMP STDMETHODCALLTYPE OnClockStart(/* [in] */ MFTIME hnsSystemTime, /* [in] */ LONGLONG llClockStartOffset);
  STDMETHODIMP STDMETHODCALLTYPE OnClockStop(/* [in] */ MFTIME hnsSystemTime);
  STDMETHODIMP STDMETHODCALLTYPE OnClockPause(/* [in] */ MFTIME hnsSystemTime);
  STDMETHODIMP STDMETHODCALLTYPE OnClockRestart(/* [in] */ MFTIME hnsSystemTime);
  STDMETHODIMP STDMETHODCALLTYPE OnClockSetRate(/* [in] */ MFTIME hnsSystemTime, /* [in] */ float flRate);

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
    STDMETHODIMP IsInTrustedVideoMode(BOOL *pYes);
    STDMETHODIMP CanConstrict(BOOL *pYes);
    STDMETHODIMP SetConstriction(DWORD dwKPix);
    STDMETHODIMP DisableImageExport(BOOL bDisable);

	// IDirect3DDeviceManager9
	STDMETHODIMP	ResetDevice(IDirect3DDevice9 *pDevice,UINT resetToken);
	STDMETHODIMP	OpenDeviceHandle(HANDLE *phDevice);
	STDMETHODIMP	CloseDeviceHandle(HANDLE hDevice); 
    STDMETHODIMP	TestDevice(HANDLE hDevice);
	STDMETHODIMP	LockDevice(HANDLE hDevice, IDirect3DDevice9 **ppDevice, BOOL fBlock);
	STDMETHODIMP	UnlockDevice(HANDLE hDevice, BOOL fSaveState);
	STDMETHODIMP	GetVideoService(HANDLE hDevice, REFIID riid, void **ppService);
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
  CCritSec								m_SampleQueueLock;
  CCritSec								m_ImageProcessingLock;
  
	// Stats variable for IQualProp
  UINT									m_pcFrames;
  UINT									m_nDroppedUpdate;
  UINT									m_pcFramesDrawn;	// Retrieves the number of frames drawn since streaming started
  UINT									m_piAvg;
  UINT									m_piDev;

  CInterfaceList<IMFSample, &IID_IMFSample>		m_FreeSamples;
  CInterfaceList<IMFSample, &IID_IMFSample>		m_ScheduledSamples;
  IMFSample *								m_pCurrentDisplaydSample;
  bool									m_bWaitingSample;
  bool									m_bLastSampleOffsetValid;
  LONGLONG								m_StarvationClock;
  bool									m_bSignaledStarvation;
  //stuff from mediaportal
  LONGLONG								m_LastFrameTime;
  LONGLONG								m_FrameTimeDiff;
  int                                   m_iExpectedFrameDuration;
  int       m_iMaxFrameTimeDiff;
  int       m_iMinFrameTimeDiff;
  int                                   m_iFramesForStats;
  DWORD                                 m_dwVariance;


  LONGLONG								    m_LastScheduledSampleTime;
  double									m_LastScheduledSampleTimeFP;
  LONGLONG								m_LastScheduledUncorrectedSampleTime;
  LONGLONG								m_MaxSampleDuration;
  LONGLONG								m_LastSampleOffset;
  LONGLONG								m_VSyncOffsetHistory[5];
  LONGLONG								m_LastPredictedSync;
	int										m_VSyncOffsetHistoryPos;
  void									RenderThread();
  void									ResetStats();
  static DWORD WINAPI						PresentThread(LPVOID lpParam);
  void									CompleteFrameStep(bool bCancel);
  void									CheckWaitingSampleFromMixer();
  bool									   GetImageFromMixer();
  void									   GetMixerThread();
  static DWORD WINAPI						GetMixerThreadStatic(LPVOID lpParam);
  void									RemoveAllSamples();
  HRESULT									GetFreeSample(IMFSample** ppSample);
  HRESULT									GetScheduledSample(IMFSample** ppSample, int &_Count);
  void									MoveToFreeList(IMFSample* pSample, bool bTail);
  void									MoveToScheduledList(IMFSample* pSample, bool _bSorted);
  void									FlushSamples();
  void									FlushSamplesInternal();
  bool                                      m_bPendingRenegotiate;
  bool                                      m_bPendingMediaFinished;
  bool                                      m_bPendingResetDevice;
  bool										m_bNeedPendingResetDevice;


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
//Dx9Allocator
  CCritSec					m_RenderLock;
  long					m_nUsedBuffer;
  REFERENCE_TIME			m_rtTimePerFrame;
protected:
  CComPtr<IDirect3D9> m_D3D;
  CComPtr<IDirect3DDevice9> m_D3DDev;
  CComPtr<IDirect3DDeviceManager9> m_pDeviceManager;
  CComPtr<IMFVideoMediaType> m_pMediaType;
  CComPtr<IMFTransform> m_pMixer;
  CComPtr<IMediaEventSink> m_pSink;
  CComPtr<IMFClock> m_pClock;
  UINT m_iResetToken;
//Rendering function
  HRESULT RenderPresent(int surfaceIndex);
  void StartWorkerThreads();
  void StopWorkerThreads();
  LONGLONG GetCurrentTimestamp();
//Trace evr is only temporary TODO Remove it
  HRESULT TRACE_EVR(const char* strTrace);
//Dx9Allocator
  virtual HRESULT AllocSurfaces(D3DFORMAT Format = D3DFMT_A8R8G8B8);
  virtual bool ResetD3dDevice();
  virtual void DeleteSurfaces();
  void			OnResetDevice();
  LONGLONG m_ModeratedTimeLast;
  LONGLONG m_ModeratedClockLast;
  CComPtr<IDirect3DTexture9>		m_pVideoTexture;
  CComPtr<IDirect3DSurface9>		m_pVideoSurface;
  CComPtr<IDirect3DSwapChain9>      m_pInternalSwapchains[7];
  CComPtr<IDirect3DTexture9>		m_pInternalVideoTexture[7];
  CComPtr<IDirect3DSurface9>		m_pInternalVideoSurface[7];
  CComPtr<IMFSample>                m_pInternalVideoSamples[7];

  int                               m_nNbDXSurface;      //Total number of dx surface
  int                               m_nCurSurface;
  int                               m_iVideoWidth;
  int                               m_iVideoHeight;
  int                               m_fps;
  D3DFORMAT                         m_SurfaceType;

  double					m_DetectedFrameTimeStdDev;
  bool					m_bCorrectedFrameTime;
  int					m_DetectedFrameTimePos;
  LONGLONG				m_DetectedFrameTimeHistory[60];
  bool					m_DetectedLock;
  double					m_DetectedFrameTimeHistoryHistory[500];
};

#endif