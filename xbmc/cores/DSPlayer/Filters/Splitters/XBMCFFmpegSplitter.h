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

#include "../BaseFilters/BaseSplitter.h"
#include "XBMCFFmpegOutputPin.h"
#include "qnetwork.h" //ITrackInfo
#include "ITrackInfo.h"
#include "DSStreamInfo.h"
#include "DVDPlayer/DVDInputStreams/DVDInputStream.h"
#include "DVDPlayer/DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDPlayer/DVDDemuxers/DVDDemux.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "DVDPlayer/DVDDemuxers/DVDFactoryDemuxer.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxFFmpeg.h"
#include "TimeUtils.h"
#define INT64_C __int64
#define MINPACKETS 100      // Beliyaal: Changed the dsmin number of packets to allow Bluray playback over network
#define MINPACKETSIZE 256*1024  // Beliyaal: Changed the dsmin packet size to allow Bluray playback over network
#define MAXPACKETS 10000
#define MAXPACKETSIZE 1024*1024*5

using namespace std;

class CXBMCFFmpegOutputPin;
[uuid("B98D13E7-55DB-4385-A33D-09FD1BA26338")]
class CXBMCFFmpegSplitter : public CBaseSplitterFilter, 
                            public ITrackInfo
{
  Com::SmartAutoVectorPtr<DWORD> m_tFrame;

protected:
  //std::auto_ptr<CAviFile> m_pFile; not used do we need a fake?
  CDVDDemux* m_pDemuxer;
  CDVDInputStream* m_pInputStream;
  CStdString m_filenameA;
  HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

  bool DemuxInit();
  void DemuxSeek(REFERENCE_TIME rt);
  bool DemuxLoop();
  bool ReadPacket(DemuxPacket*& DsPacket, CDemuxStream*& stream);
public:
  CXBMCFFmpegSplitter(LPUNKNOWN pUnk, HRESULT* phr);
  virtual ~CXBMCFFmpegSplitter();
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

  //STDMETHODIMP GetKeyFrameCount(UINT& nKFs);
  //STDMETHODIMP GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs);
 
  // ITrackInfo TODO
  STDMETHODIMP_(UINT) GetTrackCount() {return 0;}
	STDMETHODIMP_(BOOL) GetTrackInfo(UINT aTrackIdx, struct TrackElement* pStructureToFill){return 0;}
	STDMETHODIMP_(BOOL) GetTrackExtendedInfo(UINT aTrackIdx, void* pStructureToFill){return 0;}
	STDMETHODIMP_(BSTR) GetTrackName(UINT aTrackIdx){return L"";}
	STDMETHODIMP_(BSTR) GetTrackCodecID(UINT aTrackIdx){return L"";}
	STDMETHODIMP_(BSTR) GetTrackCodecName(UINT aTrackIdx){return L"";}
	STDMETHODIMP_(BSTR) GetTrackCodecInfoURL(UINT aTrackIdx){return L"";}
	STDMETHODIMP_(BSTR) GetTrackCodecDownloadURL(UINT aTrackIdx){return L"";}
};


[uuid("82F6C688-7383-4885-BE5F-AFE6938B157F")]
class CXBMCFFmpegSourceFilter : public CXBMCFFmpegSplitter
{
public:
	CXBMCFFmpegSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};