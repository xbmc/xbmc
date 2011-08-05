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

#include "stdafx.h"
#include "SubtitleInputPin.h"
#include "VobSubFile.h"
#include "RTS.h"
#include "SSF.h"
#include "RenderedHdmvSubtitle.h"

#include <initguid.h>
#include <moreuuids.h>

// our first format id
#define __GAB1__ "GAB1"

// our tags for __GAB1__ (ushort) + size (ushort)

// "lang" + '0'
#define __GAB1_LANGUAGE__ 0
// (int)start+(int)stop+(char*)line+'0'
#define __GAB1_ENTRY__ 1
// L"lang" + '0'
#define __GAB1_LANGUAGE_UNICODE__ 2
// (int)start+(int)stop+(WCHAR*)line+'0'
#define __GAB1_ENTRY_UNICODE__ 3

// same as __GAB1__, but the size is (uint) and only __GAB1_LANGUAGE_UNICODE__ is valid
#define __GAB2__ "GAB2"

// (BYTE*)
#define __GAB1_RAWTEXTSUBTITLE__ 4

CSubtitleInputPin::CSubtitleInputPin(CBaseFilter* pFilter, CCritSec* pLock, CCritSec* pSubLock, HRESULT* phr)
  : CBaseInputPin(NAME("CSubtitleInputPin"), pFilter, pLock, phr, L"Input")
  , m_pSubLock(pSubLock)
{
  m_bCanReconnectWhenActive = TRUE;
}

HRESULT CSubtitleInputPin::CheckMediaType(const CMediaType* pmt)
{
  return pmt->majortype == MEDIATYPE_Text && (pmt->subtype == MEDIASUBTYPE_NULL || pmt->subtype == FOURCCMap((DWORD)0))
    || pmt->majortype == MEDIATYPE_Subtitle && pmt->subtype == MEDIASUBTYPE_UTF8
    || pmt->majortype == MEDIATYPE_Subtitle && (pmt->subtype == MEDIASUBTYPE_SSA || pmt->subtype == MEDIASUBTYPE_ASS || pmt->subtype == MEDIASUBTYPE_ASS2)
    || pmt->majortype == MEDIATYPE_Subtitle && pmt->subtype == MEDIASUBTYPE_SSF
    || pmt->majortype == MEDIATYPE_Subtitle && (pmt->subtype == MEDIASUBTYPE_VOBSUB)
    || IsHdmvSub(pmt)
    ? S_OK 
    : E_FAIL;
}

HRESULT CSubtitleInputPin::CompleteConnect(IPin* pReceivePin)
{
  if(m_mt.majortype == MEDIATYPE_Text)
  {
    if(!(m_pSubStream = DNew CRenderedTextSubtitle(m_pSubLock))) return E_FAIL;
    CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pSubStream;
    pRTS->m_name = CStdString(GetPinName(pReceivePin)) + _T(" (embeded)");
    pRTS->m_dstScreenSize = Com::SmartSize(384, 288);
    pRTS->CreateDefaultStyle(DEFAULT_CHARSET);
  }
  else if(m_mt.majortype == MEDIATYPE_Subtitle)
  {
    SUBTITLEINFO* psi = (SUBTITLEINFO*)m_mt.pbFormat;
    DWORD     dwOffset = 0;
    CStdString   name;
    LCID      lcid = 0;

    if (psi != NULL)
    {
      dwOffset = psi->dwOffset;

      name = ISO6392ToLanguage(psi->IsoLang);
      lcid = ISO6392ToLcid(psi->IsoLang);
      if(name.IsEmpty()) name = _T("Unknown");
      if(wcslen(psi->TrackName) > 0) name += _T(" (") + CStdString(psi->TrackName) + _T(")");
    }

    if(m_mt.subtype == MEDIASUBTYPE_UTF8 
    /*|| m_mt.subtype == MEDIASUBTYPE_USF*/
    || m_mt.subtype == MEDIASUBTYPE_SSA 
    || m_mt.subtype == MEDIASUBTYPE_ASS 
    || m_mt.subtype == MEDIASUBTYPE_ASS2)
    {
      if(!(m_pSubStream = DNew CRenderedTextSubtitle(m_pSubLock))) return E_FAIL;
      CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pSubStream;
      pRTS->m_name = name;
      pRTS->m_lcid = lcid;
      pRTS->m_dstScreenSize = Com::SmartSize(384, 288);
      pRTS->CreateDefaultStyle(DEFAULT_CHARSET);

      if(dwOffset > 0 && m_mt.cbFormat - dwOffset > 0)
      {
        CMediaType mt = m_mt;
        if(mt.pbFormat[dwOffset+0] != 0xef
        && mt.pbFormat[dwOffset+1] != 0xbb
        && mt.pbFormat[dwOffset+2] != 0xfb)
        {
          dwOffset -= 3;
          mt.pbFormat[dwOffset+0] = 0xef;
          mt.pbFormat[dwOffset+1] = 0xbb;
          mt.pbFormat[dwOffset+2] = 0xbf;
        }

        pRTS->Open(mt.pbFormat + dwOffset, mt.cbFormat - dwOffset, DEFAULT_CHARSET, pRTS->m_name);
      }

    }
    else if(m_mt.subtype == MEDIASUBTYPE_SSF)
    {
      if(!(m_pSubStream = DNew ssf::CRenderer(m_pSubLock))) return E_FAIL;
      ssf::CRenderer* pSSF = (ssf::CRenderer*)(ISubStream*)m_pSubStream;
      pSSF->Open(ssf::MemoryInputStream(m_mt.pbFormat + dwOffset, m_mt.cbFormat - dwOffset, false, false), name);
    }
    else if(m_mt.subtype == MEDIASUBTYPE_VOBSUB)
    {
      if(!(m_pSubStream = DNew CVobSubStream(m_pSubLock))) return E_FAIL;
      CVobSubStream* pVSS = (CVobSubStream*)(ISubStream*)m_pSubStream;
      pVSS->Open(name, m_mt.pbFormat + dwOffset, m_mt.cbFormat - dwOffset);
    }
    else if (IsHdmvSub(&m_mt))
    {
      if(!(m_pSubStream = DNew CRenderedHdmvSubtitle(m_pSubLock, (m_mt.subtype == MEDIASUBTYPE_DVB_SUBTITLES) ? ST_DVB : ST_HDMV)))
        return E_FAIL;
    }
  }

  AddSubStream(m_pSubStream);

  return __super::CompleteConnect(pReceivePin);
}

HRESULT CSubtitleInputPin::BreakConnect()
{
  RemoveSubStream(m_pSubStream);
  m_pSubStream = NULL;

  ASSERT(IsStopped());

    return __super::BreakConnect();
}

STDMETHODIMP CSubtitleInputPin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
  if(m_Connected)
  {
    RemoveSubStream(m_pSubStream);
    m_pSubStream = NULL;

    m_Connected->Release();
    m_Connected = NULL;
  }

  return __super::ReceiveConnection(pConnector, pmt);
}

STDMETHODIMP CSubtitleInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
  CAutoLock cAutoLock(&m_csReceive);

  if(m_mt.majortype == MEDIATYPE_Text
  || m_mt.majortype == MEDIATYPE_Subtitle 
    && (m_mt.subtype == MEDIASUBTYPE_UTF8 
    /*|| m_mt.subtype == MEDIASUBTYPE_USF*/
    || m_mt.subtype == MEDIASUBTYPE_SSA 
    || m_mt.subtype == MEDIASUBTYPE_ASS 
    || m_mt.subtype == MEDIASUBTYPE_ASS2))
  {
    CAutoLock cAutoLock(m_pSubLock);
    CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pSubStream;
    pRTS->clear();
    pRTS->CreateSegments();
  }
  else if(m_mt.majortype == MEDIATYPE_Subtitle && m_mt.subtype == MEDIASUBTYPE_SSF)
  {
    CAutoLock cAutoLock(m_pSubLock);
    ssf::CRenderer* pSSF = (ssf::CRenderer*)(ISubStream*)m_pSubStream;
    // LAME, implement RemoveSubtitles
    DWORD dwOffset = ((SUBTITLEINFO*)m_mt.pbFormat)->dwOffset;
    pSSF->Open(ssf::MemoryInputStream(m_mt.pbFormat + dwOffset, m_mt.cbFormat - dwOffset, false, false), _T(""));
    // pSSF->RemoveSubtitles();
  }
  else if(m_mt.majortype == MEDIATYPE_Subtitle && (m_mt.subtype == MEDIASUBTYPE_VOBSUB))
  {
    CAutoLock cAutoLock(m_pSubLock);
    CVobSubStream* pVSS = (CVobSubStream*)(ISubStream*)m_pSubStream;
    pVSS->RemoveAll();
  }
	else if (IsHdmvSub(&m_mt))
  {
    CAutoLock cAutoLock(m_pSubLock);
    CRenderedHdmvSubtitle* pHdmvSubtitle = (CRenderedHdmvSubtitle*)(ISubStream*)m_pSubStream;
    pHdmvSubtitle->NewSegment (tStart, tStop, dRate);
  }

  return __super::NewSegment(tStart, tStop, dRate);
}

interface __declspec(uuid("D3D92BC3-713B-451B-9122-320095D51EA5"))
IMpeg2DemultiplexerTesting : public IUnknown
{
	STDMETHOD(GetMpeg2StreamType)(ULONG* plType) = NULL;
	STDMETHOD(toto)() = NULL;
};


STDMETHODIMP CSubtitleInputPin::Receive(IMediaSample* pSample)
{
  HRESULT hr;

  hr = __super::Receive(pSample);
  if(FAILED(hr)) return hr;

  CAutoLock cAutoLock(&m_csReceive);

  REFERENCE_TIME tStart, tStop;
  pSample->GetTime(&tStart, &tStop);
  tStart += m_tStart; 
  tStop += m_tStart;

  BYTE* pData = NULL;
  hr = pSample->GetPointer(&pData);
  if(FAILED(hr) || pData == NULL) return hr;

  int len = pSample->GetActualDataLength();

  bool fInvalidate = false;

  if(m_mt.majortype == MEDIATYPE_Text)
  {
    CAutoLock cAutoLock(m_pSubLock);
    CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pSubStream;

    if(!strncmp((char*)pData, __GAB1__, strlen(__GAB1__)))
    {
      char* ptr = (char*)&pData[strlen(__GAB1__)+1];
      char* end = (char*)&pData[len];

      while(ptr < end)
      {
        WORD tag = *((WORD*)(ptr)); ptr += 2;
        WORD size = *((WORD*)(ptr)); ptr += 2;

        if(tag == __GAB1_LANGUAGE__)
        {
          pRTS->m_name = CStdString(ptr);
        }
        else if(tag == __GAB1_ENTRY__)
        {
          pRTS->Add(AToW(&ptr[8]), false, *(int*)ptr, *(int*)(ptr+4));
          fInvalidate = true;
        }
        else if(tag == __GAB1_LANGUAGE_UNICODE__)
        {
          pRTS->m_name = (WCHAR*)ptr;
        }
        else if(tag == __GAB1_ENTRY_UNICODE__)
        {
          pRTS->Add((WCHAR*)(ptr+8), true, *(int*)ptr, *(int*)(ptr+4));
          fInvalidate = true;
        }

        ptr += size;
      }
    }
    else if(!strncmp((char*)pData, __GAB2__, strlen(__GAB2__)))
    {
      char* ptr = (char*)&pData[strlen(__GAB2__)+1];
      char* end = (char*)&pData[len];

      while(ptr < end)
      {
        WORD tag = *((WORD*)(ptr)); ptr += 2;
        DWORD size = *((DWORD*)(ptr)); ptr += 4;

        if(tag == __GAB1_LANGUAGE_UNICODE__)
        {
          pRTS->m_name = (WCHAR*)ptr;
        }
        else if(tag == __GAB1_RAWTEXTSUBTITLE__)
        {
          pRTS->Open((BYTE*)ptr, size, DEFAULT_CHARSET, pRTS->m_name);
          fInvalidate = true;
        }

        ptr += size;
      }
    }
    else if(pData != 0 && len > 1 && *pData != 0)
    {
      CStdStringA str((char*)pData, len);

      str.Replace("\r\n", "\n");
      str.Trim();

      if(!str.IsEmpty())
      {
        pRTS->Add(AToW(str), false, (int)(tStart / 10000), (int)(tStop / 10000));
        fInvalidate = true;
      }
    }
  }
  else if(m_mt.majortype == MEDIATYPE_Subtitle)
  {
    CAutoLock cAutoLock(m_pSubLock);

    if(m_mt.subtype == MEDIASUBTYPE_UTF8)
    {
      CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pSubStream;

      CStdStringW str = UTF8To16(CStdStringA((LPCSTR)pData, len)).Trim();
      if(!str.IsEmpty())
      {
        pRTS->Add(str, true, (int)(tStart / 10000), (int)(tStop / 10000));
        fInvalidate = true;
      }
    }
    else if(m_mt.subtype == MEDIASUBTYPE_SSA || m_mt.subtype == MEDIASUBTYPE_ASS || m_mt.subtype == MEDIASUBTYPE_ASS2)
    {
      CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pSubStream;

      CStdStringW str = UTF8To16(CStdStringA((LPCSTR)pData, len)).Trim();
      if(!str.IsEmpty())
      {
        STSEntry stse;

        int fields = m_mt.subtype == MEDIASUBTYPE_ASS2 ? 10 : 9;

        std::list<CStdStringW> sl;
        Explode(str, sl, ',', fields);
        if(sl.size() == fields)
        {
          stse.readorder = wcstol(sl.front(), NULL, 10); sl.pop_front();
          stse.layer = wcstol(sl.front(), NULL, 10); sl.pop_front();
          stse.style = sl.front(); sl.pop_front();
          stse.actor = sl.front(); sl.pop_front();
          stse.marginRect.left = wcstol(sl.front(), NULL, 10); sl.pop_front();
          stse.marginRect.right = wcstol(sl.front(), NULL, 10); sl.pop_front();
          stse.marginRect.top = stse.marginRect.bottom = wcstol(sl.front(), NULL, 10); sl.pop_front();
          if(fields == 10) { stse.marginRect.bottom = wcstol(sl.front(), NULL, 10); sl.pop_front(); }
          stse.effect = sl.front(); sl.pop_front();
          stse.str = sl.front(); sl.pop_front();
        }

        if(!stse.str.IsEmpty())
        {
          pRTS->Add(stse.str, true, (int)(tStart / 10000), (int)(tStop / 10000), 
            stse.style, stse.actor, stse.effect, stse.marginRect, stse.layer, stse.readorder);
          fInvalidate = true;
        }
      }
    }
    else if(m_mt.subtype == MEDIASUBTYPE_SSF)
    {
      ssf::CRenderer* pSSF = (ssf::CRenderer*)(ISubStream*)m_pSubStream;

      CStdStringW str = UTF8To16(CStdStringA((LPCSTR)pData, len)).Trim();
      if(!str.IsEmpty())
      {
        pSSF->Append(tStart, tStop, str);
        fInvalidate = true;
      }
    }
    else if(m_mt.subtype == MEDIASUBTYPE_VOBSUB)
    {
      CVobSubStream* pVSS = (CVobSubStream*)(ISubStream*)m_pSubStream;
      pVSS->Add(tStart, tStop, pData, len);
    }
    else if (IsHdmvSub(&m_mt))
    {
      CAutoLock cAutoLock(m_pSubLock);
      CRenderedHdmvSubtitle* pHdmvSubtitle = (CRenderedHdmvSubtitle*)(ISubStream*)m_pSubStream;
      pHdmvSubtitle->ParseSample (pSample);
    }
  }

  if(fInvalidate)
  {
    //TRACE(_T("InvalidateSubtitle(%I64d, ..)\n"), tStart);
    // IMPORTANT: m_pSubLock must not be locked when calling this
    InvalidateSubtitle(tStart, m_pSubStream);
  }

  hr = S_OK;

  return hr;
}

bool CSubtitleInputPin::IsHdmvSub(const CMediaType* pmt)
{
  return pmt->majortype == MEDIATYPE_Subtitle && (pmt->subtype == MEDIASUBTYPE_HDMVSUB ||       // Blu ray presentation graphics
                                                  pmt->subtype == MEDIASUBTYPE_DVB_SUBTITLES || // DVB subtitles
                                                 (pmt->subtype == MEDIASUBTYPE_NULL && pmt->formattype == FORMAT_SubtitleInfo)) // Workaround : support for Haali PGS
    ? true
    : false;
}