/*
 * $Id: TextPassThruFilter.cpp 193 2007-09-09 09:12:21Z alexwild $
 *
 * (C) 2003-2006 Gabest
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
#include <d3d9.h>
#include "..\subpic\ISubPic.h"
#include "SubManager.h"
#include "TextPassThruFilter.h"
#include "..\DSUtil\DSUtil.h"
#include "..\subtitles\SubtitleInputPin.h"

//
// CTextPassThruInputPin
//

class CTextPassThruInputPin : public CSubtitleInputPin
{
  CTextPassThruFilter* m_pTPTFilter;

protected:
  void AddSubStream(ISubStream* pSubStream)
  {
    if(pSubStream) m_pTPTFilter->m_pMainFrame->SetTextPassThruSubStream(pSubStream);
  }

  void RemoveSubStream(ISubStream* pSubStream)
  {
  }

  void InvalidateSubtitle(REFERENCE_TIME rtStart, ISubStream* pSubStream)
  {
    m_pTPTFilter->m_pMainFrame->InvalidateSubtitle((DWORD_PTR)pSubStream, rtStart);
  }

public:
  CTextPassThruInputPin(CTextPassThruFilter* pTPTFilter, CCritSec* pLock, CCritSec* pSubLock, HRESULT* phr);
  STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
  STDMETHODIMP Receive(IMediaSample* pSample);
  STDMETHODIMP EndOfStream();
  STDMETHODIMP BeginFlush();
  STDMETHODIMP EndFlush();
};

//
// CTextPassThruOutputPin
//

class CTextPassThruOutputPin : public CBaseOutputPin
{
  CTextPassThruFilter* m_pTPTFilter;

public:
  CTextPassThruOutputPin(CTextPassThruFilter* pTPTFilter, CCritSec* pLock, HRESULT* phr);

  HRESULT CheckMediaType(const CMediaType* mtOut);
  HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
  HRESULT GetMediaType(int iPosition, CMediaType* pmt);
  STDMETHODIMP Notify(IBaseFilter* pSender, Quality q) {return S_OK;}
};

///////////

CTextPassThruInputPin::CTextPassThruInputPin(CTextPassThruFilter* pTPTFilter, CCritSec* pLock, CCritSec* pSubLock, HRESULT* phr)
  : CSubtitleInputPin(pTPTFilter, pLock, pSubLock, phr)
  , m_pTPTFilter(pTPTFilter)
{
}

STDMETHODIMP CTextPassThruInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
  HRESULT hr = __super::NewSegment(tStart, tStop, dRate);
  if(FAILED(hr)) return hr;
  return m_pTPTFilter->m_pOutput->DeliverNewSegment(tStart, tStop, dRate);
}

STDMETHODIMP CTextPassThruInputPin::Receive(IMediaSample* pSample)
{
  HRESULT hr = __super::Receive(pSample);
  if(FAILED(hr)) return hr;
  return m_pTPTFilter->m_pOutput->Deliver(pSample);
}

STDMETHODIMP CTextPassThruInputPin::EndOfStream()
{
  HRESULT hr = __super::EndOfStream();
  if(FAILED(hr)) return hr;
  return m_pTPTFilter->m_pOutput->DeliverEndOfStream();
}

STDMETHODIMP CTextPassThruInputPin::BeginFlush()
{
  HRESULT hr = __super::BeginFlush();
  if(FAILED(hr)) return hr;
  return m_pTPTFilter->m_pOutput->DeliverBeginFlush();
}

STDMETHODIMP CTextPassThruInputPin::EndFlush()
{
  HRESULT hr = __super::EndFlush();
  if(FAILED(hr)) return hr;
  return m_pTPTFilter->m_pOutput->DeliverEndFlush();
}

//

CTextPassThruOutputPin::CTextPassThruOutputPin(CTextPassThruFilter* pTPTFilter, CCritSec* pLock, HRESULT* phr)
  : CBaseOutputPin(NAME(""), pTPTFilter, pLock, phr, L"Out")
  , m_pTPTFilter(pTPTFilter)
{
}

HRESULT CTextPassThruOutputPin::CheckMediaType(const CMediaType* mtOut)
{
  CMediaType mt;
  return S_OK == m_pTPTFilter->m_pInput->ConnectionMediaType(&mt) && mt == *mtOut
    ? S_OK 
    : E_FAIL;
}

HRESULT CTextPassThruOutputPin::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
  if(m_pTPTFilter->m_pInput->IsConnected() == FALSE)
    return E_UNEXPECTED;

  Com::SmartPtr<IMemAllocator> pAllocatorIn;
  m_pTPTFilter->m_pInput->GetAllocator(&pAllocatorIn);
  if(!pAllocatorIn) return E_UNEXPECTED;

  pAllocatorIn->GetProperties(pProperties);

  HRESULT hr;
  ALLOCATOR_PROPERTIES Actual;
  if(FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) 
    return hr;

  return(pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
    ? E_FAIL
    : NOERROR);
}

HRESULT CTextPassThruOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
  if(m_pTPTFilter->m_pInput->IsConnected() == FALSE)
    return E_UNEXPECTED;

  if(iPosition < 0) return E_INVALIDARG;
  if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

  m_pTPTFilter->m_pInput->ConnectionMediaType(pmt);

  return S_OK;
}

//
// CTextPassThruFilter
//
#pragma warning(disable : 4355)

CTextPassThruFilter::CTextPassThruFilter(CSubManager* pMainFrame) 
  : CBaseFilter(NAME("CTextPassThruFilter"), NULL, this, __uuidof(this))
  , m_pMainFrame(pMainFrame)
{
  HRESULT hr;
  m_pInput = new CTextPassThruInputPin(this, this, &m_pMainFrame->m_csSubLock, &hr);
  m_pOutput = new CTextPassThruOutputPin(this, this, &hr);
}

CTextPassThruFilter::~CTextPassThruFilter()
{
  delete m_pInput; m_pInput = NULL;
  delete m_pOutput; m_pOutput = NULL;
}

STDMETHODIMP CTextPassThruFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
  if(m_pInput && riid == __uuidof(ISubStream))
  {
    if(Com::SmartPtr<ISubStream> pSubStream = m_pInput->GetSubStream())
    {
      *ppv = pSubStream.Detach();
      return S_OK;
    }
  }

  return __super::NonDelegatingQueryInterface(riid, ppv);
}

int CTextPassThruFilter::GetPinCount()
{
  return 2;
}

CBasePin* CTextPassThruFilter::GetPin(int n)
{
  if(n == 0) return m_pInput;
  else if(n == 1) return m_pOutput;
  return NULL;
}
