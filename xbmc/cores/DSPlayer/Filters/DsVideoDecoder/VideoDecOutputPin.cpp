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


#include "VideoDecOutputPin.h"
#include "VideoDecDXVAAllocator.h"
#include "XBMCVideoDecFilter.h"
#include "dshowutil/dshowutil.h"

CVideoDecOutputPin::CVideoDecOutputPin(TCHAR* pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName)
				  : CBaseVideoOutputPin(pObjectName, pFilter, phr, pName)
{
	m_pVideoDecFilter		= static_cast<CXBMCVideoDecFilter*> (pFilter);
	m_pDXVA2Allocator		= NULL;
	m_dwDXVA1SurfaceCount	= 0;
	m_GuidDecoderDXVA1		= GUID_NULL;
	memset (&m_ddUncompPixelFormat, 0, sizeof(m_ddUncompPixelFormat));
}

CVideoDecOutputPin::~CVideoDecOutputPin(void)
{
}


HRESULT CVideoDecOutputPin::InitAllocator(IMemAllocator **ppAlloc)
{
	//TRACE("CVideoDecOutputPin::InitAllocator");
	if (m_pVideoDecFilter->UseDXVA2())
	{
		HRESULT hr = S_FALSE;
		m_pDXVA2Allocator = DNew CVideoDecDXVAAllocator(m_pVideoDecFilter, &hr);
		if (!m_pDXVA2Allocator)
		{
			return E_OUTOFMEMORY;
		}
		if (FAILED(hr))
		{
			delete m_pDXVA2Allocator;
			return hr;
		}
		// Return the IMemAllocator interface.
		return m_pDXVA2Allocator->QueryInterface(__uuidof(IMemAllocator), (void **)ppAlloc);
	}
	else
		return __super::InitAllocator(ppAlloc);
}

STDMETHODIMP CVideoDecOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IAMVideoAcceleratorNotify)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}


// === IAMVideoAcceleratorNotify
STDMETHODIMP CVideoDecOutputPin::GetUncompSurfacesInfo(const GUID *pGuid, LPAMVAUncompBufferInfo pUncompBufferInfo)
{
	HRESULT			hr = E_INVALIDARG;
	
	if (SUCCEEDED (m_pVideoDecFilter->CheckDXVA1Decoder (pGuid)))
	{
		Com::SmartQIPtr<IAMVideoAccelerator>		pAMVideoAccelerator	= GetConnected();

		if (pAMVideoAccelerator)
		{
			pUncompBufferInfo->dwMaxNumSurfaces		= m_pVideoDecFilter->GetPicEntryNumber();
			pUncompBufferInfo->dwMinNumSurfaces		= m_pVideoDecFilter->GetPicEntryNumber();

			hr = m_pVideoDecFilter->FindDXVA1DecoderConfiguration (pAMVideoAccelerator, pGuid, &pUncompBufferInfo->ddUncompPixelFormat);
			if (SUCCEEDED (hr))
			{
				memcpy (&m_ddUncompPixelFormat, &pUncompBufferInfo->ddUncompPixelFormat, sizeof(DDPIXELFORMAT));
				m_GuidDecoderDXVA1 = *pGuid;
			}
		}
	}
	return hr;
}

STDMETHODIMP CVideoDecOutputPin::SetUncompSurfacesInfo(DWORD dwActualUncompSurfacesAllocated)      
{
	m_dwDXVA1SurfaceCount = dwActualUncompSurfacesAllocated;
	return S_OK;
}

STDMETHODIMP CVideoDecOutputPin::GetCreateVideoAcceleratorData(const GUID *pGuid, LPDWORD pdwSizeMiscData, LPVOID *ppMiscData)
{
	HRESULT								hr						= E_UNEXPECTED;
	AMVAUncompDataInfo					UncompInfo;
	AMVACompBufferInfo					CompInfo[30];
	DWORD								dwNumTypesCompBuffers	= countof(CompInfo);
	Com::SmartQIPtr<IAMVideoAccelerator>		pAMVideoAccelerator		= GetConnected();
	DXVA_ConnectMode*					pConnectMode;	

	if (pAMVideoAccelerator)
	{
		memcpy (&UncompInfo.ddUncompPixelFormat, &m_ddUncompPixelFormat, sizeof (DDPIXELFORMAT));
		UncompInfo.dwUncompWidth		= m_pVideoDecFilter->PictWidthRounded();
		UncompInfo.dwUncompHeight		= m_pVideoDecFilter->PictHeightRounded();
		hr = pAMVideoAccelerator->GetCompBufferInfo(&m_GuidDecoderDXVA1, &UncompInfo, &dwNumTypesCompBuffers, CompInfo);

		if (SUCCEEDED (hr))
		{
			hr = m_pVideoDecFilter->CreateDXVA1Decoder (pAMVideoAccelerator, pGuid, m_dwDXVA1SurfaceCount);

			if (SUCCEEDED (hr))
			{
				m_pVideoDecFilter->SetDXVA1Params (&m_GuidDecoderDXVA1, &m_ddUncompPixelFormat);

				pConnectMode					= (DXVA_ConnectMode*)CoTaskMemAlloc (sizeof(DXVA_ConnectMode));
				pConnectMode->guidMode			= m_GuidDecoderDXVA1;
				pConnectMode->wRestrictedMode	= m_pVideoDecFilter->GetDXVA1RestrictedMode();
				*pdwSizeMiscData				= sizeof(DXVA_ConnectMode);
				*ppMiscData						= pConnectMode;
			}
		}
	}


	return hr;
}
