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

#define MINPACKETS 100      // Beliyaal: Changed the dsmin number of packets to allow Bluray playback over network
#define MINPACKETSIZE 256*1024  // Beliyaal: Changed the dsmin packet size to allow Bluray playback over network
#define MAXPACKETS 10000
#define MAXPACKETSIZE 1024*1024*5

using namespace std;

class CXBMCFFmpegOutputPin;
[uuid("B98D13E7-55DB-4385-A33D-09FD1BA26338")]
class CXBMCFFmpegSplitter : public CBaseSplitterFilter, 
  public ITrackInfo,
  public IAMStreamSelect/*,
                        public IAMExtendedSeeking*/
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
  DECLARE_IUNKNOWN;

  CXBMCFFmpegSplitter(LPUNKNOWN pUnk, HRESULT* phr);
  virtual ~CXBMCFFmpegSplitter();


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

  // IAMStreamSelect
  STDMETHODIMP Count(DWORD* pcStreams);
  STDMETHODIMP Enable(long lIndex, DWORD dwFlags);
  STDMETHODIMP Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk);

public:
  struct stream
  {
    CDSStreamInfo *streamInfo;
    WORD pid;
    struct stream() {pid = 0;}
    operator DWORD() const {return pid ? pid : 0;}
    bool operator == (const struct stream& s) const {return (DWORD)*this == (DWORD)s;}
  };

  enum StreamTypes {VIDEO, AUDIO, SUBPIC, UNKNOWN};

  class CStreamList : public vector<stream>
  {
  public:
    static CStdStringW ToString(int type)
    {
      return
        type == VIDEO ? L"Video" :
        type == AUDIO ? L"Audio" :
        type == SUBPIC ? L"Subtitle" :
        L"Unknown";
    }

    const stream* FindStream(int pid)
    {
      vector<stream>::const_iterator it;
      for(it = begin(); it != end(); ++it) {
        if(it->pid == pid)
          return &(*it);
      }
      return NULL;
    }

    void clear()
    {
      vector<stream>::iterator it;
      for(it = begin(); it != end(); ++it) {
        if(it->streamInfo) {
          delete it->streamInfo;
        }
      }
      __super::clear();
    }
  } m_streams[UNKNOWN];
};


[uuid("82F6C688-7383-4885-BE5F-AFE6938B157F")]
class CXBMCFFmpegSourceFilter : public CXBMCFFmpegSplitter
{
public:
  CXBMCFFmpegSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};