/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
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

//#include "StdAfx.h"
#include "NullRenderers.h"
#include <moreuuids.h>

#define USE_DXVA

#ifdef USE_DXVA

#include <d3dx9.h>
#include <dxva.h>
#include <dxva2api.h>		// DXVA2
#include <evr.h>
//#include <mfidl.h>
#include <mfapi.h>	// API Media Foundation
#include <Mferror.h>

// dxva.dll
typedef HRESULT (__stdcall *PTR_DXVA2CreateDirect3DDeviceManager9)(UINT* pResetToken, IDirect3DDeviceManager9** ppDeviceManager);
typedef HRESULT (__stdcall *PTR_DXVA2CreateVideoService)(IDirect3DDevice9* pDD, REFIID riid, void** ppService);


class CNullVideoRendererInputPin : public CRendererInputPin, 
								   public IMFGetService,
								   public IDirectXVideoMemoryConfiguration,
								   public IMFVideoDisplayControl
{
public :
    CNullVideoRendererInputPin(CBaseRenderer *pRenderer, HRESULT *phr, LPCWSTR Name);

	DECLARE_IUNKNOWN
    STDMETHODIMP	NonDelegatingQueryInterface(REFIID riid, void** ppv);

	STDMETHODIMP	GetAllocator(IMemAllocator **ppAllocator)
	{
		// Renderer shouldn't manage allocator for DXVA
		return E_NOTIMPL;
	}

	STDMETHODIMP	GetAllocatorRequirements(ALLOCATOR_PROPERTIES* pProps)
	{
		// 1 buffer required
		memset (pProps, 0, sizeof(ALLOCATOR_PROPERTIES));
		pProps->cbBuffer = 1;
		return S_OK;
	}


	// IMFGetService
	STDMETHODIMP	GetService(REFGUID guidService, REFIID riid, LPVOID *ppvObject);

	// IDirectXVideoMemoryConfiguration
	STDMETHODIMP	GetAvailableSurfaceTypeByIndex(DWORD dwTypeIndex, DXVA2_SurfaceType *pdwType);
	STDMETHODIMP	SetSurfaceType(DXVA2_SurfaceType dwType);


	// IMFVideoDisplayControl
	STDMETHODIMP	GetNativeVideoSize(SIZE *pszVideo, SIZE *pszARVideo) { return E_NOTIMPL; };        
	STDMETHODIMP	GetIdealVideoSize(SIZE *pszMin, SIZE *pszMax) { return E_NOTIMPL; };        
	STDMETHODIMP	SetVideoPosition(const MFVideoNormalizedRect *pnrcSource, const LPRECT prcDest) { return E_NOTIMPL; };        
	STDMETHODIMP	GetVideoPosition(MFVideoNormalizedRect *pnrcSource, LPRECT prcDest) { return E_NOTIMPL; };        
	STDMETHODIMP	SetAspectRatioMode(DWORD dwAspectRatioMode) { return E_NOTIMPL; };        
	STDMETHODIMP	GetAspectRatioMode(DWORD *pdwAspectRatioMode) { return E_NOTIMPL; };        
	STDMETHODIMP	SetVideoWindow(HWND hwndVideo) { return E_NOTIMPL; };        
	STDMETHODIMP	GetVideoWindow(HWND *phwndVideo);        
	STDMETHODIMP	RepaintVideo( void) { return E_NOTIMPL; };        
	STDMETHODIMP	GetCurrentImage(BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, LONGLONG *pTimeStamp) { return E_NOTIMPL; };        
	STDMETHODIMP	SetBorderColor(COLORREF Clr) { return E_NOTIMPL; };        
	STDMETHODIMP	GetBorderColor(COLORREF *pClr) { return E_NOTIMPL; };        
	STDMETHODIMP	SetRenderingPrefs(DWORD dwRenderFlags) { return E_NOTIMPL; };        
	STDMETHODIMP	GetRenderingPrefs(DWORD *pdwRenderFlags) { return E_NOTIMPL; };        
	STDMETHODIMP	SetFullscreen(BOOL fFullscreen) { return E_NOTIMPL; };        
	STDMETHODIMP	GetFullscreen(BOOL *pfFullscreen) { return E_NOTIMPL; };

private :

	PTR_DXVA2CreateDirect3DDeviceManager9	pfDXVA2CreateDirect3DDeviceManager9;
	PTR_DXVA2CreateVideoService				pfDXVA2CreateVideoService;

	CComPtr<IDirect3D9>						m_pD3D;
	CComPtr<IDirect3DDevice9>				m_pD3DDev;
	CComPtr<IDirect3DDeviceManager9>		m_pD3DDeviceManager;
	UINT									m_nResetTocken;
	HANDLE									m_hDevice;
	HWND									m_hWnd;

	void		CreateSurface();
};



CNullVideoRendererInputPin::CNullVideoRendererInputPin(CBaseRenderer *pRenderer, HRESULT *phr, LPCWSTR Name)
	: CRendererInputPin(pRenderer, phr, Name)
{
	HMODULE		hLib;

	CreateSurface();

	hLib = LoadLibrary (L"dxva2.dll");
	pfDXVA2CreateDirect3DDeviceManager9	= hLib ? (PTR_DXVA2CreateDirect3DDeviceManager9) GetProcAddress (hLib, "DXVA2CreateDirect3DDeviceManager9") : NULL;
	pfDXVA2CreateVideoService			= hLib ? (PTR_DXVA2CreateVideoService)           GetProcAddress (hLib, "DXVA2CreateVideoService") : NULL;

	HRESULT hr;
	if (hLib != NULL)
	{
		pfDXVA2CreateDirect3DDeviceManager9 (&m_nResetTocken, &m_pD3DDeviceManager);
	}

	// Initialize Device Manager with DX surface
	if (m_pD3DDev)
	{
		hr = m_pD3DDeviceManager->ResetDevice (m_pD3DDev, m_nResetTocken);
		hr = m_pD3DDeviceManager->OpenDeviceHandle(&m_hDevice);
	}
}


void CNullVideoRendererInputPin::CreateSurface()
{
	HRESULT		hr;
	m_pD3D.Attach(Direct3DCreate9(D3D_SDK_VERSION));
	if(!m_pD3D) 
	{
		m_pD3D.Attach(Direct3DCreate9(D3D9b_SDK_VERSION));
	}

	m_hWnd = NULL;	// TODO : put true window

	D3DDISPLAYMODE d3ddm;
	ZeroMemory(&d3ddm, sizeof(d3ddm));
	m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp, sizeof(pp));

	pp.Windowed = TRUE;
	pp.hDeviceWindow = m_hWnd;
	pp.SwapEffect = D3DSWAPEFFECT_COPY;
	pp.Flags = D3DPRESENTFLAG_VIDEO;
	pp.BackBufferCount = 1; 
	pp.BackBufferWidth = d3ddm.Width;
	pp.BackBufferHeight = d3ddm.Height;
	pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	hr = m_pD3D->CreateDevice(
						D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd,
						D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED, //D3DCREATE_MANAGED 
						&pp, &m_pD3DDev);
}

STDMETHODIMP CNullVideoRendererInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		(riid == __uuidof(IMFGetService)) ? GetInterface((IMFGetService*)this, ppv) :
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CNullVideoRendererInputPin::GetService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
	if (m_pD3DDeviceManager != NULL && guidService == MR_VIDEO_ACCELERATION_SERVICE)
	{
		if (riid == __uuidof(IDirect3DDeviceManager9))
		{
			return m_pD3DDeviceManager->QueryInterface (riid, ppvObject);
		}
		else if (riid == __uuidof(IDirectXVideoDecoderService) || riid == __uuidof(IDirectXVideoProcessorService) )
		{
			return m_pD3DDeviceManager->GetVideoService (m_hDevice, riid, ppvObject);
		}
		else if (riid == __uuidof(IDirectXVideoAccelerationService))
		{
			// TODO : to be tested....
			return pfDXVA2CreateVideoService(m_pD3DDev, riid, ppvObject);
		}
		else if (riid == __uuidof(IDirectXVideoMemoryConfiguration))
		{
			GetInterface ((IDirectXVideoMemoryConfiguration*)this, ppvObject);
			return S_OK;
		}
	}
	else if (guidService == MR_VIDEO_RENDER_SERVICE)
	{
		if (riid == __uuidof(IMFVideoDisplayControl))
		{
			GetInterface ((IMFVideoDisplayControl*)this, ppvObject);
			return S_OK;
		}
	}
	//else if (guidService == MR_VIDEO_MIXER_SERVICE)
	//{
	//	if (riid == __uuidof(IMFVideoMixerBitmap))
	//	{
	//		GetInterface ((IMFVideoMixerBitmap*)this, ppvObject);
	//		return S_OK;
	//	}
	//}
	return E_NOINTERFACE;
}


STDMETHODIMP CNullVideoRendererInputPin::GetAvailableSurfaceTypeByIndex(DWORD dwTypeIndex, DXVA2_SurfaceType *pdwType)
{
	if (dwTypeIndex == 0)
	{
		*pdwType = DXVA2_SurfaceType_DecoderRenderTarget;
		return S_OK;
	}
	else
		return MF_E_NO_MORE_TYPES;
}

STDMETHODIMP CNullVideoRendererInputPin::SetSurfaceType(DXVA2_SurfaceType dwType)
{
	return S_OK;
}


STDMETHODIMP CNullVideoRendererInputPin::GetVideoWindow(HWND *phwndVideo)
{
	CheckPointer(phwndVideo, E_POINTER);
	*phwndVideo = m_hWnd;	// Important to implement this method (used by mpc)
	return S_OK;
}


#endif

//
// CNullRenderer
//

CNullRenderer::CNullRenderer(REFCLSID clsid, TCHAR* pName, LPUNKNOWN pUnk, HRESULT* phr) 
	: CBaseRenderer(clsid, pName, pUnk, phr)
{
}

//
// CNullVideoRenderer
//

CNullVideoRenderer::CNullVideoRenderer(LPUNKNOWN pUnk, HRESULT* phr) 
	: CNullRenderer(__uuidof(this), NAME("Null Video Renderer"), pUnk, phr)
{
}

HRESULT CNullVideoRenderer::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Video
		|| pmt->subtype == MEDIASUBTYPE_MPEG2_VIDEO
		? S_OK
		: E_FAIL;
}

//
// CNullUVideoRenderer
//

CNullUVideoRenderer::CNullUVideoRenderer(LPUNKNOWN pUnk, HRESULT* phr) 
	: CNullRenderer(__uuidof(this), NAME("Null Video Renderer (Uncompressed)"), pUnk, phr)
{
#ifdef USE_DXVA
	m_pInputPin = new CNullVideoRendererInputPin(this,phr,L"In");
#endif
}

HRESULT CNullUVideoRenderer::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Video
		&& (pmt->subtype == MEDIASUBTYPE_YV12
		|| pmt->subtype == MEDIASUBTYPE_I420
		|| pmt->subtype == MEDIASUBTYPE_YUYV
		|| pmt->subtype == MEDIASUBTYPE_IYUV
		|| pmt->subtype == MEDIASUBTYPE_YVU9
		|| pmt->subtype == MEDIASUBTYPE_Y411
		|| pmt->subtype == MEDIASUBTYPE_Y41P
		|| pmt->subtype == MEDIASUBTYPE_YUY2
		|| pmt->subtype == MEDIASUBTYPE_YVYU
		|| pmt->subtype == MEDIASUBTYPE_UYVY
		|| pmt->subtype == MEDIASUBTYPE_Y211
		|| pmt->subtype == MEDIASUBTYPE_RGB1
		|| pmt->subtype == MEDIASUBTYPE_RGB4
		|| pmt->subtype == MEDIASUBTYPE_RGB8
		|| pmt->subtype == MEDIASUBTYPE_RGB565
		|| pmt->subtype == MEDIASUBTYPE_RGB555
		|| pmt->subtype == MEDIASUBTYPE_RGB24
		|| pmt->subtype == MEDIASUBTYPE_RGB32
		|| pmt->subtype == MEDIASUBTYPE_ARGB1555
		|| pmt->subtype == MEDIASUBTYPE_ARGB4444
		|| pmt->subtype == MEDIASUBTYPE_ARGB32
		|| pmt->subtype == MEDIASUBTYPE_A2R10G10B10
		|| pmt->subtype == MEDIASUBTYPE_A2B10G10R10)
		? S_OK
		: E_FAIL;
}

HRESULT CNullUVideoRenderer::DoRenderSample(IMediaSample* pSample) 
{
#ifdef USE_DXVA
	CComQIPtr<IMFGetService>		pService = pSample;
	if (pService != NULL)
	{
		CComPtr<IDirect3DSurface9>	pSurface;
		pService->GetService (MR_BUFFER_SERVICE, __uuidof(IDirect3DSurface9), (void**)&pSurface);
		// TODO : render surface...
	}
#endif

	return S_OK;
}

//
// CNullAudioRenderer
//

CNullAudioRenderer::CNullAudioRenderer(LPUNKNOWN pUnk, HRESULT* phr) 
	: CNullRenderer(__uuidof(this), NAME("Null Audio Renderer"), pUnk, phr)
{
}

HRESULT CNullAudioRenderer::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Audio
		|| pmt->majortype == MEDIATYPE_Midi
		|| pmt->subtype == MEDIASUBTYPE_MPEG2_AUDIO
		|| pmt->subtype == MEDIASUBTYPE_DOLBY_AC3
		|| pmt->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
		|| pmt->subtype == MEDIASUBTYPE_DTS
		|| pmt->subtype == MEDIASUBTYPE_SDDS
		|| pmt->subtype == MEDIASUBTYPE_MPEG1AudioPayload
		|| pmt->subtype == MEDIASUBTYPE_MPEG1Audio
		|| pmt->subtype == MEDIASUBTYPE_MPEG1Audio
		? S_OK
		: E_FAIL;
}

//
// CNullUAudioRenderer
//

CNullUAudioRenderer::CNullUAudioRenderer(LPUNKNOWN pUnk, HRESULT* phr) 
	: CNullRenderer(__uuidof(this), NAME("Null Audio Renderer (Uncompressed)"), pUnk, phr)
{
}

HRESULT CNullUAudioRenderer::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Audio
		&& (pmt->subtype == MEDIASUBTYPE_PCM
		|| pmt->subtype == MEDIASUBTYPE_IEEE_FLOAT
		|| pmt->subtype == MEDIASUBTYPE_DRM_Audio
		|| pmt->subtype == MEDIASUBTYPE_DOLBY_AC3_SPDIF
		|| pmt->subtype == MEDIASUBTYPE_RAW_SPORT
		|| pmt->subtype == MEDIASUBTYPE_SPDIF_TAG_241h)
		? S_OK
		: E_FAIL;
}

//
// CNullTextRenderer
//

HRESULT CNullTextRenderer::CTextInputPin::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Text
		|| pmt->majortype == MEDIATYPE_ScriptCommand
		|| pmt->majortype == MEDIATYPE_Subtitle 
		|| pmt->subtype == MEDIASUBTYPE_DVD_SUBPICTURE 
		|| pmt->subtype == MEDIASUBTYPE_CVD_SUBPICTURE 
		|| pmt->subtype == MEDIASUBTYPE_SVCD_SUBPICTURE 
		? S_OK 
		: E_FAIL;
}

#pragma warning (disable : 4355)
CNullTextRenderer::CNullTextRenderer(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseFilter(NAME("CNullTextRenderer"), pUnk, this, __uuidof(this), phr) 
{
	m_pInput.Attach(DNew CTextInputPin(this, this, phr));
}
#pragma warning (default : 4355)
