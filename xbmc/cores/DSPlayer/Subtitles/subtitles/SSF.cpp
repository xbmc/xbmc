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
 *  TODO: 
 *  - fill effect
 *  - outline bkg still very slow
 *
 */

#include "stdafx.h"
#include <xmmintrin.h>
#include <emmintrin.h>
#include "SSF.h"
#include "..\subpic\MemSubPic.h"

namespace ssf
{
  CRenderer::CRenderer(CCritSec* pLock)
    : ISubPicProviderImpl(pLock)
  {
  }

  CRenderer::~CRenderer()
  {
  }

  bool CRenderer::Open(CStdString fn, CStdString name)
  {
    m_fn.Empty();
    m_name.Empty();
    m_file.release();
    m_renderer.release();

    if(name.IsEmpty())
    {
      CStdString str = fn;
      str.Replace('\\', '/');
      name = str.Left(str.ReverseFind('.'));
      name = name.Mid(name.ReverseFind('/')+1);
      name = name.Mid(name.ReverseFind('.')+1);
    }

    try
    {
      if(Open(FileInputStream(fn), name)) 
      {
        m_fn = fn;
        return true;
      }
    }
    catch(Exception& e)
    {
      UNREFERENCED_PARAMETER(e);
      //TRACE(_T("%s\n"), e.ToString());
    }

    return false;  
  }

  bool CRenderer::Open(InputStream& s, CStdString name)
  {
    m_fn.Empty();
    m_name.Empty();
    m_file.release();
    m_renderer.release();

    try
    {
      m_file.reset(DNew SubtitleFile());
      m_file->Parse(s);
      m_renderer.reset(DNew Renderer());
      m_name = name;
      return true;
    }
    catch(Exception& e)
    {
      UNREFERENCED_PARAMETER(e);
      //TRACE(_T("%s\n"), e.ToString());
    }

    return false;
  }

  void CRenderer::Append(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, LPCWSTR str)
  {
    if(!m_file.get() ) return;

    try
    {
      m_file->Append(ssf::WCharInputStream(str), (float)rtStart / 10000000, (float)rtStop / 10000000);
    }
    catch(Exception& e)
    {
      UNREFERENCED_PARAMETER(e);
      //TRACE(_T("%s\n"), e.ToString());
    }
  }

  STDMETHODIMP CRenderer::NonDelegatingQueryInterface(REFIID riid, void** ppv)
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

  STDMETHODIMP_(int) CRenderer::GetStartPosition(REFERENCE_TIME rt, double fps)
  {
    size_t k;
    return m_file.get() && m_file->m_segments.Lookup((float)rt/10000000, k) ? ++k : NULL;
  }

  STDMETHODIMP_(int) CRenderer::GetNext(int pos)
  {
    size_t k = (size_t)pos;
    return m_file.get() && m_file->m_segments.GetSegment(k) ? (++k) : NULL;
  }

	STDMETHODIMP_(REFERENCE_TIME) CRenderer::GetStart(int pos, double fps)
  {
    size_t k = (size_t)pos-1;
    const SubtitleFile::Segment* s = m_file.get() ? m_file->m_segments.GetSegment(k) : NULL;
    return s ? (REFERENCE_TIME)(s->m_start*10000000) : 0;
  }

	STDMETHODIMP_(REFERENCE_TIME) CRenderer::GetStop(int pos, double fps)
  {
    CheckPointer(m_file.get(), 0);

    size_t k = (size_t)pos-1;
    const SubtitleFile::Segment* s = m_file.get() ? m_file->m_segments.GetSegment(k) : NULL;
    return s ? (REFERENCE_TIME)(s->m_stop*10000000) : 0;
  }

	STDMETHODIMP_(bool) CRenderer::IsAnimated(int pos)
  {
    return true;
  }

  STDMETHODIMP CRenderer::Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox)
  {
    CheckPointer(m_file.get(), E_UNEXPECTED);
    CheckPointer(m_renderer.get(), E_UNEXPECTED);  

    if(spd.type != MSP_RGB32) return E_INVALIDARG;

    CAutoLock csAutoLock(m_pLock);

    Com::SmartRect bbox2;
    bbox2.SetRectEmpty();

    std::list<boost::shared_ptr<Subtitle>> subs;
    m_file->Lookup((float)rt/10000000, subs);

    m_renderer->NextSegment(subs);

    std::list<boost::shared_ptr<Subtitle>>::const_iterator pos = subs.begin();
    while(pos != subs.end())
    {
      const Subtitle s = (*pos->get()); pos++;
      const RenderedSubtitle* rs = m_renderer->Lookup(&s, Com::SmartSize(spd.w, spd.h), spd.vidrect);
      if(rs) bbox2 |= rs->Draw(spd);
    }

    bbox = bbox2 & Com::SmartRect(0, 0, spd.w, spd.h);

    return S_OK;
  }

  // IPersist

  STDMETHODIMP CRenderer::GetClassID(CLSID* pClassID)
  {
    return pClassID ? *pClassID = __uuidof(this), S_OK : E_POINTER;
  }

  // ISubStream

  STDMETHODIMP_(int) CRenderer::GetStreamCount()
  {
    return 1;
  }

  STDMETHODIMP CRenderer::GetStreamInfo(int iStream, WCHAR** ppName, LCID* pLCID)
  {
    if(iStream != 0) return E_INVALIDARG;

    if(ppName)
    {
      if(!(*ppName = (WCHAR*)CoTaskMemAlloc((m_name.GetLength()+1)*sizeof(WCHAR))))
        return E_OUTOFMEMORY;

      wcscpy(*ppName, CStdStringW(m_name));
    }

    if(pLCID)
    {
      *pLCID = 0; // TODO
    }

    return S_OK;
  }

  STDMETHODIMP_(int) CRenderer::GetStream()
  {
    return 0;
  }

  STDMETHODIMP CRenderer::SetStream(int iStream)
  {
    return iStream == 0 ? S_OK : E_FAIL;
  }

  STDMETHODIMP CRenderer::Reload()
  {
    CAutoLock csAutoLock(m_pLock);

    return !m_fn.IsEmpty() && Open(m_fn, m_name) ? S_OK : E_FAIL;
  }
}
