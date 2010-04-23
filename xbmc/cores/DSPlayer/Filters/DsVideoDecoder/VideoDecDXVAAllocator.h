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
//#include "streams.h"
#include "DShowUtil/DShowUtil.h"
#include <dxva.h>
#include <dxva2api.h>
#include "mfidl.h"

class CXBMCVideoDecFilter;
class CVideoDecDXVAAllocator;


[uuid("AE7EC2A2-1913-4a80-8DD6-DF1497ABA494")]
interface IMPCDXVA2Sample : public IUnknown
{
	STDMETHOD_(int, GetDXSurfaceId()) = 0;
};

class CDXVA2Sample : public CMediaSample, public IMFGetService, public IMPCDXVA2Sample
{
    friend class CVideoDecDXVAAllocator;

public:

    CDXVA2Sample(CVideoDecDXVAAllocator *pAlloc, HRESULT *phr);

    // Note: CMediaSample does not derive from CUnknown, so we cannot use the
    //       DECLARE_IUNKNOWN macro that is used by most of the filter classes.

	STDMETHODIMP			QueryInterface(REFIID riid, __deref_out void **ppv);
    STDMETHODIMP_(ULONG)	AddRef();
    STDMETHODIMP_(ULONG)	Release();

    // IMFGetService::GetService
    STDMETHODIMP GetService(REFGUID guidService, REFIID riid, LPVOID *ppv);

	// IMPCDXVA2Sample
	STDMETHODIMP_(int) GetDXSurfaceId();

    // Override GetPointer because this class does not manage a system memory buffer.
    // The EVR uses the MR_BUFFER_SERVICE service to get the Direct3D surface.
    STDMETHODIMP GetPointer(BYTE ** ppBuffer);

private:

    // Sets the pointer to the Direct3D surface. 
    void SetSurface(DWORD surfaceId, IDirect3DSurface9 *pSurf);

    Com::SmartPtr<IDirect3DSurface9>	m_pSurface;
    DWORD						m_dwSurfaceId;
};




class CVideoDecDXVAAllocator : public CBaseAllocator
{
public:
	CVideoDecDXVAAllocator(CXBMCVideoDecFilter* pVideoDecFilter, HRESULT* phr);
	virtual ~CVideoDecDXVAAllocator();

 //   STDMETHODIMP GetBuffer(__deref_out IMediaSample **ppBuffer,		// Try for a circular buffer!
 //                          __in_opt REFERENCE_TIME * pStartTime,
 //                          __in_opt REFERENCE_TIME * pEndTime,
 //                          DWORD dwFlags);
 //
 //    STDMETHODIMP ReleaseBuffer(IMediaSample *pBuffer);
 //	    CAtlList<int>				m_FreeSurface;


protected:
	HRESULT		Alloc(void);
	void		Free(void);


private :
	CXBMCVideoDecFilter*		m_pVideoDecFilter;

	IDirect3DSurface9**		m_ppRTSurfaceArray;
	UINT					m_nSurfaceArrayCount;

};
