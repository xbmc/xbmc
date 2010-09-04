/* 
 * $Id: RenderedHdmvSubtitle.cpp 955 2009-01-06 21:29:13Z casimir666 $
 *
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


#include "stdafx.h"
#include "HdmvSub.h"
#include "DVBSub.h"
#include "RenderedHdmvSubtitle.h"
#include "atlwfile.h"

CRenderedHdmvSubtitle::CRenderedHdmvSubtitle(CCritSec* pLock, SUBTITLE_TYPE nType)
           : ISubPicProviderImpl(pLock)
{
  switch (nType)
  {
  case ST_DVB :
    m_pSub = DNew CDVBSub();
    m_name = "DVB Embedded Subtitle";
    break;
  case ST_HDMV :
    m_pSub = DNew CHdmvSub();
    m_name = "HDMV Embedded Subtitle";
    break;
  default :
    ASSERT (FALSE);
    m_pSub = NULL;
  }
  m_rtStart = 0;
}

CRenderedHdmvSubtitle::~CRenderedHdmvSubtitle(void)
{
  delete m_pSub;
}


STDMETHODIMP CRenderedHdmvSubtitle::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);
    *ppv = NULL;

    return 
    QI(IPersist)
    QI(ISubStream)
    QI(ISubPicProvider)
    __super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPicProvider

STDMETHODIMP_(int) CRenderedHdmvSubtitle::GetStartPosition(REFERENCE_TIME rt, double fps)
{
  CAutoLock cAutoLock(&m_csCritSec);
  return m_pSub->GetStartPosition(rt - m_rtStart, fps);
}

STDMETHODIMP_(int) CRenderedHdmvSubtitle::GetNext(int pos)
{
  CAutoLock cAutoLock(&m_csCritSec);
  return m_pSub->GetNext (pos);
}

STDMETHODIMP_(REFERENCE_TIME) CRenderedHdmvSubtitle::GetStart(int pos, double fps)
{
  CAutoLock cAutoLock(&m_csCritSec);
  return m_pSub->GetStart(pos) + m_rtStart;
}

STDMETHODIMP_(REFERENCE_TIME) CRenderedHdmvSubtitle::GetStop(int pos, double fps)
{
  CAutoLock cAutoLock(&m_csCritSec);
  return m_pSub->GetStop(pos) + m_rtStart;
}

STDMETHODIMP_(bool) CRenderedHdmvSubtitle::IsAnimated(int pos)
{
  return(false);
}

STDMETHODIMP CRenderedHdmvSubtitle::Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox)
{
  CAutoLock cAutoLock(&m_csCritSec);
  m_pSub->Render (spd, rt - m_rtStart, bbox);

  return S_OK;
}

STDMETHODIMP CRenderedHdmvSubtitle::GetTextureSize (int pos, SIZE& MaxTextureSize, SIZE& VideoSize, POINT& VideoTopLeft)
{ 
  CAutoLock cAutoLock(&m_csCritSec);
  HRESULT hr = m_pSub->GetTextureSize(pos, MaxTextureSize, VideoSize, VideoTopLeft); 
  return hr;
};

// IPersist

STDMETHODIMP CRenderedHdmvSubtitle::GetClassID(CLSID* pClassID)
{
  return pClassID ? *pClassID = __uuidof(this), S_OK : E_POINTER;
}

// ISubStream

STDMETHODIMP_(int) CRenderedHdmvSubtitle::GetStreamCount()
{
  return (1);
}

STDMETHODIMP CRenderedHdmvSubtitle::GetStreamInfo(int iStream, WCHAR** ppName, LCID* pLCID)
{
  if(iStream != 0) return E_INVALIDARG;

  if(ppName)
  {
    if(!(*ppName = (WCHAR*)CoTaskMemAlloc((m_name.GetLength()+1)*sizeof(WCHAR))))
      return E_OUTOFMEMORY;

    wcscpy_s (*ppName, m_name.GetLength()+1, CStdStringW(m_name));
  }

  if(pLCID)
  {
    *pLCID = m_lcid;
  }

  return S_OK;
}

STDMETHODIMP_(int) CRenderedHdmvSubtitle::GetStream()
{
  return(0);
}

STDMETHODIMP CRenderedHdmvSubtitle::SetStream(int iStream)
{
  return iStream == 0 ? S_OK : E_FAIL;
}

STDMETHODIMP CRenderedHdmvSubtitle::Reload()
{
  return S_OK;
}

HRESULT CRenderedHdmvSubtitle::ParseSample (IMediaSample* pSample)
{
  CAutoLock cAutoLock(&m_csCritSec);
  HRESULT    hr;

  hr = m_pSub->ParseSample (pSample);
  return hr;
}

HRESULT CRenderedHdmvSubtitle::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
  CAutoLock cAutoLock(&m_csCritSec);

  m_pSub->Reset();
  m_rtStart = tStart;
  return S_OK;
}


CRenderedHdmvSubtitleFile::CRenderedHdmvSubtitleFile(CCritSec* pLock, SUBTITLE_TYPE nType)
           : ISubPicProviderImpl(pLock)
{
  switch (nType)
  {
  case ST_DVB :
    m_pSub = DNew CDVBSub();
    m_name = "DVB Embedded Subtitle";
    break;
  case ST_HDMV :
    m_pSub = DNew CHdmvSub(true);
    m_name = "HDMV External Subtitle";
    break;
  default :
    ASSERT (FALSE);
    m_pSub = NULL;
  }
  m_rtStart = 0;
}

CRenderedHdmvSubtitleFile::~CRenderedHdmvSubtitleFile(void)
{
  delete m_pSub;
}


STDMETHODIMP CRenderedHdmvSubtitleFile::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);
    *ppv = NULL;

    return 
    QI(IPersist)
    QI(ISubStream)
    QI(ISubPicProvider)
    __super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPicProvider

STDMETHODIMP_(int) CRenderedHdmvSubtitleFile::GetStartPosition(REFERENCE_TIME rt, double fps)
{
  CAutoLock cAutoLock(&m_csCritSec);
  return	m_pSub->GetStartPosition(rt - m_rtStart, fps);
}

STDMETHODIMP_(int) CRenderedHdmvSubtitleFile::GetNext(int pos)
{
  CAutoLock cAutoLock(&m_csCritSec);
  return m_pSub->GetNext (pos);
}

STDMETHODIMP_(REFERENCE_TIME) CRenderedHdmvSubtitleFile::GetStart(int pos, double fps)
{
  CAutoLock cAutoLock(&m_csCritSec);
  return m_pSub->GetStart(pos) + m_rtStart;
}

STDMETHODIMP_(REFERENCE_TIME) CRenderedHdmvSubtitleFile::GetStop(int pos, double fps)
{
  CAutoLock cAutoLock(&m_csCritSec);
  return m_pSub->GetStop(pos) + m_rtStart;
}

STDMETHODIMP_(bool) CRenderedHdmvSubtitleFile::IsAnimated(int pos)
{
  return(false);
}

STDMETHODIMP CRenderedHdmvSubtitleFile::Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox)
{
  CAutoLock cAutoLock(&m_csCritSec);
  m_pSub->Render (spd, rt - m_rtStart, bbox);

  return S_OK;
}

STDMETHODIMP CRenderedHdmvSubtitleFile::GetTextureSize (int pos, SIZE& MaxTextureSize, SIZE& VideoSize, POINT& VideoTopLeft)
{ 
  CAutoLock cAutoLock(&m_csCritSec);
  HRESULT hr = m_pSub->GetTextureSize(pos, MaxTextureSize, VideoSize, VideoTopLeft); 
  return hr;
};

// IPersist

STDMETHODIMP CRenderedHdmvSubtitleFile::GetClassID(CLSID* pClassID)
{
  return pClassID ? *pClassID = __uuidof(this), S_OK : E_POINTER;
}

// ISubStream

STDMETHODIMP_(int) CRenderedHdmvSubtitleFile::GetStreamCount()
{
  return (1);
}

STDMETHODIMP CRenderedHdmvSubtitleFile::GetStreamInfo(int iStream, WCHAR** ppName, LCID* pLCID)
{
  if(iStream != 0) return E_INVALIDARG;

  if(ppName)
  {
    if(!(*ppName = (WCHAR*)CoTaskMemAlloc((m_name.GetLength()+1)*sizeof(WCHAR))))
      return E_OUTOFMEMORY;

    wcscpy_s (*ppName, m_name.GetLength()+1, CStdStringW(m_name));
  }

  if(pLCID)
  {
    *pLCID = m_lcid;
  }

  return S_OK;
}

STDMETHODIMP_(int) CRenderedHdmvSubtitleFile::GetStream()
{
  return(0);
}

STDMETHODIMP CRenderedHdmvSubtitleFile::SetStream(int iStream)
{
  return iStream == 0 ? S_OK : E_FAIL;
}

STDMETHODIMP CRenderedHdmvSubtitleFile::Reload()
{
  return S_OK;
}

HRESULT CRenderedHdmvSubtitleFile::ParseSample (IMediaSample* pSample)
{
  CAutoLock cAutoLock(&m_csCritSec);
  HRESULT    hr;

  hr = m_pSub->ParseSample (pSample);
  return hr;
}

bool CRenderedHdmvSubtitleFile::Open(CStdString fn)
{
  ATL::CFile f;
  if(! f.Open(fn, ATL::CFile::modeRead|ATL::CFile::typeBinary|ATL::CFile::shareDenyNone))
    return false;

  //m_sub.SetLength(f.GetLength());
  m_pMemBuffer.seekp(0); m_pMemBuffer.seekg(0);
  m_pMemBuffer.str("");
  m_pMemBuffer.clear();

  int len = 0;
  BYTE buff[2048];
  while((len = f.Read(buff, sizeof(buff))) > 0)
  {
    m_pMemBuffer.write((const char*)buff, len);
    if (m_pMemBuffer.bad())
      return false;
  }

  m_pMemBuffer.seekg(0, std::ios_base::end);
  uint64_t totalLen = m_pMemBuffer.tellg();
  m_pMemBuffer.seekg(0);

  while (! m_pMemBuffer.eof() && ! (m_pMemBuffer.tellg() > totalLen))
  {

    BYTE pData[4];
    uint16_t header = 0;
    REFERENCE_TIME rtStart = 0, rtStop = 0;
    m_pMemBuffer.read((char *) &header, 2);
    if (m_pMemBuffer.eof())
      return true;

    if (header != 0x4750)
      return false;

    m_pMemBuffer.read((char *) pData, 4);
    rtStart = (pData[3] + ((int64_t)pData[2] << 8) + ((int64_t)pData[1] << 0x10) + ((int64_t)pData[0] << 0x18)) / 90;

    m_pMemBuffer.read((char *) pData, 4);
    rtStop = (pData[3] + ((int64_t)pData[2] << 8) + ((int64_t)pData[1] << 0x10) + ((int64_t)pData[0] << 0x18)) / 90;
    //TRACE(L"Found PGS data: from %10I64dms to %10I64dms", rtStart, rtStop);

    rtStart *= 10000;
    rtStop *= 10000;
    if (rtStop == 0)
      rtStop = rtStart;

    m_pMemBuffer.read((char *) pData, 3);
    // pData[0] : segment type
    size_t datalen = pData[2] + (((uint32_t)pData[1]) << 8);

    BYTE * pBuffer = new BYTE[datalen + 3];
    memcpy(pBuffer, pData, 3);
    m_pMemBuffer.read((char *) &pBuffer[3], datalen);

    if (FAILED(m_pSub->ParseData(rtStart, rtStop, pBuffer, datalen + 3)))
    {
      delete[] pBuffer;
      return false;
    }

    delete[] pBuffer;
  }

  return true;
}

HRESULT CRenderedHdmvSubtitleFile::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
  CAutoLock cAutoLock(&m_csCritSec);

  m_pSub->Reset();
  m_rtStart = tStart;
  return S_OK;
}

