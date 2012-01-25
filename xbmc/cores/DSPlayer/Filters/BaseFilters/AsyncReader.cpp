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



#include "AsyncReader.h"
//#include <afxsock.h>
//#include <afxinet.h>
#include "DShowUtil/DShowUtil.h"

//
// CAsyncFileReader
//

CAsyncFileReader::CAsyncFileReader(CStdString fn, HRESULT& hr) 
	: CUnknown(NAME("CAsyncFileReader"), NULL, &hr)
	, m_len(-1)
	, m_hBreakEvent(NULL)
	, m_lOsError(0)
{
	hr = Open(fn, modeRead|shareDenyNone|typeBinary|osSequentialScan) ? S_OK : E_FAIL;
	if(SUCCEEDED(hr)) m_len = GetLength();
}

CAsyncFileReader::CAsyncFileReader(CAtlList<CHdmvClipInfo::PlaylistItem>& Items, HRESULT& hr) 
	: CUnknown(NAME("CAsyncFileReader"), NULL, &hr)
	, m_len(-1)
	, m_hBreakEvent(NULL)
	, m_lOsError(0)
{
	hr = OpenFiles(Items, modeRead|shareDenyNone|typeBinary|osSequentialScan) ? S_OK : E_FAIL;
	if(SUCCEEDED(hr)) m_len = GetLength();
}
STDMETHODIMP CAsyncFileReader::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IAsyncReader)
		QI(ISyncReader)
		QI(IFileHandle)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IAsyncReader

STDMETHODIMP CAsyncFileReader::SyncRead(LONGLONG llPosition, LONG lLength, BYTE* pBuffer)
{
	do
	{
		try
		{
			if(llPosition+lLength > GetLength()) return E_FAIL; // strange, but the Seek below can return llPosition even if the file is not that big (?)
			if(llPosition != Seek(llPosition, begin)) return E_FAIL;
			if((UINT)lLength < Read(pBuffer, lLength)) return E_FAIL;

#if 0 // def DEBUG
			static __int64 s_total = 0, s_laststoppos = 0;
			s_total += lLength;
			if(s_laststoppos > llPosition)
				TRACE(_T("[%I64d - %I64d] %d (%I64d)\n"), llPosition, llPosition + lLength, lLength, s_total);
			s_laststoppos = llPosition + lLength;
#endif

			return S_OK;
		}
		catch(CFileException* e)
		{
			m_lOsError = e->m_lOsError;
			e->Delete();
			Sleep(1);
			CStdString fn = m_strFileName;
			try {Close();} catch(CFileException* e) {e->Delete();}
			try {Open(fn, modeRead|shareDenyNone|typeBinary|osSequentialScan);} catch(CFileException* e) {e->Delete();}
			m_strFileName = fn;
		}
	}
	while(m_hBreakEvent && WaitForSingleObject(m_hBreakEvent, 0) == WAIT_TIMEOUT);

	return E_FAIL;
}

STDMETHODIMP CAsyncFileReader::Length(LONGLONG* pTotal, LONGLONG* pAvailable)
{
	LONGLONG len = m_len >= 0 ? m_len : GetLength();
	if(pTotal) *pTotal = len;
	if(pAvailable) *pAvailable = len;
	return S_OK;
}

// IFileHandle

STDMETHODIMP_(HANDLE) CAsyncFileReader::GetFileHandle()
{
	return m_hFile;
}

STDMETHODIMP_(LPCTSTR) CAsyncFileReader::GetFileName()
{
	return m_nCurPart != -1 ? m_strFiles[m_nCurPart] : m_strFiles[0];
}

//
// CAsyncUrlReader
//

CAsyncUrlReader::CAsyncUrlReader(CStdString url, HRESULT& hr) 
	: CAsyncFileReader(url, hr)
{
	if(SUCCEEDED(hr)) return;

	m_url = url;

	if(CAMThread::Create())
		CallWorker(CMD_INIT);

	hr = Open(m_fn, modeRead|shareDenyRead|typeBinary|osSequentialScan) ? S_OK : E_FAIL;
	m_len = -1; // force GetLength() return actual length always
}

CAsyncUrlReader::~CAsyncUrlReader()
{
	if(ThreadExists())
		CallWorker(CMD_EXIT);

	if(!m_fn.IsEmpty())
	{
		CMultiFiles::Close();
		DeleteFile(m_fn);
	}
}

// IAsyncReader

STDMETHODIMP CAsyncUrlReader::Length(LONGLONG* pTotal, LONGLONG* pAvailable)
{
	if(pTotal) *pTotal = 0;
	return __super::Length(NULL, pAvailable);
}

// CAMThread

DWORD CAsyncUrlReader::ThreadProc()
{
	AfxSocketInit(NULL);

	DWORD cmd = GetRequest();
	if(cmd != CMD_INIT) {Reply(E_FAIL); return E_FAIL;}

	try
	{
		CInternetSession is;
		CAutoPtr<CStdioFile> fin(is.OpenURL(m_url, 1, INTERNET_FLAG_TRANSFER_BINARY|INTERNET_FLAG_EXISTING_CONNECT|INTERNET_FLAG_NO_CACHE_WRITE));

		TCHAR path[_MAX_PATH], fn[_MAX_PATH];
		CFile fout;
		if(GetTempPath(MAX_PATH, path) && GetTempFileName(path, _T("mpc_http"), 0, fn)
		&& fout.Open(fn, modeCreate|modeWrite|shareDenyWrite|typeBinary))
		{
			m_fn = fn;

			char buff[1024];
			int len = fin->Read(buff, sizeof(buff));
			if(len > 0) fout.Write(buff, len);

			Reply(S_OK);

			while(!CheckRequest(&cmd))
			{
				int len = fin->Read(buff, sizeof(buff));
				if(len > 0) fout.Write(buff, len);
			}
		}
		else
		{
			Reply(E_FAIL);
		}

		fin->Close(); // must close it because the destructor doesn't seem to do it and we will get an exception when "is" is destroying
	}
	catch(CInternetException* ie)
	{
		ie->Delete();
		Reply(E_FAIL);
	}

	//

	cmd = GetRequest();
	ASSERT(cmd == CMD_EXIT);
	Reply(S_OK);

	//

	m_hThread = NULL;

	return S_OK;
}
