/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
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
#include "DShowUtil/DShowUtil.h"

template<class TStream>
class CBaseSource
  : public CSource
  , public IFileSourceFilter
  , public IAMFilterMiscFlags
{
protected:
  CStdStringW m_fn;

public:
  CBaseSource(TCHAR* name, LPUNKNOWN lpunk, HRESULT* phr, const CLSID& clsid)
    : CSource(name, lpunk, clsid)
  {
    if(phr) *phr = S_OK;
  }

  DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
  {
    CheckPointer(ppv, E_POINTER);

    return 
      QI(IFileSourceFilter)
      QI(IAMFilterMiscFlags)
      __super::NonDelegatingQueryInterface(riid, ppv);
  }

  // IFileSourceFilter

  STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
  {
    // TODO: destroy any already existing pins and create new, now we are just going die nicely instead of doing it :)
    if(GetPinCount() > 0)
      return VFW_E_ALREADY_CONNECTED;

    HRESULT hr = S_OK;
    if(!(DNew TStream(pszFileName, this, &hr)))
      return E_OUTOFMEMORY;

    if(FAILED(hr))
      return hr;

    m_fn = pszFileName;

    return S_OK;
  }

  STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
  {
    size_t    nCount;
    if(!ppszFileName) return E_POINTER;
    
    nCount = m_fn.GetLength()+1;
    if(!(*ppszFileName = (LPOLESTR)CoTaskMemAlloc(nCount*sizeof(WCHAR))))
      return E_OUTOFMEMORY;

    wcscpy_s(*ppszFileName, nCount, m_fn);

    return S_OK;
  }

  // IAMFilterMiscFlags

  STDMETHODIMP_(ULONG) GetMiscFlags()
  {
    return AM_FILTER_MISC_FLAGS_IS_SOURCE;
  }
};
#if 0
class CBaseStream 
  : public CSourceStream
  , public CSourceSeeking
{
protected:
  CCritSec m_cSharedState;

  REFERENCE_TIME m_AvgTimePerFrame;
  REFERENCE_TIME m_rtSampleTime, m_rtPosition;

  BOOL m_bDiscontinuity, m_bFlushing;

  HRESULT OnThreadStartPlay();
  HRESULT OnThreadCreate();

private:
  void UpdateFromSeek();
  STDMETHODIMP SetRate(double dRate);

  HRESULT ChangeStart();
    HRESULT ChangeStop();
    HRESULT ChangeRate() {return S_OK;}

public:
    CBaseStream(TCHAR* name, CSource* pParent, HRESULT* phr);
    virtual ~CBaseStream();

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    HRESULT FillBuffer(IMediaSample* pSample);

    virtual HRESULT FillBuffer(IMediaSample* pSample, int nFrame, BYTE* pOut, long& len /*in+out*/) = 0;

  STDMETHODIMP Notify(IBaseFilter* pSender, Quality q);
};
#endif
