/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
 *
 *  Copyright (C) 2005-2010 Team XBMC
 *  http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef HAS_DS_PLAYER

#include "EVRAllocatorPresenter.h"
#include <algorithm>
#include <Mferror.h>
#include "Utils/IPinHook.h"
#include "Utils/MacrovisionKicker.h"
#include "DSUtil/DSUtil.h"
#include "DSUtil/SmartPtr.h"
#include "Utils/TimeUtils.h"
#include "Application.h"
#include "windowing/WindowingFactory.h"

#if (0)    // Set to 1 to activate EVR traces
  #define TRACE_EVR    CLog::DebugLog
#else
  #define TRACE_EVR
#endif

typedef enum 
{
  MSG_MIXERIN,
  MSG_MIXEROUT
} EVR_STATS_MSG;

// Guid to tag IMFSample with DirectX surface index
static const GUID GUID_SURFACE_INDEX = { 0x30c8e9f6, 0x415, 0x4b81, { 0xa3, 0x15, 0x1, 0xa, 0xc6, 0xa9, 0xda, 0x19 } };


// === Helper functions
#define CheckHR(exp) {if(FAILED(hr = exp)) return hr;}

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


/// === Outer EVR


  class COuterEVR
    : public CUnknown
    , public IVMRffdshow9
    , public IVMRMixerBitmap9
    , public IBaseFilter
  {
    Com::SmartPtr<IUnknown>  m_pEVR;
    VMR9AlphaBitmap*  m_pVMR9AlphaBitmap;
    CEVRAllocatorPresenter *m_pAllocatorPresenter;

  public:

    // IBaseFilter
    virtual HRESULT STDMETHODCALLTYPE EnumPins(__out  IEnumPins **ppEnum)
    {
      Com::SmartPtr<IBaseFilter> pEVRBase;
      if (m_pEVR)
        m_pEVR->QueryInterface(&pEVRBase);
      if (pEVRBase)
        return pEVRBase->EnumPins(ppEnum);
      return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE FindPin(LPCWSTR Id, __out  IPin **ppPin)
    {
      Com::SmartPtr<IBaseFilter> pEVRBase;
      if (m_pEVR)
        m_pEVR->QueryInterface(&pEVRBase);
      if (pEVRBase)
        return pEVRBase->FindPin(Id, ppPin);
      return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryFilterInfo(__out  FILTER_INFO *pInfo)
    {
      Com::SmartPtr<IBaseFilter> pEVRBase;
      if (m_pEVR)
        m_pEVR->QueryInterface(&pEVRBase);
      if (pEVRBase)
        return pEVRBase->QueryFilterInfo(pInfo);
      return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE JoinFilterGraph(__in_opt  IFilterGraph *pGraph, __in_opt  LPCWSTR pName)
    {
      Com::SmartPtr<IBaseFilter> pEVRBase;
      if (m_pEVR)
        m_pEVR->QueryInterface(&pEVRBase);
      if (pEVRBase)
        return pEVRBase->JoinFilterGraph(pGraph, pName);
      return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryVendorInfo(__out  LPWSTR *pVendorInfo)
    {
      Com::SmartPtr<IBaseFilter> pEVRBase;
      if (m_pEVR)
        m_pEVR->QueryInterface(&pEVRBase);
      if (pEVRBase)
        return pEVRBase->QueryVendorInfo(pVendorInfo);
      return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Stop( void)
    {
      Com::SmartPtr<IBaseFilter> pEVRBase;
      if (m_pEVR)
        m_pEVR->QueryInterface(&pEVRBase);
      if (pEVRBase)
      {
        // Prevents dead lock. Since we are stopping, set the drawing event
        m_pAllocatorPresenter->m_drawingIsDone.Set();
        return pEVRBase->Stop();
      }
      return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Pause( void)
    {
      Com::SmartPtr<IBaseFilter> pEVRBase;
      if (m_pEVR)
        m_pEVR->QueryInterface(&pEVRBase);
      if (pEVRBase)
        return pEVRBase->Pause();
      return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Run( REFERENCE_TIME tStart)
    {
      Com::SmartPtr<IBaseFilter> pEVRBase;
      if (m_pEVR)
        m_pEVR->QueryInterface(&pEVRBase);
      if (pEVRBase)
        return pEVRBase->Run(tStart);
      return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetState( DWORD dwMilliSecsTimeout, __out  FILTER_STATE *State);

    virtual HRESULT STDMETHODCALLTYPE SetSyncSource(__in_opt  IReferenceClock *pClock)
    {
      Com::SmartPtr<IBaseFilter> pEVRBase;
      if (m_pEVR)
        m_pEVR->QueryInterface(&pEVRBase);
      if (pEVRBase)
        return pEVRBase->SetSyncSource(pClock);
      return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetSyncSource(__deref_out_opt  IReferenceClock **pClock)
    {
      Com::SmartPtr<IBaseFilter> pEVRBase;
      if (m_pEVR)
        m_pEVR->QueryInterface(&pEVRBase);
      if (pEVRBase)
        return pEVRBase->GetSyncSource(pClock);
      return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetClassID(__RPC__out CLSID *pClassID)
    {
      Com::SmartPtr<IBaseFilter> pEVRBase;
      if (m_pEVR)
        m_pEVR->QueryInterface(&pEVRBase);
      if (pEVRBase)
        return pEVRBase->GetClassID(pClassID);
      return E_NOTIMPL;
    }

    COuterEVR(const TCHAR* pName, LPUNKNOWN pUnk, HRESULT& hr, VMR9AlphaBitmap* pVMR9AlphaBitmap, CEVRAllocatorPresenter *pAllocatorPresenter) : CUnknown(pName, pUnk)
    {
      hr = m_pEVR.CoCreateInstance(CLSID_EnhancedVideoRenderer, GetOwner());
      m_pVMR9AlphaBitmap = pVMR9AlphaBitmap;
      m_pAllocatorPresenter = pAllocatorPresenter;
    }

    ~COuterEVR();

    DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
    {
      HRESULT hr;

      if(riid == __uuidof(IVMRMixerBitmap9))
      {
        return GetInterface((IVMRMixerBitmap9*)this, ppv);
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
      if(m_pEVR && FAILED(hr))
      {
        if(riid == __uuidof(IVMRffdshow9)) // Support ffdshow queueing. We show ffdshow that this is patched Media Player Classic.
          return GetInterface((IVMRffdshow9*)this, ppv);
      }

      return SUCCEEDED(hr) ? hr : __super::NonDelegatingQueryInterface(riid, ppv);
    }

    // IVMRffdshow9
    STDMETHODIMP support_ffdshow()
    {
      queue_ffdshow_support = true;
      return S_OK;
    }

    // IVMRMixerBitmap9
    STDMETHODIMP GetAlphaBitmapParameters(VMR9AlphaBitmap* pBmpParms);

    STDMETHODIMP SetAlphaBitmap(const VMR9AlphaBitmap*  pBmpParms);

    STDMETHODIMP UpdateAlphaBitmapParameters(const VMR9AlphaBitmap* pBmpParms);
  };


HRESULT STDMETHODCALLTYPE COuterEVR::GetState( DWORD dwMilliSecsTimeout, __out  FILTER_STATE *State)
{
  HRESULT ReturnValue;
  if (m_pAllocatorPresenter->GetState(dwMilliSecsTimeout, State, ReturnValue))
    return ReturnValue;
  Com::SmartPtr<IBaseFilter> pEVRBase;
  if (m_pEVR)
    m_pEVR->QueryInterface(&pEVRBase);
  if (pEVRBase)
    return pEVRBase->GetState(dwMilliSecsTimeout, State);
  return E_NOTIMPL;
}

STDMETHODIMP COuterEVR::GetAlphaBitmapParameters(VMR9AlphaBitmap* pBmpParms)
{
  CheckPointer(pBmpParms, E_POINTER);
  CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
  memcpy (pBmpParms, m_pVMR9AlphaBitmap, sizeof(VMR9AlphaBitmap));
  return S_OK;
}

STDMETHODIMP COuterEVR::SetAlphaBitmap(const VMR9AlphaBitmap*  pBmpParms)
{
  CheckPointer(pBmpParms, E_POINTER);
  CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
  memcpy (m_pVMR9AlphaBitmap, pBmpParms, sizeof(VMR9AlphaBitmap));
  m_pVMR9AlphaBitmap->dwFlags |= VMRBITMAP_UPDATE;
  m_pAllocatorPresenter->UpdateAlphaBitmap();
  return S_OK;
}

STDMETHODIMP COuterEVR::UpdateAlphaBitmapParameters(const VMR9AlphaBitmap* pBmpParms)
{
  CheckPointer(pBmpParms, E_POINTER);
  CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
  memcpy (m_pVMR9AlphaBitmap, pBmpParms, sizeof(VMR9AlphaBitmap));
  m_pVMR9AlphaBitmap->dwFlags |= VMRBITMAP_UPDATE;
  m_pAllocatorPresenter->UpdateAlphaBitmap();
  return S_OK;
}

COuterEVR::~COuterEVR()
{
}

CEVRAllocatorPresenter::CEVRAllocatorPresenter(HWND hWnd, HRESULT& hr, CStdString &_Error)
  : CDX9AllocatorPresenter(hWnd, hr, true, _Error)
{
  HMODULE    hLib;
  //g_dsSettings.LoadConfig();
  m_nResetToken   = 0;
  m_hThread     = INVALID_HANDLE_VALUE;
  m_hGetMixerThread= INVALID_HANDLE_VALUE;
  m_hEvtFlush     = INVALID_HANDLE_VALUE;
  m_hEvtQuit     = INVALID_HANDLE_VALUE;
  m_bEvtQuit = 0;
  m_bEvtFlush = 0;
  m_ModeratedTime = 0;
  m_ModeratedTimeLast = -1;
  m_ModeratedClockLast = -1;

  if (FAILED (hr)) 
  {
    _Error += "DX9AllocatorPresenter failed\n";

    return;
  }

  // Load EVR specifics DLLs
  hLib = LoadLibrary("dxva2.dll");
  pfDXVA2CreateDirect3DDeviceManager9  = hLib ? (PTR_DXVA2CreateDirect3DDeviceManager9) GetProcAddress (hLib, "DXVA2CreateDirect3DDeviceManager9") : NULL;
  
  // Load EVR functions
  hLib = LoadLibrary("evr.dll");
  pfMFCreateDXSurfaceBuffer      = hLib ? (PTR_MFCreateDXSurfaceBuffer)      GetProcAddress (hLib, "MFCreateDXSurfaceBuffer") : NULL;
  pfMFCreateVideoSampleFromSurface  = hLib ? (PTR_MFCreateVideoSampleFromSurface)  GetProcAddress (hLib, "MFCreateVideoSampleFromSurface") : NULL;
  pfMFCreateVideoMediaType      = hLib ? (PTR_MFCreateVideoMediaType)      GetProcAddress (hLib, "MFCreateVideoMediaType") : NULL;

  if (!pfDXVA2CreateDirect3DDeviceManager9 || !pfMFCreateDXSurfaceBuffer || !pfMFCreateVideoSampleFromSurface || !pfMFCreateVideoMediaType)
  {
    if (!pfDXVA2CreateDirect3DDeviceManager9)
      _Error += "Could not find DXVA2CreateDirect3DDeviceManager9 (dxva2.dll)\n";
    if (!pfMFCreateDXSurfaceBuffer)
      _Error += "Could not find MFCreateDXSurfaceBuffer (evr.dll)\n";
    if (!pfMFCreateVideoSampleFromSurface)
      _Error += "Could not find MFCreateVideoSampleFromSurface (evr.dll)\n";
    if (!pfMFCreateVideoMediaType)
      _Error += "Could not find MFCreateVideoMediaType (evr.dll)\n";
    hr = E_FAIL;
    return;
  }

  // Load Vista specifics DLLs
  hLib = LoadLibrary ("AVRT.dll");
  pfAvSetMmThreadCharacteristicsW    = hLib ? (PTR_AvSetMmThreadCharacteristicsW)  GetProcAddress (hLib, "AvSetMmThreadCharacteristicsW") : NULL;
  pfAvSetMmThreadPriority        = hLib ? (PTR_AvSetMmThreadPriority)      GetProcAddress (hLib, "AvSetMmThreadPriority") : NULL;
  pfAvRevertMmThreadCharacteristics  = hLib ? (PTR_AvRevertMmThreadCharacteristics)  GetProcAddress (hLib, "AvRevertMmThreadCharacteristics") : NULL;

  // Init DXVA manager
  hr = pfDXVA2CreateDirect3DDeviceManager9(&m_nResetToken, &m_pD3DManager);
  if (SUCCEEDED (hr)) 
  {
    hr = m_pD3DManager->ResetDevice(m_pD3DDev, m_nResetToken);
    if (!SUCCEEDED (hr)) 
    {
      _Error += "m_pD3DManager->ResetDevice failed\n";
    }
  }
  else
    _Error += "DXVA2CreateDirect3DDeviceManager9 failed\n";

  Com::SmartPtr<IDirectXVideoDecoderService>  pDecoderService;
  HANDLE              hDevice;
  if (SUCCEEDED (m_pD3DManager->OpenDeviceHandle(&hDevice)) &&
    SUCCEEDED (m_pD3DManager->GetVideoService (hDevice, __uuidof(IDirectXVideoDecoderService), (void**)&pDecoderService)))
  {
    TRACE_EVR ("EVR: DXVA2 : device handle = 0x%08x", hDevice);
    HookDirectXVideoDecoderService (pDecoderService);

    m_pD3DManager->CloseDeviceHandle (hDevice);
  }


  // Bufferize frame only with 3D texture!
  if (g_dsSettings.pRendererSettings->apSurfaceUsage == VIDRNDT_AP_TEXTURE3D)
    m_nNbDXSurface  = std::max ( std::min (((CEVRRendererSettings *)g_dsSettings.pRendererSettings)->buffers, MAX_PICTURE_SLOTS-3), 4);
  else
    m_nNbDXSurface = 1;

  ResetStats();
  m_nRenderState        = Shutdown;
  m_fUseInternalTimer      = false;
  m_LastSetOutputRange    = -1;
  m_bPendingRenegotiate    = false;
  m_bPendingMediaFinished    = false;
  m_bWaitingSample      = false;
  m_pCurrentDisplaydSample  = NULL;
  m_nStepCount        = 0;
  m_dwVideoAspectRatioMode  = MFVideoARMode_PreservePicture;
  m_dwVideoRenderPrefs    = (MFVideoRenderPrefs)0;
  m_BorderColor        = RGB (0,0,0);
  m_bSignaledStarvation    = false;
  m_StarvationClock      = 0;
  m_pOuterEVR          = NULL;
  m_LastScheduledSampleTime  = -1;
  m_LastScheduledUncorrectedSampleTime = -1;
  m_MaxSampleDuration      = 0;
  m_LastSampleOffset      = 0;
  ZeroMemory(m_VSyncOffsetHistory, sizeof(m_VSyncOffsetHistory));
  m_VSyncOffsetHistoryPos = 0;
  m_bLastSampleOffsetValid  = false;
}

CEVRAllocatorPresenter::~CEVRAllocatorPresenter(void)
{
  StopWorkerThreads();  // If not already done...
  m_pMediaType  = NULL;
  m_pClock    = NULL;

  m_pD3DManager  = NULL;
}


void CEVRAllocatorPresenter::ResetStats()
{
  m_pcFrames      = 0;
  m_nDroppedUpdate  = 0;
  m_pcFramesDrawn    = 0;
  m_piAvg        = 0;
  m_piDev        = 0;
}


HRESULT CEVRAllocatorPresenter::CheckShutdown() const 
{
    if (m_nRenderState == Shutdown)
    {
        return MF_E_SHUTDOWN;
    }
    else
    {
        return S_OK;
    }
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

    m_drawingIsDone.Set();

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


STDMETHODIMP CEVRAllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
    CheckPointer(ppRenderer, E_POINTER);

  *ppRenderer = NULL;

  HRESULT          hr = E_FAIL;

  do
  {
    CMacrovisionKicker*    pMK  = DNew CMacrovisionKicker(NAME("CMacrovisionKicker"), NULL);
    Com::SmartPtr<IUnknown>    pUnk = (IUnknown*)(INonDelegatingUnknown*)pMK;

    COuterEVR *pOuterEVR = DNew COuterEVR(NAME("COuterEVR"), pUnk, hr, &m_VMR9AlphaBitmap, this);
    m_pOuterEVR = pOuterEVR;

    pMK->SetInner((IUnknown*)(INonDelegatingUnknown*)pOuterEVR);
    Com::SmartQIPtr<IBaseFilter> pBF = pUnk;

    if (FAILED (hr)) break;

    // Set EVR custom presenter
    Com::SmartPtr<IMFVideoPresenter>    pVP;
    Com::SmartPtr<IMFVideoRenderer>    pMFVR;
    Com::SmartQIPtr<IMFGetService, &__uuidof(IMFGetService)> pMFGS = pBF;

    hr = pMFGS->GetService (MR_VIDEO_RENDER_SERVICE, IID_IMFVideoRenderer, (void**)&pMFVR);

    if(SUCCEEDED(hr)) hr = QueryInterface (__uuidof(IMFVideoPresenter), (void**)&pVP);
    if(SUCCEEDED(hr)) hr = pMFVR->InitializeRenderer (NULL, pVP);

#if 1
    Com::SmartPtr<IPin>           pPin          = GetFirstPin(pBF);
    Com::SmartQIPtr<IMemInputPin> pMemInputPin  = pPin;
    
    // No NewSegment : no chocolate :o)
    m_fUseInternalTimer = HookNewSegmentAndReceive((IPinC*)(IPin*)pPin, (IMemInputPinC*)(IMemInputPin*)pMemInputPin);
#else
    m_fUseInternalTimer = false;
#endif

    if(FAILED(hr))
      *ppRenderer = NULL;
    else
      *ppRenderer = pBF.Detach();

  } while (0);

  return hr;
}

STDMETHODIMP_(bool) CEVRAllocatorPresenter::Paint(bool fAll)
{
  return __super::Paint (fAll);
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
    hr = m_pD3DManager->QueryInterface (__uuidof(IDirect3DDeviceManager9), (void**) ppv);
  else
    hr = __super::NonDelegatingQueryInterface(riid, ppv);

  return hr;
}


// IMFClockStateSink
STDMETHODIMP CEVRAllocatorPresenter::OnClockStart(MFTIME hnsSystemTime,  int64_t llClockStartOffset)
{
  m_nRenderState    = Started;

  TRACE_EVR ("EVR: OnClockStart  hnsSystemTime = %I64d,   llClockStartOffset = %I64d\n", hnsSystemTime, llClockStartOffset);
  m_ModeratedTimeLast = -1;
  m_ModeratedClockLast = -1;

  return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockStop(MFTIME hnsSystemTime)
{
  TRACE_EVR ("EVR: OnClockStop  hnsSystemTime = %I64d\n", hnsSystemTime);
  m_nRenderState    = Stopped;

  m_ModeratedClockLast = -1;
  m_ModeratedTimeLast = -1;
  return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockPause(MFTIME hnsSystemTime)
{
  TRACE_EVR ("EVR: OnClockPause  hnsSystemTime = %I64d\n", hnsSystemTime);
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
  TRACE_EVR ("EVR: OnClockRestart  hnsSystemTime = %I64d\n", hnsSystemTime);

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
    unsigned int nSamples = std::max(m_nNbDXSurface / 2, 1U);
    if ((m_ScheduledSamples.GetCount() < nSamples || m_LastSampleOffset < -m_rtTimePerFrame*2) && !g_bNoDuration)
    {      
      *State = (FILTER_STATE)Paused;
      _ReturnValue = VFW_S_STATE_INTERMEDIATE;
      return true;
    }
    m_bSignaledStarvation = false;
  }
  return false;
}

// IQualProp
STDMETHODIMP CEVRAllocatorPresenter::get_FramesDroppedInRenderer(int *pcFrames)
{
  *pcFrames  = m_pcFrames;
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::get_FramesDrawn(int *pcFramesDrawn)
{
  *pcFramesDrawn = m_pcFramesDrawn;
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::get_AvgFrameRate(int *piAvgFrameRate)
{
  *piAvgFrameRate = (int)(m_fAvrFps * 100);
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::get_Jitter(int *iJitter)
{
  *iJitter = (int)((m_fJitterStdDev/10000.0) + 0.5);
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::get_AvgSyncOffset(int *piAvg)
{
  *piAvg = (int)((m_fSyncOffsetAvr/10000.0) + 0.5);
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::get_DevSyncOffset(int *piDev)
{
  *piDev = (int)((m_fSyncOffsetStdDev/10000.0) + 0.5);
  return S_OK;
}


// IMFRateSupport
STDMETHODIMP CEVRAllocatorPresenter::GetSlowestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate)
{
  // TODO : not finished...
  *pflRate = 0;
  return S_OK;
}
    
STDMETHODIMP CEVRAllocatorPresenter::GetFastestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate)
{
  HRESULT    hr = S_OK;
  float    fMaxRate = 0.0f;

  CAutoLock lock(this);

  CheckPointer(pflRate, E_POINTER);
  CheckHR(CheckShutdown());
    
  // Get the maximum forward rate.
  fMaxRate = GetMaxRate(fThin);

  // For reverse playback, swap the sign.
  if (eDirection == MFRATE_REVERSE)
    fMaxRate = -fMaxRate;

  *pflRate = fMaxRate;

  return hr;
}
    
STDMETHODIMP CEVRAllocatorPresenter::IsRateSupported(BOOL fThin, float flRate, float *pflNearestSupportedRate)
{
    // fRate can be negative for reverse playback.
    // pfNearestSupportedRate can be NULL.

    CAutoLock lock(this);

    HRESULT hr = S_OK;
    float   fMaxRate = 0.0f;
    float   fNearestRate = flRate;   // Default.

  CheckPointer (pflNearestSupportedRate, E_POINTER);
    CheckHR(hr = CheckShutdown());

    // Find the maximum forward rate.
    fMaxRate = GetMaxRate(fThin);

    if (fabsf(flRate) > fMaxRate)
    {
        // The (absolute) requested rate exceeds the maximum rate.
        hr = MF_E_UNSUPPORTED_RATE;

        // The nearest supported rate is fMaxRate.
        fNearestRate = fMaxRate;
        if (flRate < 0)
        {
            // For reverse playback, swap the sign.
            fNearestRate = -fNearestRate;
        }
    }

    // Return the nearest supported rate if the caller requested it.
    if (pflNearestSupportedRate != NULL)
        *pflNearestSupportedRate = fNearestRate;

    return hr;
}


float CEVRAllocatorPresenter::GetMaxRate(BOOL bThin)
{
  float   fMaxRate    = FLT_MAX;  // Default.
  UINT32  fpsNumerator  = 0, fpsDenominator = 0;
  UINT    MonitorRateHz  = 0; 

  if (!bThin && (m_pMediaType != NULL))
  {
    // Non-thinned: Use the frame rate and monitor refresh rate.
        
    // Frame rate:
    MFGetAttributeRatio(m_pMediaType, MF_MT_FRAME_RATE, 
      &fpsNumerator, &fpsDenominator);

    // Monitor refresh rate:
    MonitorRateHz = m_RefreshRate; // D3DDISPLAYMODE

    if (fpsDenominator && fpsNumerator && MonitorRateHz)
    {
      // Max Rate = Refresh Rate / Frame Rate
      fMaxRate = (float)MulDiv(
        MonitorRateHz, fpsDenominator, fpsNumerator);
    }
  }
  return fMaxRate;
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

// IMFVideoPresenter
STDMETHODIMP CEVRAllocatorPresenter::ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam)
{
  HRESULT            hr = S_OK;

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
    m_drawingIsDone.Set();

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


HRESULT CEVRAllocatorPresenter::IsMediaTypeSupported(IMFMediaType* pMixerType)
{
  HRESULT        hr;
  AM_MEDIA_TYPE*    pAMMedia;
  UINT        nInterlaceMode;

  CheckHR (pMixerType->GetRepresentation(FORMAT_VideoInfo2, (void**)&pAMMedia));
  CheckHR (pMixerType->GetUINT32 (MF_MT_INTERLACE_MODE, &nInterlaceMode));


/*  if ( (pAMMedia->majortype != MEDIATYPE_Video) ||
     (nInterlaceMode != MFVideoInterlace_Progressive) ||
     ( (pAMMedia->subtype != MEDIASUBTYPE_RGB32) && (pAMMedia->subtype != MEDIASUBTYPE_RGB24) &&
       (pAMMedia->subtype != MEDIASUBTYPE_YUY2)  && (pAMMedia->subtype != MEDIASUBTYPE_NV12) ) )
       hr = MF_E_INVALIDMEDIATYPE;*/
  if ( (pAMMedia->majortype != MEDIATYPE_Video))
    hr = MF_E_INVALIDMEDIATYPE;
  pMixerType->FreeRepresentation (FORMAT_VideoInfo2, (void*)pAMMedia);
  return hr;
}


HRESULT CEVRAllocatorPresenter::CreateProposedOutputType(IMFMediaType* pMixerType, IMFMediaType** pType)
{
  HRESULT        hr;
  AM_MEDIA_TYPE*    pAMMedia = NULL;
  LARGE_INTEGER    i64Size;
  MFVIDEOFORMAT*    VideoFormat;

  CheckHR (pMixerType->GetRepresentation  (FORMAT_MFVideoFormat, (void**)&pAMMedia));
  
  VideoFormat = (MFVIDEOFORMAT*)pAMMedia->pbFormat;
  hr = pfMFCreateVideoMediaType(VideoFormat, &m_pMediaType);

  if (0)
  {
    // This code doesn't work, use same method as VMR9 instead
    if (VideoFormat->videoInfo.FramesPerSecond.Numerator != 0)
    {
      switch (VideoFormat->videoInfo.InterlaceMode)
      {
        case MFVideoInterlace_Progressive:
        case MFVideoInterlace_MixedInterlaceOrProgressive:
        default:
          {
            m_rtTimePerFrame = (10000000I64*VideoFormat->videoInfo.FramesPerSecond.Denominator)/VideoFormat->videoInfo.FramesPerSecond.Numerator;
            m_bInterlaced = false;
          }
          break;
        case MFVideoInterlace_FieldSingleUpper:
        case MFVideoInterlace_FieldSingleLower:
        case MFVideoInterlace_FieldInterleavedUpperFirst:
        case MFVideoInterlace_FieldInterleavedLowerFirst:
          {
            m_rtTimePerFrame = (20000000I64*VideoFormat->videoInfo.FramesPerSecond.Denominator)/VideoFormat->videoInfo.FramesPerSecond.Numerator;
            m_bInterlaced = true;
          }
          break;
      }
    }
  }

  m_AspectRatio.cx  = VideoFormat->videoInfo.PixelAspectRatio.Numerator;
  m_AspectRatio.cy  = VideoFormat->videoInfo.PixelAspectRatio.Denominator;

  if (SUCCEEDED (hr))
  {
    i64Size.HighPart = VideoFormat->videoInfo.dwWidth;
    i64Size.LowPart   = VideoFormat->videoInfo.dwHeight;
    m_pMediaType->SetUINT64 (MF_MT_FRAME_SIZE, i64Size.QuadPart);

    m_pMediaType->SetUINT32 (MF_MT_PAN_SCAN_ENABLED, 0);
    
    if ( ((CEVRRendererSettings *)g_dsSettings.pRendererSettings)->outputRange == OUTPUT_RANGE_16_235)
      m_pMediaType->SetUINT32 (MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_16_235);
    else
      m_pMediaType->SetUINT32 (MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_0_255);

    m_LastSetOutputRange = ((CEVRRendererSettings *)g_dsSettings.pRendererSettings)->outputRange;

    i64Size.HighPart = m_AspectRatio.cx;
    i64Size.LowPart  = m_AspectRatio.cy;
    m_pMediaType->SetUINT64 (MF_MT_PIXEL_ASPECT_RATIO, i64Size.QuadPart);

    MFVideoArea Area = MakeArea (0, 0, VideoFormat->videoInfo.dwWidth, VideoFormat->videoInfo.dwHeight);
    m_pMediaType->SetBlob(MF_MT_GEOMETRIC_APERTURE, (UINT8*)&Area, sizeof(MFVideoArea));

  }

  m_AspectRatio.cx  *= VideoFormat->videoInfo.dwWidth;
  m_AspectRatio.cy  *= VideoFormat->videoInfo.dwHeight;

  bool bDoneSomething = true;

  if(m_AspectRatio.cx >= 1 && m_AspectRatio.cy >= 1){ //if any of these is 0, it will stuck into a infinite loop
    while (bDoneSomething)
    {
      bDoneSomething = false;
      int MinNum = std::min(m_AspectRatio.cx, m_AspectRatio.cy);
      int i = 0;
      for (i = 2; i < MinNum+1; ++i)
      {
        if (m_AspectRatio.cx%i == 0 && m_AspectRatio.cy%i ==0)
          break;
      }
      if (i != MinNum + 1)
      {
        m_AspectRatio.cx = m_AspectRatio.cx / i;
        m_AspectRatio.cy = m_AspectRatio.cy / i;
        bDoneSomething = true;
      }
    }
  }
  
  pMixerType->FreeRepresentation (FORMAT_MFVideoFormat, (void*)pAMMedia);
  m_pMediaType->QueryInterface (__uuidof(IMFMediaType), (void**) pType);

  return hr;
}

HRESULT CEVRAllocatorPresenter::SetMediaType(IMFMediaType* pType)
{
  HRESULT        hr;
  AM_MEDIA_TYPE*    pAMMedia = NULL;
  CStdString        strTemp;

  CheckPointer (pType, E_POINTER);
  CheckHR (pType->GetRepresentation(FORMAT_VideoInfo2, (void**)&pAMMedia));
  
  hr = InitializeDevice (pAMMedia);
  if (SUCCEEDED (hr))
  {
    strTemp = GetMediaTypeName (pAMMedia->subtype);
    strTemp.Replace ("MEDIASUBTYPE_", "");
    m_strStatsMsg[MSG_MIXEROUT].Format ("Mixer output : %s", strTemp);
  }

  pType->FreeRepresentation (FORMAT_VideoInfo2, (void*)pAMMedia);

  return hr;
}

int64_t GetMediaTypeMerit(IMFMediaType *pMediaType)
{
  AM_MEDIA_TYPE*    pAMMedia = NULL;
  MFVIDEOFORMAT*    VideoFormat;

  HRESULT hr;
  CheckHR (pMediaType->GetRepresentation  (FORMAT_MFVideoFormat, (void**)&pAMMedia));
  VideoFormat = (MFVIDEOFORMAT*)pAMMedia->pbFormat;

  int64_t Merit = 0;
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

HRESULT CEVRAllocatorPresenter::RenegotiateMediaType()
{
    HRESULT      hr = S_OK;

    Com::SmartPtr<IMFMediaType>  pMixerType;
    Com::SmartPtr<IMFMediaType>  pType;

    if (!m_pMixer)
    {
        return MF_E_INVALIDREQUEST;
    }

    std::vector<Com::SmartPtr<IMFMediaType>> ValidMixerTypes;

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
        int64_t Merit = GetMediaTypeMerit(pType);

        int nTypes = ValidMixerTypes.size();
        int iInsertPos = 0;
        for (int i = 0; i < nTypes; ++i)
        {
          int64_t ThisMerit = GetMediaTypeMerit(ValidMixerTypes.at(i));
          if (Merit > ThisMerit)
          {
            iInsertPos = i;
            break;
          }
          else
            iInsertPos = i+1;
        }
        ValidMixerTypes.insert(ValidMixerTypes.begin() + iInsertPos, pType);
      }
    }


    int nValidTypes = ValidMixerTypes.size();
    for (int i = 0; i < nValidTypes; ++i)
    {
      // Step 3. Adjust the mixer's type to match our requirements.
      pType = ValidMixerTypes[i];    
      TRACE_EVR("EVR: Trying mixer output type: %ws\n", GetMediaTypeFormatDesc(pType));

      // Step 5. Try to set the media type on ourselves.
      hr = SetMediaType(pType);

      // Step 6. Set output media type on mixer.
      if (SUCCEEDED(hr))
      {
        hr = m_pMixer->SetOutputType(0, pType, 0);

        // If something went wrong, clear the media type.
        if (FAILED(hr))
          SetMediaType(NULL);
        else
          break;
      }
    }

    pMixerType  = NULL;
    pType    = NULL;
    return hr;
}


bool CEVRAllocatorPresenter::GetImageFromMixer()
{
  MFT_OUTPUT_DATA_BUFFER    Buffer;
  HRESULT                   hr = S_OK;
  DWORD                     dwStatus;
  REFERENCE_TIME            nsSampleTime;
  int64_t                   llClockBefore = 0;
  int64_t                   llClockAfter  = 0;
  int64_t                   llMixerLatency;
  UINT                      dwSurface;
  bool                      bDoneSomething = false;

  while (SUCCEEDED(hr))
  {
    Com::SmartPtr<IMFSample>    pSample;

    if (FAILED (GetFreeSample (&pSample)))
    {
      m_bWaitingSample = true;
      break;
    }

    memset (&Buffer, 0, sizeof(Buffer));
    Buffer.pSample  = pSample;
    pSample->GetUINT32 (GUID_SURFACE_INDEX, &dwSurface);

    {
      llClockBefore = CTimeUtils::GetPerfCounter();
      hr = m_pMixer->ProcessOutput (0 , 1, &Buffer, &dwStatus);
      llClockAfter = CTimeUtils::GetPerfCounter();
    }

    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) 
    {
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
    //Dont give a fuck about the tearing test
    /*if (AfxGetMyApp()->m_fTearingTest)
    {
      RECT    rcTearing;
      
      rcTearing.left    = m_nTearingPos;
      rcTearing.top    = 0;
      rcTearing.right    = rcTearing.left + 4;
      rcTearing.bottom  = m_NativeVideoSize.cy;
      m_pD3DDev->ColorFill (m_pVideoSurface[dwSurface], &rcTearing, D3DCOLOR_ARGB (255,255,0,0));

      rcTearing.left  = (rcTearing.right + 15) % m_NativeVideoSize.cx;
      rcTearing.right  = rcTearing.left + 4;
      m_pD3DDev->ColorFill (m_pVideoSurface[dwSurface], &rcTearing, D3DCOLOR_ARGB (255,255,0,0));
      m_nTearingPos = (m_nTearingPos + 7) % m_NativeVideoSize.cx;
    }  */

    int64_t TimePerFrame = m_rtTimePerFrame;
    TRACE_EVR ("EVR: Get from Mixer : %d  (%I64d) (%I64d)\n", dwSurface, nsSampleTime, TimePerFrame!=0?nsSampleTime/TimePerFrame:0);

    MoveToScheduledList (pSample, false);
    bDoneSomething = true;
    if (m_rtTimePerFrame == 0)
      break;
  }

  return bDoneSomething;
}



STDMETHODIMP CEVRAllocatorPresenter::GetCurrentMediaType(__deref_out  IMFVideoMediaType **ppMediaType)
{
    HRESULT hr = S_OK;
    CAutoLock lock(this);  // Hold the critical section.

    CheckPointer (ppMediaType, E_POINTER);
    CheckHR (CheckShutdown());

    if (m_pMediaType == NULL)
        CheckHR(MF_E_NOT_INITIALIZED);

    CheckHR(m_pMediaType->QueryInterface( __uuidof(IMFVideoMediaType), (void**)&ppMediaType));

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
    return m_pD3DManager->QueryInterface (__uuidof(IDirect3DDeviceManager9), (void**) ppvObject);

  return E_NOINTERFACE;
}


// IMFAsyncCallback
STDMETHODIMP CEVRAllocatorPresenter::GetParameters(  /* [out] */ __RPC__out DWORD *pdwFlags, /* [out] */ __RPC__out DWORD *pdwQueue)
{
  return E_NOTIMPL;
}

STDMETHODIMP CEVRAllocatorPresenter::Invoke     (  /* [in] */ __RPC__in_opt IMFAsyncResult *pAsyncResult)
{
  return E_NOTIMPL;
}


// IMFVideoDisplayControl
STDMETHODIMP CEVRAllocatorPresenter::GetNativeVideoSize(SIZE *pszVideo, SIZE *pszARVideo)
{
  if (pszVideo)
  {
    pszVideo->cx  = m_NativeVideoSize.cx;
    pszVideo->cy  = m_NativeVideoSize.cy;
  }
  if (pszARVideo)
  {
    pszARVideo->cx  = m_NativeVideoSize.cx * m_AspectRatio.cx;
    pszARVideo->cy  = m_NativeVideoSize.cy * m_AspectRatio.cy;
  }
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::GetIdealVideoSize(SIZE *pszMin, SIZE *pszMax)
{
  if (pszMin)
  {
    pszMin->cx  = 1;
    pszMin->cy  = 1;
  }

  if (pszMax)
  {
    D3DDISPLAYMODE  d3ddm;

    ZeroMemory(&d3ddm, sizeof(d3ddm));
    if(SUCCEEDED(m_pD3D->GetAdapterDisplayMode(GetAdapter(m_pD3D), &d3ddm)))
    {
      pszMax->cx  = d3ddm.Width;
      pszMax->cy  = d3ddm.Height;
    }
  }

  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::SetVideoPosition(const MFVideoNormalizedRect *pnrcSource, const LPRECT prcDest)
{
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::GetVideoPosition(MFVideoNormalizedRect *pnrcSource, LPRECT prcDest)
{
  // Always all source rectangle ?
  if (pnrcSource)
  {
    pnrcSource->left  = 0.0;
    pnrcSource->top    = 0.0;
    pnrcSource->right  = 1.0;
    pnrcSource->bottom  = 1.0;
  }

  if (prcDest)
    memcpy (prcDest, &m_VideoRect, sizeof(m_VideoRect));//GetClientRect (m_hWnd, prcDest);

  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::SetAspectRatioMode(DWORD dwAspectRatioMode)
{
  m_dwVideoAspectRatioMode = (MFVideoAspectRatioMode)dwAspectRatioMode;
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::GetAspectRatioMode(DWORD *pdwAspectRatioMode)
{
  CheckPointer (pdwAspectRatioMode, E_POINTER);
  *pdwAspectRatioMode = m_dwVideoAspectRatioMode;
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::SetVideoWindow(HWND hwndVideo)
{
  ASSERT (m_hWnd == hwndVideo);  // What if not ??
//  m_hWnd = hwndVideo;
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::GetVideoWindow(HWND *phwndVideo)
{
  CheckPointer (phwndVideo, E_POINTER);
  *phwndVideo = m_hWnd;
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::RepaintVideo()
{
  assert(false);
  Paint (true);
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::GetCurrentImage(BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, int64_t *pTimeStamp)
{
  ASSERT (FALSE);
  return E_NOTIMPL;
}
STDMETHODIMP CEVRAllocatorPresenter::SetBorderColor(COLORREF Clr)
{
  m_BorderColor = Clr;
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::GetBorderColor(COLORREF *pClr)
{
  CheckPointer (pClr, E_POINTER);
  *pClr = m_BorderColor;
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::SetRenderingPrefs(DWORD dwRenderFlags)
{
  m_dwVideoRenderPrefs = (MFVideoRenderPrefs)dwRenderFlags;
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::GetRenderingPrefs(DWORD *pdwRenderFlags)
{
  CheckPointer(pdwRenderFlags, E_POINTER);
  *pdwRenderFlags = m_dwVideoRenderPrefs;
  return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::SetFullscreen(BOOL fFullscreen)
{
  ASSERT (FALSE);
  return E_NOTIMPL;
}
STDMETHODIMP CEVRAllocatorPresenter::GetFullscreen(BOOL *pfFullscreen)
{
  ASSERT (FALSE);
  return E_NOTIMPL;
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
  HRESULT    hr = m_pD3DManager->ResetDevice (pDevice, resetToken);
  return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::OpenDeviceHandle(HANDLE *phDevice)
{
  HRESULT    hr = m_pD3DManager->OpenDeviceHandle (phDevice);
  return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::CloseDeviceHandle(HANDLE hDevice)
{
  HRESULT    hr = m_pD3DManager->CloseDeviceHandle(hDevice);
  return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::TestDevice(HANDLE hDevice)
{
  HRESULT    hr = m_pD3DManager->TestDevice(hDevice);
  return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::LockDevice(HANDLE hDevice, IDirect3DDevice9 **ppDevice, BOOL fBlock)
{
  HRESULT    hr = m_pD3DManager->LockDevice(hDevice, ppDevice, fBlock);
  return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::UnlockDevice(HANDLE hDevice, BOOL fSaveState)
{
  HRESULT    hr = m_pD3DManager->UnlockDevice(hDevice, fSaveState);
  return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::GetVideoService(HANDLE hDevice, REFIID riid, void **ppService)
{
  HRESULT    hr = m_pD3DManager->GetVideoService(hDevice, riid, ppService);

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


STDMETHODIMP CEVRAllocatorPresenter::GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
{
  // This function should be called...
  ASSERT (FALSE);

  if(lpWidth)    *lpWidth  = m_NativeVideoSize.cx;
  if(lpHeight)  *lpHeight  = m_NativeVideoSize.cy;
  if(lpARWidth)  *lpARWidth  = m_AspectRatio.cx;
  if(lpARHeight)  *lpARHeight  = m_AspectRatio.cy;
  return S_OK;
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
  int                     w = vih2->bmiHeader.biWidth;
  int                     h = abs(vih2->bmiHeader.biHeight);

  m_NativeVideoSize = Com::SmartSize(w, h);
  if (m_bHighColorResolution)
    hr = AllocSurfaces(D3DFMT_A2R10G10B10);
  else
    hr = AllocSurfaces(D3DFMT_X8R8G8B8);
  

  for(size_t i = 0; i < m_nNbDXSurface; i++)
  {
    Com::SmartPtr<IMFSample>    pMFSample;
    hr = pfMFCreateVideoSampleFromSurface (m_pVideoSurface[i], &pMFSample);

    if (SUCCEEDED (hr))
    {
      pMFSample->SetUINT32 (GUID_SURFACE_INDEX, i);
      m_FreeSamples.InsertBack(pMFSample);
    }
    ASSERT (SUCCEEDED (hr));
  }
  return hr;
}


DWORD WINAPI CEVRAllocatorPresenter::GetMixerThreadStatic(LPVOID lpParam)
{
  //SetThreadName((DWORD)-1, "CEVRPresenter::MixerThread");
  CEVRAllocatorPresenter*    pThis = (CEVRAllocatorPresenter*) lpParam;
  pThis->GetMixerThread();
  return 0;
}


DWORD WINAPI CEVRAllocatorPresenter::PresentThread(LPVOID lpParam)
{
  //SetThreadName((DWORD)-1, "CEVRPresenter::PresentThread");
  CEVRAllocatorPresenter*    pThis = (CEVRAllocatorPresenter*) lpParam;
  pThis->RenderThread();
  return 0;
}


void CEVRAllocatorPresenter::CheckWaitingSampleFromMixer()
{
  if (m_bWaitingSample)
  {
    m_bWaitingSample = false;
    //GetImageFromMixer(); // Do this in processing thread instead
  }
}


bool ExtractInterlaced(const AM_MEDIA_TYPE* pmt)
{
  if (pmt->formattype==FORMAT_VideoInfo)
    return false;
  else if (pmt->formattype==FORMAT_VideoInfo2)
    return (((VIDEOINFOHEADER2*)pmt->pbFormat)->dwInterlaceFlags & AMINTERLACE_IsInterlaced) != 0;
  else if (pmt->formattype==FORMAT_MPEGVideo)
    return false;
  else if (pmt->formattype==FORMAT_MPEG2Video)
    return (((MPEG2VIDEOINFO*)pmt->pbFormat)->hdr.dwInterlaceFlags & AMINTERLACE_IsInterlaced) != 0;
  else
    return false;
}


void CEVRAllocatorPresenter::GetMixerThread()
{
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
    dwResolution = std::min( std::max(tc.wPeriodMin, (UINT) 0), tc.wPeriodMax);
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
          //CAutoLock lock(this);
          //CAutoLock lock2(&m_ImageProcessingLock);
          //CAutoLock cRenderLock(&m_RenderLock);

          // Use the code from VMR9 to get the movie fps, as this method is reliable.
          Com::SmartPtr<IPin>      pPin;
          CMediaType        mt;
          if (
            SUCCEEDED (m_pOuterEVR->FindPin(L"EVR Input0", &pPin)) &&
            SUCCEEDED (pPin->ConnectionMediaType(&mt)) )
          {
            ExtractAvgTimePerFrame (&mt, m_rtTimePerFrame);

            m_bInterlaced = ExtractInterlaced(&mt);

          }
          // If framerate not set by Video Decoder choose 23.97...
          if (m_rtTimePerFrame == 0) 
            m_rtTimePerFrame = 417166;

          m_fps = (float)(10000000.0 / m_rtTimePerFrame);
          if (!g_renderManager.IsConfigured())
          {
            g_renderManager.Configure(m_NativeVideoSize.cx, m_NativeVideoSize.cy, m_AspectRatio.cx, m_AspectRatio.cy, m_fps,
              CONF_FLAGS_FULLSCREEN, 0);
            CLog::Log(LOGDEBUG, "%s Render manager configured (FPS: %f)", __FUNCTION__, m_fps);
          }
        }

      }
      break;
    }
  }

  timeEndPeriod (dwResolution);
//  if (pfAvRevertMmThreadCharacteristics) pfAvRevertMmThreadCharacteristics (hAvrt);
}

void ModerateFloat(double& Value, double Target, double& ValuePrim, double ChangeSpeed)
{
  double xbiss = (-(ChangeSpeed)*(ValuePrim) - (Value-Target)*(ChangeSpeed*ChangeSpeed)*0.25f);
  ValuePrim += xbiss;
  Value += ValuePrim;
}

int64_t CEVRAllocatorPresenter::GetClockTime(int64_t PerformanceCounter)
{
  int64_t       llClockTime;
  MFTIME        nsCurrentTime;
  if (! m_pClock)
    return 0;

  m_pClock->GetCorrelatedTime(0, &llClockTime, &nsCurrentTime);
  DWORD Characteristics = 0;
  m_pClock->GetClockCharacteristics(&Characteristics);
  MFCLOCK_STATE State;
  m_pClock->GetState(0, &State);

  if (!(Characteristics & MFCLOCK_CHARACTERISTICS_FLAG_FREQUENCY_10MHZ))
  {
    MFCLOCK_PROPERTIES Props;
    if (m_pClock->GetProperties(&Props) == S_OK)
      llClockTime = (llClockTime * 10000000) / Props.qwClockFrequency; // Make 10 MHz

  }
  int64_t llPerf = PerformanceCounter;
//  return llClockTime + (llPerf - nsCurrentTime);
  double Target = llClockTime + (llPerf - nsCurrentTime) * m_ModeratedTimeSpeed;

  bool bReset = false;
  if (m_ModeratedTimeLast < 0 || State != m_LastClockState || m_ModeratedClockLast < 0)
  {
    bReset = true;
    m_ModeratedTimeLast = llPerf;
    m_ModeratedClockLast = llClockTime;
  }

  m_LastClockState = State;

  double TimeChange = (double) llPerf - m_ModeratedTimeLast;
  double ClockChange = (double) llClockTime - m_ModeratedClockLast;

  m_ModeratedTimeLast = llPerf;
  m_ModeratedClockLast = llClockTime;

#if 1

  if (bReset)
  {
    m_ModeratedTimeSpeed = 1.0;
    m_ModeratedTimeSpeedPrim = 0.0;
    ZeroMemory(m_TimeChangeHistory, sizeof(m_TimeChangeHistory));
    ZeroMemory(m_ClockChangeHistory, sizeof(m_ClockChangeHistory));
    m_ClockTimeChangeHistoryPos = 0;
  }
  if (TimeChange)
  {
    int Pos = m_ClockTimeChangeHistoryPos % 100;
    int nHistory = std::min(m_ClockTimeChangeHistoryPos, 100);
    ++m_ClockTimeChangeHistoryPos;
    if (nHistory > 50)
    {
      int iLastPos = (Pos - (nHistory)) % 100;
      if (iLastPos < 0)
        iLastPos += 100;

      double TimeChange = llPerf - m_TimeChangeHistory[iLastPos];
      double ClockChange = llClockTime - m_ClockChangeHistory[iLastPos];

      double ClockSpeedTarget = ClockChange / TimeChange;
      double ChangeSpeed = 0.1;
      if (ClockSpeedTarget > m_ModeratedTimeSpeed)
      {
        if (ClockSpeedTarget / m_ModeratedTimeSpeed > 0.1)
          ChangeSpeed = 0.1;
        else
          ChangeSpeed = 0.01;
      }
      else 
      {
        if (m_ModeratedTimeSpeed / ClockSpeedTarget > 0.1)
          ChangeSpeed = 0.1;
        else
          ChangeSpeed = 0.01;
      }
      ModerateFloat(m_ModeratedTimeSpeed, ClockSpeedTarget, m_ModeratedTimeSpeedPrim, ChangeSpeed);
//      m_ModeratedTimeSpeed = TimeChange / ClockChange;
    }
    m_TimeChangeHistory[Pos] = (double) llPerf;
    m_ClockChangeHistory[Pos] = (double) llClockTime;
  }

  return (int64_t) Target;
#else
  double EstimateTime = m_ModeratedTime + TimeChange * m_ModeratedTimeSpeed + m_ClockDiffCalc;
  double Diff = Target - EstimateTime;
  
  // > 5 ms just set it
  if ((fabs(Diff) > 50000.0 || bReset))
  {

//    TRACE_EVR("EVR: Reset clock at diff: %f ms\n", (m_ModeratedTime - Target) /10000.0);
    if (State == MFCLOCK_STATE_RUNNING)
    {
      if (bReset)
      {
        m_ModeratedTimeSpeed = 1.0;
        m_ModeratedTimeSpeedPrim = 0.0;
        m_ClockDiffCalc = 0;
        m_ClockDiffPrim = 0;
        m_ModeratedTime = Target;
        m_ModeratedTimer = llPerf;
      }
      else
      {
        EstimateTime = m_ModeratedTime + TimeChange * m_ModeratedTimeSpeed;
        Diff = Target - EstimateTime;
        m_ClockDiffCalc = Diff;
        m_ClockDiffPrim = 0;
      }
    }
    else
    {
      m_ModeratedTimeSpeed = 0.0;
      m_ModeratedTimeSpeedPrim = 0.0;
      m_ClockDiffCalc = 0;
      m_ClockDiffPrim = 0;
      m_ModeratedTime = Target;
      m_ModeratedTimer = llPerf;
    }
  }

  {
    int64_t ModerateTime = 10000;
    double ChangeSpeed = 1.00;
/*    if (m_ModeratedTimeSpeedPrim != 0.0)
    {
      if (m_ModeratedTimeSpeedPrim < 0.1)
        ChangeSpeed = 0.1;
    }*/

    int nModerate = 0;
    double Change = 0;
    while (m_ModeratedTimer < llPerf - ModerateTime)
    {
      m_ModeratedTimer += ModerateTime;
      m_ModeratedTime += double(ModerateTime) * m_ModeratedTimeSpeed;

      double TimerDiff = llPerf - m_ModeratedTimer;

      double Diff = (double)(m_ModeratedTime - (Target - TimerDiff));

      double TimeSpeedTarget;
      double AbsDiff = fabs(Diff);
      TimeSpeedTarget = 1.0 - (Diff / 1000000.0);
//      TimeSpeedTarget = m_ModeratedTimeSpeed - (Diff / 100000000000.0);
      //if (AbsDiff > 20000.0)
//        TimeSpeedTarget = 1.0 - (Diff / 1000000.0);
      /*else if (AbsDiff > 5000.0)
        TimeSpeedTarget = 1.0 - (Diff / 100000000.0);
      else
        TimeSpeedTarget = 1.0 - (Diff / 500000000.0);*/
      double StartMod = m_ModeratedTimeSpeed;
      ModerateFloat(m_ModeratedTimeSpeed, TimeSpeedTarget, m_ModeratedTimeSpeedPrim, ChangeSpeed);
      m_ModeratedTimeSpeed = TimeSpeedTarget;
      ++nModerate;
      Change += m_ModeratedTimeSpeed - StartMod;
    }
    if (nModerate)
      m_ModeratedTimeSpeedDiff = Change / nModerate;
    
    double Ret = m_ModeratedTime + double(llPerf - m_ModeratedTimer) * m_ModeratedTimeSpeed;
    double Diff = Target - Ret;
    ModerateFloat(m_ClockDiffCalc, Diff, m_ClockDiffPrim, ChangeSpeed*0.1);
    
    Ret += m_ClockDiffCalc;
    Diff = Target - Ret;
    m_ClockDiff = Diff;
    return int64_t(Ret + 0.5);
  }

  return Target;
  return int64_t(m_ModeratedTime + 0.5);
#endif
}

void CEVRAllocatorPresenter::OnVBlankFinished(bool fAll, int64_t PerformanceCounter)
{
  if (m_pCurrentDisplaydSampleQueue.empty() || !m_OrderedPaint || !fAll)
    return;

  int64_t      llClockTime;
  int64_t      nsSampleTime;
  int64_t      SampleDuration = 0; 
  if (!m_bSignaledStarvation)
  {
    llClockTime = GetClockTime(PerformanceCounter);
    m_StarvationClock = llClockTime;
  }
  else
  {
    llClockTime = m_StarvationClock;
  }

  IMFSample * currentSample = NULL;
  {
    CSingleLock lock(m_DisplaydSampleQueueLock);
    assert(m_pCurrentDisplaydSampleQueue.size() == 1);
    currentSample = m_pCurrentDisplaydSampleQueue.front(); m_pCurrentDisplaydSampleQueue.pop();
  }
  currentSample->GetSampleDuration(&SampleDuration);

  currentSample->GetSampleTime(&nsSampleTime);
  int64_t TimePerFrame = m_rtTimePerFrame;
  if (!TimePerFrame)
    return;
  if (SampleDuration > 1)
    TimePerFrame = SampleDuration;

  {
    m_nNextSyncOffset = (m_nNextSyncOffset+1) % NB_JITTER;
    int64_t SyncOffset = nsSampleTime - llClockTime;

    m_pllSyncOffset[m_nNextSyncOffset] = SyncOffset;
    TRACE_EVR("EVR: SyncOffset(%d, %d): %8I64d     %8I64d     %8I64d \n", m_nCurSurface, m_VSyncMode, m_LastPredictedSync, -SyncOffset, m_LastPredictedSync - (-SyncOffset));

    m_MaxSyncOffset = MINLONG64;
    m_MinSyncOffset = MAXLONG64;

    int64_t AvrageSum = 0;
    for (int i=0; i<NB_JITTER; i++)
    {
      int64_t Offset = m_pllSyncOffset[i];
      AvrageSum += Offset;
      m_MaxSyncOffset = std::max(m_MaxSyncOffset, Offset);
      m_MinSyncOffset = std::min(m_MinSyncOffset, Offset);
    }
    double MeanOffset = double(AvrageSum)/NB_JITTER;
    double DeviationSum = 0;
    for (int i=0; i<NB_JITTER; i++)
    {
      double Deviation = double(m_pllSyncOffset[i]) - MeanOffset;
      DeviationSum += Deviation*Deviation;
    }
    double StdDev = sqrt(DeviationSum/NB_JITTER);

    m_fSyncOffsetAvr = MeanOffset;
     m_bSyncStatsAvailable = true;
    m_fSyncOffsetStdDev = StdDev;

    
  }
}

void CEVRAllocatorPresenter::RenderThread()
{
  HANDLE        hAvrt;
  DWORD         dwTaskIndex   = 0;
  HANDLE        hEvts[]       = { m_hEvtQuit, m_hEvtFlush};
  bool          bQuit         = false;
  TIMECAPS      tc;
  DWORD         dwResolution;
  MFTIME        nsSampleTime;
  int64_t       llClockTime;
  DWORD         dwUser = 0;
  DWORD         dwObject;

  
  // Tell Vista Multimedia Class Scheduler we are a playback thretad (increase priority)
  if (pfAvSetMmThreadCharacteristicsW)  hAvrt = pfAvSetMmThreadCharacteristicsW (L"Playback", &dwTaskIndex);
  if (pfAvSetMmThreadPriority)      pfAvSetMmThreadPriority (hAvrt, AVRT_PRIORITY_HIGH /*AVRT_PRIORITY_CRITICAL*/);

    timeGetDevCaps(&tc, sizeof(TIMECAPS));
    dwResolution = std::min(std::max(tc.wPeriodMin, (UINT) 0), tc.wPeriodMax);
    dwUser    = timeBeginPeriod(dwResolution);

  int NextSleepTime = 1;
  while (!bQuit)
  {
    int64_t  llPerf = CTimeUtils::GetPerfCounter();
    if (! g_dsSettings.pRendererSettings->vSyncAccurate && NextSleepTime == 0)
      NextSleepTime = 1;
    dwObject = WaitForMultipleObjects (countof(hEvts), hEvts, FALSE, std::max(NextSleepTime < 0 ? 1 : NextSleepTime, 0));
/*    dwObject = WAIT_TIMEOUT;
    if (m_bEvtFlush)
      dwObject = WAIT_OBJECT_0 + 1;
    else if (m_bEvtQuit)
      dwObject = WAIT_OBJECT_0;*/
//    if (NextSleepTime)
//      TRACE_EVR("EVR: Sleep: %7.3f\n", double(AfxGetMyApp()->GetPerfCounter()-llPerf) / 10000.0);
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
      m_drawingIsDone.Reset();
      TRACE_EVR ("EVR: Flush done!\n");
      break;

    case WAIT_TIMEOUT :

      if (m_LastSetOutputRange != -1 && m_LastSetOutputRange != ((CEVRRendererSettings *)g_dsSettings.pRendererSettings)->outputRange || m_bPendingRenegotiate)
      {
        FlushSamples();
        RenegotiateMediaType();
        m_bPendingRenegotiate = false;
      }

      {
        Com::SmartPtr<IMFSample>    pMFSample;
        int64_t  llPerf = CTimeUtils::GetPerfCounter();
        int                         nSamplesLeft = 0;
        if (SUCCEEDED (GetScheduledSample(&pMFSample, nSamplesLeft)))
        { 
          m_pCurrentDisplaydSample = pMFSample;

          bool bValidSampleTime = true;
          HRESULT hGetSampleTime = pMFSample->GetSampleTime (&nsSampleTime);
          if (hGetSampleTime != S_OK || nsSampleTime == 0)
          {
            bValidSampleTime = false;
          }
          // We assume that all samples have the same duration
          int64_t SampleDuration = 0; 
          pMFSample->GetSampleDuration(&SampleDuration);

//          TRACE_EVR ("EVR: RenderThread ==>> Presenting surface %d  (%I64d)\n", m_nCurSurface, nsSampleTime);

          bool bStepForward = false;

          if (m_nStepCount < 0)
          {
            // Drop frame
            TRACE_EVR ("EVR: Dropped frame\n");
            m_pcFrames++;
            bStepForward = true;
            m_nStepCount = 0;
          }
          else if (m_nStepCount > 0)
          {
            pMFSample->GetUINT32(GUID_SURFACE_INDEX, (UINT32 *)&m_nCurSurface);
            ++m_OrderedPaint;
            if (!g_bExternalSubtitleTime)
              __super::SetTime (g_tSegmentStart + nsSampleTime);
            
            PaintInternal();

            m_nDroppedUpdate = 0;
            CompleteFrameStep (false);
            bStepForward = true;
          }
          else if ((m_nRenderState == Started))
          {
            int64_t CurrentCounter = CTimeUtils::GetPerfCounter();
            // Calculate wake up timer
            if (!m_bSignaledStarvation)
            {
              llClockTime = GetClockTime(CurrentCounter);
              m_StarvationClock = llClockTime;
            }
            else
            {
              llClockTime = m_StarvationClock;
            }

            if (!bValidSampleTime)
            {
              // Just play as fast as possible
              bStepForward = true;
              pMFSample->GetUINT32(GUID_SURFACE_INDEX, (UINT32 *)&m_nCurSurface);
              ++m_OrderedPaint;
              if (!g_bExternalSubtitleTime)
                __super::SetTime (g_tSegmentStart + nsSampleTime);
              
              PaintInternal();

            }
            else
            {
              int64_t TimePerFrame = (int64_t) (GetFrameTime() * 10000000);
              int64_t DrawTime = (int64_t) ((m_PaintTime) * 0.9 - 20000); // 2 ms offset
              //if (!s.iVMR9VSync)
                DrawTime = 0;

              int64_t SyncOffset = 0;
              int64_t VSyncTime = 0;
              int64_t TimeToNextVSync = -1;
              bool bVSyncCorrection = false;
              double DetectedRefreshTime;
              double DetectedScanlinesPerFrame;
              double DetectedScanlineTime;
              int DetectedRefreshRatePos;
              {
                CAutoLock Lock(&m_RefreshRateLock);  
                DetectedRefreshTime = m_DetectedRefreshTime;
                DetectedRefreshRatePos = m_DetectedRefreshRatePos;
                DetectedScanlinesPerFrame = m_DetectedScanlinesPerFrame;
                DetectedScanlineTime = m_DetectedScanlineTime;
              }

              if (DetectedRefreshRatePos < 20 || !DetectedRefreshTime || !DetectedScanlinesPerFrame)
              {
                DetectedRefreshTime = 1.0/m_RefreshRate;
                DetectedScanlinesPerFrame = m_ScreenSize.cy;
                DetectedScanlineTime = DetectedRefreshTime / double(m_ScreenSize.cy);
              }

              if (g_dsSettings.pRendererSettings->vSync)
              {
                bVSyncCorrection = true;
                double TargetVSyncPos = GetVBlackPos();
                double RefreshLines = DetectedScanlinesPerFrame;
                double ScanlinesPerSecond = 1.0/DetectedScanlineTime;
                double CurrentVSyncPos = fmod(double(m_VBlankStartMeasure) + ScanlinesPerSecond * ((CurrentCounter - m_VBlankStartMeasureTime) / 10000000.0), RefreshLines);
                double LinesUntilVSync = 0;
                //TargetVSyncPos -= ScanlinesPerSecond * (DrawTime/10000000.0);
                //TargetVSyncPos -= 10;
                TargetVSyncPos = fmod(TargetVSyncPos, RefreshLines);
                if (TargetVSyncPos < 0)
                  TargetVSyncPos += RefreshLines;
                if (TargetVSyncPos > CurrentVSyncPos)
                  LinesUntilVSync = TargetVSyncPos - CurrentVSyncPos;
                else
                  LinesUntilVSync = (RefreshLines - CurrentVSyncPos) + TargetVSyncPos;
                double TimeUntilVSync = LinesUntilVSync * DetectedScanlineTime;
                TimeToNextVSync = (int64_t) (TimeUntilVSync * 10000000.0);
                VSyncTime = (int64_t) (DetectedRefreshTime * 10000000.0);

                int64_t ClockTimeAtNextVSync = llClockTime + (int64_t) ((TimeUntilVSync * 10000000.0) * m_ModeratedTimeSpeed);
  
                SyncOffset = (nsSampleTime - ClockTimeAtNextVSync);

//                if (SyncOffset < 0)
//                  TRACE_EVR("EVR: SyncOffset(%d): %I64d     %I64d     %I64d\n", m_nCurSurface, SyncOffset, TimePerFrame, VSyncTime);
              }
              else
                SyncOffset = (nsSampleTime - llClockTime);
              
              //int64_t SyncOffset = nsSampleTime - llClockTime;
              TRACE_EVR ("EVR: SyncOffset: %I64d SampleFrame: %I64d ClockFrame: %I64d\n", SyncOffset, TimePerFrame!=0 ? nsSampleTime/TimePerFrame : 0, TimePerFrame!=0 ? llClockTime /TimePerFrame : 0);
              if (SampleDuration > 1 && !m_DetectedLock)
                TimePerFrame = SampleDuration;

              int64_t MinMargin;
              if (m_FrameTimeCorrection && 0)
                MinMargin = 15000;
              else
                MinMargin = 15000 + std::min((int64_t) (m_DetectedFrameTimeStdDev), 20000i64);
              int64_t TimePerFrameMargin = (int64_t) std::min((__int64) (TimePerFrame * 0.11), std::max((__int64) (TimePerFrame * 0.02), MinMargin));
              int64_t TimePerFrameMargin0 = (int64_t) TimePerFrameMargin/2;
              int64_t TimePerFrameMargin1 = 0;

              if (m_DetectedLock && TimePerFrame < VSyncTime)
                VSyncTime = TimePerFrame;

              if (m_VSyncMode == 1)
                TimePerFrameMargin1 = -TimePerFrameMargin;
              else if (m_VSyncMode == 2)
                TimePerFrameMargin1 = TimePerFrameMargin;

              m_LastSampleOffset = SyncOffset;
              m_bLastSampleOffsetValid = true;

              int64_t VSyncOffset0 = 0;
              bool bDoVSyncCorrection = false;
              if ((SyncOffset < -(TimePerFrame + TimePerFrameMargin0 - TimePerFrameMargin1)) && nSamplesLeft > 0) // Only drop if we have something else to display at once
              {
                // Drop frame
                TRACE_EVR ("EVR: Dropped frame\n");
                m_pcFrames++;
                bStepForward = true;
                ++m_nDroppedUpdate;
                NextSleepTime = 0;
//                VSyncOffset0 = (-SyncOffset) - VSyncTime;
                //VSyncOffset0 = (-SyncOffset) - VSyncTime + TimePerFrameMargin1;
                //m_LastPredictedSync = VSyncOffset0;
                bDoVSyncCorrection = false;
              }
              else if (SyncOffset < TimePerFrameMargin1)
              {

                if (bVSyncCorrection)
                {
//                  VSyncOffset0 = -SyncOffset;
                  VSyncOffset0 = -SyncOffset;
                  bDoVSyncCorrection = true;
                }

                // Paint and prepare for next frame
                TRACE_EVR ("EVR: Normalframe\n");
                m_nDroppedUpdate = 0;
                bStepForward = true;
                pMFSample->GetUINT32(GUID_SURFACE_INDEX, (UINT32 *)&m_nCurSurface);
                m_LastFrameDuration = nsSampleTime - m_LastSampleTime;
                m_LastSampleTime = nsSampleTime;
                m_LastPredictedSync = VSyncOffset0;

                ++m_OrderedPaint;

                if (!g_bExternalSubtitleTime)
                  __super::SetTime (g_tSegmentStart + nsSampleTime);

                PaintInternal();
                
                NextSleepTime = 0;
                m_pcFramesDrawn++;
              }
              else
              {
                if (TimeToNextVSync >= 0 && SyncOffset > 0)
                {
                  NextSleepTime = (int) ((TimeToNextVSync)/10000) - 2;
                }
                else
                  NextSleepTime = (int) ((SyncOffset)/10000) - 2;

                if (NextSleepTime > TimePerFrame)
                  NextSleepTime = 1;

                if (NextSleepTime < 0)
                  NextSleepTime = 0;
                NextSleepTime = 1;
                TRACE_EVR ("EVR: Delay\n");
              }

              if (bDoVSyncCorrection)
              {
                //int64_t VSyncOffset0 = (((SyncOffset) % VSyncTime) + VSyncTime) % VSyncTime;
                int64_t Margin = TimePerFrameMargin;

                int64_t VSyncOffsetMin = 30000000000000;
                int64_t VSyncOffsetMax = -30000000000000;
                for (int i = 0; i < 5; ++i)
                {
                  VSyncOffsetMin = std::min(m_VSyncOffsetHistory[i], VSyncOffsetMin);
                  VSyncOffsetMax = std::max(m_VSyncOffsetHistory[i], VSyncOffsetMax);
                }

                m_VSyncOffsetHistory[m_VSyncOffsetHistoryPos] = VSyncOffset0;
                m_VSyncOffsetHistoryPos = (m_VSyncOffsetHistoryPos + 1) % 5;

//                int64_t VSyncTime2 = VSyncTime2 + (VSyncOffsetMax - VSyncOffsetMin);
                //VSyncOffsetMin; = (((VSyncOffsetMin) % VSyncTime) + VSyncTime) % VSyncTime;
                //VSyncOffsetMax = (((VSyncOffsetMax) % VSyncTime) + VSyncTime) % VSyncTime;

                TRACE_EVR("EVR: SyncOffset(%d, %d): %8I64d     %8I64d     %8I64d     %8I64d\n", m_nCurSurface, m_VSyncMode,VSyncOffset0, VSyncOffsetMin, VSyncOffsetMax, VSyncOffsetMax - VSyncOffsetMin);

                if (m_VSyncMode == 0)
                {
                  // 23.976 in 60 Hz
                  if (VSyncOffset0 < Margin && VSyncOffsetMax > (VSyncTime - Margin))
                  {
                    m_VSyncMode = 2;
                  }
                  else if (VSyncOffset0 > (VSyncTime - Margin) && VSyncOffsetMin < Margin)
                  {
                    m_VSyncMode = 1;
                  }
                }
                else if (m_VSyncMode == 2)
                {
                  if (VSyncOffsetMin > (Margin))
                  {
                    m_VSyncMode = 0;
                  }
                }
                else if (m_VSyncMode == 1)
                {
                  if (VSyncOffsetMax < (VSyncTime - Margin))
                  {
                    m_VSyncMode = 0;
                  }
                }
              }

            }
          }
          m_pCurrentDisplaydSample = NULL;
          if (bStepForward)
          {
            MoveToFreeList(pMFSample, true);
            CheckWaitingSampleFromMixer();
            m_MaxSampleDuration = std::max(SampleDuration, m_MaxSampleDuration);
          }
          else
            MoveToScheduledList(pMFSample, true);
        }
        else if (m_bLastSampleOffsetValid && m_LastSampleOffset < -10000000) // Only starve if we are 1 seconds behind
        {
          if (m_nRenderState == Started && !g_bNoDuration)
          {
            m_pSink->Notify(EC_STARVATION, 0, 0);
            m_bSignaledStarvation = true;
          }
        }
        //GetImageFromMixer();
      }
//      else
//      {        
//        TRACE_EVR ("EVR: RenderThread ==>> Flush before rendering frame!\n");
//      }

      break;
    }
  }

  timeEndPeriod (dwResolution);
  if (pfAvRevertMmThreadCharacteristics) pfAvRevertMmThreadCharacteristics (hAvrt);
}

void CEVRAllocatorPresenter::PaintInternal()
{
  {
    CSingleLock lock(m_DisplaydSampleQueueLock);
    // Since the Drawing isn't in the same thread that rendering, we need to keep track of current displayed sample
    if (m_pCurrentDisplaydSampleQueue.empty() || m_pCurrentDisplaydSampleQueue.back() != m_pCurrentDisplaydSample)
      m_pCurrentDisplaydSampleQueue.push(m_pCurrentDisplaydSample); //add only one time the sample to the queue
  }

  m_drawingIsDone.Reset();
  g_application.NewFrame();
  m_drawingIsDone.Wait(); // Wait until the drawing is done

  {
    CSingleLock lock(m_DisplaydSampleQueueLock);
    while (! m_pCurrentDisplaydSampleQueue.empty())
      m_pCurrentDisplaydSampleQueue.pop();
  }
}

void CEVRAllocatorPresenter::BeforeDeviceReset()
{
  // Pause playback
  g_dsGraph->Pause();

  this->Lock();
  m_ImageProcessingLock.Lock();
  m_RenderLock.Lock();

  // The device is going to be recreated, free ressources
  RemoveAllSamples();
  __super::BeforeDeviceReset();
}

void CEVRAllocatorPresenter::AfterDeviceReset()
{
  __super::AfterDeviceReset();

  HRESULT hr = m_pD3DManager->ResetDevice(m_pD3DDev, m_nResetToken);
  ASSERT(SUCCEEDED(hr));

  // Not necessary, but Microsoft documentation say Presenter should send this message...
  if (m_pSink)
    m_pSink->Notify (EC_DISPLAY_CHANGED, 0, 0);

  for(size_t i = 0; i < m_nNbDXSurface; i++)
  {
    Com::SmartPtr<IMFSample>  pMFSample;
    HRESULT hr = pfMFCreateVideoSampleFromSurface (m_pVideoSurface[i], &pMFSample);

    if (SUCCEEDED (hr))
    {
      pMFSample->SetUINT32 (GUID_SURFACE_INDEX, i);
      m_FreeSamples.InsertBack(pMFSample);
    }
    ASSERT (SUCCEEDED (hr));
  }

  m_RenderLock.Unlock();
  m_ImageProcessingLock.Unlock();
  this->Unlock();

  // Restart playback
  g_dsGraph->Play();
}



void CEVRAllocatorPresenter::RemoveAllSamples()
{
  CAutoLock AutoLock(&m_ImageProcessingLock);

  FlushSamples();
  m_ScheduledSamples.Clear();
  m_FreeSamples.Clear();

  {
    CSingleLock lock(m_DisplaydSampleQueueLock);
    while (!m_pCurrentDisplaydSampleQueue.empty())
      m_pCurrentDisplaydSampleQueue.pop();
  }

  m_LastScheduledSampleTime = -1;
  m_LastScheduledUncorrectedSampleTime = -1;
  //ASSERT(m_nUsedBuffer == 0);
  m_nUsedBuffer = 0;
}

HRESULT CEVRAllocatorPresenter::GetFreeSample(IMFSample** ppSample)
{
  CAutoLock lock(&m_SampleQueueLock);
  HRESULT    hr = S_OK;

  if (m_FreeSamples.GetCount() > 1)  // <= Cannot use first free buffer (can be currently displayed)
  {
    InterlockedIncrement (&m_nUsedBuffer);
    //*ppSample = m_FreeSamples.RemoveHead().Detach();
    m_FreeSamples.RemoveFront(ppSample);
  }
  else
    hr = MF_E_SAMPLEALLOCATOR_EMPTY;

  return hr;
}


HRESULT CEVRAllocatorPresenter::GetScheduledSample(IMFSample** ppSample, int &_Count)
{
  CAutoLock lock(&m_SampleQueueLock);
  HRESULT    hr = S_OK;

  _Count = m_ScheduledSamples.GetCount();
  if (_Count > 0)
  {
    //*ppSample = m_ScheduledSamples.RemoveHead().Detach();
    m_ScheduledSamples.RemoveFront(ppSample);
    --_Count;
  }
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
    m_FreeSamples.InsertBack(pSample);
  else
    m_FreeSamples.InsertFront(pSample);
}


void CEVRAllocatorPresenter::MoveToScheduledList(IMFSample* pSample, bool _bSorted)
{

  if (_bSorted)
  {
    CAutoLock lock(&m_SampleQueueLock);
    m_ScheduledSamples.InsertFront(pSample);
  }
  else
  {

    CAutoLock lock(&m_SampleQueueLock);

    double ForceFPS = 0.0;
//    double ForceFPS = 59.94;
//    double ForceFPS = 23.976;
    if (ForceFPS != 0.0)
      m_rtTimePerFrame = (int64_t) (10000000.0 / ForceFPS);
    int64_t Duration = m_rtTimePerFrame;
    int64_t PrevTime = m_LastScheduledUncorrectedSampleTime;
    int64_t Time;
    int64_t SetDuration;
    pSample->GetSampleDuration(&SetDuration);
    pSample->GetSampleTime(&Time);
    m_LastScheduledUncorrectedSampleTime = Time;

    m_bCorrectedFrameTime = false;

    int64_t Diff2 = PrevTime - (int64_t) (m_LastScheduledSampleTimeFP*10000000.0);
    int64_t Diff = Time - PrevTime;
    if (PrevTime == -1)
      Diff = 0;
    if (Diff < 0)
      Diff = -Diff;
    if (Diff2 < 0)
      Diff2 = -Diff2;
    if (Diff < m_rtTimePerFrame*8 && m_rtTimePerFrame && Diff2 < m_rtTimePerFrame*8) // Detect seeking
    {
      int iPos = (m_DetectedFrameTimePos++) % 60;
      int64_t Diff = Time - PrevTime;
      if (PrevTime == -1)
        Diff = 0;
      m_DetectedFrameTimeHistory[iPos] = Diff;
    
      if (m_DetectedFrameTimePos >= 10)
      {
        int nFrames = std::min(m_DetectedFrameTimePos, 60);
        int64_t DectedSum = 0;
        for (int i = 0; i < nFrames; ++i)
        {
          DectedSum += m_DetectedFrameTimeHistory[i];
        }

        double Average = double(DectedSum) / double(nFrames);
        double DeviationSum = 0.0;
        for (int i = 0; i < nFrames; ++i)
        {
          double Deviation = m_DetectedFrameTimeHistory[i] - Average;
          DeviationSum += Deviation*Deviation;
        }
    
        double StdDev = sqrt(DeviationSum/double(nFrames));

        m_DetectedFrameTimeStdDev = StdDev;

        double DetectedRate = 1.0/ (double(DectedSum) / (nFrames * 10000000.0) );

        double AllowedError = 0.0003;

        static double AllowedValues[] = {60.0, 59.94, 50.0, 48.0, 47.952, 30.0, 29.97, 25.0, 24.0, 23.976};

        int nAllowed = sizeof(AllowedValues) / sizeof(AllowedValues[0]);
        for (int i = 0; i < nAllowed; ++i)
        {
          if (fabs(1.0 - DetectedRate / AllowedValues[i]) < AllowedError)
          {
            DetectedRate = AllowedValues[i];
            break;
          }
        }

        m_DetectedFrameTimeHistoryHistory[m_DetectedFrameTimePos % 500] = DetectedRate;

        class CAutoInt
        {
        public:

          int m_Int;

          CAutoInt()
          {
            m_Int = 0;
          }
          CAutoInt(int _Other)
          {
            m_Int = _Other;
          }

          operator int () const
          {
            return m_Int;
          }

          CAutoInt &operator ++ ()
          {
            ++m_Int;
            return *this;
          }
        };

        std::map<double,CAutoInt> Map;
        for( int i=0;i<500;++i )
        {
          std::map<double,CAutoInt>::iterator it;
          it = Map.find(m_DetectedFrameTimeHistoryHistory[i]);
          if (it == Map.end())
            Map.insert(std::make_pair(m_DetectedFrameTimeHistoryHistory[i],1));
          else
            ++Map[m_DetectedFrameTimeHistoryHistory[i]];
        }

        std::map<double,CAutoInt>::iterator it=Map.begin();
        double BestVal = 0.0;
        int BestNum = 5;
        while (it != Map.end())
        {
          if ((*it).second > BestNum && (*it).first != 0.0)
          {
            BestNum = (*it).second;
            BestVal = (*it).first;
          }
          ++it;
        }
        
        /*CMap<double, double, CAutoInt, CAutoInt> Map;

        for (int i = 0; i < 500; ++i)
        {
          ++Map[m_DetectedFrameTimeHistoryHistory[i]];
        }

        POSITION Pos = Map.GetStartPosition();
        double BestVal = 0.0;
        int BestNum = 5;
        while (Pos)
        {
          double Key;
          CAutoInt Value;
          Map.GetNextAssoc(Pos, Key, Value);
          if (Value.m_Int > BestNum && Key != 0.0)
          {
            BestNum = Value.m_Int;
            BestVal = Key;
          }
        }*/

        m_DetectedLock = false;
        for (int i = 0; i < nAllowed; ++i)
        {
          if (BestVal == AllowedValues[i])
          {
            m_DetectedLock = true;
            break;
          }
        }
        if (BestVal != 0.0)
        {
          m_DetectedFrameRate = BestVal;
          m_DetectedFrameTime = 1.0 / BestVal;
        }
      }

      int64_t PredictedNext = PrevTime + m_rtTimePerFrame;
      int64_t PredictedDiff = PredictedNext - Time;
      if (PredictedDiff < 0)
        PredictedDiff = -PredictedDiff;

      if (m_DetectedFrameTime != 0.0 
        //&& PredictedDiff > 15000 
        && m_DetectedLock && ((CEVRRendererSettings *)g_dsSettings.pRendererSettings)->enableFrameTimeCorrection)
      {
        double CurrentTime = Time / 10000000.0;
        double LastTime = m_LastScheduledSampleTimeFP;
        double PredictedTime = LastTime + m_DetectedFrameTime;
        if (fabs(PredictedTime - CurrentTime) > 0.0015) // 1.5 ms wrong, lets correct
        {
          CurrentTime = PredictedTime;
          Time = (int64_t) (CurrentTime * 10000000.0);
          pSample->SetSampleTime(Time);
          pSample->SetSampleDuration((int64_t) (m_DetectedFrameTime * 10000000.0));
          m_bCorrectedFrameTime = true;
          m_FrameTimeCorrection = 30;
        }
        m_LastScheduledSampleTimeFP = CurrentTime;
      }
      else
        m_LastScheduledSampleTimeFP = Time / 10000000.0;
    }
    else
    {
      m_LastScheduledSampleTimeFP = Time / 10000000.0;
      if (Diff > m_rtTimePerFrame*8)
      {
        // Seek
        m_bSignaledStarvation = false;
        m_DetectedFrameTimePos = 0;
        m_DetectedLock = false;
      }
    }

    TRACE_EVR("EVR: Time: %f %f %f\n", Time / 10000000.0, SetDuration / 10000000.0, m_DetectedFrameRate);
    if (!m_bCorrectedFrameTime && m_FrameTimeCorrection)
      --m_FrameTimeCorrection;

#if 0
    if (Time <= m_LastScheduledUncorrectedSampleTime && m_LastScheduledSampleTime >= 0)
      PrevTime = m_LastScheduledSampleTime;

    m_bCorrectedFrameTime = false;
    if (PrevTime != -1 && (Time >= PrevTime - ((Duration*20)/9) || Time == 0) || ForceFPS != 0.0)
    {
      if (Time - PrevTime > ((Duration*20)/9) && Time - PrevTime < Duration * 8 || Time == 0 || ((Time - PrevTime) < (Duration / 11)) || ForceFPS != 0.0)
      {
        // Error!!!!
        Time = PrevTime + Duration;
        pSample->SetSampleTime(Time);
        pSample->SetSampleDuration(Duration);
        m_bCorrectedFrameTime = true;
        TRACE_EVR("EVR: Corrected invalid sample time\n");
      }
    }
    if (Time+Duration*10 < m_LastScheduledSampleTime)
    {
      // Flush when repeating movie
      FlushSamplesInternal();
    }
#endif

#if 0
    static int64_t LastDuration = 0;
    int64_t SetDuration = m_rtTimePerFrame;
    pSample->GetSampleDuration(&SetDuration);
    if (SetDuration != LastDuration)
    {
      TRACE_EVR("EVR: Old duration: %I64d New duration: %I64d\n", LastDuration, SetDuration);
    }
    LastDuration = SetDuration;
#endif
    m_LastScheduledSampleTime = Time;

    m_ScheduledSamples.InsertBack(pSample);

  }
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
    Com::SmartPtr<IMFSample>    pMFSample;

    //pMFSample = m_ScheduledSamples.RemoveHead();
    m_ScheduledSamples.RemoveFront(&pMFSample);
    MoveToFreeList (pMFSample, true);
  }

  m_LastSampleOffset      = 0;
  m_bLastSampleOffsetValid  = false;
  m_bSignaledStarvation = false;
}

#endif