/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef HAS_DS_PLAYER

#include <streams.h>

#include "XBMCFileReader.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "DSUtil/DSUtil.h"


CXBMCFileStream::CXBMCFileStream(CStdString filepath, HRESULT& hr)
	: CUnknown(NAME("CXBMCFileStream"), NULL, &hr),
	  m_llLength(-1),
	  m_hBreakEvent(NULL),
	  m_lOsError(0),
    m_strCurrentFile(filepath)
{
  m_pFile.Close();
	//hr = Open(fn, modeRead|shareDenyNone|typeBinary|osSequentialScan) ? S_OK : E_FAIL;
  if (m_pFile.Open(filepath, READ_TRUNCATED))
    hr = S_OK;
  else
    hr = E_FAIL;
  if(SUCCEEDED(hr)) 
    m_llLength = m_pFile.GetLength();
}

STDMETHODIMP CXBMCFileStream::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IAsyncReader)
		QI(ISyncReader)
		QI(IFileHandle)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IAsyncReader

STDMETHODIMP CXBMCFileStream::SyncRead(LONGLONG llPosition, LONG lLength, BYTE* pBuffer)
{
	do
	{
		try
		{
			if(llPosition+lLength > m_pFile.GetLength())
        return E_FAIL; // strange, but the Seek below can return llPosition even if the file is not that big (?)
			if(llPosition != m_pFile.Seek(llPosition, SEEK_SET))
        return E_FAIL;
			if((UINT)lLength < m_pFile.Read(pBuffer, lLength)) 
        return E_FAIL;

			return S_OK;
		}
    catch (...)
    {
      CLog::Log(LOGERROR,"%s Error while reading the file in xbmcfilesource filter",__FUNCTION__);
    }
	}
	while(m_hBreakEvent && WaitForSingleObject(m_hBreakEvent, 0) == WAIT_TIMEOUT);

	return E_FAIL;
}

STDMETHODIMP CXBMCFileStream::Length(LONGLONG* pTotal, LONGLONG* pAvailable)
{
	LONGLONG len = m_llLength >= 0 ? m_llLength : m_pFile.GetLength();
	if(pTotal) *pTotal = len;
	if(pAvailable) *pAvailable = len;
	return S_OK;
}

// IFileHandle

STDMETHODIMP_(HANDLE) CXBMCFileStream::GetFileHandle()
{
	return NULL;
}

STDMETHODIMP_(LPCTSTR) CXBMCFileStream::GetFileName()
{
	return m_strCurrentFile.c_str();
}

#endif