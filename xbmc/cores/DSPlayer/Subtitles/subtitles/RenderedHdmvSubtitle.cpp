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
  return	m_pSub->GetStartPosition(rt - m_rtStart, fps);
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

