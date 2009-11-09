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

#include "stdafx.h"
#include <ddraw.h>
#include <d3d.h>
#include "DX7SubPic.h"

//
// CDX7SubPic
//

CDX7SubPic::CDX7SubPic(IDirect3DDevice7* pD3DDev, IDirectDrawSurface7* pSurface)
	: m_pSurface(pSurface)
	, m_pD3DDev(pD3DDev)
{
	DDSURFACEDESC2 ddsd;
	INITDDSTRUCT(ddsd);
	if(SUCCEEDED(m_pSurface->GetSurfaceDesc(&ddsd)))
	{
		m_maxsize.SetSize(ddsd.dwWidth, ddsd.dwHeight);
		m_rcDirty.SetRect(0, 0, ddsd.dwWidth, ddsd.dwHeight);
	}
}

// ISubPic

STDMETHODIMP_(void*) CDX7SubPic::GetObject()
{
	return (void*)(IDirectDrawSurface7*)m_pSurface;
}

STDMETHODIMP CDX7SubPic::GetDesc(SubPicDesc& spd)
{
	DDSURFACEDESC2 ddsd;
	INITDDSTRUCT(ddsd);
	if(FAILED(m_pSurface->GetSurfaceDesc(&ddsd)))
		return E_FAIL;

	spd.type = 0;
	spd.w = m_size.cx;
	spd.h = m_size.cy;
	spd.bpp = (WORD)ddsd.ddpfPixelFormat.dwRGBBitCount;
	spd.pitch = ddsd.lPitch;
	spd.bits = ddsd.lpSurface; // should be NULL
	spd.vidrect = m_vidrect;

	return S_OK;
}

STDMETHODIMP CDX7SubPic::CopyTo(ISubPic* pSubPic)
{
	HRESULT hr;
	if(FAILED(hr = __super::CopyTo(pSubPic)))
		return hr;

	XbmcCPoint p = m_rcDirty.TopLeft();
	hr = m_pD3DDev->Load((IDirectDrawSurface7*)pSubPic->GetObject(), &p, (IDirectDrawSurface7*)GetObject(), m_rcDirty, 0);

	return SUCCEEDED(hr) ? S_OK : E_FAIL;
}

STDMETHODIMP CDX7SubPic::ClearDirtyRect(DWORD color)
{
	if(m_rcDirty.IsRectEmpty())
		return S_FALSE;

	DDBLTFX fx;
	INITDDSTRUCT(fx);
	fx.dwFillColor = color;
	m_pSurface->Blt(&m_rcDirty, NULL, NULL, DDBLT_WAIT|DDBLT_COLORFILL, &fx);
	
	m_rcDirty.SetRectEmpty();

	return S_OK;
}

STDMETHODIMP CDX7SubPic::Lock(SubPicDesc& spd)
{
	DDSURFACEDESC2 ddsd;
	INITDDSTRUCT(ddsd);
	if(FAILED(m_pSurface->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR|DDLOCK_WAIT, NULL)))
		return E_FAIL;

	spd.type = 0;
	spd.w = m_size.cx;
	spd.h = m_size.cy;
	spd.bpp = (WORD)ddsd.ddpfPixelFormat.dwRGBBitCount;
	spd.pitch = ddsd.lPitch;
	spd.bits = ddsd.lpSurface;
	spd.vidrect = m_vidrect;

	return S_OK;
}

STDMETHODIMP CDX7SubPic::Unlock(RECT* pDirtyRect)
{
	m_pSurface->Unlock(NULL);

	if(pDirtyRect)
	{
		m_rcDirty = *pDirtyRect;
		m_rcDirty.InflateRect(1, 1);
		m_rcDirty &= XbmcCRect(XbmcCPoint(0, 0), m_size);
	}
	else
	{
		m_rcDirty = XbmcCRect(XbmcCPoint(0, 0), m_size);
	}

	return S_OK;
}

STDMETHODIMP CDX7SubPic::AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget)
{
	ASSERT(pTarget == NULL);

	if(!m_pD3DDev || !m_pSurface || !pSrc || !pDst)
		return E_POINTER;

	XbmcCRect src(*pSrc), dst(*pDst);

	HRESULT hr;

    do
	{
		DDSURFACEDESC2 ddsd;
		INITDDSTRUCT(ddsd);
		if(FAILED(hr = m_pSurface->GetSurfaceDesc(&ddsd)))
			break;

        float w = (float)ddsd.dwWidth;
        float h = (float)ddsd.dwHeight;

		struct
		{
			float x, y, z, rhw;
			float tu, tv;
		}
		pVertices[] =
		{
			{(float)dst.left, (float)dst.top, 0.5f, 2.0f, (float)src.left / w, (float)src.top / h},
			{(float)dst.right, (float)dst.top, 0.5f, 2.0f, (float)src.right / w, (float)src.top / h},
			{(float)dst.left, (float)dst.bottom, 0.5f, 2.0f, (float)src.left / w, (float)src.bottom / h},
			{(float)dst.right, (float)dst.bottom, 0.5f, 2.0f, (float)src.right / w, (float)src.bottom / h},
		};
/*
		for(int i = 0; i < countof(pVertices); i++)
		{
			pVertices[i].x -= 0.5;
			pVertices[i].y -= 0.5;
		}
*/
        hr = m_pD3DDev->SetTexture(0, m_pSurface);

        m_pD3DDev->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
        m_pD3DDev->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);
        m_pD3DDev->SetRenderState(D3DRENDERSTATE_BLENDENABLE, TRUE);
        m_pD3DDev->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE); // pre-multiplied src and ...
        m_pD3DDev->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCALPHA); // ... inverse alpha channel for dst

		m_pD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        m_pD3DDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

        m_pD3DDev->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
        m_pD3DDev->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
        m_pD3DDev->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_LINEAR);

        m_pD3DDev->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);

		/*//

		D3DDEVICEDESC7 d3ddevdesc;
		m_pD3DDev->GetCaps(&d3ddevdesc);
		if(d3ddevdesc.dpcTriCaps.dwAlphaCmpCaps & D3DPCMPCAPS_LESS)
		{
			m_pD3DDev->SetRenderState(D3DRENDERSTATE_ALPHAREF, (DWORD)0x000000FE);
			m_pD3DDev->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, TRUE); 
			m_pD3DDev->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DPCMPCAPS_LESS);
		}

        *///

        if(FAILED(hr = m_pD3DDev->BeginScene()))
			break;
        
		hr = m_pD3DDev->DrawPrimitive(D3DPT_TRIANGLESTRIP,
										D3DFVF_XYZRHW | D3DFVF_TEX1,
										pVertices, 4, D3DDP_WAIT);
		m_pD3DDev->EndScene();

        //

		m_pD3DDev->SetTexture(0, NULL);

		return S_OK;
    }
	while(0);

    return E_FAIL;
}

//
// CDX7SubPicAllocator
//

CDX7SubPicAllocator::CDX7SubPicAllocator(IDirect3DDevice7* pD3DDev, SIZE maxsize, bool fPow2Textures) 
	: ISubPicAllocatorImpl(maxsize, true, fPow2Textures)
	, m_pD3DDev(pD3DDev)
	, m_maxsize(maxsize)
{
}

// ISubPicAllocator

STDMETHODIMP CDX7SubPicAllocator::ChangeDevice(IUnknown* pDev)
{
	CComQIPtr<IDirect3DDevice7, &IID_IDirect3DDevice7> pD3DDev = pDev;
	if(!pD3DDev) return E_NOINTERFACE;

	CAutoLock cAutoLock(this);
	m_pD3DDev = pD3DDev;

	return __super::ChangeDevice(pDev);
}

// ISubPicAllocatorImpl

bool CDX7SubPicAllocator::Alloc(bool fStatic, ISubPic** ppSubPic)
{
	if(!ppSubPic) 
		return(false);

	CAutoLock cAutoLock(this);

	DDSURFACEDESC2 ddsd;
	INITDDSTRUCT(ddsd);
	ddsd.dwFlags = DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | (fStatic ? DDSCAPS_SYSTEMMEMORY : 0);
	ddsd.ddsCaps.dwCaps2 = fStatic ? 0 : (DDSCAPS2_TEXTUREMANAGE|DDSCAPS2_HINTSTATIC);
	ddsd.dwWidth = m_maxsize.cx;
	ddsd.dwHeight = m_maxsize.cy;
	ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB|DDPF_ALPHAPIXELS;
	ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
	ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000;
	ddsd.ddpfPixelFormat.dwRBitMask        = 0x00FF0000;
	ddsd.ddpfPixelFormat.dwGBitMask        = 0x0000FF00;
	ddsd.ddpfPixelFormat.dwBBitMask        = 0x000000FF;

	if(m_fPow2Textures)
	{
		ddsd.dwWidth = ddsd.dwHeight = 1;
		while(ddsd.dwWidth < (DWORD)m_maxsize.cx) ddsd.dwWidth <<= 1;
		while(ddsd.dwHeight < (DWORD)m_maxsize.cy) ddsd.dwHeight <<= 1;
	}


	CComPtr<IDirect3D7> pD3D;
	CComQIPtr<IDirectDraw7, &IID_IDirectDraw7> pDD;
	if(FAILED(m_pD3DDev->GetDirect3D(&pD3D)) || !pD3D || !(pDD = pD3D))
		return(false);

	CComPtr<IDirectDrawSurface7> pSurface;
	if(FAILED(pDD->CreateSurface(&ddsd, &pSurface, NULL)))
		return(false);

	if(!(*ppSubPic = DNew CDX7SubPic(m_pD3DDev, pSurface)))
		return(false);

	(*ppSubPic)->AddRef();

	return(true);
}
