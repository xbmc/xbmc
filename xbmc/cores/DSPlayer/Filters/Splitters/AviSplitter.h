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

#include "../BaseFilters/BaseSplitter.h"
#include <boost/shared_ptr.hpp>

class CAviFile;

class CAviSplitterOutputPin : public CBaseSplitterOutputPin
{
public:
  CAviSplitterOutputPin(vector<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

  HRESULT CheckConnect(IPin* pPin);
};

[uuid("9736D831-9D6C-4E72-B6E7-560EF9181001")]
class CAviSplitterFilter : public CBaseSplitterFilter
{
  Com::SmartAutoVectorPtr<DWORD> m_tFrame;

protected:
  Com::Auto_Ptr<CAviFile> m_pFile;
  HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

  bool DemuxInit();
  void DemuxSeek(REFERENCE_TIME rt);
  bool DemuxLoop();

  HRESULT ReIndex(__int64 end, UINT64* pSize);

public:
  CAviSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);

  DECLARE_IUNKNOWN;
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

  // IMediaSeeking

  STDMETHODIMP GetDuration(LONGLONG* pDuration);

  // TODO: this is too ugly, integrate this with the baseclass somehow
  GUID m_timeformat;
  STDMETHODIMP IsFormatSupported(const GUID* pFormat);
  STDMETHODIMP GetTimeFormat(GUID* pFormat);
  STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat);
  STDMETHODIMP SetTimeFormat(const GUID* pFormat);
  STDMETHODIMP GetStopPosition(LONGLONG* pStop);
  STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
  STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);

  HRESULT SetPositionsInternal(void* id, LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);

  // IKeyFrameInfo

  STDMETHODIMP GetKeyFrameCount(UINT& nKFs);
  STDMETHODIMP GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs);
};

[uuid("CEA8DEFF-0AF7-4DB9-9A38-FB3C3AEFC0DE")]
class CAviSourceFilter : public CAviSplitterFilter
{
public:
  CAviSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};
