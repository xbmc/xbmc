/*
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2007 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "DShowUtil/dshowutil.h"
#include "DShowUtil/DSGeometry.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "WindowingFactory.h" //d3d device and d3d interface
#include "DX9AllocatorPresenter.h"
#include "application.h"
#include "ApplicationRenderer.h"
#include "utils/log.h"
#include "MacrovisionKicker.h"
#include "IPinHook.h"


class COuterVMR9
  : public CUnknown
  , public IVideoWindow
  , public IBasicVideo2
  , public IVMRWindowlessControl
  , public IVMRMixerBitmap9
{
  CComPtr<IUnknown>  m_pVMR;
  VMR9AlphaBitmap*  m_pVMR9AlphaBitmap;
  CDX9AllocatorPresenter *m_pAllocatorPresenter;

public:

  COuterVMR9(const TCHAR* pName, LPUNKNOWN pUnk, VMR9AlphaBitmap* pVMR9AlphaBitmap, CDX9AllocatorPresenter *_pAllocatorPresenter) : CUnknown(pName, pUnk)
  {
    m_pVMR.CoCreateInstance(CLSID_VideoMixingRenderer9, GetOwner());
    m_pVMR9AlphaBitmap = pVMR9AlphaBitmap;
    m_pAllocatorPresenter = _pAllocatorPresenter;
  }

  ~COuterVMR9()
  {
    m_pVMR = NULL;
  }

  DECLARE_IUNKNOWN;
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
  {
    HRESULT hr;

    // Casimir666 : en mode Renderless faire l'incrustation à la place du VMR
    if(riid == __uuidof(IVMRMixerBitmap9))
      return GetInterface((IVMRMixerBitmap9*)this, ppv);

    hr = m_pVMR ? m_pVMR->QueryInterface(riid, ppv) : E_NOINTERFACE;
    if(m_pVMR && FAILED(hr))
    {
      if(riid == __uuidof(IVideoWindow))
        return GetInterface((IVideoWindow*)this, ppv);
      if(riid == __uuidof(IBasicVideo))
        return GetInterface((IBasicVideo*)this, ppv);
      if(riid == __uuidof(IBasicVideo2))
        return GetInterface((IBasicVideo2*)this, ppv);
/*      if(riid == __uuidof(IVMRWindowlessControl))
        return GetInterface((IVMRWindowlessControl*)this, ppv);
*/
    }

    return SUCCEEDED(hr) ? hr : __super::NonDelegatingQueryInterface(riid, ppv);
  }

  // IVMRWindowlessControl

  STDMETHODIMP GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
  {
    if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
    {
      return pWC9->GetNativeVideoSize(lpWidth, lpHeight, lpARWidth, lpARHeight);
    }

    return E_NOTIMPL;
  }
  STDMETHODIMP GetMinIdealVideoSize(LONG* lpWidth, LONG* lpHeight) {return E_NOTIMPL;}
  STDMETHODIMP GetMaxIdealVideoSize(LONG* lpWidth, LONG* lpHeight) {return E_NOTIMPL;}
  STDMETHODIMP SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect) {return E_NOTIMPL;}
    STDMETHODIMP GetVideoPosition(LPRECT lpSRCRect, LPRECT lpDSTRect)
  {
    if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
    {
      return pWC9->GetVideoPosition(lpSRCRect, lpDSTRect);
    }

    return E_NOTIMPL;
  }
  STDMETHODIMP GetAspectRatioMode(DWORD* lpAspectRatioMode)
  {
    if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
    {
      *lpAspectRatioMode = VMR_ARMODE_NONE;
      return S_OK;
    }

    return E_NOTIMPL;
  }
  STDMETHODIMP SetAspectRatioMode(DWORD AspectRatioMode) {return E_NOTIMPL;}
  STDMETHODIMP SetVideoClippingWindow(HWND hwnd) {return E_NOTIMPL;}
  STDMETHODIMP RepaintVideo(HWND hwnd, HDC hdc) {return E_NOTIMPL;}
  STDMETHODIMP DisplayModeChanged() {return E_NOTIMPL;}
  STDMETHODIMP GetCurrentImage(BYTE** lpDib) {return E_NOTIMPL;}
  STDMETHODIMP SetBorderColor(COLORREF Clr) {return E_NOTIMPL;}
  STDMETHODIMP GetBorderColor(COLORREF* lpClr) {return E_NOTIMPL;}
  STDMETHODIMP SetColorKey(COLORREF Clr) {return E_NOTIMPL;}
  STDMETHODIMP GetColorKey(COLORREF* lpClr) {return E_NOTIMPL;}

  // IVideoWindow
  STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) {return E_NOTIMPL;}
  STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) {return E_NOTIMPL;}
  STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId) {return E_NOTIMPL;}
  STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {return E_NOTIMPL;}
    STDMETHODIMP put_Caption(BSTR strCaption) {return E_NOTIMPL;}
    STDMETHODIMP get_Caption(BSTR* strCaption) {return E_NOTIMPL;}
  STDMETHODIMP put_WindowStyle(long WindowStyle) {return E_NOTIMPL;}
  STDMETHODIMP get_WindowStyle(long* WindowStyle) {return E_NOTIMPL;}
  STDMETHODIMP put_WindowStyleEx(long WindowStyleEx) {return E_NOTIMPL;}
  STDMETHODIMP get_WindowStyleEx(long* WindowStyleEx) {return E_NOTIMPL;}
  STDMETHODIMP put_AutoShow(long AutoShow) {return E_NOTIMPL;}
  STDMETHODIMP get_AutoShow(long* AutoShow) {return E_NOTIMPL;}
  STDMETHODIMP put_WindowState(long WindowState) {return E_NOTIMPL;}
  STDMETHODIMP get_WindowState(long* WindowState) {return E_NOTIMPL;}
  STDMETHODIMP put_BackgroundPalette(long BackgroundPalette) {return E_NOTIMPL;}
  STDMETHODIMP get_BackgroundPalette(long* pBackgroundPalette) {return E_NOTIMPL;}
  STDMETHODIMP put_Visible(long Visible) {return E_NOTIMPL;}
  STDMETHODIMP get_Visible(long* pVisible) {return E_NOTIMPL;}
  STDMETHODIMP put_Left(long Left) {return E_NOTIMPL;}
  STDMETHODIMP get_Left(long* pLeft) {return E_NOTIMPL;}
  STDMETHODIMP put_Width(long Width) {return E_NOTIMPL;}
  STDMETHODIMP get_Width(long* pWidth)
  {
    if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
    {
      tagRECT s, d;
      HRESULT hr = pWC9->GetVideoPosition(&s, &d);
      *pWidth = d.right-d.left;
      //*pWidth = d.Width();
      return hr;
    }

    return E_NOTIMPL;
  }
  STDMETHODIMP put_Top(long Top) {return E_NOTIMPL;}
  STDMETHODIMP get_Top(long* pTop) {return E_NOTIMPL;}
  STDMETHODIMP put_Height(long Height) {return E_NOTIMPL;}
  STDMETHODIMP get_Height(long* pHeight)
  {
    if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
    {
      tagRECT s, d;
      HRESULT hr = pWC9->GetVideoPosition(&s, &d);
      *pHeight = g_geometryHelper.GetHeight(d);
      return hr;
    }

    return E_NOTIMPL;
  }
  STDMETHODIMP put_Owner(OAHWND Owner) {return E_NOTIMPL;}
  STDMETHODIMP get_Owner(OAHWND* Owner) {return E_NOTIMPL;}
  STDMETHODIMP put_MessageDrain(OAHWND Drain) {return E_NOTIMPL;}
  STDMETHODIMP get_MessageDrain(OAHWND* Drain) {return E_NOTIMPL;}
  STDMETHODIMP get_BorderColor(long* Color) {return E_NOTIMPL;}
  STDMETHODIMP put_BorderColor(long Color) {return E_NOTIMPL;}
  STDMETHODIMP get_FullScreenMode(long* FullScreenMode) {return E_NOTIMPL;}
  STDMETHODIMP put_FullScreenMode(long FullScreenMode) {return E_NOTIMPL;}
    STDMETHODIMP SetWindowForeground(long Focus) {return E_NOTIMPL;}
    STDMETHODIMP NotifyOwnerMessage(OAHWND hwnd, long uMsg, LONG_PTR wParam, LONG_PTR lParam) {return E_NOTIMPL;}
    STDMETHODIMP SetWindowPosition(long Left, long Top, long Width, long Height) {return E_NOTIMPL;}
  STDMETHODIMP GetWindowPosition(long* pLeft, long* pTop, long* pWidth, long* pHeight) {return E_NOTIMPL;}
  STDMETHODIMP GetMinIdealImageSize(long* pWidth, long* pHeight) {return E_NOTIMPL;}
  STDMETHODIMP GetMaxIdealImageSize(long* pWidth, long* pHeight) {return E_NOTIMPL;}
  STDMETHODIMP GetRestorePosition(long* pLeft, long* pTop, long* pWidth, long* pHeight) {return E_NOTIMPL;}
  STDMETHODIMP HideCursor(long HideCursor) {return E_NOTIMPL;}
  STDMETHODIMP IsCursorHidden(long* CursorHidden) {return E_NOTIMPL;}

  // IBasicVideo2
    STDMETHODIMP get_AvgTimePerFrame(REFTIME* pAvgTimePerFrame) {return E_NOTIMPL;}
    STDMETHODIMP get_BitRate(long* pBitRate) {return E_NOTIMPL;}
    STDMETHODIMP get_BitErrorRate(long* pBitErrorRate) {return E_NOTIMPL;}
    STDMETHODIMP get_VideoWidth(long* pVideoWidth) {return E_NOTIMPL;}
    STDMETHODIMP get_VideoHeight(long* pVideoHeight) {return E_NOTIMPL;}
    STDMETHODIMP put_SourceLeft(long SourceLeft) {return E_NOTIMPL;}
    STDMETHODIMP get_SourceLeft(long* pSourceLeft) {return E_NOTIMPL;}
    STDMETHODIMP put_SourceWidth(long SourceWidth) {return E_NOTIMPL;}
    STDMETHODIMP get_SourceWidth(long* pSourceWidth) {return E_NOTIMPL;}
    STDMETHODIMP put_SourceTop(long SourceTop) {return E_NOTIMPL;}
    STDMETHODIMP get_SourceTop(long* pSourceTop) {return E_NOTIMPL;}
    STDMETHODIMP put_SourceHeight(long SourceHeight) {return E_NOTIMPL;}
    STDMETHODIMP get_SourceHeight(long* pSourceHeight) {return E_NOTIMPL;}
    STDMETHODIMP put_DestinationLeft(long DestinationLeft) {return E_NOTIMPL;}
    STDMETHODIMP get_DestinationLeft(long* pDestinationLeft) {return E_NOTIMPL;}
    STDMETHODIMP put_DestinationWidth(long DestinationWidth) {return E_NOTIMPL;}
    STDMETHODIMP get_DestinationWidth(long* pDestinationWidth) {return E_NOTIMPL;}
    STDMETHODIMP put_DestinationTop(long DestinationTop) {return E_NOTIMPL;}
    STDMETHODIMP get_DestinationTop(long* pDestinationTop) {return E_NOTIMPL;}
    STDMETHODIMP put_DestinationHeight(long DestinationHeight) {return E_NOTIMPL;}
    STDMETHODIMP get_DestinationHeight(long* pDestinationHeight) {return E_NOTIMPL;}
    STDMETHODIMP SetSourcePosition(long Left, long Top, long Width, long Height) {return E_NOTIMPL;}
    STDMETHODIMP GetSourcePosition(long* pLeft, long* pTop, long* pWidth, long* pHeight)
  {
    // DVD Nav. bug workaround fix
    {
      *pLeft = *pTop = 0;
      return GetVideoSize(pWidth, pHeight);
    }
/*
    if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
    {
      tagRECT s, d;
      HRESULT hr = pWC9->GetVideoPosition(&s, &d);
      *pLeft = s.left;
      *pTop = s.top;
      *pWidth = s.Width();
      *pHeight = s.Height();
      return hr;
    }
*/
    return E_NOTIMPL;
  }
    STDMETHODIMP SetDefaultSourcePosition() {return E_NOTIMPL;}
    STDMETHODIMP SetDestinationPosition(long Left, long Top, long Width, long Height) {return E_NOTIMPL;}
    STDMETHODIMP GetDestinationPosition(long* pLeft, long* pTop, long* pWidth, long* pHeight)
  {
    if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
    {
      tagRECT s, d;
      HRESULT hr = pWC9->GetVideoPosition(&s, &d);
      *pLeft = d.left;
      *pTop = d.top;
      *pWidth = g_geometryHelper.GetWidth(d);
      *pHeight = g_geometryHelper.GetHeight(d);
      return hr;
    }

    return E_NOTIMPL;
  }
    STDMETHODIMP SetDefaultDestinationPosition() {return E_NOTIMPL;}
    STDMETHODIMP GetVideoSize(long* pWidth, long* pHeight)
  {
    if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
    {
      LONG aw, ah;
      HRESULT hr = pWC9->GetNativeVideoSize(pWidth, pHeight, &aw, &ah);
      *pWidth = *pHeight * aw / ah;
      return hr;
    }

    return E_NOTIMPL;
  }

    STDMETHODIMP GetVideoPaletteEntries(long StartIndex, long Entries, long* pRetrieved, long* pPalette) {return E_NOTIMPL;}
    STDMETHODIMP GetCurrentImage(long* pBufferSize, long* pDIBImage) {return E_NOTIMPL;}
    STDMETHODIMP IsUsingDefaultSource() {return E_NOTIMPL;}
    STDMETHODIMP IsUsingDefaultDestination() {return E_NOTIMPL;}

  STDMETHODIMP GetPreferredAspectRatio(long* plAspectX, long* plAspectY)
  {
    if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
    {
      LONG w, h;
      return pWC9->GetNativeVideoSize(&w, &h, plAspectX, plAspectY);
    }

    return E_NOTIMPL;
  }

  // IVMRMixerBitmap9
  STDMETHODIMP GetAlphaBitmapParameters(VMR9AlphaBitmap* pBmpParms)
  {
    CheckPointer(pBmpParms, E_POINTER);
    CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
    memcpy (pBmpParms, m_pVMR9AlphaBitmap, sizeof(VMR9AlphaBitmap));
    return S_OK;
  }
  
  STDMETHODIMP SetAlphaBitmap(const VMR9AlphaBitmap*  pBmpParms)
  {
    CheckPointer(pBmpParms, E_POINTER);
    CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
    memcpy (m_pVMR9AlphaBitmap, pBmpParms, sizeof(VMR9AlphaBitmap));
    m_pVMR9AlphaBitmap->dwFlags |= VMRBITMAP_UPDATE;
    m_pAllocatorPresenter->UpdateAlphaBitmap();
    return S_OK;
  }

  STDMETHODIMP UpdateAlphaBitmapParameters(const VMR9AlphaBitmap* pBmpParms)
  {
    CheckPointer(pBmpParms, E_POINTER);
    CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
    memcpy (m_pVMR9AlphaBitmap, pBmpParms, sizeof(VMR9AlphaBitmap));
    m_pVMR9AlphaBitmap->dwFlags |= VMRBITMAP_UPDATE;
    m_pAllocatorPresenter->UpdateAlphaBitmap();
    return S_OK;
  }
};

CDX9AllocatorPresenter::CDX9AllocatorPresenter(HRESULT& hr, CStdString &_Error)
: m_refCount(1)
{
  m_D3D = g_Windowing.Get3DObject();
  m_D3DDev = g_Windowing.Get3DDevice();
  hr = S_OK;
  CAutoLock Lock(&m_ObjectLock);
  g_renderManager.PreInit();
  m_renderTarget = NULL; 
  m_pRequireResetDevice = false;
  hr = m_D3DDev->GetRenderTarget( 0, &m_renderTarget );

}

CDX9AllocatorPresenter::~CDX9AllocatorPresenter()
{
    DeleteSurfaces();
  g_renderManager.UnInit();
}

void CDX9AllocatorPresenter::DeleteSurfaces()
{
    CAutoLock Lock(&m_ObjectLock);

    // clear out the private texture
    m_pVideoTexture = NULL;

    for( size_t i = 0; i < m_surfaces.size(); ++i ) 
    {
        m_surfaces[i] = NULL;
    }
}

//IVMRSurfaceAllocator9
HRESULT CDX9AllocatorPresenter::InitializeDevice(DWORD_PTR dwUserID ,VMR9AllocationInfo *lpAllocInfo, DWORD *lpNumBuffers)
{
  m_pPrevEndFrame=0;
  CLog::Log(LOGNOTICE,"vmr9:InitializeDevice() %dx%d AR %d:%d flags:%d buffers:%d  fmt:(%x) %c%c%c%c", 
    lpAllocInfo->dwWidth ,lpAllocInfo->dwHeight ,lpAllocInfo->szAspectRatio.cx,lpAllocInfo->szAspectRatio.cy,
    lpAllocInfo->dwFlags ,*lpNumBuffers, lpAllocInfo->Format, ((char)lpAllocInfo->Format&0xff),
	((char)(lpAllocInfo->Format>>8)&0xff) ,((char)(lpAllocInfo->Format>>16)&0xff) ,((char)(lpAllocInfo->Format>>24)&0xff));

  if(!lpAllocInfo || !lpNumBuffers)
    return E_POINTER;
  if( m_lpIVMRSurfAllocNotify == NULL )
    return E_FAIL;
  HRESULT hr = S_OK;

  if(lpAllocInfo->Format == '21VY' || lpAllocInfo->Format == '024I')
    return E_FAIL;
  
  //To do implement the texture surface on the present image
  //if(lpAllocInfo->dwFlags & VMR9AllocFlag_3DRenderTarget)
  //lpAllocInfo->dwFlags |= VMR9AllocFlag_TextureSurface;
  CLog::Log(LOGNOTICE,"vmr9:flags:");
  if (lpAllocInfo->dwFlags & VMR9AllocFlag_3DRenderTarget)   CLog::Log(LOGNOTICE,"vmr9:  3drendertarget");
  if (lpAllocInfo->dwFlags & VMR9AllocFlag_DXVATarget)      CLog::Log(LOGNOTICE,"vmr9:  DXVATarget");
  if (lpAllocInfo->dwFlags & VMR9AllocFlag_OffscreenSurface) CLog::Log(LOGNOTICE,"vmr9:  OffscreenSurface");
  if (lpAllocInfo->dwFlags & VMR9AllocFlag_RGBDynamicSwitch) CLog::Log(LOGNOTICE,"vmr9:  RGBDynamicSwitch");
  if (lpAllocInfo->dwFlags & VMR9AllocFlag_TextureSurface)   CLog::Log(LOGNOTICE,"vmr9:  TextureSurface");
    //DeleteSurfaces();

    m_surfaces.resize(*lpNumBuffers);
    hr = m_lpIVMRSurfAllocNotify->AllocateSurfaceHelper(lpAllocInfo, lpNumBuffers, & m_surfaces.at(0) );
    if(FAILED(hr))
  {
    CLog::Log(LOGERROR,"vmr9:InitializeDevice()   AllocateSurfaceHelper returned:0x%x",hr);
    return hr;
  }
  m_iVideoWidth=lpAllocInfo->dwWidth;
  m_iVideoHeight=lpAllocInfo->dwHeight;

  //INITIALIZE VIDEO SURFACE THERE
  if(FAILED(hr = AllocVideoSurface()))
  { 
    CLog::Log(LOGERROR,"vmr9:InitializeDevice()   AllocVideoSurface returned:0x%x",hr);
    return hr;
  }
  hr = m_D3DDev->ColorFill(m_pVideoSurface, NULL, 0);
  // test if the colorspace is acceptable

  //m_D3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  //hr = m_D3DDev->StretchRect(m_surfaces[0], NULL, m_pVideoSurface, NULL, D3DTEXF_NONE);
  //m_D3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  if(FAILED(hr))
  {
    CLog::Log(LOGERROR,"vmr9:InitializeDevice()   StretchRect returned:0x%x",hr);
    DeleteSurfaces();
    return E_FAIL;
  }

  if(FAILED(hr = m_D3DDev->ColorFill(m_pVideoSurface, NULL, 0)))
  {
    CLog::Log(LOGERROR,"vmr9:InitializeDevice()   ColorFill returned:0x%x",hr);
    DeleteSurfaces();
    return E_FAIL;
  }
  return hr;
}

//from mediaportal
HRESULT CDX9AllocatorPresenter::AllocVideoSurface(D3DFORMAT Format)
{
  CAutoLock Lock(&m_ObjectLock);
  HRESULT hr;
  m_pVideoTexture = NULL;
  m_pVideoSurface = NULL;
  m_SurfaceType = Format;
  D3DDISPLAYMODE dm;
  hr = m_D3DDev->GetDisplayMode(NULL, &dm);
  
  if (FAILED(hr = m_D3DDev->CreateTexture(m_iVideoWidth,
                                          m_iVideoHeight,
                                          1,
                                          D3DUSAGE_RENDERTARGET,
                                          dm.Format,//m_SurfaceType, // default is D3DFMT_A8R8G8B8
                                          D3DPOOL_DEFAULT,
                                          &m_pVideoTexture,
                                          NULL ) ) )
    return hr;
  if(FAILED(hr = m_pVideoTexture->GetSurfaceLevel(0, &m_pVideoSurface)))
    return hr;
  return hr;
}

HRESULT CDX9AllocatorPresenter::TerminateDevice(DWORD_PTR dwID)
{
    DeleteSurfaces();
    return S_OK;
}
    
HRESULT CDX9AllocatorPresenter::GetSurface(DWORD_PTR dwUserID ,DWORD SurfaceIndex ,DWORD SurfaceFlags ,IDirect3DSurface9 **lplpSurface)
{
  if( lplpSurface == NULL )
    return E_POINTER;

  if (SurfaceIndex >= m_surfaces.size() ) 
    return E_FAIL;
  CAutoLock Lock(&m_ObjectLock);
  *lplpSurface = m_surfaces[SurfaceIndex];
  (*lplpSurface)->AddRef();

  return S_OK;
}
    
HRESULT CDX9AllocatorPresenter::AdviseNotify(IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify)
{
    CAutoLock Lock(&m_ObjectLock);
    HRESULT hr;
    m_lpIVMRSurfAllocNotify = lpIVMRSurfAllocNotify;
    HMONITOR hMonitor = m_D3D->GetAdapterMonitor(GetAdapter(m_D3D));
    hr = m_lpIVMRSurfAllocNotify->SetD3DDevice( m_D3DDev, hMonitor);
    return hr;
}

HRESULT CDX9AllocatorPresenter::StartPresenting( 
    /* [in] */ DWORD_PTR dwUserID)
{
    CAutoLock Lock(&m_ObjectLock);

    ASSERT( m_D3DDev );
    if( m_D3DDev == NULL )
    {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT CDX9AllocatorPresenter::StopPresenting( 
    /* [in] */ DWORD_PTR dwUserID)
{
    return S_OK;
}

HRESULT CDX9AllocatorPresenter::PresentImage( 
    /* [in] */ DWORD_PTR dwUserID,
    /* [in] */ VMR9PresentationInfo *lpPresInfo)
{
    HRESULT hr;
    CAutoLock Lock(&m_ObjectLock);
  if(!m_lpIVMRSurfAllocNotify)
  {
    CLog::Log(LOGERROR, "vmr9:PresentImage() allocNotify not set");
    return E_FAIL;
  }
  if(!lpPresInfo || !lpPresInfo->lpSurf)
  {
    CLog::Log(LOGERROR,"vmr9:PresentImage() no surface");
    return E_POINTER;
  }

  m_pPrevEndFrame=lpPresInfo->rtEnd;
  m_fps = 10000000.0 / (lpPresInfo->rtEnd - lpPresInfo->rtStart);
  
  if (!g_renderManager.IsConfigured())
  {
    CComPtr<IBaseFilter> pVMR9;
    CComPtr<IPin> pPin;
    CMediaType mt;
    if (SUCCEEDED (m_lpIVMRSurfAllocNotify->QueryInterface (__uuidof(IBaseFilter), (void**)&pVMR9)) &&
        SUCCEEDED (pVMR9->FindPin(L"VMR Input0", &pPin)) &&
        SUCCEEDED (pPin->ConnectionMediaType(&mt)) )
    {
      if (mt.formattype==FORMAT_VideoInfo || mt.formattype==FORMAT_MPEGVideo)
      {
        VIDEOINFOHEADER *vh = (VIDEOINFOHEADER*)mt.pbFormat;
        m_iVideoWidth = vh->bmiHeader.biWidth;
        m_iVideoHeight = abs(vh->bmiHeader.biHeight);
        if (vh->rcTarget.right - vh->rcTarget.left > 0)
          m_iVideoWidth = vh->rcTarget.right - vh->rcTarget.left;
        else if (vh->rcSource.right - vh->rcSource.left > 0)
          m_iVideoWidth = vh->rcSource.right - vh->rcSource.left;

        if (vh->rcTarget.bottom - vh->rcTarget.top > 0)
          m_iVideoHeight = vh->rcTarget.bottom - vh->rcTarget.top;
        else if (vh->rcSource.bottom - vh->rcSource.top > 0)
          m_iVideoHeight = vh->rcSource.bottom - vh->rcSource.top;
      }
      else if (mt.formattype==FORMAT_VideoInfo2 || mt.formattype==FORMAT_MPEG2Video)
      {
        VIDEOINFOHEADER2 *vh = (VIDEOINFOHEADER2*)mt.pbFormat;
        m_iVideoWidth = vh->bmiHeader.biWidth;
        m_iVideoHeight = abs(vh->bmiHeader.biHeight);
        if (vh->rcTarget.right - vh->rcTarget.left > 0)
          m_iVideoWidth = vh->rcTarget.right - vh->rcTarget.left;
        else if (vh->rcSource.right - vh->rcSource.left > 0)
          m_iVideoWidth = vh->rcSource.right - vh->rcSource.left;

        if (vh->rcTarget.bottom - vh->rcTarget.top > 0)
          m_iVideoHeight = vh->rcTarget.bottom - vh->rcTarget.top;
        else if (vh->rcSource.bottom - vh->rcSource.top > 0)
          m_iVideoHeight = vh->rcSource.bottom - vh->rcSource.top;
      }
    }
    g_renderManager.Configure(m_iVideoWidth, m_iVideoHeight, m_iVideoWidth, m_iVideoHeight, m_fps, CONF_FLAGS_USE_DIRECTSHOW |CONF_FLAGS_FULLSCREEN);
  }
  
  CComPtr<IDirect3DTexture9> pTexture;
  hr = lpPresInfo->lpSurf->GetContainer(IID_IDirect3DTexture9, (void**)&pTexture);
  if(pTexture)
  {
    // when using lpAllocInfo->dwFlags |= VMR9AllocFlag_TextureSurface;
    // pTexture will be allocated still need to code this
    //g_renderManager.PaintVideoTexture(pTexture);
    //hr = m_D3DDev->StretchRect(lpPresInfo->lpSurf, NULL, m_pVideoSurface, NULL, D3DTEXF_NONE);
  }
  else
  {
    
    hr = m_D3DDev->StretchRect(lpPresInfo->lpSurf, NULL, m_pVideoSurface, NULL, D3DTEXF_NONE);
    g_renderManager.PaintVideoTexture(m_pVideoTexture, m_pVideoSurface);
  }
  g_application.NewFrame();
  //Give .1 sec to the gui to render
  g_application.WaitFrame(100);
  if (CheckDevice())
  {
    HMONITOR hMonitor = g_Windowing.Get3DObject()->GetAdapterMonitor(GetAdapter(g_Windowing.Get3DObject()));
	m_lpIVMRSurfAllocNotify->ChangeD3DDevice(g_Windowing.Get3DDevice(),hMonitor);
    m_D3D = g_Windowing.Get3DObject();
	m_D3DDev = g_Windowing.Get3DDevice();
	//DeleteSurfaces();

    //AllocVideoSurface();
  }
  
  return hr;
}

bool CDX9AllocatorPresenter::CheckDevice()
{
  m_D3DDev->GetCreationParameters(&m_currentD3DParam);

  if ( m_D3D->GetAdapterMonitor(m_currentD3DParam.AdapterOrdinal) != m_D3D->GetAdapterMonitor(GetAdapter(m_D3D) ) )
    return true;

  return false;
}

// IUnknown
HRESULT CDX9AllocatorPresenter::QueryInterface( 
        REFIID riid,
        void** ppvObject)
{
    HRESULT hr = E_NOINTERFACE;

    if( ppvObject == NULL ) {
        hr = E_POINTER;
    } 
    else if( riid == IID_IVMRSurfaceAllocator9 ) {
        *ppvObject = static_cast<IVMRSurfaceAllocator9*>( this );
        AddRef();
        hr = S_OK;
    } 
    else if( riid == IID_IVMRImagePresenter9 ) {
        *ppvObject = static_cast<IVMRImagePresenter9*>( this );
        AddRef();
        hr = S_OK;
    }
    else if( riid == IID_IUnknown ) {
        *ppvObject = 
            static_cast<IUnknown*>( 
            static_cast<IVMRSurfaceAllocator9*>( this ) );
        AddRef();
        hr = S_OK;    
    }

    return hr;
}

ULONG CDX9AllocatorPresenter::AddRef()
{
    return InterlockedIncrement(& m_refCount);
}

ULONG CDX9AllocatorPresenter::Release()
{
    ULONG ret = InterlockedDecrement(& m_refCount);
    if( ret == 0 )
    {
        delete this;
    }

    return ret;
}

UINT CDX9AllocatorPresenter::GetAdapter(IDirect3D9* pD3D)
{
  HMONITOR hMonitor = MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTONEAREST);
  if(hMonitor == NULL) return D3DADAPTER_DEFAULT;

  for(UINT adp = 0, num_adp = pD3D->GetAdapterCount(); adp < num_adp; ++adp)
  {
    HMONITOR hAdpMon = pD3D->GetAdapterMonitor(adp);
    if(hAdpMon == hMonitor) return adp;
  }

  return D3DADAPTER_DEFAULT;
}

STDMETHODIMP CDX9AllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
  CheckPointer(ppRenderer, E_POINTER);

  *ppRenderer = NULL;

  HRESULT hr;

  do
  {
    CMacrovisionKicker* pMK = new CMacrovisionKicker(NAME("CMacrovisionKicker"), NULL);
    CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)pMK;

    COuterVMR9 *pOuter = new COuterVMR9(NAME("COuterVMR9"), pUnk, &m_VMR9AlphaBitmap, this);


    pMK->SetInner((IUnknown*)(INonDelegatingUnknown*)pOuter);
    CComQIPtr<IBaseFilter> pBF = pUnk;

    CComPtr<IPin> pPin = DShowUtil::GetFirstPin(pBF);
    CComQIPtr<IMemInputPin> pMemInputPin = pPin;
    m_fUseInternalTimer = HookNewSegmentAndReceive((IPinC*)(IPin*)pPin, (IMemInputPinC*)(IMemInputPin*)pMemInputPin);

    if(CComQIPtr<IAMVideoAccelerator> pAMVA = pPin)
      HookAMVideoAccelerator((IAMVideoAcceleratorC*)(IAMVideoAccelerator*)pAMVA);

    CComQIPtr<IVMRFilterConfig9> pConfig = pBF;
    if(!pConfig)
      break;

    if(1)//s.fVMR9MixerMode)
    {
      if(FAILED(hr = pConfig->SetNumberOfStreams(1)))
        break;

      if(CComQIPtr<IVMRMixerControl9> pMC = pBF)
      {
        DWORD dwPrefs;
        pMC->GetMixingPrefs(&dwPrefs);  

        // See http://msdn.microsoft.com/en-us/library/dd390928(VS.85).aspx
        dwPrefs |= MixerPref9_NonSquareMixing;
        dwPrefs |= MixerPref9_NoDecimation;
		if(1)//DShowUtil::IsVistaOrAbove())
        {
          dwPrefs &= ~MixerPref9_RenderTargetMask; 
          dwPrefs |= MixerPref9_RenderTargetYUV;
        }
        pMC->SetMixingPrefs(dwPrefs);    
      }
    }

    if(FAILED(hr = pConfig->SetRenderingMode(VMR9Mode_Renderless)))
      break;

    CComQIPtr<IVMRSurfaceAllocatorNotify9> pSAN = pBF;
    if(!pSAN)
      break;
    DWORD_PTR MY_USER_ID = 0xACDCACDC;
    if(FAILED(hr = pSAN->AdviseSurfaceAllocator(MY_USER_ID, static_cast<IVMRSurfaceAllocator9*>(this)))
    || FAILED(hr = AdviseNotify(pSAN)))
      break;

    *ppRenderer = (IUnknown*)pBF.Detach();

    return S_OK;
  }
  while(0);

    return E_FAIL;
}

void CDX9AllocatorPresenter::UpdateAlphaBitmap()
{
  m_VMR9AlphaBitmapData.Free();

  if ((m_VMR9AlphaBitmap.dwFlags & VMRBITMAP_DISABLE) == 0)
  {
    HBITMAP      hBitmap = (HBITMAP)GetCurrentObject (m_VMR9AlphaBitmap.hdc, OBJ_BITMAP);
    if (!hBitmap)
      return;
    DIBSECTION    info = {0};
    if (!::GetObject(hBitmap, sizeof( DIBSECTION ), &info ))
      return;

    m_VMR9AlphaBitmapRect = g_geometryHelper.CreateRect(0, 0, info.dsBm.bmWidth, info.dsBm.bmHeight);
    m_VMR9AlphaBitmapWidthBytes = info.dsBm.bmWidthBytes;

    if (m_VMR9AlphaBitmapData.Allocate(info.dsBm.bmWidthBytes * info.dsBm.bmHeight))
    {
      memcpy((BYTE *)m_VMR9AlphaBitmapData, info.dsBm.bmBits, info.dsBm.bmWidthBytes * info.dsBm.bmHeight);
    }
  }
}
