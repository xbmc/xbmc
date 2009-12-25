#ifndef _EVRALLOCATORPRESENTER_H
#define _EVRALLOCATORPRESENTER_H
#pragma once



#include <evr.h>
#include <dxva2api.h>
#include <atlcomcli.h>
#include <streams.h>
#include <d3d9.h>

#include "idsrenderer.h"
#include "D3DResource.h"
#include "IEvrPresenter.h"
#include "EvrPresentEngine.h"
#include "EvrScheduler.h"
#include "EvrHelper.h"
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
typedef BOOL  (__stdcall *PTR_AvSetMmThreadPriority)(HANDLE AvrtHandle, AVRT_PRIORITY Priority);
typedef BOOL  (__stdcall *PTR_AvRevertMmThreadCharacteristics)(HANDLE AvrtHandle);

static const GUID MFSamplePresenter_SampleCounter = 
{ 0xb0bb83cc, 0xf10f, 0x4e2e, { 0xaa, 0x2b, 0x29, 0xea, 0x5e, 0x92, 0xef, 0x85 } };

// Guid to tag IMFSample with DirectX surface index
static const GUID GUID_SURFACE_INDEX = { 0x30c8e9f6, 0x415, 0x4b81, { 0xa3, 0x15, 0x1, 0xa, 0xc6, 0xa9, 0xda, 0x19 } };

enum RENDER_STATE
{
    RENDER_STATE_STARTED = 1,
    RENDER_STATE_STOPPED,
    RENDER_STATE_PAUSED,
    RENDER_STATE_SHUTDOWN,  // Initial state. 
};

// FRAMESTEP_STATE: Defines the presenter's state with respect to frame-stepping.
enum FRAMESTEP_STATE
{
    FRAMESTEP_NONE,             // Not frame stepping.
    FRAMESTEP_WAITING_START,    // Frame stepping, but the clock is not started.
    FRAMESTEP_PENDING,          // Clock is started. Waiting for samples.
    FRAMESTEP_SCHEDULED,        // Submitted a sample for rendering.
    FRAMESTEP_COMPLETE          // Sample was rendered. 
};

class COuterEVR;
[uuid("7612B889-0929-4363-9BA3-580D735AA0F6")]
class CEVRAllocatorPresenter : public IDsRenderer,
                               public IMFVideoDeviceID,
                               public IMFVideoPresenter,
                               public IMFGetService,
                               public IMFTopologyServiceLookupClient,
                               public IMFVideoDisplayControl,
                               public IEVRPresenterRegisterCallback,
	                             public IEVRPresenterSettings,
                               public IEVRTrustedVideoPlugin,
                               public IQualProp,
                               public ID3DResource
{
public:
  CEVRAllocatorPresenter(HRESULT& hr, CStdString &_Error);
  virtual ~CEVRAllocatorPresenter();
  // IUnknown methods
  STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
  STDMETHOD_(ULONG, AddRef)();
  STDMETHOD_(ULONG, Release)();
//IDsRenderer
  STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);
  
  
  //IBaseFilter delegate
  bool GetState( DWORD dwMilliSecsTimeout, FILTER_STATE *State, HRESULT &_ReturnValue);

  
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

  //IQualProp
  virtual HRESULT STDMETHODCALLTYPE get_FramesDroppedInRenderer(int *frameDropped);
  virtual HRESULT STDMETHODCALLTYPE get_FramesDrawn(int *frameDrawn);
  virtual HRESULT STDMETHODCALLTYPE get_AvgFrameRate(int *avgFrameRate);
  virtual HRESULT STDMETHODCALLTYPE get_Jitter(int *iJitter);
  virtual HRESULT STDMETHODCALLTYPE get_AvgSyncOffset(int *piAvg);
  virtual HRESULT STDMETHODCALLTYPE get_DevSyncOffset(int *piDev);

  //IMFAsyncCallback
  virtual HRESULT STDMETHODCALLTYPE GetParameters(DWORD *pdwFlags ,DWORD *pdwQueue){return E_NOTIMPL;};
  virtual HRESULT STDMETHODCALLTYPE Invoke(IMFAsyncResult *pAsyncResult){return E_NOTIMPL;};

  //IMFVideoPresenter
  STDMETHODIMP ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam);
  STDMETHODIMP GetCurrentMediaType(__deref_out  IMFVideoMediaType **ppMediaType);

  //IMFTopologyServiceLookupClient
  STDMETHODIMP  InitServicePointers(/* [in] */ __in  IMFTopologyServiceLookup *pLookup);
  STDMETHODIMP  ReleaseServicePointers();

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

  // IEVRPresenterRegisterCallback methods
	STDMETHOD(RegisterCallback)(IEVRPresenterCallback *pCallback);

	// IEVRPresenterSettings methods
	STDMETHOD(SetBufferCount)(int bufferCount);

  // IEVRTrustedVideoPlugin
  STDMETHODIMP IsInTrustedVideoMode(BOOL *pYes);
  STDMETHODIMP CanConstrict(BOOL *pYes);
  STDMETHODIMP SetConstriction(DWORD dwKPix);
  STDMETHODIMP DisableImageExport(BOOL bDisable);

  // ID3dRessource
  virtual void OnLostDevice();
  virtual void OnDestroyDevice();
  virtual void OnCreateDevice();
private:
  long m_refCount;
  
  COuterEVR *m_pOuterEVR;
  bool m_fUseInternalTimer;
  bool m_bNeedNewDevice;

// === Functions pointers on Vista / .Net3 specifics library
  PTR_DXVA2CreateDirect3DDeviceManager9  pfDXVA2CreateDirect3DDeviceManager9;
  PTR_MFCreateDXSurfaceBuffer        pfMFCreateDXSurfaceBuffer;
  PTR_MFCreateVideoSampleFromSurface    pfMFCreateVideoSampleFromSurface;
  PTR_MFCreateVideoMediaType        pfMFCreateVideoMediaType;
//vista specific                    
  PTR_AvSetMmThreadCharacteristicsW    pfAvSetMmThreadCharacteristicsW;
  PTR_AvSetMmThreadPriority        pfAvSetMmThreadPriority;
  PTR_AvRevertMmThreadCharacteristics    pfAvRevertMmThreadCharacteristics;
  


protected:
inline HRESULT CheckShutdown() const 
    {
        if (m_RenderState == RENDER_STATE_SHUTDOWN)
        {
            return MF_E_SHUTDOWN;
        }
        else
        {
            return S_OK;
        }
    }

    // IsActive: The "active" state is started or paused.
    inline BOOL IsActive() const
    {
        return ((m_RenderState == RENDER_STATE_STARTED) || (m_RenderState == RENDER_STATE_PAUSED));
    }

    // IsScrubbing: Scrubbing occurs when the frame rate is 0.
    inline BOOL IsScrubbing() const { return m_fRate == 0.0f; }

    // NotifyEvent: Send an event to the EVR through its IMediaEventSink interface.
    void NotifyEvent(long EventCode, LONG_PTR Param1, LONG_PTR Param2)
    {
        if (m_pMediaEventSink)
        {
            m_pMediaEventSink->Notify(EventCode, Param1, Param2);
        }
    }

    float GetMaxRate(BOOL bThin);

    // Mixer operations
    HRESULT ConfigureMixer(IMFTransform *pMixer);

    // Formats
    HRESULT CreateOptimalVideoType(IMFMediaType* pProposed, IMFMediaType **ppOptimal);
    HRESULT CalculateOutputRectangle(IMFMediaType *pProposed, RECT *prcOutput);
    HRESULT SetMediaType(IMFMediaType *pType);
    HRESULT IsMediaTypeSupported(IMFMediaType *pMediaType);

    // Message handlers
    HRESULT Flush();
    HRESULT RenegotiateMediaType();
    HRESULT ProcessInputNotify();
    HRESULT BeginStreaming();
    HRESULT EndStreaming();
    HRESULT CheckEndOfStream();

    // Managing samples
    void    ProcessOutputLoop();   
	HRESULT ProcessOutput();
    HRESULT DeliverSample(IMFSample *pSample, BOOL bRepaint);
    HRESULT TrackSample(IMFSample *pSample);
    void    ReleaseResources();

    // Frame-stepping
    HRESULT PrepareFrameStep(DWORD cSteps);
    HRESULT StartFrameStep();
    HRESULT DeliverFrameStepSample(IMFSample *pSample);
    HRESULT CompleteFrameStep(IMFSample *pSample);
    HRESULT CancelFrameStep();

    // Callbacks

    // Callback when a video sample is released.
    HRESULT OnSampleFree(IMFAsyncResult *pResult);
    AsyncCallback<CEVRAllocatorPresenter>   m_SampleFreeCB;
  
protected:

    // FrameStep: Holds information related to frame-stepping. 
    // Note: The purpose of this structure is simply to organize the data in one variable.
    struct FrameStep
    {
        FrameStep() : state(FRAMESTEP_NONE), steps(0), pSampleNoRef(NULL)
        {
        }

        FRAMESTEP_STATE     state;          // Current frame-step state.
        VideoSampleList     samples;        // List of pending samples for frame-stepping.
        DWORD               steps;          // Number of steps left.
        DWORD_PTR           pSampleNoRef;   // Identifies the frame-step sample.
    };


protected:

    RENDER_STATE                m_RenderState;          // Rendering state.
    FrameStep                   m_FrameStep;            // Frame-stepping information.

    CCritSec                     m_ObjectLock;			// Serializes our public methods.  

	// Samples and scheduling
    
    SamplePool                  m_SamplePool;           // Pool of allocated samples.
    DWORD                       m_TokenCounter;         // Counter. Incremented whenever we create new samples.

	// Rendering state
	BOOL						m_bSampleNotify;		// Did the mixer signal it has an input sample?
	BOOL						m_bRepaint;				// Do we need to repaint the last sample?
	BOOL						m_bPrerolled;	        // Have we presented at least one sample?
    BOOL                        m_bEndStreaming;		// Did we reach the end of the stream (EOS)?

    MFVideoNormalizedRect       m_nrcSource;            // Source rectangle.
    float                       m_fRate;                // Playback rate.

    // Deletable objects.
    D3DPresentEngine            *m_pD3DPresentEngine;    // Rendering engine. (Never null if the constructor succeeds.)
    CEvrScheduler               m_scheduler;			       // Manages scheduling of samples.

    // COM interfaces.
    IMFClock                    *m_pClock;               // The EVR's clock.
    IMFTransform                *m_pMixer;               // The mixer.
    IMediaEventSink             *m_pMediaEventSink;      // The EVR's event-sink interface.
    IMFMediaType                *m_pMediaType;           // Output media type
};

#endif