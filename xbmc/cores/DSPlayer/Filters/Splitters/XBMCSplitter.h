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
#include "XBMCStreamVideo.h"
#include "XBMCStreamAudio.h"
#include "Codecs/DllAvFormat.h"
#include "Codecs/DllAvCodec.h"

#include "qnetwork.h"
#include "DSStreamInfo.h"
#include "DVDPlayer/DVDInputStreams/DVDInputStream.h"
#include "DVDPlayer/DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDPlayer/DVDDemuxers/DVDDemux.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "DVDPlayer/DVDDemuxers/DVDFactoryDemuxer.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxFFmpeg.h"

#define INT64_C __int64
#define MINPACKETS 100      // Beliyaal: Changed the dsmin number of packets to allow Bluray playback over network
#define MINPACKETSIZE 256*1024  // Beliyaal: Changed the dsmin packet size to allow Bluray playback over network
#define MAXPACKETS 10000
#define MAXPACKETSIZE 1024*1024*5

using namespace std;


class CDSVideoStream;
class CDSAudioStream;
[uuid("B98D13E7-55DB-4385-A33D-09FD1BA26338")]
class CXBMCSplitterFilter : public CSource,
                            public IMediaSeeking,
                            public IFileSourceFilter,
                            protected CAMThread,
                            public CCritSec
{
public:
  DECLARE_IUNKNOWN;
  
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
  {	
		if (riid == IID_IFileSourceFilter) 
			return GetInterface((IFileSourceFilter *)this, ppv);
    else if (riid == IID_IMediaSeeking) 
			return GetInterface((IMediaSeeking *)this, ppv);
    else
			return CSource::NonDelegatingQueryInterface(riid,ppv);
  }	

// --- IFileSourceFilter ---
	STDMETHODIMP Load(LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt);
  STDMETHODIMP GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *pmt) { ppszFileName = (LPOLESTR*)m_pFileName; return S_OK;};

// --- IMediaSeeking ---
  STDMETHODIMP IsFormatSupported(const GUID * pFormat);
  STDMETHODIMP QueryPreferredFormat(GUID *pFormat);
  STDMETHODIMP SetTimeFormat(const GUID * pFormat);
  STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);
  STDMETHODIMP GetTimeFormat(GUID *pFormat);
  STDMETHODIMP GetDuration(LONGLONG *pDuration);
  STDMETHODIMP GetStopPosition(LONGLONG *pStop);
  STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
  STDMETHODIMP GetCapabilities( DWORD * pCapabilities );
  STDMETHODIMP CheckCapabilities( DWORD * pCapabilities );
  STDMETHODIMP ConvertTimeFormat( LONGLONG * pTarget, const GUID * pTargetFormat, LONGLONG    Source, const GUID * pSourceFormat );
  STDMETHODIMP SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags, LONGLONG * pStop,  DWORD StopFlags );	
  STDMETHODIMP GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );	
  STDMETHODIMP GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest );
  STDMETHODIMP SetRate( double dRate);
  STDMETHODIMP GetRate( double * pdRate);
  STDMETHODIMP GetPreroll(LONGLONG *pPreroll);
  
  //IMediaFilter
  STDMETHODIMP Stop();
  STDMETHODIMP Pause();
  STDMETHODIMP Run(REFERENCE_TIME tStart);
	
protected:
  void Flush();
  //Demux thread
  enum {CMD_EXIT, CMD_SEEK};
  DWORD ThreadProc();
  DWORD m_priority;
  virtual HRESULT OnThreadCreate();
  
	CCritSec m_Lock;
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
  void AddStream(int streamindex);
  CDVDDemux* GetDemuxer() {return m_pDemuxer;}
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
  double m_dRateSeeking;
  // seeking capabilities
  DWORD m_dwSeekingCaps;
  CCritSec m_SeekLock;
  REFERENCE_TIME m_rtDuration; // derived filter should set this at the end of CreateOutputs
  REFERENCE_TIME m_rtStart, m_rtStop, m_rtCurrent, m_rtNewStart, m_rtNewStop;

  bool ReadPacket(DemuxPacket*& DsPacket, CDemuxStream*& stream);
  void ProcessPacket(CDemuxStream* pStream, DemuxPacket* pPacket);
  //void ProcessAudioData(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessVideoData(CDemuxStream* pStream, DemuxPacket* pPacket);
  //void ProcessSubData(CDemuxStream* pStream, DemuxPacket* pPacket);
  //void ProcessTeletextData(CDemuxStream* pStream, DemuxPacket* pPacket);
  CDVDDemux* m_pDemuxer;
  CDSVideoStream m_pVideoStream; //Video output pin
  void*          m_pCurrentVideoStream;
  CDSAudioStream m_pAudioStream; //Audio output pin
  void*          m_pCurrentAudioStream;
};

