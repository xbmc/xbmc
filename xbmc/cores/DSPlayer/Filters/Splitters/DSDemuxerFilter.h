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

#pragma once

#include "Thread.h"

#include "DVDPlayer/DVDMessageQueue.h"
#include "DVDPlayer/DVDClock.h"

#include "DSStreamInfo.h"

#include "TimeUtils.h"

#define DSPLAYER_AUDIO    1
#define DSPLAYER_VIDEO    2
#define DSPLAYER_SUBTITLE 3
#define DSPLAYER_TELETEXT 4



class CDSDemuxerThread;
class CDSOutputPin;
[uuid("B98D13E7-55DB-4385-A33D-09FD1BA26338")]
class CDSDemuxerFilter : public CSource
                       , public IMediaSeeking
                       , public IFileSourceFilter                        
//TODO add IAMStreamSelect and IAMMediaContent
{
public:
  CDSDemuxerFilter(LPUNKNOWN pUnk, HRESULT* phr);
  virtual ~CDSDemuxerFilter();

  // IUnknown
  DECLARE_IUNKNOWN;
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

  // IBaseFilter
  STDMETHODIMP Run(REFERENCE_TIME tStart);
  STDMETHODIMP Stop();
  STDMETHODIMP Pause();
 
  // IFileSourceFilter
  STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
  STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);

  // IMediaSeeking
  STDMETHODIMP GetCapabilities(DWORD* pCapabilities);
  STDMETHODIMP CheckCapabilities(DWORD* pCapabilities);
  STDMETHODIMP IsFormatSupported(const GUID* pFormat);
  STDMETHODIMP QueryPreferredFormat(GUID* pFormat);
  STDMETHODIMP GetTimeFormat(GUID* pFormat);
  STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat);
  STDMETHODIMP SetTimeFormat(const GUID* pFormat);
  STDMETHODIMP GetDuration(LONGLONG* pDuration);
  STDMETHODIMP GetStopPosition(LONGLONG* pStop);
  STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
  STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
  STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);
  STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);
  STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest);
  STDMETHODIMP SetRate(double dRate);
  STDMETHODIMP GetRate(double* pdRate);
  STDMETHODIMP GetPreroll(LONGLONG* pllPreroll);
  //Called from demux thread
  void         SetCurrentPosition(LONGLONG pos);
  

  // IAMStreamSelect
  //STDMETHODIMP Count(DWORD* pcStreams);
  //STDMETHODIMP Enable(long lIndex, DWORD dwFlags);
  //STDMETHODIMP Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk);
protected:
  DWORD ThreadProc();
private:
  CDSDemuxerThread *m_pDemuxerThread;

  CStdString m_filenameA; // holds the actual filename
  LPOLESTR m_filenameW;
  
  HRESULT ChangeStart();
  HRESULT ChangeStop();
  HRESULT ChangeRate();
	
  CRefTime m_rtPosition;      // current position
  CRefTime m_rtDuration;      // length of stream
  CRefTime m_rtStart;         // source will start here
  CRefTime m_rtStop;          // source will stop here
  double m_dRateSeeking;

  // seeking capabilities
  DWORD m_dwSeekingCaps;
	
  CCritSec m_SeekLock;
};