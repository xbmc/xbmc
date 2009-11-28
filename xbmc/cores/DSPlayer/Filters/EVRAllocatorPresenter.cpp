
#include "MacrovisionKicker.h"
#include "utils/log.h"
#include "EVRAllocatorPresenter.h"
#include "application.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "dshowutil/dshowutil.h"
#include <mfapi.h>
#include <mferror.h>
#include "vmr9.h"
#include "IPinHook.h"


class COuterEVR
  : public CUnknown
  //, public IVMRMixerBitmap9
  , public IBaseFilter
{
  CComPtr<IUnknown>  m_pEVR;
  VMR9AlphaBitmap*  m_pVMR9AlphaBitmap;
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
    //m_pVMR9AlphaBitmap = pVMR9AlphaBitmap;
    m_pAllocatorPresenter = pAllocatorPresenter;


  }

  ~COuterEVR();

  DECLARE_IUNKNOWN;
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
  {
    HRESULT hr;

    if(riid == __uuidof(IVMRMixerBitmap9))
      return GetInterface((IVMRMixerBitmap9*)this, ppv);

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

  // IVMRMixerBitmap9
  /*STDMETHODIMP GetAlphaBitmapParameters(VMR9AlphaBitmap* pBmpParms);
  
  STDMETHODIMP SetAlphaBitmap(const VMR9AlphaBitmap*  pBmpParms);

  STDMETHODIMP UpdateAlphaBitmapParameters(const VMR9AlphaBitmap* pBmpParms);*/
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

/*STDMETHODIMP COuterEVR::GetAlphaBitmapParameters(VMR9AlphaBitmap* pBmpParms)
{
  CheckPointer(pBmpParms, E_POINTER);
  CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
  memcpy (pBmpParms, m_pVMR9AlphaBitmap, sizeof(VMR9AlphaBitmap));
  return S_OK;
}*/

/*STDMETHODIMP COuterEVR::SetAlphaBitmap(const VMR9AlphaBitmap*  pBmpParms)
{
  CheckPointer(pBmpParms, E_POINTER);
  CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
  memcpy (m_pVMR9AlphaBitmap, pBmpParms, sizeof(VMR9AlphaBitmap));
  m_pVMR9AlphaBitmap->dwFlags |= VMRBITMAP_UPDATE;
  m_pAllocatorPresenter->UpdateAlphaBitmap();
  return S_OK;
}*/

/*STDMETHODIMP COuterEVR::UpdateAlphaBitmapParameters(const VMR9AlphaBitmap* pBmpParms)
{
  CheckPointer(pBmpParms, E_POINTER);
  CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
  memcpy (m_pVMR9AlphaBitmap, pBmpParms, sizeof(VMR9AlphaBitmap));
  m_pVMR9AlphaBitmap->dwFlags |= VMRBITMAP_UPDATE;
  m_pAllocatorPresenter->UpdateAlphaBitmap();
  return S_OK;
}*/

COuterEVR::~COuterEVR()
{
}

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

CEVRAllocatorPresenter::CEVRAllocatorPresenter(HRESULT& hr, HWND wnd, CStdString &_Error,IDirect3D9* d3d, IDirect3DDevice9* d3dd):
m_refCount(1),
m_D3D(d3d),
m_D3DDev(d3dd),
m_nUsedBuffer(0),
m_bPendingResetDevice(0),
m_nNbDXSurface(1),
m_nCurSurface(0),
m_rtTimePerFrame(0)
{
  
  HMODULE    hLib;
  // Load EVR specifics DLLs
  hLib = LoadLibrary ("dxva2.dll");
  pfDXVA2CreateDirect3DDeviceManager9  = hLib ? (PTR_DXVA2CreateDirect3DDeviceManager9) GetProcAddress (hLib, "DXVA2CreateDirect3DDeviceManager9") : NULL;
  
  // Load EVR functions
  hLib = LoadLibrary ("evr.dll");
  pfMFCreateDXSurfaceBuffer      = hLib ? (PTR_MFCreateDXSurfaceBuffer)      GetProcAddress (hLib, "MFCreateDXSurfaceBuffer") : NULL;
  pfMFCreateVideoSampleFromSurface  = hLib ? (PTR_MFCreateVideoSampleFromSurface)  GetProcAddress (hLib, "MFCreateVideoSampleFromSurface") : NULL;
  pfMFCreateVideoMediaType      = hLib ? (PTR_MFCreateVideoMediaType)      GetProcAddress (hLib, "MFCreateVideoMediaType") : NULL;

  if (!pfDXVA2CreateDirect3DDeviceManager9 || !pfMFCreateDXSurfaceBuffer || !pfMFCreateVideoSampleFromSurface || !pfMFCreateVideoMediaType)
  {
    if (!pfDXVA2CreateDirect3DDeviceManager9)
      _Error += L"Could not find DXVA2CreateDirect3DDeviceManager9 (dxva2.dll)\n";
    if (!pfMFCreateDXSurfaceBuffer)
      _Error += L"Could not find MFCreateDXSurfaceBuffer (evr.dll)\n";
    if (!pfMFCreateVideoSampleFromSurface)
      _Error += L"Could not find MFCreateVideoSampleFromSurface (evr.dll)\n";
    if (!pfMFCreateVideoMediaType)
      _Error += L"Could not find MFCreateVideoMediaType (evr.dll)\n";
    hr = E_FAIL;
    return;
  }

  // Load Vista specifics DLLs
  hLib = LoadLibrary ("AVRT.dll");
  pfAvSetMmThreadCharacteristicsW    = hLib ? (PTR_AvSetMmThreadCharacteristicsW)  GetProcAddress (hLib, "AvSetMmThreadCharacteristicsW") : NULL;
  pfAvSetMmThreadPriority        = hLib ? (PTR_AvSetMmThreadPriority)      GetProcAddress (hLib, "AvSetMmThreadPriority") : NULL;
  pfAvRevertMmThreadCharacteristics  = hLib ? (PTR_AvRevertMmThreadCharacteristics)  GetProcAddress (hLib, "AvRevertMmThreadCharacteristics") : NULL;

  if (SUCCEEDED(pfDXVA2CreateDirect3DDeviceManager9(&m_iResetToken,&m_pDeviceManager)))
  {
    CLog::Log(LOGERROR,"Sucess to create DXVA2CreateDirect3DDeviceManager9");
    hr = m_pDeviceManager->ResetDevice(d3dd, m_iResetToken);
  }
  CComPtr<IDirectXVideoDecoderService>  pDecoderService;
  HANDLE              hDevice;
  hr = m_pDeviceManager->OpenDeviceHandle(&hDevice);
  hr = m_pDeviceManager->GetVideoService(hDevice,__uuidof(IDirectXVideoDecoderService), (void**)&pDecoderService);
  if (SUCCEEDED(hr))
  {
    HookDirectXVideoDecoderService (pDecoderService);
    m_pDeviceManager->CloseDeviceHandle (hDevice);
  }
  ResetStats();
  m_hThread		 = INVALID_HANDLE_VALUE;
  m_hSemPicture	 = INVALID_HANDLE_VALUE;
  m_hEvtPresent	 = INVALID_HANDLE_VALUE;
  m_hEvtFrameTimer = INVALID_HANDLE_VALUE;
  m_hEvtFlush		 = INVALID_HANDLE_VALUE;
  m_hEvtQuit		 = INVALID_HANDLE_VALUE;
  m_nRenderState = Shutdown;
 
  m_bPendingRenegotiate		= false;
  m_bSignaledStarvation		= false;
  m_bPendingMediaFinished		= false;
  m_bWaitingSample			= false;
  m_StarvationClock			= 0;
  m_nNbDXSurface = 5;
  m_bEvtQuit = 0;
  m_bEvtFlush = 0;
  m_ModeratedClockLast = -1;
  m_ModeratedTimeLast = -1;
  m_pCurrentDisplaydSample  = NULL;
  m_nStepCount        = 0;
  m_pOuterEVR					= NULL;
  m_LastScheduledSampleTime	= -1;
  m_LastScheduledUncorrectedSampleTime = -1;
  m_MaxSampleDuration			= 0;
  m_LastSampleOffset			= 0;
  g_renderManager.PreInit();
  m_fRate = 1.0f;
}

CEVRAllocatorPresenter::~CEVRAllocatorPresenter()
{
  g_renderManager.UnInit();
}

void CEVRAllocatorPresenter::ResetStats()
{
	m_pcFrames			= 0;
	m_nDroppedUpdate	= 0;
	m_pcFramesDrawn		= 0;
	m_piAvg				= 0;
	m_piDev				= 0;
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
    CLog::Log(LOGERROR,"Create enchanced video renderer");
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
  m_nRenderState    = Started;
  SetEvent(m_hEvtPresent);
  CLog::Log(LOGDEBUG,"%s hnsSystemTime = %I64d,   llClockStartOffset = %I64d\n", __FUNCTION__,hnsSystemTime, llClockStartOffset);

  return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockStop(MFTIME hnsSystemTime)
{
  CLog::Log(LOGDEBUG,"%s OnClockStop  hnsSystemTime = %I64d\n", __FUNCTION__, hnsSystemTime);
  m_nRenderState    = Stopped;

  return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockPause(MFTIME hnsSystemTime)
{
  CLog::Log(LOGDEBUG,"%s OnClockPause  hnsSystemTime = %I64d\n", __FUNCTION__, hnsSystemTime);
  m_nRenderState    = Paused;
  return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockRestart(MFTIME hnsSystemTime)
{
  m_nRenderState  = Started;
  SetEvent(m_hEvtPresent);
  CLog::Log(LOGDEBUG,"%s OnClockRestart  hnsSystemTime = %I64d\n", __FUNCTION__, hnsSystemTime);
  return S_OK;
}


STDMETHODIMP CEVRAllocatorPresenter::OnClockSetRate(MFTIME hnsSystemTime, float flRate)
{
  ASSERT (FALSE);
  return E_NOTIMPL;
}

// IBaseFilter delegate
bool CEVRAllocatorPresenter::GetState( DWORD dwMilliSecsTimeout, FILTER_STATE *State, HRESULT &_ReturnValue)
{
  CAutoLock lock(&m_SampleQueueLock);

  if (m_bSignaledStarvation)
  {
    int nSamples = max(m_nNbDXSurface / 2, 1);
    if ((m_ScheduledSamples.GetCount() < nSamples || m_LastSampleOffset < -m_rtTimePerFrame*2))
    {      
      *State = (FILTER_STATE)Paused;
      _ReturnValue = VFW_S_STATE_INTERMEDIATE;
      return true;
    }
    m_bSignaledStarvation = false;
  }
  return false;
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
  else if(riid == __uuidof(IDirect3DDeviceManager9))
//    hr = GetInterface((IDirect3DDeviceManager9*)this, ppv);
    hr = m_pDeviceManager->QueryInterface (__uuidof(IDirect3DDeviceManager9), (void**) ppv);
  /*else
    hr = __super::NonDelegatingQueryInterface(riid, ppv);*/

  return hr;
}

// IUnknown
HRESULT CEVRAllocatorPresenter::QueryInterface( 
        REFIID riid,
        void** ppvObject)
{
HRESULT hr = E_NOINTERFACE;
  //Log( "QueryInterface"  );
  //LogIID( riid );
  if( ppvObject == NULL ) 
  {
    hr = E_POINTER;
  } 
  else if( riid == IID_IMFVideoDeviceID) 
  {
    *ppvObject = static_cast<IMFVideoDeviceID*>( this );
    AddRef();
    hr = S_OK;
  } 
  else if( riid == IID_IMFTopologyServiceLookupClient) 
  {
    *ppvObject = static_cast<IMFTopologyServiceLookupClient*>( this );
    AddRef();
    hr = S_OK;
  }
  else if( riid == IID_IMFVideoPresenter) 
  {
    *ppvObject = static_cast<IMFVideoPresenter*>( this );
    AddRef();
    hr = S_OK;
  } 
  else if( riid == IID_IMFGetService) 
  {
    *ppvObject = static_cast<IMFGetService*>( this );
    AddRef();
    hr = S_OK;
  } 
  else if( riid == IID_IQualProp) 
  {
    *ppvObject = static_cast<IQualProp*>( this );
    AddRef();
    hr = S_OK;
  }
  /*else if( riid == IID_IMFRateSupport) 
  {
    *ppvObject = static_cast<IMFRateSupport*>( this );
    AddRef();
    hr = S_OK;
  }*/
  else if( riid == IID_IMFVideoDisplayControl ) 
  {
    *ppvObject = static_cast<IMFVideoDisplayControl*>( this );
    AddRef();
    //Log( "QueryInterface:IID_IMFVideoDisplayControl:%x",(*ppvObject) );
    hr = S_OK;
  } 
  else if( riid == IID_IEVRTrustedVideoPlugin ) 
  {
    *ppvObject = static_cast<IEVRTrustedVideoPlugin*>( this );
    AddRef();
    //Log( "QueryInterface:IID_IEVRTrustedVideoPlugin:%x",(*ppvObject) );
    hr = S_OK;
  } 
  /*else if( riid == IID_IMFVideoPositionMapper ) 
  {
    *ppvObject = static_cast<IMFVideoPositionMapper*>( this );
    AddRef();
    hr = S_OK;
  } */
  else if( riid == IID_IUnknown ) 
  {
    *ppvObject = static_cast<IUnknown*>( static_cast<IMFVideoDeviceID*>( this ) );
    AddRef();
    hr = S_OK;    
  }
  else
  {
    //LogIID( riid );
    *ppvObject=NULL;
    hr=E_NOINTERFACE;
  }
  if ( FAILED(hr) ) 
    CLog::Log(LOGERROR,"Failed to query %s",DShowUtil::CStringFromGUID(riid).c_str());
  return hr;
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

//IMFVideoPresenter
HRESULT STDMETHODCALLTYPE CEVRAllocatorPresenter::ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam)
{
  HRESULT hr = S_OK;
  switch (eMessage)
  {
  case MFVP_MESSAGE_BEGINSTREAMING :      // The EVR switched from stopped to paused. The presenter should allocate resources
    ResetStats();
    TRACE_EVR ("EVR: MFVP_MESSAGE_BEGINSTREAMING\n");
	SetEvent(m_hEvtPresent);
    break;

  case MFVP_MESSAGE_CANCELSTEP :        // Cancels a frame step
    TRACE_EVR ("EVR: MFVP_MESSAGE_CANCELSTEP\n");
    CompleteFrameStep (true);
    break;

  case MFVP_MESSAGE_ENDOFSTREAM :        // All input streams have ended. 
    TRACE_EVR ("EVR: MFVP_MESSAGE_ENDOFSTREAM\n");
    m_pSink->Notify (EC_COMPLETE, 0, 0);
    break;

  case MFVP_MESSAGE_ENDSTREAMING :      // The EVR switched from running or paused to stopped. The presenter should free resources
    TRACE_EVR ("EVR: MFVP_MESSAGE_ENDSTREAMING\n");
    break;

  case MFVP_MESSAGE_FLUSH :          // The presenter should discard any pending samples
    SetEvent(m_hEvtFlush);
	TRACE_EVR ("MFVP_MESSAGE_FLUSH\n");
	while (WaitForSingleObject(m_hEvtFlush, 1) == WAIT_OBJECT_0);
	SetEvent(m_hEvtPresent);
    break;

  case MFVP_MESSAGE_INVALIDATEMEDIATYPE :    // The mixer's output format has changed. The EVR will initiate format negotiation, as described previously
    /*
      1) The EVR sets the media type on the reference stream.
      2) The EVR calls IMFVideoPresenter::ProcessMessage on the presenter with the MFVP_MESSAGE_INVALIDATEMEDIATYPE message.
      3) The presenter sets the media type on the mixer's output stream.
      4) The EVR sets the media type on the substreams.
    */
    RenegotiateMediaType();
    break;

  case MFVP_MESSAGE_PROCESSINPUTNOTIFY :    // One input stream on the mixer has received a new sample
    GetImageFromMixer();
    break;

  case MFVP_MESSAGE_STEP :          // Requests a frame step.
    TRACE_EVR ("EVR: MFVP_MESSAGE_STEP\n");
    m_nStepCount = ulParam;
    hr = S_OK;
    break;

  default :
    ASSERT (FALSE);
    break;
  }
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
  HRESULT            hr;
  DWORD            dwObjects = 1;

  CLog::Log(LOGDEBUG,"%s getting mixer, render eventsink and renderclock",__FUNCTION__);
  hr = pLookup->LookupService (MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_MIXER_SERVICE,
                               __uuidof (IMFTransform), (void**)&m_pMixer, &dwObjects);

  hr = pLookup->LookupService (MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_RENDER_SERVICE,
                               __uuidof (IMediaEventSink ), (void**)&m_pSink, &dwObjects);

  hr = pLookup->LookupService (MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_RENDER_SERVICE,
                               __uuidof (IMFClock ), (void**)&m_pClock, &dwObjects);


  StartWorkerThreads();
  return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::ReleaseServicePointers()
{
  CLog::Log(LOGDEBUG,"%s releasing mixer, render eventsink and renderclock",__FUNCTION__);
  StopWorkerThreads();
  m_pMixer  = NULL;
  m_pSink    = NULL;
  m_pClock  = NULL;
  return S_OK;
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
  if (guidService == MR_VIDEO_RENDER_SERVICE)
    return NonDelegatingQueryInterface (riid, ppvObject);
  else if (guidService == MR_VIDEO_ACCELERATION_SERVICE)
    return m_pDeviceManager->QueryInterface (__uuidof(IDirect3DDeviceManager9), (void**) ppvObject);

  return E_NOINTERFACE;
}


void CEVRAllocatorPresenter::StartWorkerThreads()
{
  DWORD    dwThreadId;

  if (m_nRenderState == Shutdown)
  {
    m_hEvtQuit		= CreateEvent (NULL, TRUE, FALSE, NULL);
	m_hEvtPresent	= CreateEvent (NULL, FALSE, FALSE, NULL);
	m_hEvtFrameTimer= CreateEvent (NULL, FALSE, FALSE, NULL);
	m_hEvtFlush		= CreateEvent (NULL, TRUE, FALSE, NULL);
	m_hSemPicture	= CreateSemaphore(NULL, 0, m_nNbDXSurface, NULL);

	m_hThread		= ::CreateThread(NULL, 0, PresentThread, (LPVOID)this, 0, &dwThreadId);

	m_nRenderState		= Stopped;
		TRACE_EVR ("Worker threads started...\n");
  }
}

void CEVRAllocatorPresenter::StopWorkerThreads()
{
  if (m_nRenderState != Shutdown)
	{
		SetEvent (m_hEvtFlush);
		SetEvent (m_hEvtQuit);
		if ((m_hThread != INVALID_HANDLE_VALUE) && (WaitForSingleObject (m_hThread, 10000) == WAIT_TIMEOUT))
		{
			ASSERT (FALSE);
			TerminateThread (m_hThread, 0xDEAD);
		}

		if (m_hThread		 != INVALID_HANDLE_VALUE) CloseHandle (m_hThread);
		if (m_hSemPicture	 != INVALID_HANDLE_VALUE) CloseHandle (m_hSemPicture);
		if (m_hEvtPresent	 != INVALID_HANDLE_VALUE) CloseHandle (m_hEvtPresent);
		if (m_hEvtFrameTimer != INVALID_HANDLE_VALUE) CloseHandle (m_hEvtFrameTimer);
		if (m_hEvtFlush		 != INVALID_HANDLE_VALUE) CloseHandle (m_hEvtFlush);
		if (m_hEvtQuit		 != INVALID_HANDLE_VALUE) CloseHandle (m_hEvtQuit);

		TRACE_EVR ("Worker threads stopped...\n");
	}
	m_nRenderState = Shutdown;
}

void CEVRAllocatorPresenter::RemoveAllSamples()
{
	FlushSamples();
	m_ScheduledSamples.RemoveAll();
	m_FreeSamples.RemoveAll();
}

HRESULT CEVRAllocatorPresenter::GetFreeSample(IMFSample** ppSample)
{
CAutoLock lock(this);
	HRESULT		hr = S_OK;

	if (m_FreeSamples.GetCount() > 1)	// <= Cannot use first free buffer (can be currently displayed)
		*ppSample = m_FreeSamples.RemoveHead().Detach();
	else
		hr = MF_E_SAMPLEALLOCATOR_EMPTY;

	return hr;}

void CEVRAllocatorPresenter::FlushSamples()
{
  CAutoLock				lock(this);

	m_nUsedBuffer = 0;
	while (m_ScheduledSamples.GetCount() > 0)
	{
		CComPtr<IMFSample>		pMFSample;

		pMFSample = m_ScheduledSamples.RemoveHead();
		MoveToFreeList (pMFSample, true);

		WaitForSingleObject (m_hSemPicture, 0);
	}
}

HRESULT CEVRAllocatorPresenter::GetScheduledSample(IMFSample** ppSample)
{
	CAutoLock lock(this);
	HRESULT		hr = S_OK;

	if (m_ScheduledSamples.GetCount() > 0)
		*ppSample = m_ScheduledSamples.RemoveHead().Detach();
	else
		hr = MF_E_SAMPLEALLOCATOR_EMPTY;

	return hr;
}

void CEVRAllocatorPresenter::MoveToFreeList(IMFSample* pSample, bool bTail)
{
  CAutoLock lock(&m_SampleQueueLock);
  InterlockedDecrement (&m_nUsedBuffer);
  if (m_bPendingMediaFinished && m_nUsedBuffer == 0)
  {
    m_bPendingMediaFinished = false;
    m_pSink->Notify (EC_COMPLETE, 0, 0);
  }
  if (bTail)
    m_FreeSamples.AddTail(pSample);
  else
    m_FreeSamples.AddHead(pSample);
}


void CEVRAllocatorPresenter::MoveToScheduledList(IMFSample* pSample)
{
	CAutoLock lock(this);
	m_ScheduledSamples.AddTail (pSample);
}

HRESULT CEVRAllocatorPresenter::GetImageFromMixer()
{
  MFT_OUTPUT_DATA_BUFFER		Buffer;
	HRESULT						hr = S_OK;
	DWORD						dwStatus;
	REFERENCE_TIME				nsSampleTime;
	MFTIME						nsCurrentTime;
	LONGLONG					llClockBefore = 0;
	LONGLONG					llClockAfter  = 0;
	LONGLONG					llMixerLatency;
	UINT						dwSurface;

	while (SUCCEEDED(hr))
	{
		CComPtr<IMFSample>		pSample;

		if (FAILED (GetFreeSample (&pSample)))
		{
			m_bWaitingSample = true;
			break;
		}

		memset (&Buffer, 0, sizeof(Buffer));
		Buffer.pSample	= pSample;
		pSample->GetUINT32 (GUID_SURFACE_INDEX, &dwSurface);

		if (m_pClock) m_pClock->GetCorrelatedTime(0, &llClockBefore, &nsCurrentTime);			
		hr = m_pMixer->ProcessOutput (0 , 1, &Buffer, &dwStatus);
		if (m_pClock) m_pClock->GetCorrelatedTime(0, &llClockAfter, &nsCurrentTime);

		if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) 
		{
			MoveToFreeList (pSample, false);
			break;
		}

		if (m_pSink) 
		{
			CAutoLock autolock(this);
			llMixerLatency = llClockAfter - llClockBefore;
			m_pSink->Notify (EC_PROCESSING_LATENCY, (LONG_PTR)&llMixerLatency, 0);
		}

		pSample->GetSampleTime (&nsSampleTime);

		//CLog::Log(LOGDEBUG,"%s Get sample from mixer at time %I64d)", __FUNCTION__ ,nsSampleTime);

		MoveToScheduledList (pSample);
		ReleaseSemaphore (m_hSemPicture, 1, NULL);
		InterlockedIncrement (&m_nUsedBuffer);
	}

	return hr;
}


DWORD WINAPI CEVRAllocatorPresenter::PresentThread(LPVOID lpParam)
{
  CEVRAllocatorPresenter*    pThis = (CEVRAllocatorPresenter*) lpParam;
  pThis->RenderThread();
  return 0;
}

void CEVRAllocatorPresenter::CheckWaitingSampleFromMixer()
{
  if (m_bWaitingSample)
	{
		m_bWaitingSample = false;
		GetImageFromMixer();
	}
}

void CEVRAllocatorPresenter::CompleteFrameStep(bool bCancel)
{
	if (m_nStepCount > 0)
	{
		if (bCancel || (m_nStepCount == 1)) 
		{
			m_pSink->Notify(EC_STEP_COMPLETE, bCancel ? TRUE : FALSE, 0);
			m_nStepCount = 0;
		}
		else
			m_nStepCount--;
	}
}

void CEVRAllocatorPresenter::RenderThread()
{
  	HANDLE				hAvrt;
	DWORD				dwTaskIndex	= 0;
	HANDLE				hEvts[]		= { m_hEvtQuit, m_hEvtFlush, m_hEvtPresent, m_hEvtFrameTimer};
	HANDLE				hEvtsBuff[]	= { m_hEvtQuit, m_hEvtFlush, m_hSemPicture };
	bool				bQuit		= false;
    TIMECAPS			tc;
	DWORD				dwResolution;
	MFTIME				nsSampleTime;
	MFTIME				nsCurrentTime;
	LONGLONG			llClockTime;
	long				lDelay;
	DWORD				dwUser = 0;
	DWORD				dwObject;

	UINT				nNbPlayingFrame	= 0;

	// Tell Vista Multimedia Class Scheduler we are a playback thretad (increase priority)
	if (pfAvSetMmThreadCharacteristicsW)	hAvrt = pfAvSetMmThreadCharacteristicsW (L"Playback", &dwTaskIndex);
	if (pfAvSetMmThreadPriority)			pfAvSetMmThreadPriority (hAvrt, AVRT_PRIORITY_HIGH /*AVRT_PRIORITY_CRITICAL*/);

    timeGetDevCaps(&tc, sizeof(TIMECAPS));
    dwResolution = min(max(tc.wPeriodMin, 0), tc.wPeriodMax);
    dwUser		= timeBeginPeriod(dwResolution);

	
	while (!bQuit)
	{
		dwObject = WaitForMultipleObjects (countof(hEvts), hEvts, FALSE, INFINITE);
		switch (dwObject)
		{
		case WAIT_OBJECT_0 :
			bQuit = true;
			break;
		case WAIT_OBJECT_0 + 1 :
			// Flush pending samples!
			if (dwUser != -1) timeKillEvent (dwUser);
			dwUser = -1;		

			FlushSamples();
			ResetEvent(m_hEvtFlush);
			TRACE_EVR ("Flush done!\n");
			break;

		case WAIT_OBJECT_0 + 2 :
		case WAIT_OBJECT_0 + 3 :
			// Discard timer events if playback stop
			if ((dwObject == WAIT_OBJECT_0 + 3) && (m_nRenderState != Started)) continue;

//			TRACE_EVR ("RenderThread ==>> Waiting buffer\n");
			
			if (WaitForMultipleObjects (countof(hEvtsBuff), hEvtsBuff, FALSE, INFINITE) == WAIT_OBJECT_0+2)
			{
				CComPtr<IMFSample>		pMFSample;
				if (SUCCEEDED (GetScheduledSample(&pMFSample)))
				{
					pMFSample->GetUINT32 (GUID_SURFACE_INDEX, (UINT32*)&m_nCurSurface);
					pMFSample->GetSampleTime (&nsSampleTime);

					//TRACE_EVR ("RenderThread ==>> Presenting surface %d  (%I64d)\n", m_nCurSurface, nsSampleTime);

					//__super::SetTime (g_tSegmentStart + nsSampleTime);
					//Paint(true);
					RenderPresent(m_nCurSurface);

					m_pcFramesDrawn++;
					InterlockedDecrement (&m_nUsedBuffer);
					CompleteFrameStep (false);

					if ((m_nRenderState == Started) && (m_nStepCount == 0))
					{
						// Calculate wake up timer
						m_pClock->GetCorrelatedTime(0, &llClockTime, &nsCurrentTime);			

						//if (m_rtTimePerFrame == 0) CalculateFrameRate(/*nsSampleTime*/);
						// Wakup 1/2 refresh rate before next VSync!
						lDelay = (UINT)((nsSampleTime + m_rtTimePerFrame - llClockTime) / 10000) - (500/m_RefreshRate);

						if (lDelay > 0)
						{
	//						TRACE_EVR ("RenderThread ==>> Set timer %d   %I64d  Cons=%d \n", lDelay, nsSampleTime, m_nCurSurface);
							dwUser			= timeSetEvent (lDelay, dwResolution, (LPTIMECALLBACK)m_hEvtFrameTimer, NULL, TIME_CALLBACK_EVENT_SET); 

							// Update statistics
							nNbPlayingFrame++;
						}
						else
						{
							dwUser = -1;
							if (m_nRenderState == Started) m_pcFrames++;
	//						TRACE_EVR ("RenderThread ==>> immediate display   %I64d  (delay=%d)  Cons=%d\n", nsSampleTime/417188, lDelay, m_nCurSurface);
							SetEvent (m_hEvtPresent);
						}
					}

					MoveToFreeList(pMFSample, true);
					CheckWaitingSampleFromMixer();
				}
			}
			else
			{				
				TRACE_EVR ("RenderThread ==>> Flush before rendering frame!\n");
			}

			break;
		}
	}

	timeEndPeriod (dwResolution);
	if (pfAvRevertMmThreadCharacteristics) pfAvRevertMmThreadCharacteristics (hAvrt);
}

HRESULT CEVRAllocatorPresenter::RenderPresent(int surfaceIndex)
{
  CAutoLock cRenderLock(&m_RenderLock);
  HRESULT hr=S_OK;
  //CLog::Log(LOGNOTICE,"%s Rendering frame", __FUNCTION__);
  if (!g_renderManager.IsConfigured())
    g_renderManager.Configure(m_iVideoWidth, m_iVideoHeight, m_iVideoWidth, m_iVideoHeight, m_fps, CONF_FLAGS_USE_DIRECTSHOW |CONF_FLAGS_FULLSCREEN);
  hr = m_D3DDev->StretchRect(m_pInternalVideoSurface[surfaceIndex], NULL, m_pVideoSurface, NULL, D3DTEXF_NONE);
  if (SUCCEEDED(hr))
  {
    g_renderManager.PaintVideoTexture(m_pVideoTexture,m_pInternalVideoSurface[surfaceIndex]);
    g_application.NewFrame();
    //Give .1 sec to the gui to render
    g_application.WaitFrame(100);
  }
  else
  {
    CLog::Log(LOGERROR,"%s error rendering the surface", __FUNCTION__);
  }
  return hr;
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

// IDirect3DDeviceManager9
STDMETHODIMP CEVRAllocatorPresenter::ResetDevice(IDirect3DDevice9 *pDevice,UINT resetToken)
{
  HRESULT    hr = m_pDeviceManager->ResetDevice (pDevice, resetToken);
  return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::OpenDeviceHandle(HANDLE *phDevice)
{
  HRESULT    hr = m_pDeviceManager->OpenDeviceHandle (phDevice);
  return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::CloseDeviceHandle(HANDLE hDevice)
{
  HRESULT    hr = m_pDeviceManager->CloseDeviceHandle(hDevice);
  return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::TestDevice(HANDLE hDevice)
{
  HRESULT    hr = m_pDeviceManager->TestDevice(hDevice);
  return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::LockDevice(HANDLE hDevice, IDirect3DDevice9 **ppDevice, BOOL fBlock)
{
  HRESULT    hr = m_pDeviceManager->LockDevice(hDevice, ppDevice, fBlock);
  return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::UnlockDevice(HANDLE hDevice, BOOL fSaveState)
{
  HRESULT    hr = m_pDeviceManager->UnlockDevice(hDevice, fSaveState);
  return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::GetVideoService(HANDLE hDevice, REFIID riid, void **ppService)
{
  HRESULT    hr = m_pDeviceManager->GetVideoService(hDevice, riid, ppService);

  if (riid == __uuidof(IDirectXVideoDecoderService))
  {
    UINT    nNbDecoder = 5;
    GUID*    pDecoderGuid;
    IDirectXVideoDecoderService*    pDXVAVideoDecoder = (IDirectXVideoDecoderService*) *ppService;
    pDXVAVideoDecoder->GetDecoderDeviceGuids (&nNbDecoder, &pDecoderGuid);
  }
  else if (riid == __uuidof(IDirectXVideoProcessorService))
  {
    IDirectXVideoProcessorService*    pDXVAProcessor = (IDirectXVideoProcessorService*) *ppService;
  }

  return hr;
}

void CEVRAllocatorPresenter::OnResetDevice()
{
  HRESULT    hr;

  // Reset DXVA Manager, and get new buffers
  hr = m_pDeviceManager->ResetDevice(m_D3DDev, m_iResetToken);

  // Not necessary, but Microsoft documentation say Presenter should send this message...
  if (m_pSink)
    m_pSink->Notify (EC_DISPLAY_CHANGED, 0, 0);
}

bool CEVRAllocatorPresenter::ResetD3dDevice()
{
  //StopWorkerThreads();
  DeleteSurfaces();
  HRESULT hr;
  
  if(FAILED(hr = AllocSurfaces()))
  {
    CLog::Log(LOGERROR,"%s Failed to alloc surface",__FUNCTION__);
    return false;
  }
  OnResetDevice();
  return true;
}
void CEVRAllocatorPresenter::DeleteSurfaces()
{
  CAutoLock cAutoLock(this);
  CAutoLock cRenderLock(&m_RenderLock);

  CLog::Log(LOGDEBUG,"%s Deletings surfaces",__FUNCTION__);
  for(int i = 0; i < m_nNbDXSurface+2; i++)
  {
    m_pInternalVideoTexture[i] = NULL;
    m_pInternalVideoSurface[i] = NULL;
  }
}

HRESULT CEVRAllocatorPresenter::AllocSurfaces(D3DFORMAT Format)
{
  CAutoLock cAutoLock(this);
  CAutoLock cRenderLock(&m_RenderLock);
  HRESULT hr = S_OK;

  int nTexturesNeeded = 5;
  //default format is D3DFMT_X8R8G8B8
  for ( int i=0 ; i<m_nNbDXSurface + 2 ; i++ )
  {
    m_pInternalVideoTexture[i] = NULL;
    m_pInternalVideoSurface[i] = NULL;
  }
  m_pVideoTexture = NULL;
  m_pVideoSurface = NULL;
  m_SurfaceType = Format;


  for(int i = 0; i < nTexturesNeeded; i++)
  {
    if(FAILED(hr = m_D3DDev->CreateTexture(m_iVideoWidth, 
                                           m_iVideoHeight, 
                                           1,
                                           D3DUSAGE_RENDERTARGET, 
                                           Format/*D3DFMT_X8R8G8B8 D3DFMT_A8R8G8B8*/,
                                           D3DPOOL_DEFAULT, 
                                           &m_pInternalVideoTexture[i], 
                                           NULL)))
				return hr;

			if(FAILED(hr = m_pInternalVideoTexture[i]->GetSurfaceLevel(0, &m_pInternalVideoSurface[i])))
				return hr;
  }

  hr = m_D3DDev->ColorFill(m_pInternalVideoSurface[m_nCurSurface], NULL, 0);
  hr = m_D3DDev->CreateTexture(m_iVideoWidth, 
                               m_iVideoHeight, 
                               1, 
                               D3DUSAGE_RENDERTARGET,
                               Format,//D3DFMT_A8R8G8B8, 
                               D3DPOOL_DEFAULT, 
                               &m_pVideoTexture, 
                               NULL);

  hr = m_pVideoTexture->GetSurfaceLevel(0, &m_pVideoSurface);
  hr = m_D3DDev->ColorFill(m_pVideoSurface, NULL, 0);

  return hr;
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

HRESULT CEVRAllocatorPresenter::RenegotiateMediaType()
{
    HRESULT      hr = S_OK;

    CComPtr<IMFMediaType>  pMixerType;
    CComPtr<IMFMediaType>  pType;

    if (!m_pMixer)
    {
        return MF_E_INVALIDREQUEST;
    }

  CInterfaceArray<IMFMediaType> ValidMixerTypes;

    // Loop through all of the mixer's proposed output types.
    DWORD iTypeIndex = 0;
    while ((hr != MF_E_NO_MORE_TYPES))
    {
        pMixerType   = NULL;
        pType     = NULL;
    m_pMediaType = NULL;

        // Step 1. Get the next media type supported by mixer.
        hr = m_pMixer->GetOutputAvailableType(0, iTypeIndex++, &pMixerType);
        if (FAILED(hr))
        {
            break;
        }

        // Step 2. Check if we support this media type.
        if (SUCCEEDED(hr))
            hr = IsMediaTypeSupported(pMixerType);

        if (SUCCEEDED(hr))
          hr = CreateProposedOutputType(pMixerType, &pType);
  
        // Step 4. Check if the mixer will accept this media type.
        if (SUCCEEDED(hr))
            hr = m_pMixer->SetOutputType(0, pType, MFT_SET_TYPE_TEST_ONLY);

        if (SUCCEEDED(hr))
    {
      LONGLONG Merit = GetMediaTypeMerit(pType);

      int nTypes = ValidMixerTypes.GetCount();
      int iInsertPos = 0;
      for (int i = 0; i < nTypes; ++i)
      {
        LONGLONG ThisMerit = GetMediaTypeMerit(ValidMixerTypes[i]);
        if (Merit > ThisMerit)
        {
          iInsertPos = i;
          break;
        }
        else
          iInsertPos = i+1;
      }

      ValidMixerTypes.InsertAt(iInsertPos, pType);
    }
    }


  int nValidTypes = ValidMixerTypes.GetCount();
  for (int i = 0; i < nValidTypes; ++i)
  {
        // Step 3. Adjust the mixer's type to match our requirements.
    pType = ValidMixerTypes[i];
	CLog::Log(LOGDEBUG,"%s Valid mixer output type: %ws" , __FUNCTION__ , GetMediaTypeFormatDesc(pType));
  }

  for (int i = 0; i < nValidTypes; ++i)
  {
        // Step 3. Adjust the mixer's type to match our requirements.
    pType = ValidMixerTypes[i];

	CLog::Log(LOGDEBUG,"%s Trying mixer output type: %ws",__FUNCTION__ ,GetMediaTypeFormatDesc(pType));
        // Step 5. Try to set the media type on ourselves.
    hr = SetMediaType(pType);
	
    // Step 6. Set output media type on mixer.
    if (SUCCEEDED(hr))
    {
      hr = m_pMixer->SetOutputType(0, pType, 0);

            // If something went wrong, clear the media type.
            if (FAILED(hr))
            {
                SetMediaType(NULL);
            }
      else
        break;
        }
  }

    pMixerType  = NULL;
    pType    = NULL;
    return hr;
}

HRESULT CEVRAllocatorPresenter::SetMediaType(IMFMediaType* pType)
{
  HRESULT hr;
  AM_MEDIA_TYPE* pAMMedia = NULL;
  CStdString strTemp;

  CheckPointer (pType, E_POINTER);
  hr = pType->GetRepresentation(FORMAT_VideoInfo2, (void**)&pAMMedia);
  if (FAILED(hr))
      return hr;
  
  hr = InitializeDevice (pAMMedia);
  if (SUCCEEDED (hr))
  {
    strTemp = DShowUtil::GetMediaTypeName (pAMMedia->subtype);
    strTemp.Replace ("MEDIASUBTYPE_", "");
    CLog::Log(LOGDEBUG,"Mixer output : %s", strTemp.c_str());
  }

  pType->FreeRepresentation (FORMAT_VideoInfo2, (void*)pAMMedia);

  return hr;
}

HRESULT CEVRAllocatorPresenter::CreateProposedOutputType(IMFMediaType* pMixerType, IMFMediaType** pType)
{
  HRESULT				hr;
  AM_MEDIA_TYPE*		pAMMedia = NULL;
  LARGE_INTEGER		i64Size;
  MFVIDEOFORMAT*		VideoFormat;
  hr = pMixerType->GetRepresentation  (FORMAT_MFVideoFormat, (void**)&pAMMedia);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR,"%s Failed creating output type",__FUNCTION__);
    return hr;
  }
	VideoFormat = (MFVIDEOFORMAT*)pAMMedia->pbFormat;
	hr = pfMFCreateVideoMediaType  (VideoFormat, &m_pMediaType);

	if (VideoFormat->videoInfo.FramesPerSecond.Numerator != 0)
		m_rtTimePerFrame = (20000000I64*VideoFormat->videoInfo.FramesPerSecond.Denominator)/VideoFormat->videoInfo.FramesPerSecond.Numerator;

	m_iVideoAspectWidth = VideoFormat->videoInfo.PixelAspectRatio.Numerator;
	m_iVideoAspectHeight = VideoFormat->videoInfo.PixelAspectRatio.Denominator;

	if (SUCCEEDED (hr))
	{
		i64Size.HighPart = VideoFormat->videoInfo.dwWidth;
		i64Size.LowPart	 = VideoFormat->videoInfo.dwHeight;
		m_pMediaType->SetUINT64 (MF_MT_FRAME_SIZE, i64Size.QuadPart);

		m_pMediaType->SetUINT32 (MF_MT_PAN_SCAN_ENABLED, 0);

		i64Size.HighPart = m_iVideoAspectWidth;
		i64Size.LowPart  = m_iVideoAspectHeight;
		m_pMediaType->SetUINT64 (MF_MT_PIXEL_ASPECT_RATIO, i64Size.QuadPart);

		MFVideoArea Area = MakeArea (0, 0, VideoFormat->videoInfo.dwWidth, VideoFormat->videoInfo.dwHeight);
		m_pMediaType->SetBlob(MF_MT_GEOMETRIC_APERTURE, (UINT8*)&Area, sizeof(MFVideoArea));
	}

	pMixerType->FreeRepresentation (FORMAT_MFVideoFormat, (void*)pAMMedia);
	m_pMediaType->QueryInterface (__uuidof(IMFMediaType), (void**) pType);
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


STDMETHODIMP CEVRAllocatorPresenter::InitializeDevice(AM_MEDIA_TYPE*  pMediaType)
{
  HRESULT      hr;
  
  DeleteSurfaces();

  VIDEOINFOHEADER2*    vih2 = (VIDEOINFOHEADER2*) pMediaType->pbFormat;

  m_iVideoWidth = vih2->bmiHeader.biWidth;
  m_iVideoHeight = abs(vih2->bmiHeader.biHeight);
  
  //Allocsurfaces will alloc the samples to the surface
  hr = AllocSurfaces(D3DFMT_X8R8G8B8);
  CAutoLock lock(this);
  RemoveAllSamples();
  for(int i = 0; i < m_nNbDXSurface; i++)
  {
    CComPtr<IMFSample>		pMFSample;
    hr = pfMFCreateVideoSampleFromSurface (m_pInternalVideoSurface[i], &pMFSample);

    if (SUCCEEDED (hr))
    {
      pMFSample->SetUINT32 (GUID_SURFACE_INDEX, i);
      m_FreeSamples.AddTail (pMFSample);
    }
    ASSERT (SUCCEEDED (hr));
  }
  return hr;
}


HRESULT CEVRAllocatorPresenter::TRACE_EVR(const char *strTrace)
{
  CLog::Log(LOGNOTICE,"%s",strTrace);
  return S_OK;
}