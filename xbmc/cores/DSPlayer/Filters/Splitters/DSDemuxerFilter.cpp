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


#include "DSDemuxerFilter.h"
#include "DSOutputpin.h"
#include "DSDemuxerThread.h"
#include <list>
#include <mmreg.h>
#include "DVDPlayer/DVDDemuxers/DVDDemux.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "DVDPlayer/DVDClock.h"
#include "DShowUtil/DShowUtil.h"
#include "CharsetConverter.h"
#pragma warning(disable: 4355) //Remove warning about using "this" in base member initializer listclass
extern "C"
{
  #include "libavformat/avformat.h"
}

#define NO_SUBTITLE_PID USHRT_MAX    // Fake PID use for the "No subtitle" entry

//
// CDSDemuxerFilter
//

CDSDemuxerFilter::CDSDemuxerFilter(LPUNKNOWN pUnk, HRESULT *phr)
  : CSource(LPCSTR("CDSDemuxerFilter"), pUnk, __uuidof(this), phr)
  , m_rtStart((long)0)
  , m_filenameW(NULL)
{
  m_rtStop = _I64_MAX / 2;
  m_rtDuration = m_rtStop;
  m_dRateSeeking = 1.0;
	
  m_dwSeekingCaps = AM_SEEKING_CanGetDuration		| AM_SEEKING_CanGetStopPos |
                    AM_SEEKING_CanSeekForwards  | AM_SEEKING_CanSeekBackwards | 
                    AM_SEEKING_CanSeekAbsolute;
}

CDSDemuxerFilter::~CDSDemuxerFilter()
{
  CLog::DebugLog("%s deleting demuxer thread",__FUNCTION__);
  delete m_pDemuxerThread;
  delete [] m_filenameW;
}

STDMETHODIMP CDSDemuxerFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
  CheckPointer(ppv, E_POINTER);
  return
    QI(IFileSourceFilter)
    QI(IMediaSeeking)
    __super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CDSDemuxerFilter::Stop()
{
  CLog::DebugLog("%s",__FUNCTION__);
  if (m_State != State_Stopped)
  {
    m_pDemuxerThread->Stop();
  }
  return __super::Stop();
}

STDMETHODIMP CDSDemuxerFilter::Pause()
{
  CLog::DebugLog("%s",__FUNCTION__);
  if (m_pDemuxerThread == NULL)
		return S_FALSE;
	
	if(m_State == State_Stopped)
	{
		m_pDemuxerThread->UnBlockProc(false);
	}
	else if(m_State == State_Running)
	{
		m_pDemuxerThread->BlockProc();
	}

	return __super::Pause();
}

STDMETHODIMP CDSDemuxerFilter::Run(REFERENCE_TIME tStart)
{
  CLog::DebugLog("%s",__FUNCTION__);
  m_pDemuxerThread->UnBlockProc(false);
  return __super::Run(tStart);
}


HRESULT CDSDemuxerFilter::ChangeStart()
{
	m_pDemuxerThread->SeekTo(m_rtStart);//, m_State == State_Stopped);
	return S_OK;
}

HRESULT CDSDemuxerFilter::ChangeStop()
{
	return S_OK;
}

HRESULT CDSDemuxerFilter::ChangeRate()
{
	return S_OK;
}
// IFileSourceFilter

STDMETHODIMP CDSDemuxerFilter::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
  CheckPointer(ppszFileName, E_POINTER);
  if (m_filenameW == NULL)
    return E_FAIL;
  
  DWORD n = (lstrlenW(m_filenameW) + 1) * sizeof(WCHAR);
  *ppszFileName = (LPOLESTR) CoTaskMemAlloc(n);
  CheckPointer(*ppszFileName, E_OUTOFMEMORY);
  CopyMemory(*ppszFileName, m_filenameW, n);
  return S_OK;
}

STDMETHODIMP CDSDemuxerFilter::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
  CheckPointer(pszFileName, E_POINTER);
  HRESULT hr = S_OK;
  int cch = lstrlenW(pszFileName) + 1;
  m_filenameW = new WCHAR[cch];
  CopyMemory(m_filenameW, pszFileName, cch * sizeof(WCHAR));
  m_pDemuxerThread = new CDSDemuxerThread(this, m_filenameW);
  if (!m_pDemuxerThread)
  {
    assert(0);
    return E_FAIL;
  }
  m_pDemuxerThread->Create();
	m_rtDuration = m_rtStop = m_pDemuxerThread->GetDuration();
  return S_OK;
}

// IMediaSeeking

STDMETHODIMP CDSDemuxerFilter::GetDuration(LONGLONG* pDuration)
{
  CheckPointer(pDuration, E_POINTER);
  CAutoLock lock(&m_SeekLock);

  *pDuration = m_rtDuration;
  return S_OK;
}

// IMediaSeeking

STDMETHODIMP CDSDemuxerFilter::GetCapabilities(DWORD* pCapabilities)
{
  return pCapabilities ? *pCapabilities = 
    AM_SEEKING_CanGetStopPos|
    AM_SEEKING_CanGetDuration|
    AM_SEEKING_CanSeekAbsolute|
    AM_SEEKING_CanSeekForwards|
    AM_SEEKING_CanSeekBackwards, S_OK : E_POINTER;
}
STDMETHODIMP CDSDemuxerFilter::CheckCapabilities(DWORD* pCapabilities)
{
  CheckPointer(pCapabilities, E_POINTER);
  if(*pCapabilities == 0) return S_OK;
  DWORD caps;
  GetCapabilities(&caps);
  if((caps&*pCapabilities) == 0) return E_FAIL;
  if(caps == *pCapabilities) return S_OK;
  return S_FALSE;
}

STDMETHODIMP CDSDemuxerFilter::QueryPreferredFormat(GUID* pFormat) {return GetTimeFormat(pFormat);}
STDMETHODIMP CDSDemuxerFilter::GetCurrentPosition(LONGLONG* pCurrent) {return E_NOTIMPL;}
STDMETHODIMP CDSDemuxerFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
  DWORD StopPosBits = dwStopFlags & AM_SEEKING_PositioningBitsMask;
  DWORD StartPosBits = dwCurrentFlags & AM_SEEKING_PositioningBitsMask;

  if(dwStopFlags) {
    CheckPointer(pStop, E_POINTER);

    // accept only relative, incremental, or absolute positioning
    if(StopPosBits != dwStopFlags) {
      return E_INVALIDARG;
    }
  }

  if(dwCurrentFlags) {
    CheckPointer(pCurrent, E_POINTER);
    if(StartPosBits != AM_SEEKING_AbsolutePositioning &&
       StartPosBits != AM_SEEKING_RelativePositioning) {
      return E_INVALIDARG;
    }
  }


  // scope for autolock
  {
    
    CAutoLock lock(&m_cStateLock);

    // set start position
    if(StartPosBits == AM_SEEKING_AbsolutePositioning)
    {
      m_rtStart = *pCurrent;
    }
    else if(StartPosBits == AM_SEEKING_RelativePositioning)
    {
      m_rtStart += *pCurrent;
    }

    // set stop position
    if(StopPosBits == AM_SEEKING_AbsolutePositioning)
    {
      m_rtStop = *pStop;
    }
    else if(StopPosBits == AM_SEEKING_IncrementalPositioning)
    {
      m_rtStop = m_rtStart + *pStop;
    }
    else if(StopPosBits == AM_SEEKING_RelativePositioning)
    {
      m_rtStop = m_rtStop + *pStop;
    }
  }


  HRESULT hr = S_OK;
  if(SUCCEEDED(hr) && StopPosBits) {
    hr = ChangeStop();
  }
  if(StartPosBits) {
    hr = ChangeStart();
  }

  return hr;
}
STDMETHODIMP CDSDemuxerFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
  if(pEarliest) *pEarliest = 0;
  return GetDuration(pLatest);
}
STDMETHODIMP CDSDemuxerFilter::SetRate(double dRate) { return S_OK;}
STDMETHODIMP CDSDemuxerFilter::GetRate(double* pdRate) { *pdRate = 1.0; return S_OK;}
STDMETHODIMP CDSDemuxerFilter::GetPreroll(LONGLONG* pllPreroll) {return pllPreroll ? *pllPreroll = 0, S_OK : E_POINTER;}


STDMETHODIMP CDSDemuxerFilter::IsFormatSupported(const GUID* pFormat)
{
  CheckPointer(pFormat, E_POINTER);
  return *pFormat == TIME_FORMAT_FRAME ? S_OK : S_FALSE;
}

STDMETHODIMP CDSDemuxerFilter::GetTimeFormat(GUID* pFormat)
{
  CheckPointer(pFormat, E_POINTER);
  *pFormat = TIME_FORMAT_MEDIA_TIME;
  return S_OK;
}

STDMETHODIMP CDSDemuxerFilter::IsUsingTimeFormat(const GUID* pFormat)
{
  CheckPointer(pFormat, E_POINTER);
  return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

STDMETHODIMP CDSDemuxerFilter::SetTimeFormat(const GUID* pFormat)
{
  CheckPointer(pFormat, E_POINTER);
  return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CDSDemuxerFilter::GetStopPosition(LONGLONG* pStop)
{
  CheckPointer(pStop, E_POINTER);
  CAutoLock lock(&m_SeekLock);
  *pStop = m_rtStop;
  return S_OK;
}

STDMETHODIMP CDSDemuxerFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
  return E_NOTIMPL;
  CheckPointer(pTarget, E_POINTER);
  //TODO

  return E_FAIL;
}

STDMETHODIMP CDSDemuxerFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
  if(pCurrent)
    if(FAILED(ConvertTimeFormat(pCurrent, &TIME_FORMAT_FRAME, *pCurrent, &TIME_FORMAT_MEDIA_TIME))) return E_FAIL;
  if(pStop)
    if(FAILED(ConvertTimeFormat(pStop, &TIME_FORMAT_FRAME, *pStop, &TIME_FORMAT_MEDIA_TIME))) return E_FAIL;

  return S_OK;
}
