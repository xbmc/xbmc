#include "EVRAllocatorPresenter.h"
#include "MacrovisionKicker.h"
#include "utils/log.h"
#include "dshowutil/dshowutil.h"

#include "vmr9.h"
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
  HRESULT hrr;
  if (FAILED(DXVA2CreateDirect3DDeviceManager9(&m_iResetToken,&m_pDeviceManager)))
  {
    CLog::Log(LOGERROR,"Failed to create DXVA2CreateDirect3DDeviceManager9");
    m_pDeviceManager->ResetDevice(d3dd, m_iResetToken);
  }
}

CEVRAllocatorPresenter::~CEVRAllocatorPresenter()
{
}

STDMETHODIMP CEVRAllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
CheckPointer(ppRenderer, E_POINTER);
*ppRenderer = NULL;
HRESULT	hr = E_FAIL;
CMacrovisionKicker*		pMK  = new CMacrovisionKicker(NAME("CMacrovisionKicker"), NULL);
		CComPtr<IUnknown>		pUnk = (IUnknown*)(INonDelegatingUnknown*)pMK;
		COuterEVR *pOuterEVR = new COuterEVR(NAME("COuterEVR"), pUnk, hr, this);
		m_pOuterEVR = pOuterEVR;
		pMK->SetInner((IUnknown*)(INonDelegatingUnknown*)pOuterEVR);
CComQIPtr<IBaseFilter> pBF = pUnk;
  CComPtr<IMFVideoPresenter>		pVP;
  CComPtr<IMFVideoRenderer>		pMFVR;
  CComQIPtr<IMFGetService, &__uuidof(IMFGetService)> pMFGS = pBF;
  hr = pMFGS->GetService (MR_VIDEO_RENDER_SERVICE, IID_IMFVideoRenderer, (void**)&pMFVR);

		if(SUCCEEDED(hr)) hr = QueryInterface (__uuidof(IMFVideoPresenter), (void**)&pVP);
		if(SUCCEEDED(hr)) hr = pMFVR->InitializeRenderer (NULL, pVP);
if(FAILED(hr))
			*ppRenderer = NULL;
		else
			*ppRenderer = pBF.Detach();

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
  /*else if( riid == IID_IQualProp) 
  {
    *ppvObject = static_cast<IQualProp*>( this );
    AddRef();
    hr = S_OK;
  }
  else if( riid == IID_IMFRateSupport) 
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
  {
    CLog::Log(LOGERROR,"QueryInterface failed" );
  }
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

HRESULT CEVRAllocatorPresenter::GetCurrentMediaType(IMFVideoMediaType** ppMediaType)
{
  HRESULT hr = S_OK;

  if (ppMediaType == NULL)
    return E_POINTER;

  /*if (m_pMediaType == NULL)
  {
	  CLog::Log(LOGERROR,"MediaType is NULL");
  }*/
  hr = m_pMediaType->QueryInterface(__uuidof(IMFVideoMediaType), (void**)ppMediaType);
  if FAILED(hr)
    CLog::Log(LOGERROR,"Query interface failed in GetCurrentMediaType");

  CLog::Log(LOGNOTICE,"GetCurrentMediaType done" );
  return hr;
}

HRESULT STDMETHODCALLTYPE CEVRAllocatorPresenter::ProcessMessage( 
  MFVP_MESSAGE_TYPE eMessage,
  ULONG_PTR ulParam)
{
	HRESULT hr = S_OK;
	return hr;
}