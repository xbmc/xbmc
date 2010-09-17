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

#pragma once

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "DX9AllocatorPresenter.h"
#include <mfapi.h>  // API Media Foundation
#include <evr9.h>
#include <queue>
#include "Utils/SmartList.h"

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

  class COuterEVR;
  typedef Com::ComPtrList<IMFSample> VideoSampleList;

  class CEVRAllocatorPresenter : 
    public CDX9AllocatorPresenter,
    public IMFGetService,
    public IMFTopologyServiceLookupClient,
    public IMFVideoDeviceID,
    public IMFVideoPresenter,
    public IDirect3DDeviceManager9,

    public IMFAsyncCallback,
    public IQualProp,
    public IMFRateSupport,        
    public IMFVideoDisplayControl,
    public IEVRTrustedVideoPlugin
    /*  public IMFVideoPositionMapper,    // Non mandatory EVR Presenter Interfaces (see later...)
    */
  {
  public:
    CEVRAllocatorPresenter(HWND hWnd, HRESULT& hr, CStdString &_Error);
    ~CEVRAllocatorPresenter(void);

    DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    STDMETHODIMP  CreateRenderer(IUnknown** ppRenderer);
    STDMETHODIMP_(bool) Paint(bool fAll);
    STDMETHODIMP  GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight);
    STDMETHODIMP  InitializeDevice(AM_MEDIA_TYPE*  pMediaType);


    // IMFClockStateSink
    STDMETHODIMP  OnClockStart(/* [in] */ MFTIME hnsSystemTime, /* [in] */ int64_t llClockStartOffset);        
    STDMETHODIMP  STDMETHODCALLTYPE OnClockStop(/* [in] */ MFTIME hnsSystemTime);
    STDMETHODIMP  STDMETHODCALLTYPE OnClockPause(/* [in] */ MFTIME hnsSystemTime);
    STDMETHODIMP  STDMETHODCALLTYPE OnClockRestart(/* [in] */ MFTIME hnsSystemTime);
    STDMETHODIMP  STDMETHODCALLTYPE OnClockSetRate(/* [in] */ MFTIME hnsSystemTime, /* [in] */ float flRate);

    // IBaseFilter delegate
    bool      GetState( DWORD dwMilliSecsTimeout, FILTER_STATE *State, HRESULT &_ReturnValue);

    // IQualProp (EVR statistics window)
    STDMETHODIMP  get_FramesDroppedInRenderer    (int *pcFrames);
    STDMETHODIMP  get_FramesDrawn          (int *pcFramesDrawn);
    STDMETHODIMP  get_AvgFrameRate        (int *piAvgFrameRate);
    STDMETHODIMP  get_Jitter            (int *iJitter);
    STDMETHODIMP  get_AvgSyncOffset        (int *piAvg);
    STDMETHODIMP  get_DevSyncOffset        (int *piDev);


    // IMFRateSupport
    STDMETHODIMP  GetSlowestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate);
    STDMETHODIMP  GetFastestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate);
    STDMETHODIMP  IsRateSupported(BOOL fThin, float flRate, float *pflNearestSupportedRate);

    float         GetMaxRate(BOOL bThin);


    // IMFVideoPresenter
    STDMETHODIMP  ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam);
    STDMETHODIMP  GetCurrentMediaType(__deref_out  IMFVideoMediaType **ppMediaType);

    // IMFTopologyServiceLookupClient        
    STDMETHODIMP  InitServicePointers(/* [in] */ __in  IMFTopologyServiceLookup *pLookup);
    STDMETHODIMP  ReleaseServicePointers();

    // IMFVideoDeviceID
    STDMETHODIMP  GetDeviceID(/* [out] */  __out  IID *pDeviceID);

    // IMFGetService
    STDMETHODIMP  GetService (/* [in] */ __RPC__in REFGUID guidService,
      /* [in] */ __RPC__in REFIID riid,
      /* [iid_is][out] */ __RPC__deref_out_opt LPVOID *ppvObject);

    // IMFAsyncCallback
    STDMETHODIMP  GetParameters(  /* [out] */ __RPC__out DWORD *pdwFlags, /* [out] */ __RPC__out DWORD *pdwQueue);
    STDMETHODIMP  Invoke     (  /* [in] */ __RPC__in_opt IMFAsyncResult *pAsyncResult);

    // IMFVideoDisplayControl
    STDMETHODIMP GetNativeVideoSize(SIZE *pszVideo, SIZE *pszARVideo);    
    STDMETHODIMP GetIdealVideoSize(SIZE *pszMin, SIZE *pszMax);
    STDMETHODIMP SetVideoPosition(const MFVideoNormalizedRect *pnrcSource, const LPRECT prcDest);
    STDMETHODIMP GetVideoPosition(MFVideoNormalizedRect *pnrcSource, LPRECT prcDest);
    STDMETHODIMP SetAspectRatioMode(DWORD dwAspectRatioMode);
    STDMETHODIMP GetAspectRatioMode(DWORD *pdwAspectRatioMode);
    STDMETHODIMP SetVideoWindow(HWND hwndVideo);
    STDMETHODIMP GetVideoWindow(HWND *phwndVideo);
    STDMETHODIMP RepaintVideo( void);
    STDMETHODIMP GetCurrentImage(BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, int64_t *pTimeStamp);
    STDMETHODIMP SetBorderColor(COLORREF Clr);
    STDMETHODIMP GetBorderColor(COLORREF *pClr);
    STDMETHODIMP SetRenderingPrefs(DWORD dwRenderFlags);
    STDMETHODIMP GetRenderingPrefs(DWORD *pdwRenderFlags);
    STDMETHODIMP SetFullscreen(BOOL fFullscreen);
    STDMETHODIMP GetFullscreen(BOOL *pfFullscreen);

    // IEVRTrustedVideoPlugin
    STDMETHODIMP IsInTrustedVideoMode(BOOL *pYes);
    STDMETHODIMP CanConstrict(BOOL *pYes);
    STDMETHODIMP SetConstriction(DWORD dwKPix);
    STDMETHODIMP DisableImageExport(BOOL bDisable);

    // IDirect3DDeviceManager9
    STDMETHODIMP  ResetDevice(IDirect3DDevice9 *pDevice,UINT resetToken);        
    STDMETHODIMP  OpenDeviceHandle(HANDLE *phDevice);
    STDMETHODIMP  CloseDeviceHandle(HANDLE hDevice);        
    STDMETHODIMP  TestDevice(HANDLE hDevice);
    STDMETHODIMP  LockDevice(HANDLE hDevice, IDirect3DDevice9 **ppDevice, BOOL fBlock);
    STDMETHODIMP  UnlockDevice(HANDLE hDevice, BOOL fSaveState);
    STDMETHODIMP  GetVideoService(HANDLE hDevice, REFIID riid, void **ppService);

  protected :
    // D3D Reset
    void            BeforeDeviceReset();
    void            AfterDeviceReset();
    int64_t         GetClockTime(int64_t PerformanceCounter);

    virtual void    OnVBlankFinished(bool fAll, int64_t PerformanceCounter);

    double          m_ModeratedTime;
    int64_t         m_ModeratedTimeLast;
    int64_t         m_ModeratedClockLast;
    int64_t         m_ModeratedTimer;
    MFCLOCK_STATE   m_LastClockState;

  private:

    typedef enum
    {
      Started = State_Running,
      Stopped = State_Stopped,
      Paused = State_Paused,
      Shutdown = State_Running + 1
    } RENDER_STATE;

    COuterEVR*                                m_pOuterEVR;
    Com::SmartPtr<IMFClock>                   m_pClock;
    Com::SmartPtr<IDirect3DDeviceManager9>    m_pD3DManager;
    Com::SmartPtr<IMFTransform>               m_pMixer;
    Com::SmartPtr<IMediaEventSink>            m_pSink;
    Com::SmartPtr<IMFVideoMediaType>          m_pMediaType;
    MFVideoAspectRatioMode                    m_dwVideoAspectRatioMode;
    MFVideoRenderPrefs                        m_dwVideoRenderPrefs;
    COLORREF                                  m_BorderColor;

    HANDLE                   m_hEvtQuit;      // Stop rendering thread event
    bool                     m_bEvtQuit;
    HANDLE                   m_hEvtFlush;    // Discard all buffers
    bool                     m_bEvtFlush;

    bool                     m_fUseInternalTimer;
    long                     m_LastSetOutputRange;
    bool                     m_bPendingRenegotiate;
    bool                     m_bPendingMediaFinished;

    HANDLE                   m_hThread;
    HANDLE                   m_hGetMixerThread;
    RENDER_STATE             m_nRenderState;

    CCritSec                 m_SampleQueueLock;
    CCritSec                 m_ImageProcessingLock;
    CCriticalSection         m_DisplaydSampleQueueLock;

    VideoSampleList          m_FreeSamples;
    VideoSampleList          m_ScheduledSamples;

    std::queue<IMFSample *>  m_pCurrentDisplaydSampleQueue;
    IMFSample *              m_pCurrentDisplaydSample;
    bool                     m_bWaitingSample;
    bool                     m_bLastSampleOffsetValid;
    int64_t                  m_LastScheduledSampleTime;
    double                   m_LastScheduledSampleTimeFP;
    int64_t                  m_LastScheduledUncorrectedSampleTime;
    int64_t                  m_MaxSampleDuration;
    int64_t                  m_LastSampleOffset;
    int64_t                  m_VSyncOffsetHistory[5];
    int64_t                  m_LastPredictedSync;
    int                      m_VSyncOffsetHistoryPos;

    UINT                     m_nResetToken;
    int                      m_nStepCount;

    bool                     m_bSignaledStarvation; 
    int64_t                  m_StarvationClock;

    // Stats variable for IQualProp
    UINT                     m_pcFrames;
    UINT                     m_nDroppedUpdate;
    UINT                     m_pcFramesDrawn;  // Retrieves the number of frames drawn since streaming started
    UINT                     m_piAvg;
    UINT                     m_piDev;


    void                     GetMixerThread();
    static DWORD WINAPI      GetMixerThreadStatic(LPVOID lpParam);

    bool                     GetImageFromMixer();
    void                     RenderThread();
    static DWORD WINAPI      PresentThread(LPVOID lpParam);
    void                     ResetStats();
    void                     StartWorkerThreads();
    void                     StopWorkerThreads();
    HRESULT                  CheckShutdown() const;
    void                     CompleteFrameStep(bool bCancel);
    void                     CheckWaitingSampleFromMixer();

    void                     RemoveAllSamples();
    HRESULT                  GetFreeSample(IMFSample** ppSample);
    HRESULT                  GetScheduledSample(IMFSample** ppSample, int &_Count);
    void                     MoveToFreeList(IMFSample* pSample, bool bTail);
    void                     MoveToScheduledList(IMFSample* pSample, bool _bSorted);
    void                     FlushSamples();
    void                     FlushSamplesInternal();
    void                     PaintInternal();

    // === Media type negociation functions
    HRESULT                  RenegotiateMediaType();
    HRESULT                  IsMediaTypeSupported(IMFMediaType* pMixerType);
    HRESULT                  CreateProposedOutputType(IMFMediaType* pMixerType, IMFMediaType** pType);
    HRESULT                  SetMediaType(IMFMediaType* pType);

    // === Functions pointers on Vista / .Net3 specifics library
    PTR_DXVA2CreateDirect3DDeviceManager9   pfDXVA2CreateDirect3DDeviceManager9;
    PTR_MFCreateDXSurfaceBuffer             pfMFCreateDXSurfaceBuffer;
    PTR_MFCreateVideoSampleFromSurface      pfMFCreateVideoSampleFromSurface;
    PTR_MFCreateVideoMediaType              pfMFCreateVideoMediaType;

#if 0
    HRESULT (__stdcall *pMFCreateMediaType)(__deref_out IMFMediaType**  ppMFType);
    HRESULT (__stdcall *pMFInitMediaTypeFromAMMediaType)(__in IMFMediaType *pMFType, __in const AM_MEDIA_TYPE *pAMType);
    HRESULT (__stdcall *pMFInitAMMediaTypeFromMFMediaType)(__in IMFMediaType *pMFType, __in GUID guidFormatBlockType, __inout AM_MEDIA_TYPE *pAMType);
#endif

    PTR_AvSetMmThreadCharacteristicsW       pfAvSetMmThreadCharacteristicsW;
    PTR_AvSetMmThreadPriority               pfAvSetMmThreadPriority;
    PTR_AvRevertMmThreadCharacteristics     pfAvRevertMmThreadCharacteristics;
  };