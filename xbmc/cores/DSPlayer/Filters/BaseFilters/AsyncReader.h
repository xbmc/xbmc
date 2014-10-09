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

#pragma once
#include "streams.h"
#include "Subtitles/dsutil/HdmvClipInfo.h"
#include <vector>

[uuid("7D55F67A-826E-40B9-8A7D-3DF0CBBD272D")]
interface IFileHandle : public IUnknown
{
	STDMETHOD_(HANDLE, GetFileHandle)() = 0;
	STDMETHOD_(LPCTSTR, GetFileName)() = 0;
};

class CAsyncFileReader : public CUnknown, public IAsyncReader , public IFileHandle
{
protected:
	ULONGLONG m_len;
	HANDLE m_hBreakEvent;
	LONG m_lOsError; // CFileException::m_lOsError

public:
	CAsyncFileReader(CStdString fn, HRESULT& hr);
  CAsyncFileReader(std::vector<CHdmvClipInfo::PlaylistItem>& Items, HRESULT& hr);

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IAsyncReader

	STDMETHODIMP RequestAllocator(IMemAllocator* pPreferred, ALLOCATOR_PROPERTIES* pProps, IMemAllocator** ppActual) {return E_NOTIMPL;}
    STDMETHODIMP Request(IMediaSample* pSample, DWORD_PTR dwUser) {return E_NOTIMPL;}
    STDMETHODIMP WaitForNext(DWORD dwTimeout, IMediaSample** ppSample, DWORD_PTR* pdwUser) {return E_NOTIMPL;}
	STDMETHODIMP SyncReadAligned(IMediaSample* pSample) {return E_NOTIMPL;}
	STDMETHODIMP SyncRead(LONGLONG llPosition, LONG lLength, BYTE* pBuffer);
	STDMETHODIMP Length(LONGLONG* pTotal, LONGLONG* pAvailable);
	STDMETHODIMP BeginFlush() {return E_NOTIMPL;}
	STDMETHODIMP EndFlush() {return E_NOTIMPL;}

	// IFileHandle

	STDMETHODIMP_(HANDLE) GetFileHandle();
	STDMETHODIMP_(LPCTSTR) GetFileName();

};

class CAsyncUrlReader : public CAsyncFileReader, protected CAMThread
{
	CStdString m_url, m_fn;

protected:
	enum {CMD_EXIT, CMD_INIT};
    DWORD ThreadProc();

public:
	CAsyncUrlReader(CStdString url, HRESULT& hr);
	virtual ~CAsyncUrlReader();

	// IAsyncReader

	STDMETHODIMP Length(LONGLONG* pTotal, LONGLONG* pAvailable);
};
