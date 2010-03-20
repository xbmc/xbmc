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
#ifndef __XBMCFILESOURCE_H__
#include "filters/asyncio.h"
#include "filters/asyncrdr.h"
#include "CriticalSection.h"

using namespace XFILE;
class CXBMCFileReader;

class CXBMCFileStream : public CAsyncStream
{
public:
  CXBMCFileStream(CFile *file, IBaseFilter **pBF,HRESULT *phr);
  HRESULT SetPointer(LONGLONG llPos);
  HRESULT Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead);
  LONGLONG Size(LONGLONG *pSizeAvailable);
  DWORD Alignment();
  void Lock();
  void Unlock();


private:
    CCriticalSection  m_csLock;
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
