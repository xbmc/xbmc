
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
  m_hThread     = INVALID_HANDLE_VALUE;
  m_hGetMixerThread= INVALID_HANDLE_VALUE;
  m_hEvtFlush     = INVALID_HANDLE_VALUE;
  m_hEvtQuit     = INVALID_HANDLE_VALUE;
  m_nRenderState = Shutdown;
  m_bNeedPendingResetDevice = true;
  m_bPendingRenegotiate		= false;
  m_nNbDXSurface = 5;
  m_pFreeSamples = 0;
  m_bEvtQuit = 0;
  m_bEvtFlush = 0;
  m_ModeratedClockLast = -1;
  m_ModeratedTimeLast = -1;
  m_pCurrentDisplaydSample  = NULL;
  m_nStepCount        = 0;
  g_renderManager.PreInit();
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
  CStdString strTEvr;
  strTEvr.AppendFormat("EVR: OnClockStart  hnsSystemTime = %I64d,   llClockStartOffset = %I64d\n", hnsSystemTime, llClockStartOffset);
  TRACE_EVR (strTEvr.c_str());
  m_ModeratedTimeLast = -1;
  m_ModeratedClockLast = -1;

  return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockStop(MFTIME hnsSystemTime)
{
  CStdString strTEvr;
  strTEvr.AppendFormat("EVR: OnClockStop  hnsSystemTime = %I64d\n", hnsSystemTime);
  TRACE_EVR (strTEvr.c_str());
  m_nRenderState    = Stopped;

  m_ModeratedClockLast = -1;
  m_ModeratedTimeLast = -1;
  return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockPause(MFTIME hnsSystemTime)
{
  CStdString strTEvr;
  strTEvr.AppendFormat("EVR: OnClockPause  hnsSystemTime = %I64d\n", hnsSystemTime);
  TRACE_EVR (strTEvr.c_str());
  if (!m_bSignaledStarvation)
    m_nRenderState    = Paused;
  m_ModeratedTimeLast = -1;
  m_ModeratedClockLast = -1;
  return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockRestart(MFTIME hnsSystemTime)
{
  m_nRenderState  = Started;

  m_ModeratedTimeLast = -1;
  m_ModeratedClockLast = -1;

  CStdString strTEvr;
  strTEvr.AppendFormat("EVR: OnClockRestart  hnsSystemTime = %I64d\n", hnsSystemTime);
  TRACE_EVR (strTEvr.c_str());

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
    break;

  case MFVP_MESSAGE_CANCELSTEP :        // Cancels a frame step
    TRACE_EVR ("EVR: MFVP_MESSAGE_CANCELSTEP\n");
    CompleteFrameStep (true);
    break;

  case MFVP_MESSAGE_ENDOFSTREAM :        // All input streams have ended. 
    TRACE_EVR ("EVR: MFVP_MESSAGE_ENDOFSTREAM\n");
    m_bPendingMediaFinished = true;
    break;

  case MFVP_MESSAGE_ENDSTREAMING :      // The EVR switched from running or paused to stopped. The presenter should free resources
    TRACE_EVR ("EVR: MFVP_MESSAGE_ENDSTREAMING\n");
    break;

  case MFVP_MESSAGE_FLUSH :          // The presenter should discard any pending samples
    SetEvent(m_hEvtFlush);
    m_bEvtFlush = true;
    TRACE_EVR ("EVR: MFVP_MESSAGE_FLUSH\n");
    while (WaitForSingleObject(m_hEvtFlush, 1) == WAIT_OBJECT_0);
    break;

  case MFVP_MESSAGE_INVALIDATEMEDIATYPE :    // The mixer's output format has changed. The EVR will initiate format negotiation, as described previously
    /*
      1) The EVR sets the media type on the reference stream.
      2) The EVR calls IMFVideoPresenter::ProcessMessage on the presenter with the MFVP_MESSAGE_INVALIDATEMEDIATYPE message.
      3) The presenter sets the media type on the mixer's output stream.
      4) The EVR sets the media type on the substreams.
    */
    m_bPendingRenegotiate = true;
    while (*((volatile bool *)&m_bPendingRenegotiate))
      Sleep(1);
    break;

  case MFVP_MESSAGE_PROCESSINPUTNOTIFY :    // One input stream on the mixer has received a new sample
//    GetImageFromMixer();
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

  TRACE_EVR ("EVR: CEVRAllocatorPresenter::InitServicePointers\n");
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
  TRACE_EVR ("EVR: CEVRAllocatorPresenter::ReleaseServicePointers\n");
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
    m_hEvtQuit    = CreateEvent (NULL, TRUE, FALSE, NULL);
    m_hEvtFlush    = CreateEvent (NULL, TRUE, FALSE, NULL);

    m_hThread    = ::CreateThread(NULL, 0, PresentThread, (LPVOID)this, 0, &dwThreadId);
    SetThreadPriority(m_hThread, THREAD_PRIORITY_TIME_CRITICAL);
    m_hGetMixerThread = ::CreateThread(NULL, 0, GetMixerThreadStatic, (LPVOID)this, 0, &dwThreadId);
    SetThreadPriority(m_hGetMixerThread, THREAD_PRIORITY_HIGHEST);

    m_nRenderState    = Stopped;
    TRACE_EVR ("EVR: Worker threads started...\n");
  }
}

void CEVRAllocatorPresenter::StopWorkerThreads()
{
  if (m_nRenderState != Shutdown)
  {
    SetEvent (m_hEvtFlush);
    m_bEvtFlush = true;
    SetEvent (m_hEvtQuit);
    m_bEvtQuit = true;
    if ((m_hThread != INVALID_HANDLE_VALUE) && (WaitForSingleObject (m_hThread, 10000) == WAIT_TIMEOUT))
    {
      ASSERT (FALSE);
      TerminateThread (m_hThread, 0xDEAD);
    }
    if ((m_hGetMixerThread != INVALID_HANDLE_VALUE) && (WaitForSingleObject (m_hGetMixerThread, 10000) == WAIT_TIMEOUT))
    {
      ASSERT (FALSE);
      TerminateThread (m_hGetMixerThread, 0xDEAD);
    }

    if (m_hThread     != INVALID_HANDLE_VALUE) CloseHandle (m_hThread);
    if (m_hGetMixerThread     != INVALID_HANDLE_VALUE) CloseHandle (m_hGetMixerThread);
    if (m_hEvtFlush     != INVALID_HANDLE_VALUE) CloseHandle (m_hEvtFlush);
    if (m_hEvtQuit     != INVALID_HANDLE_VALUE) CloseHandle (m_hEvtQuit);

    m_bEvtFlush = false;
    m_bEvtQuit = false;


    TRACE_EVR ("EVR: Worker threads stopped...\n");
  }
  m_nRenderState = Shutdown;
}

void CEVRAllocatorPresenter::RemoveAllSamples()
{
  CAutoLock AutoLock(&m_ImageProcessingLock);

  FlushSamples();
  m_ScheduledSamples.RemoveAll();
  m_FreeSamples.RemoveAll();
  m_LastScheduledSampleTime = -1;
  m_LastScheduledUncorrectedSampleTime = -1;
  ASSERT(m_nUsedBuffer == 0);
  m_nUsedBuffer = 0;
}

HRESULT CEVRAllocatorPresenter::GetFreeSample(IMFSample** ppSample)
{
  CAutoLock lock(&m_SampleQueueLock);
  HRESULT    hr = S_OK;
  // Cannot use first free buffer (can be currently displayed)
  if (m_pFreeSamples ==0 )
    return E_FAIL;
  m_pFreeSamples--;
  *ppSample = m_pInternalFreeSamples[m_pFreeSamples];
  m_pInternalFreeSamples[m_pFreeSamples] = NULL;

  return hr;
}

void CEVRAllocatorPresenter::FlushSamples()
{
  CAutoLock        lock(this);
  CAutoLock        lock2(&m_SampleQueueLock);
  
  FlushSamplesInternal();
  m_LastScheduledSampleTime = -1;
}

void CEVRAllocatorPresenter::FlushSamplesInternal()
{
  while (m_ScheduledSamples.GetCount() > 0)
  {
    CComPtr<IMFSample>    pMFSample;

    pMFSample = m_ScheduledSamples.RemoveHead();
    MoveToFreeList (pMFSample, true);
  }

  m_LastSampleOffset      = 0;
  m_bLastSampleOffsetValid  = false;
  m_bSignaledStarvation = false;
}

HRESULT CEVRAllocatorPresenter::GetScheduledSample(IMFSample** ppSample, int &_Count)
{
  CAutoLock lock(&m_SampleQueueLock);
  HRESULT    hr = S_OK;

  _Count = m_ScheduledSamples.GetCount();
  if (_Count > 0)
  {
    *ppSample = m_ScheduledSamples.RemoveHead().Detach();
    --_Count;
  }
  else
    hr = MF_E_SAMPLEALLOCATOR_EMPTY;

  return hr;
}

void CEVRAllocatorPresenter::MoveToFreeList(IMFSample* pSample, bool bTail)
{
  CAutoLock lock(&m_SampleQueueLock);
  m_pInternalFreeSamples[m_pFreeSamples++] = pSample;
  
  if ( m_ScheduledSamples.GetCount() == 0 ) 
  {
    //LOG_TRACE("No scheduled samples, queue was empty -> todo, CheckForEndOfStream()");
    if (m_bPendingMediaFinished && m_nUsedBuffer == 0)
	{
      m_bPendingMediaFinished = false;
      m_pSink->Notify (EC_COMPLETE, 0, 0);
	}
  }
  /*if (bTail)
    m_FreeSamples.AddTail (pSample);
  else
    m_FreeSamples.AddHead(pSample);*/
}

void CEVRAllocatorPresenter::MoveToScheduledList(IMFSample* pSample, bool _bSorted)
{

  if (_bSorted)
  {
    CAutoLock lock(&m_SampleQueueLock);
    m_ScheduledSamples.AddHead(pSample);
  }
  else
  {
//to do
    CAutoLock lock(&m_SampleQueueLock);
    m_ScheduledSamples.AddTail(pSample);
  }
}


bool CEVRAllocatorPresenter::GetImageFromMixer()
{
  MFT_OUTPUT_DATA_BUFFER    Buffer;
  HRESULT            hr = S_OK;
  DWORD            dwStatus;
  REFERENCE_TIME        nsSampleTime;
  LONGLONG          llClockBefore = 0;
  LONGLONG          llClockAfter  = 0;
  LONGLONG          llMixerLatency;
  UINT            dwSurface;

  bool bDoneSomething = false;

  while (SUCCEEDED(hr))
  {
    CComPtr<IMFSample>    pSample;

    if (FAILED (GetFreeSample (&pSample)))
    {
      CLog::Log(LOGDEBUG,"%s No free sample",__FUNCTION__);
      m_bWaitingSample = true;
      break;
    }

    if (!m_pMixer)
      break;

    memset (&Buffer, 0, sizeof(Buffer));
    Buffer.pSample  = pSample;

	//pSample->GetUINT32 (GUID_SURFACE_INDEX, &dwSurface);
	//{
      hr = m_pMixer->ProcessOutput (0 , 1, &Buffer, &dwStatus);
	//}

    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) 
    {
      //Huge log spamming with this crap
      //CLog::Log(LOGDEBUG,"%s MF_E_TRANSFORM_NEED_MORE_INPUT",__FUNCTION__);
      MoveToFreeList (pSample, false);
      break;
    }

    if (m_pSink) 
    {
      //CAutoLock autolock(this); We shouldn't need to lock here, m_pSink is thread safe
      llMixerLatency = llClockAfter - llClockBefore;
      m_pSink->Notify (EC_PROCESSING_LATENCY, (LONG_PTR)&llMixerLatency, 0);
    }

    pSample->GetSampleTime (&nsSampleTime);
    REFERENCE_TIME        nsDuration;
    pSample->GetSampleDuration (&nsDuration);
    CLog::Log(LOGDEBUG,"%s Get sample from mixer at time %I64d)", nsSampleTime);

    MoveToScheduledList (pSample, false);
    bDoneSomething = true;
    if (m_rtTimePerFrame == 0)
      break;
  }

  return bDoneSomething;
}

void CEVRAllocatorPresenter::GetMixerThread()
{
  HANDLE        hAvrt;
  HANDLE        hEvts[]    = { m_hEvtQuit};
  bool        bQuit    = false;
    TIMECAPS      tc;
  DWORD        dwResolution;
  DWORD        dwUser = 0;
  DWORD        dwTaskIndex  = 0;

  // Tell Vista Multimedia Class Scheduler we are a playback thretad (increase priority)
//  if (pfAvSetMmThreadCharacteristicsW)  
//    hAvrt = pfAvSetMmThreadCharacteristicsW (L"Playback", &dwTaskIndex);
//  if (pfAvSetMmThreadPriority)      
//    pfAvSetMmThreadPriority (hAvrt, AVRT_PRIORITY_HIGH /*AVRT_PRIORITY_CRITICAL*/);

    timeGetDevCaps(&tc, sizeof(TIMECAPS));
    dwResolution = min(max(tc.wPeriodMin, 0), tc.wPeriodMax);
    dwUser    = timeBeginPeriod(dwResolution);

  while (!bQuit)
  {
    DWORD dwObject = WaitForMultipleObjects (countof(hEvts), hEvts, FALSE, 1);
    switch (dwObject)
    {
    case WAIT_OBJECT_0 :
      bQuit = true;
      break;
    case WAIT_TIMEOUT :
      {
        bool bDoneSomething = false;
        {
          CAutoLock AutoLock(&m_ImageProcessingLock);
          bDoneSomething = GetImageFromMixer();
        }
        if (m_rtTimePerFrame == 0 && bDoneSomething)
        {
          CAutoLock lock(this);
          CAutoLock lock2(&m_ImageProcessingLock);
          CAutoLock cRenderLock(&m_RenderLock);

          // Use the code from VMR9 to get the movie fps, as this method is reliable.
          CComPtr<IPin>      pPin;
          CMediaType        mt;
          if (
            SUCCEEDED (m_pOuterEVR->FindPin(L"EVR Input0", &pPin)) &&
            SUCCEEDED (pPin->ConnectionMediaType(&mt)) )
          {
            DShowUtil::ExtractAvgTimePerFrame (&mt, m_rtTimePerFrame);


          }
          // If framerate not set by Video Decoder choose 23.97...
          if (m_rtTimePerFrame == 0) 
            m_rtTimePerFrame = 417166;
          m_fps = (float)(10000000.0 / m_rtTimePerFrame);
          if (!g_renderManager.IsConfigured())
            g_renderManager.Configure(m_iVideoWidth, m_iVideoHeight, m_iVideoWidth, m_iVideoHeight, m_fps, CONF_FLAGS_USE_DIRECTSHOW |CONF_FLAGS_FULLSCREEN);
            

        }

      }
      break;
    }
  }

  timeEndPeriod (dwResolution);
}

DWORD WINAPI CEVRAllocatorPresenter::GetMixerThreadStatic(LPVOID lpParam)
{
  CEVRAllocatorPresenter*    pThis = (CEVRAllocatorPresenter*) lpParam;
  pThis->GetMixerThread();
  return 0;
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
    m_bWaitingSample = false;
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
  HANDLE        hAvrt;
  DWORD        dwTaskIndex  = 0;
  HANDLE        hEvts[]    = { m_hEvtQuit, m_hEvtFlush};
  bool        bQuit    = false;
    TIMECAPS      tc;
  DWORD        dwResolution;
  MFTIME        nsSampleTime;
  LONGLONG      llClockTime;
  DWORD        dwUser = 0;
  DWORD        dwObject;

  
  // Tell Vista Multimedia Class Scheduler we are a playback thretad (increase priority)
  if (pfAvSetMmThreadCharacteristicsW)  hAvrt = pfAvSetMmThreadCharacteristicsW (L"Playback", &dwTaskIndex);
  if (pfAvSetMmThreadPriority)      pfAvSetMmThreadPriority (hAvrt, AVRT_PRIORITY_HIGH /*AVRT_PRIORITY_CRITICAL*/);

    timeGetDevCaps(&tc, sizeof(TIMECAPS));
    dwResolution = min(max(tc.wPeriodMin, 0), tc.wPeriodMax);
    dwUser    = timeBeginPeriod(dwResolution);

  int NextSleepTime = 1;
  while (!bQuit)
  {
    if (NextSleepTime == 0)
      NextSleepTime = 1;
    dwObject = WaitForMultipleObjects (countof(hEvts), hEvts, FALSE, max(NextSleepTime < 0 ? 1 : NextSleepTime, 0));
    if (NextSleepTime > 1)
      NextSleepTime = 0;
    else if (NextSleepTime == 0)
      NextSleepTime = -1;
    switch (dwObject)
    {
    case WAIT_OBJECT_0 :
      bQuit = true;
      break;
    case WAIT_OBJECT_0 + 1 :
      // Flush pending samples!
      FlushSamples();
      m_bEvtFlush = false;
      ResetEvent(m_hEvtFlush);
	  CLog::Log(LOGDEBUG,"%s Flushing samples done",__FUNCTION__);
      break;
    case WAIT_TIMEOUT :
      //Need to get the media type before doing anything
      if (m_bPendingRenegotiate)
      {
        CLog::Log(LOGDEBUG,"%s Renegotiating MediaType",__FUNCTION__);
        FlushSamples();
        RenegotiateMediaType();
        m_bPendingRenegotiate = false;
      }//end if (m_bPendingRenegotiate)

      //Might doing it with the wrong timing
      if (m_bPendingResetDevice)
      {
        CLog::Log(LOGDEBUG,"%s Reseting device MediaType",__FUNCTION__);
        m_bNeedPendingResetDevice=false;
        m_bPendingResetDevice = false;
        /*CAutoLock lock(this);
        CAutoLock lock2(&m_ImageProcessingLock);
        CAutoLock cRenderLock(&m_RenderLock);
        RemoveAllSamples();
        ResetD3dDevice();*/
      }//end if (m_bPendingResetDevice)
      
      CComPtr<IMFSample>    pMFSample;
      int nSamplesLeft = 0;
      if (SUCCEEDED (GetScheduledSample(&pMFSample, nSamplesLeft)))
	  {
        if (SUCCEEDED(PresentSample(pMFSample)))
		{
		  MoveToFreeList(pMFSample, true);
          CheckWaitingSampleFromMixer();
		}
		else
          MoveToScheduledList(pMFSample, true);
      }
      break;
	  
      
	}
  }
}

HRESULT _MFGetService(IUnknown* punkObject, REFGUID guidService, REFIID riid, LPVOID* ppvObject ) 
{
  //Function need to develop on xp
  if (!ppvObject)
    return E_POINTER;
  HRESULT hr;
  IMFGetService* pSrv;
  hr = punkObject->QueryInterface(__uuidof(IMFGetService), (void**)&pSrv);
  if ( SUCCEEDED(hr) ) 
  {
    hr = pSrv->GetService(guidService, riid, ppvObject);
    SAFE_RELEASE(pSrv);
  }
  return hr;
}

HRESULT GetD3DSurfaceFromSample(IMFSample *pSample, IDirect3DSurface9 **ppSurface)
{
    *ppSurface = NULL;
    IMFMediaBuffer *pBuffer = NULL;

    HRESULT hr = pSample->GetBufferByIndex(0, &pBuffer);
    if (SUCCEEDED(hr))
    {
        hr = _MFGetService(pBuffer, MR_BUFFER_SERVICE, __uuidof(IDirect3DSurface9), (void**)ppSurface);
        pBuffer->Release();
    }

    return hr;
}

static LONGLONG       g_llLastWorkerNotification = 0;
static BOOL           g_bTimerInitializer = false;
static BOOL           g_bQPCAvail;
static LARGE_INTEGER  g_lPerfFrequency;
static CCritSec       TimeStampLock;
#define ABS64(num) (num >=0 ? num : -num)
#define LowDW(num) ((unsigned __int64)(unsigned long)(num & 0xFFFFFFFFUL))
#define HighDW(num) ((unsigned __int64)(num >> 32))
//
__int64 _stdcall cMulDiv64(__int64 operant, __int64 multiplier, __int64 divider)
{
	// Declare 128bit storage
	union {
		unsigned long DW[4];
    struct {
      unsigned __int64 LowQW;
      unsigned __int64 HighQW;
    };
	} var128, quotient;
	// Change semantics for intermediate results for Full Div by renaming the vars
	#define REMAINDER quotient
	#define QUOTIENT var128

  bool negative = ((operant ^ multiplier ^ divider) & 0x8000000000000000LL) != 0;

	// Take absolute values because algorithm is for unsigned only
	operant		 = ABS64(operant);
	multiplier = ABS64(multiplier);
	divider		 = ABS64(divider);

  // integer division by zero needs to be handled in the calling method
  if( divider == 0 )
  {
    return 1/divider;
  }
  
  // Multiply
  if ( multiplier == 0 )
  {
    return 0;
  }

  var128.HighQW = 0;

  if ( multiplier == 1 )
  {
    var128.LowQW = operant;
  }
  else if(((multiplier | operant) & 0xFFFFFFFF00000000LL) == 0)
  {
    // 32*32 multiply
    var128.LowQW = operant * multiplier;
  }
  else
  {
    // Full multiply: var128 = operant * multiplier
    var128.LowQW = LowDW(operant) * LowDW(multiplier);
    unsigned __int64 tmp = var128.DW[1] + LowDW(operant) * HighDW(multiplier);
    unsigned __int64 tmp2 = tmp + HighDW(operant) * LowDW(multiplier);
    if(tmp2 < tmp)
      var128.DW[3]++;
    var128.DW[1] = LowDW(tmp2);
    var128.DW[2] = HighDW(tmp2);
    var128.HighQW += HighDW(operant) * HighDW(multiplier);
  }
  
  // Divide
  if ( HighDW(divider) == 0 )
  {
    if ( divider != 1 )
    {
      // 32 bit divisor, do 128:32
      quotient.DW[3] = (unsigned long)(var128.DW[3] / divider);
      unsigned __int64 tmp = ((var128.DW[3] % divider) << 32) | var128.DW[2];
      quotient.DW[2] = (unsigned long)(tmp / divider);
      tmp = ((tmp % divider) << 32) | var128.DW[1];
      quotient.DW[1] = (unsigned long)(tmp / divider);
      tmp = ((tmp % divider) << 32) |  var128.DW[0];
      quotient.DW[0] = (unsigned long)(tmp / divider);
      var128 = quotient;
    }
  }
  else
  {
    // 64 bit divisor, do full division (128:64)
    int c = 128;
    quotient.LowQW = 0;
    quotient.HighQW = 0;
    do
    {
      REMAINDER.HighQW = (REMAINDER.HighQW << 1) | (REMAINDER.DW[1] >> 31);
      REMAINDER.LowQW = (REMAINDER.LowQW << 1) | (QUOTIENT.DW[3] >> 31);
      QUOTIENT.HighQW = (QUOTIENT.HighQW << 1) | (QUOTIENT.DW[1] >> 31);
      QUOTIENT.LowQW = (QUOTIENT.LowQW << 1);
      if ( REMAINDER.HighQW > 0 || REMAINDER.LowQW >= (unsigned __int64)divider )
      {
        if ( REMAINDER.LowQW < (unsigned __int64)divider )
        {
          REMAINDER.LowQW -= divider;
          REMAINDER.HighQW--;
        }
        else
        {
          REMAINDER.LowQW -= divider;
        }
        if ( ++QUOTIENT.LowQW == 0 )
        {
          QUOTIENT.HighQW++;
        }
      }
    } while( --c > 0 );
  }

  // Apply Sign
  if( negative )
  {
	  return -(__int64)var128.LowQW;
  }
  else
  {
    return (__int64)var128.LowQW;
  }
}
static LONGLONG GetCurrentTimestamp()
{
  CAutoLock lock(&TimeStampLock);
  if( !g_bTimerInitializer ) 
  {
    DWORD_PTR oldmask = SetThreadAffinityMask(GetCurrentThread(), 0);
    g_bQPCAvail = QueryPerformanceFrequency((LARGE_INTEGER*)&g_lPerfFrequency);
    SetThreadAffinityMask(GetCurrentThread(), oldmask);

    //Log("GetCurrentTimestamp(): Performance timer available: %d ", g_bQPCAvail);

    g_bTimerInitializer = true;
  }

  if( g_bQPCAvail ) 
  {
    // http://msdn.microsoft.com/en-us/library/ms644904(VS.85).aspx
    // Use always the same CPU core (should help with broken BIOS and/or HAL)
    DWORD_PTR oldmask = SetThreadAffinityMask(GetCurrentThread(), 0);

    ULARGE_INTEGER tics;
    QueryPerformanceCounter((LARGE_INTEGER*)&tics);

    SetThreadAffinityMask(GetCurrentThread(), oldmask);

    // to keep accuracy
    return cMulDiv64(tics.QuadPart, 10000000, g_lPerfFrequency.QuadPart);
  }
  else 
  {
    // ms to 100ns units
    return timeGetTime() * 10000; 
  }
}

HRESULT CEVRAllocatorPresenter::PresentSample(IMFSample* pSample)
{
  HRESULT hr = S_OK;
  IMFMediaBuffer* pBuffer = NULL;
  IDirect3DSurface9* pSurface = NULL;
  CLog::Log(LOGDEBUG,"Presenting sample");
  // Get the buffer from the sample.
  hr = pSample->GetBufferByIndex(0, &pBuffer);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR,"Failed getting buffer index");
    return hr;
    
  }
  hr = _MFGetService(pBuffer, MR_BUFFER_SERVICE, __uuidof(IDirect3DSurface9), (void**)&pSurface);
    
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR,"%s Failed on getting the surface from the sample",__FUNCTION__);
    return hr;
  }
  
  if (pSurface)
  {
    // Get the swap chain from the surface.
    /*CHECK_HR(hr = pSurface->GetContainer(__uuidof(IDirect3DSwapChain9),(void**)&pSwapChain),"failed: GetContainer");*/
    // Calculate offset to scheduled time
    m_pcFramesDrawn++;
    if ( m_pClock != NULL ) 
    {
      LONGLONG hnsTimeNow, hnsSystemTime, hnsTimeScheduled;
      m_pClock->GetCorrelatedTime(0, &hnsTimeNow, &hnsSystemTime);

      pSample->GetSampleTime(&hnsTimeScheduled);
      if ( hnsTimeScheduled > 0 )
      {
        LONGLONG deviation = hnsTimeNow - hnsTimeScheduled;
        if ( deviation < 0 ) deviation = -deviation;
        m_FrameTimeDiff += deviation;
      }
	  if ( m_LastFrameTime != 0  && m_iExpectedFrameDuration > 0 )
      {
        m_iFramesForStats ++;
        if ( m_iFramesForStats > 1000 )
        {
          m_iFramesForStats = 1;
          m_dwVariance = 0;
        }
        LONGLONG hnsDiff = hnsTimeNow - m_LastFrameTime;
        int duration = (hnsDiff/10000);
        int dev = m_iExpectedFrameDuration - duration;
        m_dwVariance += dev*dev;
        if (duration < m_iMinFrameTimeDiff) m_iMinFrameTimeDiff = duration;
        if (duration > m_iMaxFrameTimeDiff) m_iMaxFrameTimeDiff = duration;
      }
      m_LastFrameTime = hnsTimeNow;
    }
    // Present the swap surface
    LONGLONG then = GetCurrentTimestamp();
    hr = RenderPresent(pSurface);
	if (FAILED(hr))
	{
      CLog::Log(LOGERROR,"%s Failed rendering sample",__FUNCTION__);
	  return hr;
	}
    LONGLONG diff = GetCurrentTimestamp() - then;
    CLog::Log(LOGDEBUG,"Paint() latency: %I64d ms", diff/10000);
  }

  SAFE_RELEASE(pBuffer);
  SAFE_RELEASE(pSurface);
  return hr;
}
HRESULT CEVRAllocatorPresenter::RenderPresent(CComPtr<IDirect3DSurface9> pSurface)
{
  CAutoLock cRenderLock(&m_RenderLock);
  HRESULT hr=S_OK;
  CLog::Log(LOGNOTICE,"%s Rendering frame", __FUNCTION__);
  hr = m_D3DDev->StretchRect(pSurface, NULL, m_pVideoSurface, NULL, D3DTEXF_NONE);
  if (SUCCEEDED(hr))
  {
    g_renderManager.PaintVideoTexture(m_pVideoTexture,pSurface);
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
  for ( int i=0; i < 5; i++ ) 
  {
    m_pInternalVideoSamples[i] = NULL;
    m_pInternalVideoSurface[i] = NULL;
    m_pInternalSwapchains[i] = NULL;
    m_pInternalVideoTexture[i] = NULL;
    m_pInternalFreeSamples[i] = NULL;
  }
}

HRESULT CEVRAllocatorPresenter::AllocSurfaces(D3DFORMAT Format)
{
  CAutoLock cAutoLock(this);
  CAutoLock cRenderLock(&m_RenderLock);
  HRESULT hr = S_OK;

  int nTexturesNeeded = 5;
  //default format is D3DFMT_X8R8G8B8
  for ( int i=0 ; i<nTexturesNeeded ; i++ )
  {
    hr = m_D3DDev->CreateTexture(m_iVideoWidth, m_iVideoHeight, 1, D3DUSAGE_RENDERTARGET, Format, D3DPOOL_DEFAULT, &m_pInternalVideoTexture[i], NULL);
    hr = m_pInternalVideoTexture[i]->GetSurfaceLevel(0,&m_pInternalVideoSurface[i]);
	hr = pfMFCreateVideoSampleFromSurface(m_pInternalVideoSurface[i],&m_pInternalVideoSamples[i]);
	m_pInternalFreeSamples[i] = m_pInternalVideoSamples[i];
  }
  m_pFreeSamples = 5;

  m_SurfaceType = Format;

  m_pVideoTexture = NULL;
  m_pVideoSurface = NULL;
  
  hr = m_D3DDev->CreateTexture(
    m_iVideoWidth, m_iVideoHeight, 1, 
    D3DUSAGE_RENDERTARGET, /*D3DFMT_X8R8G8B8*/D3DFMT_A8R8G8B8, 
    D3DPOOL_DEFAULT, &m_pVideoTexture, NULL);
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
  HRESULT        hr;
  AM_MEDIA_TYPE*    pAMMedia = NULL;
  LARGE_INTEGER    i64Size;
  MFVIDEOFORMAT*    VideoFormat;
  hr = pMixerType->GetRepresentation  (FORMAT_MFVideoFormat, (void**)&pAMMedia);
  VideoFormat = (MFVIDEOFORMAT*)pAMMedia->pbFormat;
  hr = pfMFCreateVideoMediaType  (VideoFormat, &m_pMediaType);
  /*MFVideoArea Area = MakeArea (0, 0, VideoFormat->videoInfo.dwWidth, VideoFormat->videoInfo.dwHeight);
  m_pMediaType->SetBlob(MF_MT_GEOMETRIC_APERTURE, (UINT8*)&Area, sizeof(MFVideoArea));
  pMixerType->FreeRepresentation (FORMAT_MFVideoFormat, (void*)pAMMedia);*/
  m_pMediaType->QueryInterface (__uuidof(IMFMediaType), (void**) pType);

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


STDMETHODIMP CEVRAllocatorPresenter::InitializeDevice(AM_MEDIA_TYPE*  pMediaType)
{
  HRESULT      hr;
  CAutoLock lock(this);
  CAutoLock lock2(&m_ImageProcessingLock);
  CAutoLock cRenderLock(&m_RenderLock);

  RemoveAllSamples();
  DeleteSurfaces();

  VIDEOINFOHEADER2*    vih2 = (VIDEOINFOHEADER2*) pMediaType->pbFormat;

  m_iVideoWidth = vih2->bmiHeader.biWidth;
  m_iVideoHeight = abs(vih2->bmiHeader.biHeight);
  
  //Allocsurfaces will alloc the samples to the surface
  if (0)//m_bHighColorResolution)
    hr = AllocSurfaces(D3DFMT_A2R10G10B10);
  else
    hr = AllocSurfaces(D3DFMT_X8R8G8B8);

  return hr;
}


HRESULT CEVRAllocatorPresenter::TRACE_EVR(const char *strTrace)
{
  CLog::Log(LOGNOTICE,"%s",strTrace);
  return S_OK;
}