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

#include "Codecs/DllAvFormat.h"
#include "Codecs/DllAvCodec.h"
#include "XBMCStreamVideo.h"
#include "XBMCStreamAudio.h"
#include "qnetwork.h"

#ifndef _LINUX
#define INT64_C __int64
#endif

using namespace std;

class CDSAudioStream;
class CDSVideoStream;

[uuid("47E792CF-0BBE-4F7A-859C-194B0768650A")]
class CXBMCSplitterFilter : public CSource, 
                            public IMediaSeeking,
                            public IFileSourceFilter//, public IAMMediaContent
{
  friend class CDSVideoStream;
public:
  DECLARE_IUNKNOWN;
  
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
  {	
		if (riid == IID_IFileSourceFilter) 
			return GetInterface((IFileSourceFilter *)this, ppv);
    else if (riid == IID_IMediaSeeking) 
			return GetInterface((IMediaSeeking *)this, ppv);
    else if (riid == IID_IAMMediaContent) 
			return GetInterface((IAMMediaContent *)this, ppv);			
    else
			return CSource::NonDelegatingQueryInterface(riid,ppv);
  }	

// --- IFileSourceFilter ---
	STDMETHODIMP Load(LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt);
  STDMETHODIMP GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *pmt) { ppszFileName = (LPOLESTR*)m_pFileName; return S_OK;};

  // --- IMediaSeeking ---	
  STDMETHODIMP IsFormatSupported(const GUID * pFormat){return E_NOTIMPL;}
  STDMETHODIMP QueryPreferredFormat(GUID *pFormat){return E_NOTIMPL;}
  STDMETHODIMP SetTimeFormat(const GUID * pFormat){return E_NOTIMPL;}
  STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat){return E_NOTIMPL;}
  STDMETHODIMP GetTimeFormat(GUID *pFormat){return E_NOTIMPL;}
  STDMETHODIMP GetDuration(LONGLONG *pDuration){return E_NOTIMPL;}
  STDMETHODIMP GetStopPosition(LONGLONG *pStop){return E_NOTIMPL;}
  STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent){return E_NOTIMPL;}
  STDMETHODIMP GetCapabilities( DWORD * pCapabilities ){return E_NOTIMPL;}
  STDMETHODIMP CheckCapabilities( DWORD * pCapabilities ){return E_NOTIMPL;}
  STDMETHODIMP ConvertTimeFormat( LONGLONG * pTarget, const GUID * pTargetFormat, LONGLONG    Source, const GUID * pSourceFormat ){return E_NOTIMPL;}
  STDMETHODIMP SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags, LONGLONG * pStop,  DWORD StopFlags ){return E_NOTIMPL;}	
  STDMETHODIMP GetPositions( LONGLONG * pCurrent, LONGLONG * pStop ){return E_NOTIMPL;}	
  STDMETHODIMP GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest ){return E_NOTIMPL;}
  STDMETHODIMP SetRate( double dRate){return E_NOTIMPL;}
  STDMETHODIMP GetRate( double * pdRate){return E_NOTIMPL;}
  STDMETHODIMP GetPreroll(LONGLONG *pPreroll){return E_NOTIMPL;}
	
protected:
  DWORD ThreadProc( );
	
public:
  CXBMCSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
  virtual ~CXBMCSplitterFilter();

  AVFormatContext* m_pFormatContext;
  
  
  

  double   m_iCurrentPts; // used for stream length estimation
  

  DllAvFormat m_dllAvFormat;
  DllAvCodec  m_dllAvCodec;
  DllAvUtil   m_dllAvUtil;
  bool     m_bMatroska;
  bool     m_bAVI;
  bool OpenDemux(CStdString pFile);
  void AddStream(int iId);
  void UpdateCurrentPTS();
  double ConvertTimestamp(int64_t pts, int den, int num);
protected:
  	// --- IFileSourceFilter ---
	LPWSTR m_pFileName;

	// --- IMediaSeeking ---
  // we call this to notify changes. Override to handle them
  HRESULT ChangeStart();
  HRESULT ChangeStop();
  HRESULT ChangeRate();
  CRefTime m_rtDuration;      // length of stream
  CRefTime m_rtStart;         // source will start here
  CRefTime m_rtStop;          // source will stop here
  double m_dRateSeeking;
  // seeking capabilities
  DWORD m_dwSeekingCaps;
	
  CCritSec m_SeekLock;
  int             videoStream, audioStream;
  CDSAudioStream   m_pAudioStream;
  CDSVideoStream   m_pVideoStream;
};

