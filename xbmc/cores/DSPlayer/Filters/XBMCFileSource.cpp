#include "File.h"

using namespace XFILE;

#include <streams.h>


#include "XBMCFileSource.h"
#include "utils/log.h"

CXBMCFileStream::CXBMCFileStream(CFile *file, IBaseFilter **pBF) :
    m_llLength(0)
{
  if (! pBF)
    return;
  *pBF = 0;
  m_llLength = file->GetLength();
  m_pFile = file;
  HRESULT hr;
  CXBMCFileReader* pXBMCReader = new CXBMCFileReader(this, NULL, &hr);
  if (SUCCEEDED(hr))
  {
    *pBF = pXBMCReader;
	(*pBF)->AddRef();
	CLog::Log(LOGNOTICE,"%s Added xbmc source filter to graph",__FUNCTION__);
  }
  else
    CLog::Log(LOGERROR,"%s Failed to create xbmc source filter",__FUNCTION__);
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
  CAutoLock lck(&m_csLock);
    
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
  m_csLock.Lock();
}
void CXBMCFileStream::Unlock()
{
  m_csLock.Unlock();
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
