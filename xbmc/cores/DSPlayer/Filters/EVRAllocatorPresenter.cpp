
#include "MacrovisionKicker.h"
#include "utils/log.h"
#include "EVRAllocatorPresenter.h"
#include "application.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "WindowingFactory.h" //d3d device and d3d interface
#include "dshowutil/dshowutil.h"
#include <mfapi.h>
#include <mferror.h>
#include "IPinHook.h"


class COuterEVR
  : public CUnknown
  , public IBaseFilter
{
  CComPtr<IUnknown>  m_pEVR;
  CEVRAllocatorPresenter *m_pAllocatorPresenter;

public:

  // IBaseFilter
    virtual HRESULT STDMETHODCALLTYPE EnumPins(__out  IEnumPins **ppEnum)
  {
    CComPtr<IBaseFilter> pEVRBase;
    if (m_pEVR)
      m_pEVR->QueryInterface(&pEVRBase);
    if (pEVRBase)
      return pEVRBase->EnumPins(ppEnum);
    return E_NOTIMPL;
  }
    
    virtual HRESULT STDMETHODCALLTYPE FindPin(LPCWSTR Id, __out  IPin **ppPin)
  {
    CComPtr<IBaseFilter> pEVRBase;
    if (m_pEVR)
      m_pEVR->QueryInterface(&pEVRBase);
    if (pEVRBase)
      return pEVRBase->FindPin(Id, ppPin);
    return E_NOTIMPL;
  }
    
    virtual HRESULT STDMETHODCALLTYPE QueryFilterInfo(__out  FILTER_INFO *pInfo)
  {
    CComPtr<IBaseFilter> pEVRBase;
    if (m_pEVR)
      m_pEVR->QueryInterface(&pEVRBase);
    if (pEVRBase)
      return pEVRBase->QueryFilterInfo(pInfo);
    return E_NOTIMPL;
  }
    
    virtual HRESULT STDMETHODCALLTYPE JoinFilterGraph(__in_opt  IFilterGraph *pGraph, __in_opt  LPCWSTR pName)
  {
        CComPtr<IBaseFilter> pEVRBase;
    if (m_pEVR)
      m_pEVR->QueryInterface(&pEVRBase);
    if (pEVRBase)
      return pEVRBase->JoinFilterGraph(pGraph, pName);
    return E_NOTIMPL;
  }
    
    virtual HRESULT STDMETHODCALLTYPE QueryVendorInfo(__out  LPWSTR *pVendorInfo)
  {
    CComPtr<IBaseFilter> pEVRBase;
    if (m_pEVR)
      m_pEVR->QueryInterface(&pEVRBase);
    if (pEVRBase)
      return pEVRBase->QueryVendorInfo(pVendorInfo);
    return E_NOTIMPL;
  }

    virtual HRESULT STDMETHODCALLTYPE Stop( void)
  {
    CComPtr<IBaseFilter> pEVRBase;
    if (m_pEVR)
      m_pEVR->QueryInterface(&pEVRBase);
    if (pEVRBase)
      return pEVRBase->Stop();
    return E_NOTIMPL;
  }
    
    virtual HRESULT STDMETHODCALLTYPE Pause( void)
  {
    CComPtr<IBaseFilter> pEVRBase;
    if (m_pEVR)
      m_pEVR->QueryInterface(&pEVRBase);
    if (pEVRBase)
      return pEVRBase->Pause();
    return E_NOTIMPL;
  }
    
    virtual HRESULT STDMETHODCALLTYPE Run( REFERENCE_TIME tStart)
  {
    CComPtr<IBaseFilter> pEVRBase;
    if (m_pEVR)
      m_pEVR->QueryInterface(&pEVRBase);
    if (pEVRBase)
      return pEVRBase->Run(tStart);
    return E_NOTIMPL;
  }
    
    virtual HRESULT STDMETHODCALLTYPE GetState( DWORD dwMilliSecsTimeout, __out  FILTER_STATE *State);
    
    virtual HRESULT STDMETHODCALLTYPE SetSyncSource(__in_opt  IReferenceClock *pClock)
  {
    CComPtr<IBaseFilter> pEVRBase;
    if (m_pEVR)
      m_pEVR->QueryInterface(&pEVRBase);
    if (pEVRBase)
      return pEVRBase->SetSyncSource(pClock);
    return E_NOTIMPL;
  }
    
    virtual HRESULT STDMETHODCALLTYPE GetSyncSource(__deref_out_opt  IReferenceClock **pClock)
  {
    CComPtr<IBaseFilter> pEVRBase;
    if (m_pEVR)
      m_pEVR->QueryInterface(&pEVRBase);
    if (pEVRBase)
      return pEVRBase->GetSyncSource(pClock);
    return E_NOTIMPL;
  }

    virtual HRESULT STDMETHODCALLTYPE GetClassID(__RPC__out CLSID *pClassID)
  {
    CComPtr<IBaseFilter> pEVRBase;
    if (m_pEVR)
      m_pEVR->QueryInterface(&pEVRBase);
    if (pEVRBase)
      return pEVRBase->GetClassID(pClassID);
    return E_NOTIMPL;
  }

  COuterEVR(const TCHAR* pName, LPUNKNOWN pUnk, HRESULT& hr, CEVRAllocatorPresenter *pAllocatorPresenter) : CUnknown(pName, pUnk)
  {
    hr = m_pEVR.CoCreateInstance(CLSID_EnhancedVideoRenderer, GetOwner());
    m_pAllocatorPresenter = pAllocatorPresenter;


  }

  ~COuterEVR();

  DECLARE_IUNKNOWN;
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
  {
    HRESULT hr;
    if (riid == __uuidof(IBaseFilter))
    {
      return GetInterface((IBaseFilter*)this, ppv);
    }

    if (riid == __uuidof(IMediaFilter))
    {
      return GetInterface((IMediaFilter*)this, ppv);
    }
    if (riid == __uuidof(IPersist))
    {
      return GetInterface((IPersist*)this, ppv);
    }
    if (riid == __uuidof(IBaseFilter))
    {
      return GetInterface((IBaseFilter*)this, ppv);
    }

    hr = m_pEVR ? m_pEVR->QueryInterface(riid, ppv) : E_NOINTERFACE;
    return SUCCEEDED(hr) ? hr : __super::NonDelegatingQueryInterface(riid, ppv);
  }
};

HRESULT STDMETHODCALLTYPE COuterEVR::GetState( DWORD dwMilliSecsTimeout, __out  FILTER_STATE *State)
{
  HRESULT ReturnValue;
  if (m_pAllocatorPresenter->GetState(dwMilliSecsTimeout, State, ReturnValue))
    return ReturnValue;
  CComPtr<IBaseFilter> pEVRBase;
  if (m_pEVR)
    m_pEVR->QueryInterface(&pEVRBase);
  if (pEVRBase)
    return pEVRBase->GetState(dwMilliSecsTimeout, State);
  return E_NOTIMPL;
}

COuterEVR::~COuterEVR()
{
}

// Default frame rate.
const MFRatio g_DefaultFrameRate = { 30, 1 };

// Function declarations.
RECT    CorrectAspectRatio(const RECT& src, const MFRatio& srcPAR, const MFRatio& destPAR);
BOOL    AreMediaTypesEqual(IMFMediaType *pType1, IMFMediaType *pType2);
HRESULT ValidateVideoArea(const MFVideoArea& area, UINT32 width, UINT32 height);
HRESULT SetDesiredSampleTime(IMFSample *pSample, const LONGLONG& hnsSampleTime, const LONGLONG& hnsDuration);
HRESULT ClearDesiredSampleTime(IMFSample *pSample);
BOOL    IsSampleTimePassed(IMFClock *pClock, IMFSample *pSample);
HRESULT SetMixerSourceRect(IMFTransform *pMixer, const MFVideoNormalizedRect& nrcSource);
float MFOffsetToFloat(const MFOffset& offset);

MFOffset MakeOffset(float v)
{
    MFOffset offset;
    offset.value = short(v);
    offset.fract = WORD(65536 * (v-offset.value));
    return offset;
}

MFVideoArea MakeArea(float x, float y, DWORD width, DWORD height)
{
    MFVideoArea area;
    area.OffsetX = MakeOffset(x);
    area.OffsetY = MakeOffset(y);
    area.Area.cx = width;
    area.Area.cy = height;
    return area;
}

CEVRAllocatorPresenter::CEVRAllocatorPresenter(HRESULT& hr, CStdString &_Error):
m_RenderState(RENDER_STATE_SHUTDOWN),
    m_pD3DPresentEngine(NULL),
    m_pClock(NULL),
    m_pMixer(NULL),
m_pMediaEventSink(NULL),
m_pMediaType(NULL),
m_bSampleNotify(false),
m_bRepaint(false),
m_bEndStreaming(false),
m_bPrerolled(false),
m_fRate(1.0f),
m_TokenCounter(0),
m_SampleFreeCB(this, &CEVRAllocatorPresenter::OnSampleFree)
{
  // Initial source rectangle = (0,0,1,1)
  m_nrcSource.top = 0;
  m_nrcSource.left = 0;
  m_nrcSource.bottom = 1;
  m_nrcSource.right = 1;
  m_pD3DPresentEngine = new D3DPresentEngine(hr);
  m_bNeedNewDevice = false;
  if (!m_pD3DPresentEngine)
    hr = E_OUTOFMEMORY;
  

  m_scheduler.SetCallback(m_pD3DPresentEngine);

  HMODULE    hLib;
  // Load EVR functions
  hLib = LoadLibrary ("evr.dll");
  pfMFCreateDXSurfaceBuffer      = hLib ? (PTR_MFCreateDXSurfaceBuffer)      GetProcAddress (hLib, "MFCreateDXSurfaceBuffer") : NULL;
  pfMFCreateVideoSampleFromSurface  = hLib ? (PTR_MFCreateVideoSampleFromSurface)  GetProcAddress (hLib, "MFCreateVideoSampleFromSurface") : NULL;
  pfMFCreateVideoMediaType      = hLib ? (PTR_MFCreateVideoMediaType)      GetProcAddress (hLib, "MFCreateVideoMediaType") : NULL;

  if (!pfMFCreateDXSurfaceBuffer || !pfMFCreateVideoSampleFromSurface || !pfMFCreateVideoMediaType)
  {
    if (!pfDXVA2CreateDirect3DDeviceManager9)
      CLog::Log(LOGERROR,"Could not find DXVA2CreateDirect3DDeviceManager9 (dxva2.dll)");
    if (!pfMFCreateDXSurfaceBuffer)
      CLog::Log(LOGERROR,"Could not find MFCreateDXSurfaceBuffer (evr.dll)");
    if (!pfMFCreateVideoSampleFromSurface)
      CLog::Log(LOGERROR,"Could not find MFCreateVideoSampleFromSurface (evr.dll)");
    if (!pfMFCreateVideoMediaType)
    CLog::Log(LOGERROR,"Could not find MFCreateVideoMediaType (evr.dll)");
    hr = E_FAIL;
    return;
  }

  // Load Vista specifics DLLs
  hLib = LoadLibrary ("AVRT.dll");
  pfAvSetMmThreadCharacteristicsW    = hLib ? (PTR_AvSetMmThreadCharacteristicsW)  GetProcAddress (hLib, "AvSetMmThreadCharacteristicsW") : NULL;
  pfAvSetMmThreadPriority        = hLib ? (PTR_AvSetMmThreadPriority)      GetProcAddress (hLib, "AvSetMmThreadPriority") : NULL;
  pfAvRevertMmThreadCharacteristics  = hLib ? (PTR_AvRevertMmThreadCharacteristics)  GetProcAddress (hLib, "AvRevertMmThreadCharacteristics") : NULL;
  g_Windowing.Register(this);
  g_renderManager.PreInit(true);
}

CEVRAllocatorPresenter::~CEVRAllocatorPresenter()
{
  // COM interfaces
  SAFE_RELEASE(m_pClock);
  SAFE_RELEASE(m_pMixer);
  SAFE_RELEASE(m_pMediaEventSink);
  SAFE_RELEASE(m_pMediaType);
  // Deletable objects
  SAFE_DELETE(m_pD3DPresentEngine);
  
  g_renderManager.UnInit();
}


STDMETHODIMP CEVRAllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
  CheckPointer(ppRenderer, E_POINTER);
  *ppRenderer = NULL;
  HRESULT hr = E_FAIL;
  do
  {
    CMacrovisionKicker* pMK  = new CMacrovisionKicker(NAME("CMacrovisionKicker"), NULL);
    CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)pMK;
    COuterEVR *pOuterEVR = new COuterEVR(NAME("COuterEVR"), pUnk, hr, this);
    m_pOuterEVR = pOuterEVR;
    pMK->SetInner((IUnknown*)(INonDelegatingUnknown*)pOuterEVR);
    CComQIPtr<IBaseFilter> pBF = pUnk;
    if (FAILED (hr))
    {
      CLog::Log(LOGERROR,"%s Failed creating enchanced video renderer",__FUNCTION__);
      break;
    }
    CComPtr<IMFVideoPresenter>    pVP;
    CComPtr<IMFVideoRenderer>    pMFVR;
    CComQIPtr<IMFGetService, &__uuidof(IMFGetService)> pMFGS = pBF;
    hr = pMFGS->GetService (MR_VIDEO_RENDER_SERVICE, IID_IMFVideoRenderer, (void**)&pMFVR);
    if(SUCCEEDED(hr)) 
       hr = QueryInterface (__uuidof(IMFVideoPresenter), (void**)&pVP);
    if(SUCCEEDED(hr)) 
      hr = pMFVR->InitializeRenderer (NULL, pVP);

    //something related with no crash in vista
    CComPtr<IPin>      pPin = DShowUtil::GetFirstPin(pBF);
    CComQIPtr<IMemInputPin> pMemInputPin = pPin;    
    m_fUseInternalTimer = HookNewSegmentAndReceive((IPinC*)(IPin*)pPin, (IMemInputPinC*)(IMemInputPin*)pMemInputPin);
    if(FAILED(hr))
      *ppRenderer = NULL;
    else
      *ppRenderer = pBF.Detach();
  } while (0);

  return hr;

}

// IMFClockStateSink
STDMETHODIMP CEVRAllocatorPresenter::OnClockStart(MFTIME hnsSystemTime,  LONGLONG llClockStartOffset)
{
  HRESULT hr = S_OK;
  CAutoLock lock(&m_ObjectLock);
  // We cannot start after shutdown.
  hr = CheckShutdown();
  if (FAILED(hr))
    return hr;
  m_RenderState = RENDER_STATE_STARTED;
  // Check if the clock is already active (not stopped). 
    if (IsActive())
    {
        // If the clock position changes while the clock is active, it 
        // is a seek request. We need to flush all pending samples.
        if (llClockStartOffset != PRESENTATION_CURRENT_POSITION)
            Flush();
    }
    else
    {
        // The clock has started from the stopped state. 

        // Possibly we are in the middle of frame-stepping OR have samples waiting 
        // in the frame-step queue. Deal with these two cases first:
        CHECK_HR(hr = StartFrameStep());
    }
  ProcessOutputLoop();
  CLog::Log(LOGDEBUG,"%s hnsSystemTime = %I64d,   llClockStartOffset = %I64d", __FUNCTION__,hnsSystemTime, llClockStartOffset);
done:
  return hr;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockStop(MFTIME hnsSystemTime)
{
  CLog::Log(LOGDEBUG,"%s OnClockStop  hnsSystemTime = %I64d", __FUNCTION__, hnsSystemTime);

  CAutoLock lock(&m_ObjectLock);

  HRESULT hr = S_OK;
  hr = CheckShutdown();
  if (FAILED(hr))
    return hr;
  if (m_RenderState != RENDER_STATE_STOPPED)
  {
    m_RenderState = RENDER_STATE_STOPPED;
    Flush();

    // If we are in the middle of frame-stepping, cancel it now.
    if (m_FrameStep.state != FRAMESTEP_NONE)
      CancelFrameStep();
  }

    return hr;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockPause(MFTIME hnsSystemTime)
{
  CLog::Log(LOGDEBUG,"%s OnClockPause  hnsSystemTime = %I64d\n", __FUNCTION__, hnsSystemTime);
  CAutoLock lock(&m_ObjectLock);

  HRESULT hr = S_OK;
  // We cannot pause the clock after shutdown.
  hr = CheckShutdown();
  if (FAILED(hr))
    return hr;

  // Set the state. (No other actions are necessary.)
  m_RenderState = RENDER_STATE_PAUSED;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockRestart(MFTIME hnsSystemTime)
{
  CAutoLock lock(&m_ObjectLock);

  HRESULT hr = S_OK;
  hr = CheckShutdown();
  if (FAILED(hr))
      return hr;
  // The EVR calls OnClockRestart only while paused.
  assert(m_RenderState == RENDER_STATE_PAUSED);

  m_RenderState = RENDER_STATE_STARTED;

  // Possibly we are in the middle of frame-stepping OR we have samples waiting 
  // in the frame-step queue. Deal with these two cases first:
  hr = StartFrameStep();
  if (FAILED(hr))
    return hr;

  // Now resume the presentation loop.
  ProcessOutputLoop();
  CLog::Log(LOGDEBUG,"%s OnClockRestart  hnsSystemTime = %I64d\n", __FUNCTION__, hnsSystemTime);
  return hr;
}


STDMETHODIMP CEVRAllocatorPresenter::OnClockSetRate(MFTIME hnsSystemTime, float flRate)
{
  ASSERT (false);
  return E_NOTIMPL;
}

// IBaseFilter delegate
bool CEVRAllocatorPresenter::GetState( DWORD dwMilliSecsTimeout, FILTER_STATE *State, HRESULT &_ReturnValue)
{
  CAutoLock lock(&m_ObjectLock);

  /*if (m_bSignaledStarvation)
  {
    int nSamples = max(m_nNbDXSurface / 2, 1);
    if ((m_ScheduledSamples.GetCount() < nSamples || m_LastSampleOffset < -m_rtTimePerFrame*2))
    {      
      *State = (FILTER_STATE)Paused;
      _ReturnValue = VFW_S_STATE_INTERMEDIATE;
      return true;
    }
    m_bSignaledStarvation = false;
  }*/
  return false;
}

///////////////////////////////////////////////////////////////////////////////
//
// IUnknown methods
//
///////////////////////////////////////////////////////////////////////////////

HRESULT CEVRAllocatorPresenter::QueryInterface(REFIID riid, void ** ppv)
{
    CheckPointer(ppv, E_POINTER);

    if (riid == __uuidof(IUnknown))
    {
        *ppv = static_cast<IUnknown*>( static_cast<IMFVideoPresenter*>(this) );
    }
    else if (riid == __uuidof(IMFVideoDeviceID))
    {
        *ppv = static_cast<IMFVideoDeviceID*>(this);
    }
    else if (riid == __uuidof(IMFVideoPresenter))
    {
        *ppv = static_cast<IMFVideoPresenter*>(this);
    }
    else if (riid == __uuidof(IMFClockStateSink))    // Inherited from IMFVideoPresenter
    {
        *ppv = static_cast<IMFClockStateSink*>(this);
    }
    /*else if (riid == __uuidof(IMFRateSupport))
    {
        *ppv = static_cast<IMFRateSupport*>(this);
    }*/
    else if (riid == __uuidof(IMFGetService))
    {
        *ppv = static_cast<IMFGetService*>(this);
    }
    else if (riid == __uuidof(IMFTopologyServiceLookupClient))
    {
        *ppv = static_cast<IMFTopologyServiceLookupClient*>(this);
    }
    else if (riid == __uuidof(IMFVideoDisplayControl))
    {
        *ppv = static_cast<IMFVideoDisplayControl*>(this);
    }
	else if (riid == __uuidof(IEVRPresenterRegisterCallback))
    {
        *ppv = static_cast<IEVRPresenterRegisterCallback*>(this);
    }
	else if (riid == __uuidof(IEVRPresenterSettings))
    {
        *ppv = static_cast<IEVRPresenterSettings*>(this);
    }
	else if (riid == __uuidof(IEVRTrustedVideoPlugin))
    {
        *ppv = static_cast<IEVRTrustedVideoPlugin*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG CEVRAllocatorPresenter::AddRef()
{
  return InterlockedIncrement(& m_refCount);
}

ULONG CEVRAllocatorPresenter::Release()
{
    ULONG ret = InterlockedDecrement(& m_refCount);
    if( ret == 0 )
    {
        delete this;
    }

    return ret;
}

STDMETHODIMP CEVRAllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
  HRESULT    hr;
  if(riid == __uuidof(IMFClockStateSink))
    hr = GetInterface((IMFClockStateSink*)this, ppv);
  else if(riid == __uuidof(IMFVideoPresenter))
    hr = GetInterface((IMFVideoPresenter*)this, ppv);
  else if(riid == __uuidof(IMFTopologyServiceLookupClient))
    hr = GetInterface((IMFTopologyServiceLookupClient*)this, ppv);
  else if(riid == __uuidof(IMFVideoDeviceID))
    hr = GetInterface((IMFVideoDeviceID*)this, ppv);
  else if(riid == __uuidof(IMFGetService))
    hr = GetInterface((IMFGetService*)this, ppv);
  else if(riid == __uuidof(IMFAsyncCallback))
    hr = GetInterface((IMFAsyncCallback*)this, ppv);
  else if(riid == __uuidof(IMFVideoDisplayControl))
    hr = GetInterface((IMFVideoDisplayControl*)this, ppv);
  else if(riid == __uuidof(IEVRTrustedVideoPlugin))
    hr = GetInterface((IEVRTrustedVideoPlugin*)this, ppv);
  else if(riid == IID_IQualProp)
    hr = GetInterface((IQualProp*)this, ppv);
  else if(riid == __uuidof(IMFRateSupport))
    hr = GetInterface((IMFRateSupport*)this, ppv);
  else if(riid == __uuidof(IEVRPresenterRegisterCallback))
    hr = GetInterface((IEVRPresenterRegisterCallback*)this,ppv);
  else if (riid == __uuidof(IEVRPresenterSettings))
    hr = GetInterface((IEVRPresenterSettings*)this,ppv);
  //else if(riid == __uuidof(IDirect3DDeviceManager9))
    //hr = m_pDeviceManager->QueryInterface (__uuidof(IDirect3DDeviceManager9), (void**) ppv);

  return hr;
}


//IQualProp
HRESULT STDMETHODCALLTYPE CEVRAllocatorPresenter::get_FramesDroppedInRenderer(int *frameDropped)
{
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CEVRAllocatorPresenter::get_FramesDrawn(int *frameDrawn)
{
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CEVRAllocatorPresenter::get_AvgFrameRate(int *avgFrameRate)
{
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CEVRAllocatorPresenter::get_Jitter(int *iJitter)
{
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CEVRAllocatorPresenter::get_AvgSyncOffset(int *piAvg)
{
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CEVRAllocatorPresenter::get_DevSyncOffset(int *piDev)
{
  return E_NOTIMPL;
}

//IMFVideoPresenter
HRESULT STDMETHODCALLTYPE CEVRAllocatorPresenter::ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam)
{

  HRESULT hr = S_OK;

  CAutoLock lock(&m_ObjectLock);

  switch (eMessage)
    {
    // Flush all pending samples.
    case MFVP_MESSAGE_FLUSH:
        hr = Flush();
        break;

    // Renegotiate the media type with the mixer.
    case MFVP_MESSAGE_INVALIDATEMEDIATYPE:
        CLog::Log(LOGDEBUG,"MFVP_MESSAGE_INVALIDATEMEDIATYPE");
        hr = RenegotiateMediaType();
        break;

    // The mixer received a new input sample. 
    case MFVP_MESSAGE_PROCESSINPUTNOTIFY:
        
        hr = ProcessInputNotify();
        break;

    // Streaming is about to start.
    case MFVP_MESSAGE_BEGINSTREAMING:
      CLog::Log(LOGDEBUG,"MFVP_MESSAGE_BEGINSTREAMING");
        hr = BeginStreaming();
        break;

    // Streaming has ended. (The EVR has stopped.)
    case MFVP_MESSAGE_ENDSTREAMING:
      CLog::Log(LOGDEBUG,"MFVP_MESSAGE_ENDSTREAMING");
        hr = EndStreaming();
        break;

    // All input streams have ended.
    case MFVP_MESSAGE_ENDOFSTREAM:
        // Set the EOS flag. 
        m_bEndStreaming = TRUE; 
        // Check if it's time to send the EC_COMPLETE event to the EVR.
        hr = CheckEndOfStream();
        break;

    // Frame-stepping is starting.
    case MFVP_MESSAGE_STEP:
        hr = PrepareFrameStep(LODWORD(ulParam));
        break;

    // Cancels frame-stepping.
    case MFVP_MESSAGE_CANCELSTEP:
        hr = CancelFrameStep();
        break;

    default:
        hr = E_INVALIDARG; // Unknown message. (This case should never occur.)
        break;
    }


    return hr;
}

//-----------------------------------------------------------------------------
// ProcessInputNotify
//
// Attempts to get a new output sample from the mixer.
//
// This method is called when the EVR sends an MFVP_MESSAGE_PROCESSINPUTNOTIFY 
// message, which indicates that the mixer has a new input sample. 
//
// Note: If there are multiple input streams, the mixer might not deliver an 
// output sample for every input sample. 
//-----------------------------------------------------------------------------

HRESULT CEVRAllocatorPresenter::ProcessInputNotify()
{
    HRESULT hr = S_OK;

    // Set the flag that says the mixer has a new sample.
    m_bSampleNotify = TRUE;

    if (m_pMediaType == NULL)
    {
        // We don't have a valid media type yet.
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
    }
    else
    {
        // Try to process an output sample.
        ProcessOutputLoop();
    }
    return hr;
}

//-----------------------------------------------------------------------------
// BeginStreaming
// 
// Called when streaming begins.
//-----------------------------------------------------------------------------

HRESULT CEVRAllocatorPresenter::BeginStreaming()
{
    HRESULT hr = S_OK;

    // Start the scheduler thread. 
    hr = m_scheduler.StartScheduler(m_pClock);

    return hr;
}

//-----------------------------------------------------------------------------
// EndStreaming
// 
// Called when streaming ends.
//-----------------------------------------------------------------------------

HRESULT CEVRAllocatorPresenter::EndStreaming()
{
    HRESULT hr = S_OK;
    
    // Stop the scheduler thread.
    hr = m_scheduler.StopScheduler();

    return hr;
}
//-----------------------------------------------------------------------------
// CheckEndOfStream
// Performs end-of-stream actions if the EOS flag was set.
//
// Note: The presenter can receive the EOS notification before it has finished 
// presenting all of the scheduled samples. Therefore, signaling EOS and 
// handling EOS are distinct operations.
//-----------------------------------------------------------------------------

HRESULT CEVRAllocatorPresenter::CheckEndOfStream()
{
    if (!m_bEndStreaming)
    {
        // The EVR did not send the MFVP_MESSAGE_ENDOFSTREAM message.
        return S_OK; 
    }

    if (m_bSampleNotify)
    {
        // The mixer still has input. 
        return S_OK;
    }

    if (m_SamplePool.AreSamplesPending())
    {
        // Samples are still scheduled for rendering.
        return S_OK;
    }

    // Everything is complete. Now we can tell the EVR that we are done.
    NotifyEvent(EC_COMPLETE, (LONG_PTR)S_OK, 0);
    m_bEndStreaming = FALSE;
    return S_OK;
}



//-----------------------------------------------------------------------------
// PrepareFrameStep
//
// Gets ready to frame step. Called when the EVR sends the MFVP_MESSAGE_STEP
// message.
//
// Note: The EVR can send the MFVP_MESSAGE_STEP message before or after the 
// presentation clock starts. 
//-----------------------------------------------------------------------------
HRESULT CEVRAllocatorPresenter::PrepareFrameStep(DWORD cSteps)
{
    HRESULT hr = S_OK;

    // Cache the step count.
    m_FrameStep.steps += cSteps;

    // Set the frame-step state. 
    m_FrameStep.state = FRAMESTEP_WAITING_START;

    // If the clock is are already running, we can start frame-stepping now.
    // Otherwise, we will start when the clock starts.
    if (m_RenderState == RENDER_STATE_STARTED)
    {
        hr = StartFrameStep();       
    }

    return hr;
}

//-----------------------------------------------------------------------------
// StartFrameStep
//
// If the presenter is waiting to frame-step, this method starts the frame-step 
// operation. Called when the clock starts OR when the EVR sends the 
// MFVP_MESSAGE_STEP message (see PrepareFrameStep).
//-----------------------------------------------------------------------------

HRESULT CEVRAllocatorPresenter::StartFrameStep()
{
    assert(m_RenderState == RENDER_STATE_STARTED);

    HRESULT hr = S_OK;
    IMFSample *pSample = NULL;

    if (m_FrameStep.state == FRAMESTEP_WAITING_START)
    {

        // We have a frame-step request, and are waiting for the clock to start.
        // Set the state to "pending," which means we are waiting for samples.
        m_FrameStep.state = FRAMESTEP_PENDING;

        // If the frame-step queue already has samples, process them now.
        while (!m_FrameStep.samples.IsEmpty() && (m_FrameStep.state == FRAMESTEP_PENDING))
        {
            CHECK_HR(hr = m_FrameStep.samples.RemoveFront(&pSample));
            CHECK_HR(hr = DeliverFrameStepSample(pSample));
            SAFE_RELEASE(pSample);

            // We break from this loop when:
            //   (a) the frame-step queue is empty, or
            //   (b) the frame-step operation is complete.
        }
    }
    else if (m_FrameStep.state == FRAMESTEP_NONE)
    {
        // We are not frame stepping. Therefore, if the frame-step queue has samples, 
        // we need to process them normally.
        while (!m_FrameStep.samples.IsEmpty())
        {
            CHECK_HR(hr = m_FrameStep.samples.RemoveFront(&pSample));
            CHECK_HR(hr = DeliverSample(pSample, FALSE));
            SAFE_RELEASE(pSample);
        }
    }

done:
    SAFE_RELEASE(pSample);
    return hr;
}

//-----------------------------------------------------------------------------
// CompleteFrameStep
//
// Completes a frame-step operation. Called after the frame has been
// rendered.
//-----------------------------------------------------------------------------

HRESULT CEVRAllocatorPresenter::CompleteFrameStep(IMFSample *pSample)
{
    HRESULT hr = S_OK;
    MFTIME hnsSampleTime = 0;
    MFTIME hnsSystemTime = 0;

    // Update our state.
    m_FrameStep.state = FRAMESTEP_COMPLETE;
    m_FrameStep.pSampleNoRef = NULL;

    // Notify the EVR that the frame-step is complete.
    NotifyEvent(EC_STEP_COMPLETE, FALSE, 0); // FALSE = completed (not cancelled)

    // If we are scrubbing (rate == 0), also send the "scrub time" event.
    if (IsScrubbing())
    {
        // Get the time stamp from the sample.
        hr = pSample->GetSampleTime(&hnsSampleTime);
        if (FAILED(hr))
        {
            // No time stamp. Use the current presentation time.
            if (m_pClock)
            {
                (void)m_pClock->GetCorrelatedTime(0, &hnsSampleTime, &hnsSystemTime);
            }
            hr = S_OK; // (Not an error condition.)
        }

        NotifyEvent(EC_SCRUB_TIME, LODWORD(hnsSampleTime), HIDWORD(hnsSampleTime));
    }
    return hr;
}

//-----------------------------------------------------------------------------
// CancelFrameStep
//
// Cancels the frame-step operation.
//-----------------------------------------------------------------------------

HRESULT CEVRAllocatorPresenter::CancelFrameStep()
{
    FRAMESTEP_STATE oldState = m_FrameStep.state;

    m_FrameStep.state = FRAMESTEP_NONE;
    m_FrameStep.steps = 0;
    m_FrameStep.pSampleNoRef = NULL;
    // Don't clear the frame-step queue yet, because we might frame step again.

    if (oldState > FRAMESTEP_NONE && oldState < FRAMESTEP_COMPLETE)
    {
        // We were in the middle of frame-stepping when it was cancelled.
        // Notify the EVR.
        NotifyEvent(EC_STEP_COMPLETE, TRUE, 0); // TRUE = cancelled
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
// CreateOptimalVideoType
//
// Converts a proposed media type from the mixer into a type that is suitable for the presenter.
// 
// pProposedType: Media type that we got from the mixer.
// ppOptimalType: Receives the modfied media type.
//
// The presenter will attempt to set ppOptimalType as the mixer's output format.
//-----------------------------------------------------------------------------

HRESULT CEVRAllocatorPresenter::CreateOptimalVideoType(IMFMediaType* pProposedType, IMFMediaType **ppOptimalType)
{
  HRESULT hr = S_OK;
  
  RECT rcOutput;
  ZeroMemory(&rcOutput, sizeof(rcOutput));

  MFVideoArea displayArea;
  ZeroMemory(&displayArea, sizeof(displayArea));

  IMFMediaType *pOptimalType = NULL;
  MediaFoundationSamples::VideoTypeBuilder *pmtOptimal = NULL;

  // Create the helper object to manipulate the optimal type.
  CHECK_HR(hr = MediaFoundationSamples::MediaTypeBuilder::Create(&pmtOptimal));

  // Clone the proposed type.
  CHECK_HR(hr = pmtOptimal->CopyFrom(pProposedType));

  // Modify the new type.

  // For purposes of this SDK sample, we assume 
  // 1) The monitor's pixels are square.
  // 2) The presenter always preserves the pixel aspect ratio.

  // Set the pixel aspect ratio (PAR) to 1:1 (see assumption #1, above)
  CHECK_HR(hr = pmtOptimal->SetPixelAspectRatio(1, 1));

  // Get the output rectangle.
  rcOutput = m_pD3DPresentEngine->GetDestinationRect();

  // If the rectangle is empty calculate the output rectangle based on the media type.
  if (IsRectEmpty(&rcOutput))
    CHECK_HR(hr = CalculateOutputRectangle(pProposedType, &rcOutput));

  UINT32 fWidth,fHeight;
  if (SUCCEEDED(pmtOptimal->GetFrameDimensions(&fWidth,&fHeight)))
  {
    //HD
    if (fWidth >= 1280 || fHeight >=720)
    {
      CHECK_HR(hr = pmtOptimal->SetYUVMatrix(MFVideoTransferMatrix_BT709));
      CLog::Log(LOGNOTICE,"%s Setting to HD with Didnt get any frame dimensions on video sample defaulting the MF_MT_YUV_MATRIX to MFVideoTransferMatrix_BT709",__FUNCTION__);
    }
    //SD
    else
    {
      CHECK_HR(hr = pmtOptimal->SetYUVMatrix(MFVideoTransferMatrix_BT601));
      CLog::Log(LOGNOTICE,"%s Setting to SD with MF_MT_YUV_MATRIX to MFVideoTransferMatrix_BT601",__FUNCTION__);
    }
  }
  else
    //Defaulting to bt709
  {
    CHECK_HR(hr = pmtOptimal->SetYUVMatrix(MFVideoTransferMatrix_BT709));
    CLog::Log(LOGNOTICE,"%s Didnt get any frame dimensions on video sample defaulting the MF_MT_YUV_MATRIX to MFVideoTransferMatrix_BT709",__FUNCTION__);
  }
    // Set the extended color information: Use BT.709 

    
    //CHECK_HR(hr = pmtOptimal->SetTransferFunction(MFVideoTransFunc_709));
    //CHECK_HR(hr = pmtOptimal->SetVideoPrimaries(MFVideoPrimaries_BT709));
    //MFNominalRange_0_255 is MFNominalRange_Normal
    CHECK_HR(hr = pmtOptimal->SetVideoNominalRange(MFNominalRange_0_255));
    //MFVideoLighting_dim; for example, a living room with a television and additional low lighting.
    CHECK_HR(hr = pmtOptimal->SetVideoLighting(MFVideoLighting_dim));

    // Set the target rect dimensions. 
    CHECK_HR(hr = pmtOptimal->SetFrameDimensions(rcOutput.right, rcOutput.bottom));

    // Set the geometric aperture, and disable pan/scan.
    displayArea = MakeArea(0, 0, rcOutput.right, rcOutput.bottom);

    CHECK_HR(hr = pmtOptimal->SetPanScanEnabled(false));

    CHECK_HR(hr = pmtOptimal->SetGeometricAperture(displayArea));

    // Set the pan/scan aperture and the minimum display aperture. We don't care
    // about them per se, but the mixer will reject the type if these exceed the 
    // frame dimentions.
    CHECK_HR(hr = pmtOptimal->SetPanScanAperture(displayArea));
    CHECK_HR(hr = pmtOptimal->SetMinDisplayAperture(displayArea));

    // Return the pointer to the caller.
    CHECK_HR(hr = pmtOptimal->GetMediaType(&pOptimalType));

    *ppOptimalType = pOptimalType;
    (*ppOptimalType)->AddRef();

done:
    SAFE_RELEASE(pOptimalType);
    SAFE_RELEASE(pmtOptimal);

    return hr;

}

//-----------------------------------------------------------------------------
// CalculateOutputRectangle
// 
// Calculates the destination rectangle based on the mixer's proposed format.
// This calculation is used if the application did not specify a destination 
// rectangle.
//
// Note: The application sets the destination rectangle by calling 
// IMFVideoDisplayControl::SetVideoPosition.
//
// This method finds the display area of the mixer's proposed format and
// converts it to the pixel aspect ratio (PAR) of the display.
//-----------------------------------------------------------------------------

HRESULT CEVRAllocatorPresenter::CalculateOutputRectangle(IMFMediaType *pProposedType, RECT *prcOutput)
{
    HRESULT hr = S_OK;
    UINT32  srcWidth = 0, srcHeight = 0;

    MFRatio inputPAR = { 0, 0 };
    MFRatio outputPAR = { 0, 0 };
    RECT    rcOutput = { 0, 0, 0, 0};

    MFVideoArea displayArea;
    ZeroMemory(&displayArea, sizeof(displayArea));

    MediaFoundationSamples::VideoTypeBuilder *pmtProposed = NULL;

    // Helper object to read the media type.
    CHECK_HR(hr = MediaFoundationSamples::MediaTypeBuilder::Create(pProposedType, &pmtProposed));

    // Get the source's frame dimensions.
    CHECK_HR(hr = pmtProposed->GetFrameDimensions(&srcWidth, &srcHeight));

    // Get the source's display area. 
    CHECK_HR(hr = pmtProposed->GetVideoDisplayArea(&displayArea));

    // Calculate the x,y offsets of the display area.
    LONG offsetX = MediaFoundationSamples::GetOffset(displayArea.OffsetX);
    LONG offsetY = MediaFoundationSamples::GetOffset(displayArea.OffsetY);

    // Use the display area if valid. Otherwise, use the entire frame.
    if (displayArea.Area.cx != 0 &&
        displayArea.Area.cy != 0 &&
        offsetX + displayArea.Area.cx <= (LONG)(srcWidth) &&
        offsetY + displayArea.Area.cy <= (LONG)(srcHeight))
    {
        rcOutput.left   = offsetX;
        rcOutput.right  = offsetX + displayArea.Area.cx;
        rcOutput.top    = offsetY;
        rcOutput.bottom = offsetY + displayArea.Area.cy;
    }
    else
    {
        rcOutput.left = 0;
        rcOutput.top = 0;
        rcOutput.right = srcWidth;
        rcOutput.bottom = srcHeight;
    }

    // rcOutput is now either a sub-rectangle of the video frame, or the entire frame.

    // If the pixel aspect ratio of the proposed media type is different from the monitor's, 
    // letterbox the video. We stretch the image rather than shrink it.

    inputPAR = pmtProposed->GetPixelAspectRatio();    // Defaults to 1:1

    outputPAR.Denominator = outputPAR.Numerator = 1; // This is an assumption of the sample.

    // Adjust to get the correct picture aspect ratio.
    *prcOutput = CorrectAspectRatio(rcOutput, inputPAR, outputPAR);

done:
    SAFE_RELEASE(pmtProposed);
    return hr;
}

HRESULT CEVRAllocatorPresenter::GetCurrentMediaType(IMFVideoMediaType** ppMediaType)
{
  HRESULT hr = S_OK;

  if (ppMediaType == NULL)
    return E_POINTER;

  if (m_pMediaType == NULL)
  {
    CLog::Log(LOGERROR,"MediaType is NULL");
  }
  hr = m_pMediaType->QueryInterface(__uuidof(IMFVideoMediaType), (void**)ppMediaType);
  if FAILED(hr)
    CLog::Log(LOGERROR,"Query interface failed in GetCurrentMediaType");

  CLog::Log(LOGNOTICE,"GetCurrentMediaType done" );
  return hr;
}

// IMFTopologyServiceLookupClient        
STDMETHODIMP CEVRAllocatorPresenter::InitServicePointers(/* [in] */ __in  IMFTopologyServiceLookup *pLookup)
{
  CheckPointer(pLookup, E_POINTER);

    HRESULT             hr = S_OK;
    DWORD               dwObjects = 0;

    CAutoLock lock(&m_ObjectLock);

    // Do not allow initializing when playing or paused.
  if (IsActive())
    CHECK_HR(hr = MF_E_INVALIDREQUEST);
  dwObjects = 1;
  
  CLog::Log(LOGDEBUG,"%s getting mixer, render eventsink and renderclock",__FUNCTION__);

  SAFE_RELEASE(m_pClock);
  SAFE_RELEASE(m_pMixer);
  SAFE_RELEASE(m_pMediaEventSink);
  hr = pLookup->LookupService (MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_RENDER_SERVICE,
                               __uuidof (IMFClock ), (void**)&m_pClock, &dwObjects);

  dwObjects = 1;
  hr = pLookup->LookupService (MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_MIXER_SERVICE,
                               __uuidof (IMFTransform), (void**)&m_pMixer, &dwObjects);
// Make sure that we can work with this mixer.
  CHECK_HR(ConfigureMixer(m_pMixer));

  dwObjects = 1;
  hr = pLookup->LookupService (MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_RENDER_SERVICE,
                               __uuidof (IMediaEventSink ), (void**)&m_pMediaEventSink, &dwObjects);

  


  // Successfully initialized. Set the state to "stopped."
    m_RenderState = RENDER_STATE_STOPPED;

done:
    return hr;
}

STDMETHODIMP CEVRAllocatorPresenter::ReleaseServicePointers()
{
  CLog::Log(LOGDEBUG,"%s releasing mixer, render eventsink and renderclock",__FUNCTION__);
  HRESULT hr = S_OK;

    // Enter the shut-down state.
    {
        CAutoLock lock(&m_ObjectLock);
        m_RenderState = RENDER_STATE_SHUTDOWN;
    }

    // Flush any samples that were scheduled.
    Flush();

    // Clear the media type and release related resources (surfaces, etc).
    SetMediaType(NULL);

    // Release all services that were acquired from InitServicePointers.
    SAFE_RELEASE(m_pClock);
    SAFE_RELEASE(m_pMixer);
    SAFE_RELEASE(m_pMediaEventSink);

    return hr;
}

// IMFVideoDeviceID
STDMETHODIMP CEVRAllocatorPresenter::GetDeviceID(/* [out] */  __out  IID *pDeviceID)
{
  CheckPointer(pDeviceID, E_POINTER);
  *pDeviceID = IID_IDirect3DDevice9;
  return S_OK;
}


// IMFGetService
STDMETHODIMP CEVRAllocatorPresenter::GetService (/* [in] */ __RPC__in REFGUID guidService,
                /* [in] */ __RPC__in REFIID riid,
                /* [iid_is][out] */ __RPC__deref_out_opt LPVOID *ppvObject)
{
  HRESULT hr = S_OK;

    CheckPointer(ppvObject, E_POINTER);

    // The only service GUID that we support is MR_VIDEO_RENDER_SERVICE.
    

  if (guidService == MR_VIDEO_ACCELERATION_SERVICE)
    return m_pD3DPresentEngine->GetService(guidService,riid, (void**) ppvObject);
    
	if (guidService != MR_VIDEO_RENDER_SERVICE)
      return MF_E_UNSUPPORTED_SERVICE;
    // First try to get the service interface from the D3DPresentEngine object.
    hr = m_pD3DPresentEngine->GetService(guidService, riid, ppvObject);
    if (FAILED(hr))
    {
        // Next, QI to check if this object supports the interface.
        hr = QueryInterface(riid, ppvObject);
    }

    return hr;
  /*if (guidService == MR_VIDEO_RENDER_SERVICE)
    return NonDelegatingQueryInterface (riid, ppvObject);
  

  return E_NOINTERFACE;*/
}




HRESULT CEVRAllocatorPresenter::Flush()
{
  m_bPrerolled = false;
  // The scheduler might have samples that are waiting for
  // their presentation time. Tell the scheduler to flush.

  // This call blocks until the scheduler threads discards all scheduled samples.
  m_scheduler.Flush();

  // Flush the frame-step queue.
  m_FrameStep.samples.Clear();
  //if (m_RenderState == RENDER_STATE_STOPPED)
  // Repaint with black.
  //(void)m_pD3DPresentEngine->PresentSample(NULL, 0);

  return S_OK; 
}

// IEVRTrustedVideoPlugin
STDMETHODIMP CEVRAllocatorPresenter::IsInTrustedVideoMode(BOOL *pYes)
{
  CheckPointer(pYes, E_POINTER);
  *pYes = TRUE;
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::CanConstrict(BOOL *pYes)
{
  CheckPointer(pYes, E_POINTER);
  *pYes = TRUE;
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::SetConstriction(DWORD dwKPix)
{
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::DisableImageExport(BOOL bDisable)
{
  return S_OK;
}


LPCTSTR FindD3DFormat(const D3DFORMAT Format);

LPCTSTR GetMediaTypeFormatDesc(IMFMediaType *pMediaType)
{
  AM_MEDIA_TYPE*    pAMMedia = NULL;
  MFVIDEOFORMAT*    VideoFormat;

  HRESULT hr;
  hr = pMediaType->GetRepresentation  (FORMAT_MFVideoFormat, (void**)&pAMMedia);
  VideoFormat = (MFVIDEOFORMAT*)pAMMedia->pbFormat;

  LPCTSTR Type = FindD3DFormat((D3DFORMAT)VideoFormat->surfaceInfo.Format);
      
  pMediaType->FreeRepresentation (FORMAT_MFVideoFormat, (void*)pAMMedia);

  return Type;
}

LONGLONG GetMediaTypeMerit(IMFMediaType *pMediaType)
{
  AM_MEDIA_TYPE*    pAMMedia = NULL;
  MFVIDEOFORMAT*    VideoFormat;

  HRESULT hr;
  hr = pMediaType->GetRepresentation  (FORMAT_MFVideoFormat, (void**)&pAMMedia);
  if (FAILED(hr))
      return hr;
  VideoFormat = (MFVIDEOFORMAT*)pAMMedia->pbFormat;

  LONGLONG Merit = 0;
  switch (VideoFormat->surfaceInfo.Format)
  {
    case FCC('NV12'): Merit = 90000000; break;
    case FCC('YV12'): Merit = 80000000; break;
    case FCC('YUY2'): Merit = 70000000; break;
    case FCC('UYVY'): Merit = 60000000; break;

    case D3DFMT_X8R8G8B8: // Never opt for RGB
    case D3DFMT_A8R8G8B8: 
    case D3DFMT_R8G8B8: 
    case D3DFMT_R5G6B5: 
      Merit = 0; 
      break;
    default: Merit = 1000; break;
  }
      
  pMediaType->FreeRepresentation (FORMAT_MFVideoFormat, (void*)pAMMedia);

  return Merit;
}

//-----------------------------------------------------------------------------
// ConfigureMixer
//
// Initializes the mixer. Called from InitServicePointers.
//-----------------------------------------------------------------------------

HRESULT CEVRAllocatorPresenter::ConfigureMixer(IMFTransform *pMixer)
{
    HRESULT             hr = S_OK;
    IID                 deviceID = GUID_NULL;

    IMFVideoDeviceID    *pDeviceID = NULL;

    // Make sure that the mixer has the same device ID as ourselves.
    CHECK_HR(hr = pMixer->QueryInterface(__uuidof(IMFVideoDeviceID), (void**)&pDeviceID));
    CHECK_HR(hr = pDeviceID->GetDeviceID(&deviceID));

    if (!IsEqualGUID(deviceID, __uuidof(IDirect3DDevice9)))
    {
        CHECK_HR(hr = MF_E_INVALIDREQUEST);
    }

    // Set the zoom rectangle (ie, the source clipping rectangle).
    SetMixerSourceRect(pMixer, m_nrcSource);

done:
    SAFE_RELEASE(pDeviceID);
    return hr;
}

HRESULT CEVRAllocatorPresenter::RenegotiateMediaType()
{
    HRESULT      hr = S_OK;
  bool bFoundMediaType = false;
  IMFMediaType *pMixerType = NULL;
  IMFMediaType *pOptimalType = NULL;
  IMFVideoMediaType *pVideoType = NULL;
  if (!m_pMixer)
    return MF_E_INVALIDREQUEST;
  // Loop through all of the mixer's proposed output types.
    DWORD iTypeIndex = 0;

    while (!bFoundMediaType && (hr != MF_E_NO_MORE_TYPES))
    {
        SAFE_RELEASE(pMixerType);
        SAFE_RELEASE(pOptimalType);

        // Step 1. Get the next media type supported by mixer.
        hr = m_pMixer->GetOutputAvailableType(0, iTypeIndex++, &pMixerType);
        if (FAILED(hr))
        {
            break;
        }

        // From now on, if anything in this loop fails, try the next type,
        // until we succeed or the mixer runs out of types.

        // Step 2. Check if we support this media type. 
        if (SUCCEEDED(hr))
        {
            // Note: None of the modifications that we make later in CreateOptimalVideoType
            // will affect the suitability of the type, at least for us. (Possibly for the mixer.)
            hr = IsMediaTypeSupported(pMixerType);
        }

        // Step 3. Adjust the mixer's type to match our requirements.
        //SKIP this until i do a better type handling (TI-BEN)
        if (SUCCEEDED(hr))
        {
            hr = CreateOptimalVideoType(pMixerType, &pOptimalType);
        }

        // Step 4. Check if the mixer will accept this media type.
        if (SUCCEEDED(hr))
        {
            hr = m_pMixer->SetOutputType(0, pOptimalType, MFT_SET_TYPE_TEST_ONLY);
        }

        // Step 5. Try to set the media type on ourselves.
        if (SUCCEEDED(hr))
        {
            hr = SetMediaType(pOptimalType);
        }

        // Step 6. Set output media type on mixer.
        if (SUCCEEDED(hr))
        {
            hr = m_pMixer->SetOutputType(0, pOptimalType, 0);

            assert(SUCCEEDED(hr)); // This should succeed unless the MFT lied in the previous call.

            // If something went wrong, clear the media type.
            if (FAILED(hr))
            {
                SetMediaType(NULL);
            }
        }

        if (SUCCEEDED(hr))
        {
            bFoundMediaType = TRUE;
        }
    }

    SAFE_RELEASE(pMixerType);
    SAFE_RELEASE(pOptimalType);
    SAFE_RELEASE(pVideoType);

    return hr;
}

HRESULT CEVRAllocatorPresenter::SetMediaType(IMFMediaType* pType)
{
  // Note: pMediaType can be NULL (to clear the type)

    // Clearing the media type is allowed in any state (including shutdown).
    if (pType == NULL)
    {
        SAFE_RELEASE(m_pMediaType);
        ReleaseResources();
        return S_OK;
    }

    HRESULT hr = S_OK;
    MFRatio fps = { 0, 0 };
    VideoSampleList sampleQueue;


    IMFSample *pSample = NULL;

    // Cannot set the media type after shutdown.
    CHECK_HR(hr = CheckShutdown());
    
    // Check if the new type is actually different.
    // Note: This function safely handles NULL input parameters.
    if (AreMediaTypesEqual(m_pMediaType, pType))  
    {
        return S_OK; // Nothing more to do.
    }

    // We're really changing the type. First get rid of the old type.
    SAFE_RELEASE(m_pMediaType);
    ReleaseResources();

    // Initialize the presenter engine with the new media type.
    // The presenter engine allocates the samples. 

    CHECK_HR(hr = m_pD3DPresentEngine->CreateVideoSamples(pType, sampleQueue));

    // Mark each sample with our token counter. If this batch of samples becomes
    // invalid, we increment the counter, so that we know they should be discarded. 
    for (VideoSampleList::POSITION pos = sampleQueue.FrontPosition();
         pos != sampleQueue.EndPosition();
         pos = sampleQueue.Next(pos))
    {
        CHECK_HR(hr = sampleQueue.GetItemPos(pos, &pSample));
        CHECK_HR(hr = pSample->SetUINT32(MFSamplePresenter_SampleCounter, m_TokenCounter));

        SAFE_RELEASE(pSample);
    }


    // Add the samples to the sample pool.
    CHECK_HR(hr = m_SamplePool.Initialize(sampleQueue));

  //Getting the time per frame without using MFFrameRateToAverageTimePerFrame
  IMFVideoMediaType *pVideoMediaType;
  AM_MEDIA_TYPE* pAMMedia = NULL;
  MFVIDEOFORMAT* videoFormat = NULL;

  pType->GetRepresentation(FORMAT_MFVideoFormat,(void**)&pAMMedia);
  videoFormat = (MFVIDEOFORMAT*)pAMMedia->pbFormat;
  hr = pfMFCreateVideoMediaType(videoFormat, &pVideoMediaType);
    
  
  REFERENCE_TIME avgframe;
  //This method for getting the time per frame is coming from mediaportal
  if (videoFormat->videoInfo.FramesPerSecond.Numerator != 0)
    avgframe = (20000000I64*videoFormat->videoInfo.FramesPerSecond.Denominator)/videoFormat->videoInfo.FramesPerSecond.Numerator;
  //DShowUtil::ExtractAvgTimePerFrame(pAMMedia,avgframe);
    // Set the frame rate on the scheduler. 
    if (avgframe > 0)// SUCCEEDED(MediaFoundationSamples::GetFrameRate(pType, &fps)) && (fps.Numerator != 0) && (fps.Denominator != 0))
    {
        m_scheduler.SetFrameRate(avgframe);
    }
    else
    {
      //If anything did go wrong in the getting time per frame putthing this default frame rate
      //This is the default frame rate
        m_scheduler.SetFrameRate(400000);
    }

    // Store the media type.
    assert(pType != NULL);
    m_pMediaType = pType;
    m_pMediaType->AddRef();

done:
    if (FAILED(hr))
    {
        ReleaseResources();
    }
    return hr;
}

HRESULT CEVRAllocatorPresenter::IsMediaTypeSupported(IMFMediaType* pMixerType)
{
  HRESULT        hr;
  AM_MEDIA_TYPE*    pAMMedia;
  UINT        nInterlaceMode;

  hr=pMixerType->GetRepresentation(FORMAT_VideoInfo2, (void**)&pAMMedia);
  if (FAILED(hr))
      return hr;
  hr= pMixerType->GetUINT32 (MF_MT_INTERLACE_MODE, &nInterlaceMode);
  if (FAILED(hr))
      return hr;

  if ( (pAMMedia->majortype != MEDIATYPE_Video))
    hr = MF_E_INVALIDMEDIATYPE;
  pMixerType->FreeRepresentation (FORMAT_VideoInfo2, (void*)pAMMedia);
  return hr;
}


//-----------------------------------------------------------------------------
// ProcessOutputLoop
//
// Get video frames from the mixer and schedule them for presentation.
//-----------------------------------------------------------------------------

void CEVRAllocatorPresenter::ProcessOutputLoop()
{
    HRESULT hr = S_OK;

    // Process as many samples as possible.
    while (hr == S_OK)
    {
        // If the mixer doesn't have a new input sample, break from the loop.
        if (!m_bSampleNotify)
        {
            hr = MF_E_TRANSFORM_NEED_MORE_INPUT;
            break;
        }

        // Try to process a sample.
        hr = ProcessOutput();

        // NOTE: ProcessOutput can return S_FALSE to indicate it did not process a sample.
        // If so, we break out of the loop.
    }

    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
    {
        // The mixer has run out of input data. Check if we're at the end of the stream.
        CheckEndOfStream();
    }
}

//-----------------------------------------------------------------------------
// ProcessOutput
//
// Attempts to get a new output sample from the mixer.
//
// Called in two situations: 
// (1) ProcessOutputLoop, if the mixer has a new input sample. (m_bSampleNotify)
// (2) Repainting the last frame. (m_bRepaint)
//-----------------------------------------------------------------------------

HRESULT CEVRAllocatorPresenter::ProcessOutput()
{
    assert(m_bSampleNotify || m_bRepaint);  // See note above.

    HRESULT     hr = S_OK;
    DWORD       dwStatus = 0;
    LONGLONG    mixerStartTime = 0, mixerEndTime = 0;
    MFTIME      systemTime = 0;
    BOOL        bRepaint = m_bRepaint; // Temporarily store this state flag.  

    MFT_OUTPUT_DATA_BUFFER dataBuffer;
    ZeroMemory(&dataBuffer, sizeof(dataBuffer));

    IMFSample *pSample = NULL;

    // If the clock is not running, we present the first sample,
    // and then don't present any more until the clock starts. 

    if ((m_RenderState != RENDER_STATE_STARTED) &&  // Not running.
         !m_bRepaint &&                             // Not a repaint request.
         m_bPrerolled                               // At least one sample has been presented.
         )
    {
        return S_FALSE;
    }

    // Make sure we have a pointer to the mixer.
    if (m_pMixer == NULL)
    {
        return MF_E_INVALIDREQUEST;
    }

    // Try to get a free sample from the video sample pool.
    hr = m_SamplePool.GetSample(&pSample);
    if (hr == MF_E_SAMPLEALLOCATOR_EMPTY)
    {
        return S_FALSE; // No free samples. We'll try again when a sample is released.
    }
    CHECK_HR(hr);   // Fail on any other error code.

    // From now on, we have a valid video sample pointer, where the mixer will
    // write the video data.
    assert(pSample != NULL);

    // (If the following assertion fires, it means we are not managing the sample pool correctly.)
    assert(MFGetAttributeUINT32(pSample, MFSamplePresenter_SampleCounter, (UINT32)-1) == m_TokenCounter);

    if (m_bRepaint)
    {
        // Repaint request. Ask the mixer for the most recent sample.
        SetDesiredSampleTime(pSample, m_scheduler.LastSampleTime(), m_scheduler.FrameDuration());
        m_bRepaint = false; // OK to clear this flag now.
    }
    else
    {
        // Not a repaint request. Clear the desired sample time; the mixer will
        // give us the next frame in the stream.
        ClearDesiredSampleTime(pSample);

        if (m_pClock)
        {
            // Latency: Record the starting time for the ProcessOutput operation. 
            (void)m_pClock->GetCorrelatedTime(0, &mixerStartTime, &systemTime);
        }
    }

    // Now we are ready to get an output sample from the mixer. 
    dataBuffer.dwStreamID = 0;
    dataBuffer.pSample = pSample;
    dataBuffer.dwStatus = 0;

    hr = m_pMixer->ProcessOutput(0, 1, &dataBuffer, &dwStatus);

    if (FAILED(hr))
    {
        // Return the sample to the pool.
        HRESULT hr2 = m_SamplePool.ReturnSample(pSample);
        if (FAILED(hr2))
        {
            CHECK_HR(hr = hr2);
        }
        // Handle some known error codes from ProcessOutput.
        if (hr == MF_E_TRANSFORM_TYPE_NOT_SET)
        {
            // The mixer's format is not set. Negotiate a new format.
            hr = RenegotiateMediaType();
        }
        else if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
        {
            // There was a dynamic media type change. Clear our media type.
            SetMediaType(NULL);
        }
        else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
        {
            // The mixer needs more input. 
            // We have to wait for the mixer to get more input.
            m_bSampleNotify = false; 
        }
    }
    else
    {
        // We got an output sample from the mixer.

        if (m_pClock && !bRepaint)
        {
            // Latency: Record the ending time for the ProcessOutput operation,
            // and notify the EVR of the latency. 

            (void)m_pClock->GetCorrelatedTime(0, &mixerEndTime, &systemTime);

            LONGLONG latencyTime = mixerEndTime - mixerStartTime;
            NotifyEvent(EC_PROCESSING_LATENCY, (LONG_PTR)&latencyTime, 0);
        }

        // Set up notification for when the sample is released.
        CHECK_HR(hr = TrackSample(pSample));

        // Schedule the sample.
        if ((m_FrameStep.state == FRAMESTEP_NONE) || bRepaint)
        {
            CHECK_HR(hr = DeliverSample(pSample, bRepaint));
        }
        else
        {
            // We are frame-stepping (and this is not a repaint request).
            CHECK_HR(hr = DeliverFrameStepSample(pSample));
        }
        m_bPrerolled = TRUE; // We have presented at least one sample now.
    }

done:
    // Release any events that were returned from the ProcessOutput method. 
    // (We don't expect any events from the mixer, but this is a good practice.)
    ReleaseEventCollection(1, &dataBuffer); 

    SAFE_RELEASE(pSample);

    return hr;
}


//-----------------------------------------------------------------------------
// DeliverSample
//
// Schedule a video sample for presentation.
//
// Called from:
// - ProcessOutput
// - DeliverFrameStepSample
//-----------------------------------------------------------------------------

HRESULT CEVRAllocatorPresenter::DeliverSample(IMFSample *pSample, BOOL bRepaint)
{
  assert(pSample != NULL);

  HRESULT hr = S_OK;
  D3DPresentEngine::DeviceState state = D3DPresentEngine::DeviceOK;
    
  if (!g_renderManager.IsConfigured())
    g_renderManager.Configure(m_pD3DPresentEngine->GetVideoWidth(),m_pD3DPresentEngine->GetVideoHeight(),m_pD3DPresentEngine->GetVideoWidth(),m_pD3DPresentEngine->GetVideoHeight(),m_scheduler.GetFps(),CONF_FLAGS_FULLSCREEN);
  // If we are not actively playing, OR we are scrubbing (rate = 0) OR this is a 
  // repaint request, then we need to present the sample immediately. Otherwise, 
  // schedule it normally.

  BOOL bPresentNow = ((m_RenderState != RENDER_STATE_STARTED) ||  IsScrubbing() || bRepaint);

  // If we need new device dont deliver the sample
  if (m_bNeedNewDevice)
  {
    CLog::Log(LOGDEBUG,"%s Device Need reset to develiver sample",__FUNCTION__);
    if (SUCCEEDED(m_pD3DPresentEngine->ResetD3dDevice()))
    {
      //We got the device back so we can restart schedule sample now
      state = D3DPresentEngine::DeviceReset;
    }
  }
  else
  {
    hr = m_scheduler.ScheduleSample(pSample, bPresentNow);
  }

  if (state == D3DPresentEngine::DeviceReset)
  {
    // The Direct3D device was re-set. Notify the EVR.
    NotifyEvent(EC_DISPLAY_CHANGED, S_OK, 0);
    m_bNeedNewDevice = false;
    CLog::Log(LOGDEBUG,"%s Device reseting sending EC_DISPLAY_CHANGED to EVR",__FUNCTION__);
  }

  return hr;
}

//-----------------------------------------------------------------------------
// DeliverFrameStepSample
//
// Process a video sample for frame-stepping.
//-----------------------------------------------------------------------------

HRESULT CEVRAllocatorPresenter::DeliverFrameStepSample(IMFSample *pSample)
{
    HRESULT hr = S_OK;
    IUnknown *pUnk = NULL;

    // For rate 0, discard any sample that ends earlier than the clock time.
    if (IsScrubbing() && m_pClock && IsSampleTimePassed(m_pClock, pSample))
    {
        // Discard this sample.
    }
    else if (m_FrameStep.state >= FRAMESTEP_SCHEDULED)
    {
        // A frame was already submitted. Put this sample on the frame-step queue, 
        // in case we are asked to step to the next frame. If frame-stepping is
        // cancelled, this sample will be processed normally.
        CHECK_HR(hr = m_FrameStep.samples.InsertBack(pSample));
    }
    else
    {
        // We're ready to frame-step.

        // Decrement the number of steps.
        if (m_FrameStep.steps > 0)
        {
            m_FrameStep.steps--;
        }

        if (m_FrameStep.steps > 0)
        {
            // This is not the last step. Discard this sample.
        }
        else if (m_FrameStep.state == FRAMESTEP_WAITING_START)
        {
            // This is the right frame, but the clock hasn't started yet. Put the
            // sample on the frame-step queue. When the clock starts, the sample
            // will be processed.
            CHECK_HR(hr = m_FrameStep.samples.InsertBack(pSample));
        }
        else
        {
            // This is the right frame *and* the clock has started. Deliver this sample.
            CHECK_HR(hr = DeliverSample(pSample, FALSE));

            // QI for IUnknown so that we can identify the sample later.
            // (Per COM rules, an object alwayss return the same pointer when QI'ed for IUnknown.)
            CHECK_HR(hr = pSample->QueryInterface(__uuidof(IUnknown), (void**)&pUnk));

            // Save this value.
            m_FrameStep.pSampleNoRef = (DWORD_PTR)pUnk; // No add-ref. 

            // NOTE: We do not AddRef the IUnknown pointer, because that would prevent the 
            // sample from invoking the OnSampleFree callback after the sample is presented. 
            // We use this IUnknown pointer purely to identify the sample later; we never
            // attempt to dereference the pointer.

            // Update our state.
            m_FrameStep.state = FRAMESTEP_SCHEDULED;
        }
    }
done:
    SAFE_RELEASE(pUnk);
    return hr;
}


//-----------------------------------------------------------------------------
// TrackSample
//
// Given a video sample, sets a callback that is invoked when the sample is no 
// longer in use. 
//
// Note: The callback method returns the sample to the pool of free samples; for
// more information, see EVRCustomPresenter::OnSampleFree(). 
//
// This method uses the IMFTrackedSample interface on the video sample.
//-----------------------------------------------------------------------------

HRESULT CEVRAllocatorPresenter::TrackSample(IMFSample *pSample)
{
    HRESULT hr = S_OK;
    IMFTrackedSample *pTracked = NULL;

    CHECK_HR(hr = pSample->QueryInterface(__uuidof(IMFTrackedSample), (void**)&pTracked));
    CHECK_HR(hr = pTracked->SetAllocator(&m_SampleFreeCB, NULL)); 

done:
    SAFE_RELEASE(pTracked);
    return hr;
}


//-----------------------------------------------------------------------------
// ReleaseResources
//
// Releases resources that the presenter uses to render video. 
//
// Note: This method flushes the scheduler queue and releases the video samples.
// It does not release helper objects such as the D3DPresentEngine, or free
// the presenter's media type.
//-----------------------------------------------------------------------------

void CEVRAllocatorPresenter::ReleaseResources()
{
    // Increment the token counter to indicate that all existing video samples
    // are "stale." As these samples get released, we'll dispose of them. 
    //
    // Note: The token counter is required because the samples are shared
    // between more than one thread, and they are returned to the presenter 
    // through an asynchronous callback (OnSampleFree). Without the token, we
    // might accidentally re-use a stale sample after the ReleaseResources
    // method returns.

    m_TokenCounter++;

    Flush();

    m_SamplePool.Clear();

    m_pD3DPresentEngine->ReleaseResources();
}

//-----------------------------------------------------------------------------
// OnSampleFree
//
// Callback that is invoked when a sample is released. For more information,
// see EVRCustomPresenterTrackSample().
//-----------------------------------------------------------------------------

HRESULT CEVRAllocatorPresenter::OnSampleFree(IMFAsyncResult *pResult)
{
    HRESULT hr = S_OK;
    IUnknown *pObject = NULL;
    IMFSample *pSample = NULL;
    IUnknown *pUnk = NULL;

    // Get the sample from the async result object.
    CHECK_HR(hr = pResult->GetObject(&pObject));
    CHECK_HR(hr = pObject->QueryInterface(__uuidof(IMFSample), (void**)&pSample));

    // If this sample was submitted for a frame-step, then the frame step is complete.
    if (m_FrameStep.state == FRAMESTEP_SCHEDULED) 
    {
        // QI the sample for IUnknown and compare it to our cached value.
        CHECK_HR(hr = pSample->QueryInterface(__uuidof(IMFSample), (void**)&pUnk));

        if (m_FrameStep.pSampleNoRef == (DWORD_PTR)pUnk)
        {
            // Notify the EVR. 
            CHECK_HR(hr = CompleteFrameStep(pSample));
        }

        // Note: Although pObject is also an IUnknown pointer, it's not guaranteed
        // to be the exact pointer value returned via QueryInterface, hence the 
        // need for the second QI.
    }

    m_ObjectLock.Lock();

    if (MFGetAttributeUINT32(pSample, MFSamplePresenter_SampleCounter, (UINT32)-1) == m_TokenCounter)
    {
        // Return the sample to the sample pool.
        CHECK_HR(hr = m_SamplePool.ReturnSample(pSample));

        // Now that a free sample is available, process more data if possible.
        (void)ProcessOutputLoop();
    }

    m_ObjectLock.Unlock();

done:
    if (FAILED(hr))
    {
        NotifyEvent(EC_ERRORABORT, hr, 0);
    }
    SAFE_RELEASE(pObject);
    SAFE_RELEASE(pSample);
    SAFE_RELEASE(pUnk);
    return hr;
}

HRESULT CEVRAllocatorPresenter::RegisterCallback(IEVRPresenterCallback *pCallback) 
{ 
	this->m_pD3DPresentEngine->RegisterCallback(pCallback);
	return S_OK; 
}

HRESULT CEVRAllocatorPresenter::SetBufferCount(int bufferCount)
{
	return m_pD3DPresentEngine->SetBufferCount(bufferCount);
}

void CEVRAllocatorPresenter::OnLostDevice()
{
  //CLog::Log(LOGDEBUG,"%s",__FUNCTION__);
}
void CEVRAllocatorPresenter::OnDestroyDevice()
{
  //Only this one is required for changing the device
  CLog::Log(LOGDEBUG,"%s",__FUNCTION__);
  m_bNeedNewDevice = true;
  
}

void CEVRAllocatorPresenter::OnCreateDevice()
{
  CLog::Log(LOGDEBUG,"%s",__FUNCTION__);
}



RECT CorrectAspectRatio(const RECT& src, const MFRatio& srcPAR, const MFRatio& destPAR)
{
    // Start with a rectangle the same size as src, but offset to the origin (0,0).
    RECT rc = {0, 0, src.right - src.left, src.bottom - src.top};

    // If the source and destination have the same PAR, there is nothing to do.
    // Otherwise, adjust the image size, in two steps:
    //  1. Transform from source PAR to 1:1
    //  2. Transform from 1:1 to destination PAR.

    if ((srcPAR.Numerator != destPAR.Numerator) || (srcPAR.Denominator != destPAR.Denominator))
    {
        // Correct for the source's PAR.

        if (srcPAR.Numerator > srcPAR.Denominator)
        {
            // The source has "wide" pixels, so stretch the width.
            rc.right = MulDiv(rc.right, srcPAR.Numerator, srcPAR.Denominator);
        }
        else if (srcPAR.Numerator > srcPAR.Denominator)
        {
            // The source has "tall" pixels, so stretch the height.
            rc.bottom = MulDiv(rc.bottom, srcPAR.Denominator, srcPAR.Numerator);
        }
        // else: PAR is 1:1, which is a no-op.


        // Next, correct for the target's PAR. This is the inverse operation of the previous.

        if (destPAR.Numerator > destPAR.Denominator)
        {
            // The destination has "tall" pixels, so stretch the width.
            rc.bottom = MulDiv(rc.bottom, destPAR.Denominator, destPAR.Numerator);
        }
        else if (destPAR.Numerator > destPAR.Denominator)
        {
            // The destination has "fat" pixels, so stretch the height.
            rc.right = MulDiv(rc.right, destPAR.Numerator, destPAR.Denominator);
        }
        // else: PAR is 1:1, which is a no-op.
    }

    return rc;
}


//-----------------------------------------------------------------------------
// AreMediaTypesEqual
//
// Tests whether two IMFMediaType's are equal. Either pointer can be NULL.
// (If both pointers are NULL, returns TRUE)
//-----------------------------------------------------------------------------

BOOL AreMediaTypesEqual(IMFMediaType *pType1, IMFMediaType *pType2)
{
    if ((pType1 == NULL) && (pType2 == NULL))
    {
        return TRUE; // Both are NULL.
    }
    else if ((pType1 == NULL) || (pType2 == NULL))
    {
        return FALSE; // One is NULL.
    }

    DWORD dwFlags = 0;
    HRESULT hr = pType1->IsEqual(pType2, &dwFlags);

    return (hr == S_OK);
}

// MFOffsetToFloat: Convert a fixed-point to a float.
float MFOffsetToFloat(const MFOffset& offset)
{
  return (float)offset.value + ((float)offset.value / 65536.0f);
}

//-----------------------------------------------------------------------------
// ValidateVideoArea:
//
// Returns S_OK if an area is smaller than width x height. 
// Otherwise, returns MF_E_INVALIDMEDIATYPE.
//-----------------------------------------------------------------------------

HRESULT ValidateVideoArea(const MFVideoArea& area, UINT32 width, UINT32 height)
{

    float fOffsetX = MFOffsetToFloat(area.OffsetX);
    float fOffsetY = MFOffsetToFloat(area.OffsetY);

    if ( ((LONG)fOffsetX + area.Area.cx > (LONG)width) ||
         ((LONG)fOffsetY + area.Area.cy > (LONG)height) )
    {
        return MF_E_INVALIDMEDIATYPE;
    }
    else
    {
        return S_OK;
    }
}


//-----------------------------------------------------------------------------
// SetDesiredSampleTime
//
// Sets the "desired" sample time on a sample. This tells the mixer to output 
// an earlier frame, not the next frame. (Used when repainting a frame.)
//
// This method uses the sample's IMFDesiredSample interface.
//
// hnsSampleTime: Time stamp of the frame that the mixer should output.
// hnsDuration: Duration of the frame.
//
// Note: Before re-using the sample, call ClearDesiredSampleTime to clear
// the desired time.
//-----------------------------------------------------------------------------

HRESULT SetDesiredSampleTime(IMFSample *pSample, const LONGLONG& hnsSampleTime, const LONGLONG& hnsDuration)
{
    if (pSample == NULL)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    IMFDesiredSample *pDesired = NULL;

    hr = pSample->QueryInterface(__uuidof(IMFDesiredSample), (void**)&pDesired);
    if (SUCCEEDED(hr))
    {
        // This method has no return value.
        (void)pDesired->SetDesiredSampleTimeAndDuration(hnsSampleTime, hnsDuration);
    }

    SAFE_RELEASE(pDesired);
    return hr;
}


//-----------------------------------------------------------------------------
// ClearDesiredSampleTime
//
// Clears the desired sample time. See SetDesiredSampleTime.
//-----------------------------------------------------------------------------

HRESULT ClearDesiredSampleTime(IMFSample *pSample)
{
    if (pSample == NULL)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    
    IMFDesiredSample *pDesired = NULL;
    IUnknown *pUnkSwapChain = NULL;
    
    // We store some custom attributes on the sample, so we need to cache them
    // and reset them.
    //
    // This works around the fact that IMFDesiredSample::Clear() removes all of the
    // attributes from the sample. 

    UINT32 counter = MFGetAttributeUINT32(pSample, MFSamplePresenter_SampleCounter, (UINT32)-1);
    UINT32 curSurface;
    (void)pSample->GetUINT32(GUID_SURFACE_INDEX, (UINT32*)&curSurface);

    hr = pSample->QueryInterface(__uuidof(IMFDesiredSample), (void**)&pDesired);
    if (SUCCEEDED(hr))
    {
        // This method has no return value.
        (void)pDesired->Clear();

        CHECK_HR(hr = pSample->SetUINT32(MFSamplePresenter_SampleCounter, counter));

        /*if (pUnkSwapChain)
        {
            CHECK_HR(hr = pSample->SetUnknown(MFSamplePresenter_SampleSwapChain, pUnkSwapChain));
        }*/
    }

done:
    SAFE_RELEASE(pUnkSwapChain);
    SAFE_RELEASE(pDesired);
    return hr;
}


//-----------------------------------------------------------------------------
// IsSampleTimePassed
//
// Returns TRUE if the entire duration of pSample is in the past.
//
// Returns FALSE if all or part of the sample duration is in the future, or if
// the function cannot determined (e.g., if the sample lacks a time stamp).
//-----------------------------------------------------------------------------

BOOL IsSampleTimePassed(IMFClock *pClock, IMFSample *pSample)
{
    assert(pClock != NULL);
    assert(pSample != NULL);

    if (pSample == NULL || pClock == NULL)
    {
        return E_POINTER;
    }


    HRESULT hr = S_OK;
    MFTIME hnsTimeNow = 0;
    MFTIME hnsSystemTime = 0;
    MFTIME hnsSampleStart = 0;
    MFTIME hnsSampleDuration = 0;

    // The sample might lack a time-stamp or a duration, and the
    // clock might not report a time.

    hr = pClock->GetCorrelatedTime(0, &hnsTimeNow, &hnsSystemTime);

    if (SUCCEEDED(hr))
    {
        hr = pSample->GetSampleTime(&hnsSampleStart);
    }
    if (SUCCEEDED(hr))
    {
        hr = pSample->GetSampleDuration(&hnsSampleDuration);
    }

    if (SUCCEEDED(hr))
    {
        if (hnsSampleStart + hnsSampleDuration < hnsTimeNow)
        {
            return TRUE; 
        }
    }

    return FALSE;
}


//-----------------------------------------------------------------------------
// SetMixerSourceRect
//
// Sets the ZOOM rectangle on the mixer.
//-----------------------------------------------------------------------------

HRESULT SetMixerSourceRect(IMFTransform *pMixer, const MFVideoNormalizedRect& nrcSource)
{
    if (pMixer == NULL)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    IMFAttributes *pAttributes = NULL;

    CHECK_HR(hr = pMixer->GetAttributes(&pAttributes));

    CHECK_HR(hr = pAttributes->SetBlob(VIDEO_ZOOM_RECT, (const UINT8*)&nrcSource, sizeof(nrcSource)));
        
done:
    SAFE_RELEASE(pAttributes);
    return hr;
}