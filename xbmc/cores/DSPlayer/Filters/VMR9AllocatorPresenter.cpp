/*
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2007 see AUTHORS
 *
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
#include "DSClock.h"
#include "WindowingFactory.h" //d3d device and d3d interface
#include "VMR9AllocatorPresenter.h"
#include "application.h"
#include "utils/log.h"
#include "MacrovisionKicker.h"
#include "IPinHook.h"
#include "GuiSettings.h"
//#include "SystemInfo.h"
#include "SingleLock.h"


class COuterVMR9
  : public CUnknown
  , public IVideoWindow
  , public IBasicVideo2
  , public IVMRWindowlessControl
{
  Com::SmartPtr<IUnknown> m_pVMR;
  CVMR9AllocatorPresenter *m_pAllocatorPresenter;

public:

  COuterVMR9(const TCHAR* pName, LPUNKNOWN pUnk, CVMR9AllocatorPresenter *_pAllocatorPresenter) : CUnknown(pName, pUnk)
  {
    m_pVMR.CoCreateInstance(CLSID_VideoMixingRenderer9, GetOwner());
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
    }

    return SUCCEEDED(hr) ? hr : __super::NonDelegatingQueryInterface(riid, ppv);
  }

  // IVMRWindowlessControl

  STDMETHODIMP GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
  {
   
    if(Com::SmartQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
      return pWC9->GetNativeVideoSize(lpWidth, lpHeight, lpARWidth, lpARHeight);
    
    return E_NOTIMPL;
  }

  STDMETHODIMP GetMinIdealVideoSize(LONG* lpWidth, LONG* lpHeight) {return E_NOTIMPL;}

  STDMETHODIMP GetMaxIdealVideoSize(LONG* lpWidth, LONG* lpHeight) {return E_NOTIMPL;}

  STDMETHODIMP SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect) {return E_NOTIMPL;}

  STDMETHODIMP GetVideoPosition(LPRECT lpSRCRect, LPRECT lpDSTRect)
  {
    HRESULT hr = E_NOTIMPL;
    if(Com::SmartQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
    {
      hr = pWC9->GetVideoPosition(lpSRCRect, lpDSTRect);
    }
    return E_NOTIMPL;
  }

  STDMETHODIMP GetAspectRatioMode(DWORD* lpAspectRatioMode)
  {
    if(Com::SmartQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
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
    if(Com::SmartQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
    {
      tagRECT s, d;
      HRESULT hr = pWC9->GetVideoPosition(&s, &d);
      *pWidth = d.right-d.left;
      return hr;
    }
    return E_NOTIMPL;
  }
  STDMETHODIMP put_Top(long Top) {return E_NOTIMPL;}
  STDMETHODIMP get_Top(long* pTop) {return E_NOTIMPL;}
  STDMETHODIMP put_Height(long Height) {return E_NOTIMPL;}
  STDMETHODIMP get_Height(long* pHeight)
  {
    if(Com::SmartQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
    {
      Com::SmartRect s, d;
      HRESULT hr = pWC9->GetVideoPosition(&s, &d);
      *pHeight = d.Height();
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
    *pLeft = *pTop = 0;
    return GetVideoSize(pWidth, pHeight);
  }
/*
    if(Com::SmartQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
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
  STDMETHODIMP SetDefaultSourcePosition() {return E_NOTIMPL;}
  STDMETHODIMP SetDestinationPosition(long Left, long Top, long Width, long Height) {return E_NOTIMPL;}
  STDMETHODIMP GetDestinationPosition(long* pLeft, long* pTop, long* pWidth, long* pHeight)
  {
    if(Com::SmartQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
    {
      Com::SmartRect s, d;
      HRESULT hr = pWC9->GetVideoPosition(&s, &d);
      *pLeft = d.left;
      *pTop = d.top;
      *pWidth = d.Width();
      *pHeight = d.Height();
      return hr;
    }
    return E_NOTIMPL;
  }
  STDMETHODIMP SetDefaultDestinationPosition() {return E_NOTIMPL;}
  STDMETHODIMP GetVideoSize(long* pWidth, long* pHeight)
  {
    if(Com::SmartQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
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
    HRESULT hr = E_NOTIMPL;
    if(Com::SmartQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
    {
      LONG w, h;
      hr = pWC9->GetNativeVideoSize(&w, &h, plAspectX, plAspectY);
    }
    return hr;
  }
};

CVMR9AllocatorPresenter::CVMR9AllocatorPresenter(HRESULT& hr, CStdString &_Error)
: CDsRenderer(),
  m_pNbrSurface(0),
  m_pCurSurface(0)
{
  hr = S_OK;
  m_bNeedNewDevice = false;
  m_bVmrStop = false;
  m_FlipTimeStamp = 0;
}

CVMR9AllocatorPresenter::~CVMR9AllocatorPresenter()
{
  m_bVmrStop = true;
  DeleteSurfaces();
}

void CVMR9AllocatorPresenter::DeleteSurfaces()
{
  CSingleLock lock(*this);
  CSingleLock cRenderLock(m_RenderLock);
  int k = 0;
  for( size_t i = 0; i < m_pSurfaces.size(); ++i ) 
  {
    SAFE_RELEASE(m_pSurfaces[i]);
  }

  __super::DeleteSurfaces();
}

//IVMRSurfaceAllocator9
STDMETHODIMP CVMR9AllocatorPresenter::InitializeDevice(DWORD_PTR dwUserID ,VMR9AllocationInfo *lpAllocInfo, DWORD *lpNumBuffers)
{
  CSingleLock lock(*this);
  CSingleLock cRenderLock(m_RenderLock);

  CLog::Log(LOGDEBUG,"%s %dx%d AR %d:%d flags:%d buffers:%d  fmt:(%x) %c%c%c%c", __FUNCTION__,
    lpAllocInfo->dwWidth ,lpAllocInfo->dwHeight ,lpAllocInfo->szAspectRatio.cx,lpAllocInfo->szAspectRatio.cy,
    lpAllocInfo->dwFlags ,*lpNumBuffers, lpAllocInfo->Format, ((char)lpAllocInfo->Format&0xff),
    ((char)(lpAllocInfo->Format>>8)&0xff) ,((char)(lpAllocInfo->Format>>16)&0xff) ,((char)(lpAllocInfo->Format>>24)&0xff));

  if(lpAllocInfo->Format == '21VY' || lpAllocInfo->Format == '024I')
    return E_FAIL;

  DeleteSurfaces();

  // Be sure the format is compatible
  D3DDISPLAYMODE dm; 
  ZeroMemory(&dm, sizeof(D3DDISPLAYMODE)); 
  HRESULT hr;
  if (FAILED(g_Windowing.Get3DObject()->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dm)))
  {
    return D3DERR_INVALIDCALL;
  }

  D3DDEVICE_CREATION_PARAMETERS dd;
  if FAILED(g_Windowing.Get3DDevice()->GetCreationParameters(&dd))
    return D3DERR_INVALIDCALL;

  hr = g_Windowing.Get3DObject()->CheckDeviceFormatConversion(D3DADAPTER_DEFAULT, dd.DeviceType, lpAllocInfo->Format, dm.Format); 
  if (lpAllocInfo->Format != D3DFMT_UNKNOWN && FAILED(hr))
  {
    return D3DERR_INVALIDCALL;
  }

  /* If the GC only supports power of two textures, make sure
  our video with & height are powers of two */
  D3DCAPS9 d3dcaps;
  g_Windowing.Get3DDevice()->GetDeviceCaps( &d3dcaps );
  if( d3dcaps.TextureCaps & D3DPTEXTURECAPS_POW2 )
  {
    while( m_iVideoWidth < lpAllocInfo->dwWidth )
      m_iVideoWidth = m_iVideoWidth << 1;
    while( m_iVideoHeight < lpAllocInfo->dwHeight )
      m_iVideoHeight = m_iVideoHeight << 1;
    lpAllocInfo->dwWidth = m_iVideoWidth;
    lpAllocInfo->dwHeight = m_iVideoHeight;
  }

  int nOriginal = *lpNumBuffers;

  if (*lpNumBuffers == 1)
  {
    *lpNumBuffers = 4;
    m_nVMR9Surfaces = 4;
  }
  else
    m_nVMR9Surfaces = 0;

  m_pSurfaces.resize(*lpNumBuffers);

  m_iVideoWidth = lpAllocInfo->dwWidth;
  m_iVideoHeight = abs((int)lpAllocInfo->dwHeight);

  // Is format requires an offscreen surface to convert pixel format in PresentImage from YUV to RGB
  /*if (lpAllocInfo->Format > 0x30303030)
    lpAllocInfo->dwFlags |= VMR9AllocFlag_OffscreenSurface;
  else
    lpAllocInfo->dwFlags |= VMR9AllocFlag_TextureSurface;*/

  /*
   * If VMR9AllocFlag_TextureSurface isn't set, AllocateSurfaceHelper succeeded with YUV2 texture format.
   * However, with VMR9AllocFlag_TextureSurface, we need to set texture format to D3DFMT_X8R8G8B8 in order to
   * AllocateSurfaceHelper to succeed. The (commented) above solution works with YUV & RGB texture format
   * but still no image ... */

  lpAllocInfo->dwFlags = VMR9AllocFlag_3DRenderTarget | VMR9AllocFlag_TextureSurface;
  lpAllocInfo->Format = D3DFMT_X8R8G8B8;

  hr = m_pIVMRSurfAllocNotify->AllocateSurfaceHelper(lpAllocInfo, lpNumBuffers, &m_pSurfaces[0]);
  if(FAILED(hr)) return hr;

  m_pSurfaces.resize(*lpNumBuffers);

  m_bNeedCheckSample = true;

  if(FAILED(hr = CreateSurfaces(dm.Format)))
    return hr;

  if(!(lpAllocInfo->dwFlags & VMR9AllocFlag_TextureSurface))
  {
    // test if the colorspace is acceptable
    if(FAILED(hr = g_Windowing.Get3DDevice()->StretchRect(m_pSurfaces[0], NULL, m_pVideoSurface[m_nCurSurface], NULL, D3DTEXF_NONE)))
    {
      DeleteSurfaces();
      return E_FAIL;
    }
  }

  hr = g_Windowing.Get3DDevice()->ColorFill(m_pVideoSurface[m_nCurSurface], NULL, 0);

  if (m_nVMR9Surfaces && m_nVMR9Surfaces != *lpNumBuffers)
    m_nVMR9Surfaces = *lpNumBuffers;
  *lpNumBuffers = std::min((int) nOriginal, (int)(*lpNumBuffers));
  m_iVMR9Surface = 0;

  return hr;
}

void CVMR9AllocatorPresenter::GetCurrentVideoSize()
{
  m_bNeedCheckSample = false;
  Com::SmartPtr<IBaseFilter>  pVMR9;
  Com::SmartPtr<IPin>      pPin;
  CMediaType        mt;
  REFERENCE_TIME timeperframe = 0;
  if (SUCCEEDED (m_pIVMRSurfAllocNotify->QueryInterface (__uuidof(IBaseFilter), (void**)&pVMR9)) &&
    SUCCEEDED (pVMR9->FindPin(L"VMR Input0", &pPin)) &&
    SUCCEEDED (pPin->ConnectionMediaType(&mt)) )
  {
    DShowUtil::ExtractAvgTimePerFrame(&mt, timeperframe);

    if (mt.formattype == FORMAT_VideoInfo || mt.formattype == FORMAT_MPEGVideo) {

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

    } else if (mt.formattype==FORMAT_VideoInfo2 || mt.formattype==FORMAT_MPEG2Video) {

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

    // If 0 defaulting framerate to 23.97...
    if (timeperframe == 0) 
      timeperframe = 417166;

    m_rtTimePerFrame = (double) (timeperframe / 10);
    m_fps = (float) ( 10000000.0 / timeperframe );    
    g_renderManager.Configure(m_iVideoWidth, m_iVideoHeight, m_iVideoWidth,
      m_iVideoHeight, m_fps, CONF_FLAGS_FULLSCREEN);
  }
}

STDMETHODIMP CVMR9AllocatorPresenter::TerminateDevice(DWORD_PTR dwID)
{
    DeleteSurfaces();
    return S_OK;
}
    
STDMETHODIMP CVMR9AllocatorPresenter::GetSurface(DWORD_PTR dwUserID ,DWORD SurfaceIndex ,DWORD SurfaceFlags ,IDirect3DSurface9 **lplpSurface)
{
  if( !lplpSurface )
    return E_POINTER;

  //return if the surface index is higher than the size of the surfaces we have
  if (SurfaceIndex >= m_pSurfaces.size()) 
    return E_FAIL;
  if (m_bNeedNewDevice)
    return E_FAIL;
  CSingleLock cRenderLock(m_RenderLock);
  if (m_nVMR9Surfaces)
  {
    ++m_iVMR9Surface;
    m_iVMR9Surface = m_iVMR9Surface % m_nVMR9Surfaces;
    (*lplpSurface = m_pSurfaces[m_iVMR9Surface + SurfaceIndex])->AddRef();
  }
  else
  {
    m_iVMR9Surface = SurfaceIndex;
    (*lplpSurface = m_pSurfaces[SurfaceIndex])->AddRef();
  }

  return S_OK;
}
    
STDMETHODIMP CVMR9AllocatorPresenter::AdviseNotify(IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify)
{
  CSingleLock lock(*this);
  CSingleLock cRenderLock(m_RenderLock);
  HRESULT hr;
  m_pIVMRSurfAllocNotify = lpIVMRSurfAllocNotify;
  HMONITOR hMonitor = g_Windowing.Get3DObject()->GetAdapterMonitor(GetAdapter(g_Windowing.Get3DObject()));
  hr = m_pIVMRSurfAllocNotify->SetD3DDevice( g_Windowing.Get3DDevice(), hMonitor);
  return hr;
}

STDMETHODIMP CVMR9AllocatorPresenter::StartPresenting(DWORD_PTR dwUserID)
{
  CSingleLock lock(*this);
  CSingleLock cRenderLock(m_RenderLock);
  HRESULT hr = S_OK;
  int i = 5;
  
  while (! g_Windowing.Get3DDevice() || !i--)
    Sleep(100);

  if( !g_Windowing.Get3DDevice() )
    hr =  E_FAIL;
  
  return hr;
}

STDMETHODIMP CVMR9AllocatorPresenter::StopPresenting(DWORD_PTR dwUserID)
{
    return S_OK;
}

STDMETHODIMP CVMR9AllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
  CheckPointer(ppRenderer, E_POINTER);

  HRESULT hr;
  *ppRenderer = NULL;

  do
  {
    CMacrovisionKicker* pMK = DNew CMacrovisionKicker(NAME("CMacrovisionKicker"), NULL);
    Com::SmartPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)pMK;

    COuterVMR9 *pOuter = DNew COuterVMR9(NAME("COuterVMR9"), pUnk, this);


    pMK->SetInner((IUnknown*)(INonDelegatingUnknown*)pOuter);
    Com::SmartQIPtr<IBaseFilter> pBF = pUnk;

    Com::SmartPtr<IPin> pPin = DShowUtil::GetFirstPin(pBF);
    Com::SmartQIPtr<IMemInputPin> pMemInputPin = pPin;
    m_fUseInternalTimer = HookNewSegmentAndReceive((IPinC*)(IPin*)pPin, (IMemInputPinC*)(IMemInputPin*)pMemInputPin);

    if(Com::SmartQIPtr<IAMVideoAccelerator> pAMVA = pPin)
      HookAMVideoAccelerator((IAMVideoAcceleratorC*)(IAMVideoAccelerator*)pAMVA);

    Com::SmartQIPtr<IVMRFilterConfig9> pConfig = pBF;
    if(!pConfig)
      break;

    if(FAILED(hr = pConfig->SetNumberOfStreams(1)))
      break;

    if(Com::SmartQIPtr<IVMRMixerControl9> pMC = pBF)
    {
      DWORD dwPrefs;
      pMC->GetMixingPrefs(&dwPrefs);

        // See http://msdn.microsoft.com/en-us/library/dd390928(VS.85).aspx
      dwPrefs |= MixerPref9_NonSquareMixing;
      dwPrefs |= MixerPref9_NoDecimation;
      dwPrefs &= ~MixerPref9_RenderTargetMask; 
      dwPrefs |= MixerPref9_RenderTargetYUV; // Need this or xbmc freeze. But in YUV, AllocateSurfaceHelper failed (in win7)

      pMC->SetMixingPrefs(dwPrefs);
    }

    if(FAILED(hr = pConfig->SetRenderingMode(VMR9Mode_Renderless)))
      break;

    Com::SmartQIPtr<IVMRSurfaceAllocatorNotify9> pSAN = pBF;
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

STDMETHODIMP CVMR9AllocatorPresenter::PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo *lpPresInfo)
{
  if (!g_renderManager.IsConfigured() || m_rtTimePerFrame == 0 || m_bNeedCheckSample)
  {
    GetCurrentVideoSize();
  }

  if (!g_renderManager.IsStarted() || m_bNeedNewDevice)
    return S_OK;

  if(!lpPresInfo || !lpPresInfo->lpSurf)
    return E_POINTER;

  if ( m_FlipTimeStamp == 0 )
    m_FlipTimeStamp = g_DsClock.GetAbsoluteClock();

  CSingleLock Lock(m_RenderLock);

  Com::SmartPtr<IDirect3DTexture9> pTexture;
  lpPresInfo->lpSurf->GetContainer(IID_IDirect3DTexture9, (void**)&pTexture);

  if(pTexture)
  {
    m_pVideoSurface[m_nCurSurface] = lpPresInfo->lpSurf;
    if(m_pVideoTexture[m_nCurSurface])
      m_pVideoTexture[m_nCurSurface] = pTexture;
  }
  else
  {
    g_Windowing.Get3DDevice()->StretchRect(lpPresInfo->lpSurf, NULL, m_pVideoSurface[m_nCurSurface], NULL, D3DTEXF_NONE);
  }

  g_renderManager.PaintVideoTexture(m_pVideoTexture[m_nCurSurface], m_pVideoSurface[m_nCurSurface]);
  g_application.NewFrame();
  g_application.WaitFrame(100);
  return S_OK;
  //TODO find where i did the miscalculation when converting directshow 100 nanosec units to the one used by xbmc 1000 nanosecunits 
  double iSleepTime, iClockSleep, iFrameSleep, iCurrentClock, iFrameDuration,pts;
  
  pts = (double)(lpPresInfo->rtStart); //converting reference tiem to the current base we use for calculating the flip
  iCurrentClock = CDSClock::GetAbsoluteClock(); // snapshot current clock
  iClockSleep = pts - g_DsClock.GetClock();  //sleep calculated by pts to clock comparison
  iFrameSleep = m_FlipTimeStamp - iCurrentClock; // sleep calculated by duration of frame
  iFrameDuration = m_rtTimePerFrame*10;//pPicture->iDuration;  

  // dropping to a very low framerate is not correct (it should not happen at all)
  iClockSleep = min(iClockSleep, (double) MSEC_TO_DS_TIME(500));
  iFrameSleep = min(iFrameSleep, (double) MSEC_TO_DS_TIME(500));
  
  iSleepTime = iFrameSleep + (iClockSleep - iFrameSleep);

  m_iCurrentPts = pts - max(0.0, iSleepTime);

// timestamp when we think next picture should be displayed based on current duration
  m_FlipTimeStamp  = iCurrentClock;
  m_FlipTimeStamp += max(0.0, iSleepTime);
  m_FlipTimeStamp += iFrameDuration;
  
  g_renderManager.FlipPage(m_bVmrStop, (iCurrentClock + iSleepTime) / DS_TIME_BASE);

  return S_OK;
}

HRESULT CVMR9AllocatorPresenter::ChangeD3dDev()
{
  HRESULT hr;
  //DeleteSurfaces();
  hr = m_pIVMRSurfAllocNotify->ChangeD3DDevice(g_Windowing.Get3DDevice(),g_Windowing.Get3DObject()->GetAdapterMonitor(GetAdapter(g_Windowing.Get3DObject())));
  if (SUCCEEDED(hr))
  {
    CLog::Log(LOGDEBUG,"%s Changed d3d device",__FUNCTION__);
    m_bNeedNewDevice = false;
  }
  return hr;
}

void CVMR9AllocatorPresenter::OnLostDevice()
{
  //CLog::Log(LOGDEBUG,"%s",__FUNCTION__);
}

void CVMR9AllocatorPresenter::OnDestroyDevice()
{
  //Only this one is required for changing the device
  CLog::Log(LOGDEBUG,"%s",__FUNCTION__);
  m_bNeedNewDevice = true;
  DeleteSurfaces();
}

void CVMR9AllocatorPresenter::OnCreateDevice()
{
  CLog::Log(LOGDEBUG,"%s",__FUNCTION__);
}

void CVMR9AllocatorPresenter::OnResetDevice()
{
  ChangeD3dDev();
  CLog::Log(LOGDEBUG,"%s",__FUNCTION__);
}


STDMETHODIMP CVMR9AllocatorPresenter::NonDelegatingQueryInterface( REFIID riid, void** ppv )
{
  CheckPointer(ppv, E_POINTER);

  return 
    QI(IVMRSurfaceAllocator9)
    QI(IVMRImagePresenter9)
    __super::NonDelegatingQueryInterface(riid, ppv);
}
