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

//
//  Define an internal filter that wraps the base CBaseReader stuff
//
#pragma once

#include "filters/asyncio.h"
#include "filters/asyncrdr.h"
#include "threads/CriticalSection.h"
#include "filesystem/File.h"
using namespace XFILE;
class CXBMCASyncReader;

class CXBMCAsyncStream : 
  public CAsyncStream
{
public:
  CXBMCAsyncStream();
  virtual ~CXBMCAsyncStream();
  HRESULT SetPointer(LONGLONG llPos);
  HRESULT Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead);
  LONGLONG Size(LONGLONG *pSizeAvailable);
  DWORD Alignment();
  void Lock();
  void Unlock();

  HRESULT Load(const CStdString& file);

private:
  CCriticalSection  m_csLock;
  LONGLONG          m_llLength;
  CStdString        m_pFileName;
  CFile             m_pFile;
};


[uuid("EC5882E5-B1CB-4750-AF15-70459CAEC0D1")]
class CXBMCASyncReader :
  public CAsyncReader,
  public IFileSourceFilter
{
public:

  //  We're not going to be CoCreate'd so we don't need registration
  //  stuff etc
  STDMETHODIMP Register();
  STDMETHODIMP Unregister();

  DECLARE_IUNKNOWN
  CXBMCASyncReader(LPUNKNOWN pUnknown, HRESULT *phr);
  virtual ~CXBMCASyncReader() {}

  // IFileSourceFilter
  virtual HRESULT STDMETHODCALLTYPE Load( 
      /* [in] */ LPCOLESTR pszFileName,
      /* [annotation][unique][in] */ 
      __in_opt  const AM_MEDIA_TYPE *pmt);
        
  virtual HRESULT STDMETHODCALLTYPE GetCurFile( 
      /* [annotation][out] */ 
      __out  LPOLESTR *ppszFileName,
      /* [annotation][out] */ 
      __out_opt  AM_MEDIA_TYPE *pmt)
  {
    return E_NOTIMPL;
  }

  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

private:
  CXBMCAsyncStream m_pAsyncStream;
};