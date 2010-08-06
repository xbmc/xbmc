/*
 *      Copyright (C) 2010 Team XBMC
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

#include <string>
#include <list>
#include <set>
#include <vector>
#include "PacketQueue.h"
#include "DVDPlayer/DVDDemuxers/DVDFactoryDemuxer.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxFFmpeg.h"
#include "DShowUtil/DShowUtil.h"
#define FFMPEG_FILE_BUFFER_SIZE   32768 // default reading size for ffmpeg


class CDSStreamInfo;
class CDSOutputPin;

[uuid("B98D13E7-55DB-6969-A33D-09FD1BA26338")]
class CDSDemuxerFilter 
  : public CBaseFilter
  , public CCritSec
  , protected CAMThread
  , public IFileSourceFilter
  , public IMediaSeeking
  , public IAMStreamSelect
{
public:
  CDSDemuxerFilter(LPUNKNOWN pUnk, HRESULT* phr);
  virtual ~CDSDemuxerFilter();

  // IUnknown
  DECLARE_IUNKNOWN;
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

  // CBaseFilter methods
  int GetPinCount();
  CBasePin *GetPin(int n);

  STDMETHODIMP Stop();
  STDMETHODIMP Pause();
  STDMETHODIMP Run(REFERENCE_TIME tStart);

  // IFileSourceFilter
  STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE * pmt);
  STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName, AM_MEDIA_TYPE *pmt);

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

  // IAMStreamSelect
  STDMETHODIMP Count(DWORD *pcStreams);
  STDMETHODIMP Enable(long lIndex, DWORD dwFlags);
  STDMETHODIMP Info(long lIndex, AM_MEDIA_TYPE **ppmt, DWORD *pdwFlags, LCID *plcid, DWORD *pdwGroup, WCHAR **ppszName, IUnknown **ppObject, IUnknown **ppUnk);

  bool IsAnyPinDrying();
protected:
  // CAMThread
  enum {CMD_EXIT, CMD_SEEK};
  DWORD ThreadProc();

  REFERENCE_TIME GetStreamLength();
  void DemuxSeek(REFERENCE_TIME rtStart);
  
  HRESULT DemuxNextPacket();
  bool ReadPacket(DemuxPacket*& DsPacket, CDemuxStream*& stream);

  HRESULT DeliverPacket(Packet *pPacket);

  void DeliverBeginFlush();
  void DeliverEndFlush();

  STDMETHODIMP CreateOutputs();
  STDMETHODIMP DeleteOutputs();

public:
  struct stream {
    CDSStreamInfo *streamInfo;
    DWORD pid;
    struct stream() { streamInfo = NULL; pid = 0; }
    operator DWORD() const { return pid; }
    bool operator == (const struct stream& s) const { return (DWORD)*this == (DWORD)s; }
  };

  enum StreamType {video, audio, subpic, unknown};
  class CStreamList : public std::vector<stream>
  {
  public:
    static const WCHAR* ToString(int type);
    const stream* FindStream(DWORD pid);
    void Clear();
  } m_streams[unknown];

  CDSOutputPin *GetOutputPin(DWORD streamId);
  STDMETHODIMP RenameOutputPin(DWORD TrackNumSrc, DWORD TrackNumDst, const AM_MEDIA_TYPE* pmt);

private:
  CCritSec m_csPins;
  std::vector<CDSOutputPin *> m_pPins;
  std::set<DWORD> m_bDiscontinuitySent;
  
  CStdString   m_pFileNameA;
  std::wstring m_fileName;

  CDVDInputStream*    m_pInputStream;  // input stream for current playing file
  CDVDDemux*          m_pDemuxer;            // demuxer for current playing file

  // Times
  REFERENCE_TIME m_rtDuration; // derived filter should set this at the end of CreateOutputs
  REFERENCE_TIME m_rtStart, m_rtStop, m_rtCurrent, m_rtNewStart, m_rtNewStop;
  double m_dRate;

  // flushing
  bool m_fFlushing;
  CAMEvent m_eEndFlush;
};
