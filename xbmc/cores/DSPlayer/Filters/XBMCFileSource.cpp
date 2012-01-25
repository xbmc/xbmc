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

#include "XBMCFileSource.h"
#include "utils/log.h"
#include "threads/SingleLock.h"

CXBMCAsyncStream::CXBMCAsyncStream()
  : m_llLength(0)
{

}

HRESULT CXBMCAsyncStream::Load(const CStdString& file)
{
  m_pFileName = file;

  m_pFile.Close();
  if (!m_pFile.Open(m_pFileName, READ_TRUNCATED))
  {
    CLog::Log(LOGERROR,"%s Failed to read the file in the xbmc source filter", __FUNCTION__);
    return E_FAIL;
  }

  m_llLength = m_pFile.GetLength();
  return S_OK;
}

CXBMCAsyncStream::~CXBMCAsyncStream()
{
  m_pFile.Close();
}

HRESULT CXBMCAsyncStream::SetPointer(LONGLONG llPos)
{
  if (llPos < 0 || llPos > m_llLength) {
    return S_FALSE;
  }
  else
  {
    m_pFile.Seek(llPos);
    return S_OK;
  }
}

HRESULT CXBMCAsyncStream::Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead)
{
  CSingleLock lck(m_csLock);
    
  *pdwBytesRead = m_pFile.Read(pbBuffer, dwBytesToRead);

  return S_OK;
}

LONGLONG CXBMCAsyncStream::Size(LONGLONG *pSizeAvailable)
{
  *pSizeAvailable = m_llLength;
  return m_llLength;
}

DWORD CXBMCAsyncStream::Alignment()
{
  return 1;
}
void CXBMCAsyncStream::Lock()
{
  m_csLock.lock();
}
void CXBMCAsyncStream::Unlock()
{
  m_csLock.unlock();
}

STDMETHODIMP CXBMCASyncReader::Register()
{
  return S_OK;
}

STDMETHODIMP CXBMCASyncReader::Unregister()
{
  return S_OK;
}

CXBMCASyncReader::CXBMCASyncReader(LPUNKNOWN pUnknown, HRESULT *phr)
  : m_pAsyncStream(), CAsyncReader(NAME("XBMC File Reader\0"), NULL, &m_pAsyncStream, phr)
{
  m_mt.majortype = MEDIATYPE_Stream;
  m_mt.subtype = MEDIASUBTYPE_NULL;
}

HRESULT STDMETHODCALLTYPE CXBMCASyncReader::Load(
  /* [in] */ LPCOLESTR pszFileName,
  /* [annotation][unique][in] */ 
  __in_opt  const AM_MEDIA_TYPE *pmt)
{
  return m_pAsyncStream.Load(pszFileName);
}

STDMETHODIMP CXBMCASyncReader::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
  CheckPointer(ppv, E_POINTER);

  return (riid == __uuidof(IFileSourceFilter)) ? GetInterface((IFileSourceFilter*)this, ppv)
    : __super::NonDelegatingQueryInterface(riid, ppv);
}

#endif