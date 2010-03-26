/*
 * $Id: IPinHook.h 380 2008-01-19 11:56:29Z casimir666 $
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

#pragma once

interface IPinC;

typedef struct IPinCVtbl
{
	BEGIN_INTERFACE
		HRESULT ( STDMETHODCALLTYPE *QueryInterface )( IPinC * This, /* [in] */ REFIID riid, /* [iid_is][out] */ void **ppvObject );
		ULONG ( STDMETHODCALLTYPE *AddRef )( IPinC * This );
		ULONG ( STDMETHODCALLTYPE *Release )( IPinC * This );
		HRESULT ( STDMETHODCALLTYPE *Connect )( IPinC * This, /* [in] */ IPinC *pReceivePin, /* [in] */ const AM_MEDIA_TYPE *pmt );
		HRESULT ( STDMETHODCALLTYPE *ReceiveConnection )( IPinC * This, /* [in] */ IPinC *pConnector, /* [in] */ const AM_MEDIA_TYPE *pmt );
		HRESULT ( STDMETHODCALLTYPE *Disconnect )( IPinC * This );
		HRESULT ( STDMETHODCALLTYPE *ConnectedTo )( IPinC * This, /* [out] */ IPinC **pPin );
		HRESULT ( STDMETHODCALLTYPE *ConnectionMediaType )( IPinC * This, /* [out] */ AM_MEDIA_TYPE *pmt );
		HRESULT ( STDMETHODCALLTYPE *QueryPinInfo )( IPinC * This, /* [out] */ PIN_INFO *pInfo );
		HRESULT ( STDMETHODCALLTYPE *QueryDirection )( IPinC * This, /* [out] */ PIN_DIRECTION *pPinDir );
		HRESULT ( STDMETHODCALLTYPE *QueryId )( IPinC * This, /* [out] */ LPWSTR *Id );
		HRESULT ( STDMETHODCALLTYPE *QueryAccept )( IPinC * This, /* [in] */ const AM_MEDIA_TYPE *pmt );
		HRESULT ( STDMETHODCALLTYPE *EnumMediaTypes )( IPinC * This, /* [out] */ IEnumMediaTypes **ppEnum );
		HRESULT ( STDMETHODCALLTYPE *QueryInternalConnections )( IPinC * This, /* [out] */ IPinC **apPin, /* [out][in] */ ULONG *nPin );
		HRESULT ( STDMETHODCALLTYPE *EndOfStream )( IPinC * This );
		HRESULT ( STDMETHODCALLTYPE *BeginFlush )( IPinC * This );
		HRESULT ( STDMETHODCALLTYPE *EndFlush )( IPinC * This );
		HRESULT ( STDMETHODCALLTYPE *NewSegment )( IPinC * This, /* [in] */ REFERENCE_TIME tStart, /* [in] */ REFERENCE_TIME tStop, /* [in] */ double dRate );
	END_INTERFACE
} IPinCVtbl;

interface IPinC
{
    CONST_VTBL struct IPinCVtbl *lpVtbl;
};

interface IMemInputPinC;

typedef struct IMemInputPinCVtbl
{
	BEGIN_INTERFACE
		HRESULT ( STDMETHODCALLTYPE *QueryInterface )( IPinC * This, /* [in] */ REFIID riid, /* [iid_is][out] */ void **ppvObject );
		ULONG ( STDMETHODCALLTYPE *AddRef )( IPinC * This );
		ULONG ( STDMETHODCALLTYPE *Release )( IPinC * This );
		HRESULT ( STDMETHODCALLTYPE *GetAllocator )( IMemInputPinC * This, IMemAllocator **ppAllocator);
		HRESULT ( STDMETHODCALLTYPE *NotifyAllocator )( IMemInputPinC * This, IMemAllocator *pAllocator, BOOL bReadOnly);
		HRESULT ( STDMETHODCALLTYPE *GetAllocatorRequirements )( IMemInputPinC * This, ALLOCATOR_PROPERTIES *pProps);
		HRESULT ( STDMETHODCALLTYPE *Receive )( IMemInputPinC * This, IMediaSample *pSample);
		HRESULT ( STDMETHODCALLTYPE *ReceiveMultiple )( IMemInputPinC * This, IMediaSample **pSamples, long nSamples, long *nSamplesProcessed);
		HRESULT ( STDMETHODCALLTYPE *ReceiveCanBlock )( IMemInputPinC * This);
	END_INTERFACE
} IMemInputPinCVtbl;

interface IMemInputPinC
{
    CONST_VTBL struct IMemInputPinCVtbl *lpVtbl;
};

extern bool HookNewSegmentAndReceive(IPinC* pPinC, IMemInputPinC* pMemInputPin);
extern void UnhookNewSegmentAndReceive();
extern REFERENCE_TIME g_tSegmentStart, g_tSampleStart, g_rtTimePerFrame;
