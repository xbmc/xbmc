#include "EVRAllocatorPresenter.h"
#include "MacrovisionKicker.h"
#include "utils/log.h"
#include "dshowutil/dshowutil.h"
#include <mfapi.h>
#include "vmr9.h"
#include "IPinHook.h"
class COuterEVR
	: public CUnknown
	//, public IVMRMixerBitmap9
	, public IBaseFilter
{
	CComPtr<IUnknown>	m_pEVR;
	VMR9AlphaBitmap*	m_pVMR9AlphaBitmap;
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
m_D3DDev(d3dd)
{
  m_nRenderState = Shutdown;
	HMODULE		hLib;
  // Load EVR specifics DLLs
	hLib = LoadLibrary ("dxva2.dll");
	pfDXVA2CreateDirect3DDeviceManager9	= hLib ? (PTR_DXVA2CreateDirect3DDeviceManager9) GetProcAddress (hLib, "DXVA2CreateDirect3DDeviceManager9") : NULL;
	
	// Load EVR functions
	hLib = LoadLibrary ("evr.dll");
	pfMFCreateDXSurfaceBuffer			= hLib ? (PTR_MFCreateDXSurfaceBuffer)			GetProcAddress (hLib, "MFCreateDXSurfaceBuffer") : NULL;
	pfMFCreateVideoSampleFromSurface	= hLib ? (PTR_MFCreateVideoSampleFromSurface)	GetProcAddress (hLib, "MFCreateVideoSampleFromSurface") : NULL;
	pfMFCreateVideoMediaType			= hLib ? (PTR_MFCreateVideoMediaType)			GetProcAddress (hLib, "MFCreateVideoMediaType") : NULL;

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
	pfAvSetMmThreadCharacteristicsW		= hLib ? (PTR_AvSetMmThreadCharacteristicsW)	GetProcAddress (hLib, "AvSetMmThreadCharacteristicsW") : NULL;
	pfAvSetMmThreadPriority				= hLib ? (PTR_AvSetMmThreadPriority)			GetProcAddress (hLib, "AvSetMmThreadPriority") : NULL;
	pfAvRevertMmThreadCharacteristics	= hLib ? (PTR_AvRevertMmThreadCharacteristics)	GetProcAddress (hLib, "AvRevertMmThreadCharacteristics") : NULL;

  if (SUCCEEDED(pfDXVA2CreateDirect3DDeviceManager9(&m_iResetToken,&m_pDeviceManager)))
  {
    CLog::Log(LOGERROR,"Sucess to create DXVA2CreateDirect3DDeviceManager9");
    hr = m_pDeviceManager->ResetDevice(d3dd, m_iResetToken);
  }
  CComPtr<IDirectXVideoDecoderService>	pDecoderService;
  HANDLE							hDevice;
  hr = m_pDeviceManager->OpenDeviceHandle(&hDevice);
  hr = m_pDeviceManager->GetVideoService(hDevice,__uuidof(IDirectXVideoDecoderService), (void**)&pDecoderService);
  if (SUCCEEDED(hr))
  {
	HookDirectXVideoDecoderService (pDecoderService);
    m_pDeviceManager->CloseDeviceHandle (hDevice);
  }
}

CEVRAllocatorPresenter::~CEVRAllocatorPresenter()
{
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

// IBaseFilter delegate
bool CEVRAllocatorPresenter::GetState( DWORD dwMilliSecsTimeout, FILTER_STATE *State, HRESULT &_ReturnValue)
{
	return true;
}

STDMETHODIMP CEVRAllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	HRESULT		hr;
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
//		hr = GetInterface((IDirect3DDeviceManager9*)this, ppv);
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
  case MFVP_MESSAGE_BEGINSTREAMING :			// The EVR switched from stopped to paused. The presenter should allocate resources
		//ResetStats();
		TRACE_EVR ("EVR: MFVP_MESSAGE_BEGINSTREAMING\n");
		break;

	case MFVP_MESSAGE_CANCELSTEP :				// Cancels a frame step
		TRACE_EVR ("EVR: MFVP_MESSAGE_CANCELSTEP\n");
		//CompleteFrameStep (true);
		break;

	case MFVP_MESSAGE_ENDOFSTREAM :				// All input streams have ended. 
		TRACE_EVR ("EVR: MFVP_MESSAGE_ENDOFSTREAM\n");
		m_bPendingMediaFinished = true;
		break;

	case MFVP_MESSAGE_ENDSTREAMING :			// The EVR switched from running or paused to stopped. The presenter should free resources
		TRACE_EVR ("EVR: MFVP_MESSAGE_ENDSTREAMING\n");
		break;

	case MFVP_MESSAGE_FLUSH :					// The presenter should discard any pending samples
		SetEvent(m_hEvtFlush);
		m_bEvtFlush = true;
		TRACE_EVR ("EVR: MFVP_MESSAGE_FLUSH\n");
		while (WaitForSingleObject(m_hEvtFlush, 1) == WAIT_OBJECT_0);
		break;

	case MFVP_MESSAGE_INVALIDATEMEDIATYPE :		// The mixer's output format has changed. The EVR will initiate format negotiation, as described previously
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

	case MFVP_MESSAGE_PROCESSINPUTNOTIFY :		// One input stream on the mixer has received a new sample
//		GetImageFromMixer();
		break;

	case MFVP_MESSAGE_STEP :					// Requests a frame step.
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
	HRESULT						hr;
	DWORD						dwObjects = 1;

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
	m_pMixer	= NULL;
	m_pSink		= NULL;
	m_pClock	= NULL;
	return S_OK;
}

// IMFVideoDeviceID
STDMETHODIMP CEVRAllocatorPresenter::GetDeviceID(/* [out] */	__out  IID *pDeviceID)
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
	DWORD		dwThreadId;

	if (m_nRenderState == Shutdown)
	{
		m_hEvtQuit		= CreateEvent (NULL, TRUE, FALSE, NULL);
		m_hEvtFlush		= CreateEvent (NULL, TRUE, FALSE, NULL);

		m_hThread		= ::CreateThread(NULL, 0, PresentThread, (LPVOID)this, 0, &dwThreadId);
		SetThreadPriority(m_hThread, THREAD_PRIORITY_TIME_CRITICAL);
		//m_hGetMixerThread = ::CreateThread(NULL, 0, GetMixerThreadStatic, (LPVOID)this, 0, &dwThreadId);
		SetThreadPriority(m_hGetMixerThread, THREAD_PRIORITY_HIGHEST);

		m_nRenderState		= Stopped;
		TRACE_EVR ("EVR: Worker threads started...\n");
	}
}

DWORD WINAPI CEVRAllocatorPresenter::PresentThread(LPVOID lpParam)
{
	CEVRAllocatorPresenter*		pThis = (CEVRAllocatorPresenter*) lpParam;
	pThis->RenderThread();
	return 0;
}
void CEVRAllocatorPresenter::RenderThread()
{
  HANDLE				hAvrt;
	DWORD				dwTaskIndex	= 0;
	HANDLE				hEvts[]		= { m_hEvtQuit, m_hEvtFlush};
	bool				bQuit		= false;
    TIMECAPS			tc;
	DWORD				dwResolution;
	MFTIME				nsSampleTime;
	LONGLONG			llClockTime;
	DWORD				dwUser = 0;
	DWORD				dwObject;

	
	// Tell Vista Multimedia Class Scheduler we are a playback thretad (increase priority)
	if (pfAvSetMmThreadCharacteristicsW)	hAvrt = pfAvSetMmThreadCharacteristicsW (L"Playback", &dwTaskIndex);
	if (pfAvSetMmThreadPriority)			pfAvSetMmThreadPriority (hAvrt, AVRT_PRIORITY_HIGH /*AVRT_PRIORITY_CRITICAL*/);

    timeGetDevCaps(&tc, sizeof(TIMECAPS));
    dwResolution = min(max(tc.wPeriodMin, 0), tc.wPeriodMax);
    dwUser		= timeBeginPeriod(dwResolution);

	int NextSleepTime = 1;
  while (!bQuit)
  {
    NextSleepTime = 1;
		dwObject = WaitForMultipleObjects (countof(hEvts), hEvts, FALSE, max(NextSleepTime < 0 ? 1 : NextSleepTime, 0));
		switch (dwObject)
		{
		case WAIT_OBJECT_0 :
			bQuit = true;
			break;
		case WAIT_OBJECT_0 + 1 :
			// Flush pending samples!
			//FlushSamples();
			m_bEvtFlush = false;
			ResetEvent(m_hEvtFlush);
			TRACE_EVR ("EVR: Flush done!\n");
			break;
		case WAIT_TIMEOUT :

			if (m_bPendingRenegotiate)
			{
				//FlushSamples();
				//RenegotiateMediaType();
				m_bPendingRenegotiate = false;
			}
			if (m_bPendingResetDevice)
			{
				m_bPendingResetDevice = false;
				/*CAutoLock lock(this);
				CAutoLock lock2(&m_ImageProcessingLock);
				CAutoLock cRenderLock(&m_RenderLock);*/

				
				//RemoveAllSamples();

				//CDX9AllocatorPresenter::ResetDevice();
                CComPtr<IMFSample>		pMFSample;
				CComPtr<IDirect3DSurface9> m_pVideoSurface;
				HRESULT hr = pfMFCreateVideoSampleFromSurface (m_pVideoSurface, &pMFSample);
				/*for(int i = 0; i < m_nNbDXSurface; i++)
				{
					CComPtr<IMFSample>		pMFSample;
					HRESULT hr = pfMFCreateVideoSampleFromSurface (m_pVideoSurface[i], &pMFSample);

					if (SUCCEEDED (hr))
					{
						pMFSample->SetUINT32 (GUID_SURFACE_INDEX, i);
						m_FreeSamples.AddTail (pMFSample);
					}
					ASSERT (SUCCEEDED (hr));
				}*/

			}
			break;
		}
	}
}

HRESULT CEVRAllocatorPresenter::TRACE_EVR(const char *strTrace)
{
	CLog::Log(LOGNOTICE,"%s",strTrace);
	return S_OK;
}