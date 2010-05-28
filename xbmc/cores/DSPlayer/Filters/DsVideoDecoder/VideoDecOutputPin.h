/* 
 * $Id: VideoDecOutputPin.h 1785 2010-04-09 14:12:59Z xhmikosr $
 *
 * (C) 2006-2010 see AUTHORS
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

#pragma once

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "streams.h"
#include <videoacc.h>
#include "../BaseFilters/BaseVideoFilter.h"


class CXBMCVideoDecFilter;
class CVideoDecDXVAAllocator;


class CVideoDecOutputPin : public CBaseVideoOutputPin
						 , public IAMVideoAcceleratorNotify
{
public:
	CVideoDecOutputPin(TCHAR* pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName);

	~CVideoDecOutputPin();

	HRESULT			InitAllocator(IMemAllocator **ppAlloc);

	DECLARE_IUNKNOWN
    STDMETHODIMP	NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IAMVideoAcceleratorNotify
	STDMETHODIMP	GetUncompSurfacesInfo(const GUID *pGuid, LPAMVAUncompBufferInfo pUncompBufferInfo);        
	STDMETHODIMP	SetUncompSurfacesInfo(DWORD dwActualUncompSurfacesAllocated);        
	STDMETHODIMP	GetCreateVideoAcceleratorData(const GUID *pGuid, LPDWORD pdwSizeMiscData, LPVOID *ppMiscData);

private :
	CXBMCVideoDecFilter*			m_pVideoDecFilter;
	CVideoDecDXVAAllocator*		m_pDXVA2Allocator;
	DWORD						m_dwDXVA1SurfaceCount;
	GUID						m_GuidDecoderDXVA1;
	DDPIXELFORMAT				m_ddUncompPixelFormat;
};
