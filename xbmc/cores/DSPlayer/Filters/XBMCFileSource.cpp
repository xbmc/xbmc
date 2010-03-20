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

#include "File.h"
using namespace XFILE;

#include <streams.h>

#include "XBMCFileSource.h"
#include "utils/log.h"
#include "SingleLock.h"

CXBMCFileStream::CXBMCFileStream(CFile *file, IBaseFilter **pBF, HRESULT *phr) :
    m_llLength(0)
{
  if (! pBF)
    return;
  *pBF = 0;
  m_llLength = file->GetLength();
  m_pFile = file;
  HRESULT hr;
  CXBMCFileReader* pXBMCReader = new CXBMCFileReader(this, NULL, &hr);
  *phr = hr;
  if (SUCCEEDED(hr))
  {
    *pBF = pXBMCReader;
    (*pBF)->AddRef();
    CLog::Log(LOGNOTICE,"%s Successfully created xbmc source filter", __FUNCTION__);
  }
  else
    CLog::Log(LOGERROR,"%s Failed to create xbmc source filter", __FUNCTION__);
}

HRESULT CXBMCFileStream::SetPointer(LONGLONG llPos)
{
  if (llPos < 0 || llPos > m_llLength) {
    return S_FALSE;
  }
  else
  {
    m_pFile->Seek(llPos);
    return S_OK;
  }
}

HRESULT CXBMCFileStream::Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead)
{
  CSingleLock lck(m_csLock);
    
  *pdwBytesRead = m_pFile->Read(pbBuffer, dwBytesToRead);

  return S_OK;
}

LONGLONG CXBMCFileStream::Size(LONGLONG *pSizeAvailable)
{
  *pSizeAvailable = m_llLength;
  return m_llLength;
}

DWORD CXBMCFileStream::Alignment()
{
  return 1;
}
void CXBMCFileStream::Lock()
{
  m_csLock.getCriticalSection().Enter();
}
void CXBMCFileStream::Unlock()
{
  m_csLock.getCriticalSection().Leave();
}

STDMETHODIMP CXBMCFileReader::Register()
{
  return S_OK;
}

STDMETHODIMP CXBMCFileReader::Unregister()
{
  return S_OK;
}

CXBMCFileReader::CXBMCFileReader(CXBMCFileStream *pStream, CMediaType *pmt, HRESULT *phr) :
  CAsyncReader(NAME("XBMC File Reader\0"), NULL, pStream, phr)
{
  m_mt.majortype = MEDIATYPE_Stream;
  m_mt.subtype = MEDIASUBTYPE_NULL;
}
