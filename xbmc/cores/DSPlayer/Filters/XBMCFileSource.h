//
//  Define an internal filter that wraps the base CBaseReader stuff
//
#ifndef __XBMCFILESOURCE_H__
#include "filters/asyncio.h"
#include "filters/asyncrdr.h"


class CXBMCFileStream : public CAsyncStream
{
public:
  CXBMCFileStream(CFile *file);
  HRESULT SetPointer(LONGLONG llPos);
  HRESULT Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead);
  LONGLONG Size(LONGLONG *pSizeAvailable);
  DWORD Alignment();
  void Lock();
  void Unlock();


private:
    CCritSec       m_csLock;
    LONGLONG       m_llLength;
    CFile*         m_pFile;
};


class CXBMCFileReader : public CAsyncReader
{
public:

  //  We're not going to be CoCreate'd so we don't need registration
  //  stuff etc
  STDMETHODIMP Register();

  STDMETHODIMP Unregister();

  CXBMCFileReader(CXBMCFileStream *pStream, CMediaType *pmt, HRESULT *phr);

};

#endif
