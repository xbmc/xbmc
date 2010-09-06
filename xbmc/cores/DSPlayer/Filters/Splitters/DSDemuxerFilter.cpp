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


#include "DSDemuxerFilter.h"
#include "DSStreamInfo.h"
#include "DSOutputpin.h"
#include "DVDPlayer/DVDClock.h"
#include "DVDPlayer/DVDInputStreams/DVDInputStream.h"
#include "DVDPlayer/DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDPlayer/DVDDemuxers/DVDDemux.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "DVDPlayer/DVDDemuxers/DVDFactoryDemuxer.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxFFmpeg.h"
#include <string>
#include "TimeUtils.h"
int64_t g_seekStart = 0;
int64_t g_seekEnd = 0;

#define START_SEEK_PERF g_seekStart = CurrentHostCounter();
#define END_SEEK_PERF(msg) g_seekEnd = CurrentHostCounter(); \
  CLog::Log(LOGINFO, "%s Elapsed time: %.2fms", msg, 1000.f * (g_seekEnd - g_seekStart) / CurrentHostFrequency()); 

CDSDemuxerFilter::CDSDemuxerFilter(LPUNKNOWN pUnk, HRESULT* phr) 
  : CBaseFilter(NAME("CDSDemuxerFilter"), pUnk, this,  __uuidof(this), phr)
  , m_rtDuration(0)
  , m_rtStart(0)
  , m_rtStop(0)
  , m_rtCurrent(0)
  , m_dRate(1.0)
{
  m_pDemuxer = NULL;
  m_pInputStream = NULL;
  

  if(phr) 
   *phr = S_OK;
}

CDSDemuxerFilter::~CDSDemuxerFilter()
{
  CAutoLock cAutoLock(this);

  CAMThread::CallWorker(CMD_EXIT);
  CAMThread::Close();

  m_State = State_Stopped;
  DeleteOutputs();
}

STDMETHODIMP CDSDemuxerFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
  CheckPointer(ppv, E_POINTER);
  return
    QI(IFileSourceFilter)
    QI(IMediaSeeking)
    QI(IAMStreamSelect)
    __super::NonDelegatingQueryInterface(riid, ppv);
}

// CBaseSplitter
int CDSDemuxerFilter::GetPinCount()
{
  CAutoLock lock(this);

  int count = m_pPins.size();
  return count;
}

CBasePin *CDSDemuxerFilter::GetPin(int n)
{
  CAutoLock lock(this);

  if (n < 0 ||n >= GetPinCount()) return NULL;
  return m_pPins[n];
}

CDSOutputPin *CDSDemuxerFilter::GetOutputPin(DWORD streamId)
{
  CAutoLock lock(&m_csPins);

  std::vector<CDSOutputPin *>::iterator it;
  for(it = m_pPins.begin(); it != m_pPins.end(); it++)
  {
    if ((*it)->GetStreamId() == streamId)
      return *it;
  }
  return NULL;
}

// IFileSourceFilter
STDMETHODIMP CDSDemuxerFilter::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE * pmt)
{
  CheckPointer(pszFileName, E_POINTER);

  m_fileName = std::wstring(pszFileName);

  HRESULT hr = S_OK;

  if(FAILED(hr = DeleteOutputs()) || FAILED(hr = CreateOutputs()))
  {
    m_fileName = L"";
  }

  return hr;
}

// Get the currently loaded file
STDMETHODIMP CDSDemuxerFilter::GetCurFile(LPOLESTR *ppszFileName, AM_MEDIA_TYPE *pmt)
{
  CheckPointer(ppszFileName, E_POINTER);

  int strlen = m_fileName.length() + 1;
  *ppszFileName = (LPOLESTR)CoTaskMemAlloc(sizeof(wchar_t) * strlen);

  if(!(*ppszFileName))
    return E_OUTOFMEMORY;

  wcsncpy_s(*ppszFileName, strlen, m_fileName.c_str(), _TRUNCATE);
  return S_OK;
}

REFERENCE_TIME CDSDemuxerFilter::GetStreamLength()
{
  return MSEC_TO_DS_TIME(m_pDemuxer->GetStreamLength());
}

// Pin creation
STDMETHODIMP CDSDemuxerFilter::CreateOutputs()
{
  CAutoLock lock(this);

  m_pFileNameA = WToA(CStdStringW(m_fileName.c_str()));

  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, m_pFileNameA, "");
  if (!m_pInputStream)
  {
    CLog::Log(LOGERROR, "%s: Error creating stream for %s", __FUNCTION__, m_pFileNameA.c_str());
    goto fail;
  }
  m_pInputStream->Open(m_pFileNameA.c_str(),"");
  if(m_pDemuxer)
    SAFE_DELETE(m_pDemuxer);
  try
  {
    m_pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(m_pInputStream);
    if(!m_pDemuxer)
    {
      delete m_pInputStream;
      CLog::Log(LOGERROR, "%s - Error creating demuxer", __FUNCTION__);
      goto fail;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when opening demuxer", __FUNCTION__);
    if (m_pDemuxer)
      delete m_pDemuxer;
    delete m_pInputStream;
    goto fail;
  }

  m_rtNewStart = m_rtStart = m_rtCurrent = 0;
  m_rtNewStop = m_rtStop = m_rtDuration = GetStreamLength();

  const CDVDDemuxFFmpeg *pDemuxer = static_cast<const CDVDDemuxFFmpeg*>(m_pDemuxer);
  const char *container = pDemuxer->m_pFormatContext->iformat->name;

  for(UINT streamId = 0; streamId < (UINT)m_pDemuxer->GetNrOfStreams(); streamId++)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(streamId);
    stream s;
    s.pid = streamId;
    s.streamInfo = new CDSStreamInfo(*pStream, container);

    switch(pStream->type)
    {
    case STREAM_VIDEO:
      m_streams[video].push_back(s);
      break;
    case STREAM_AUDIO:
      m_streams[audio].push_back(s);
      break;
    case STREAM_SUBTITLE:
      m_streams[subpic].push_back(s);
      break;
    default:
      // unsupported stream
      delete s.streamInfo;
      break;
    }
  }

  HRESULT hr = S_OK;

  // Try to create pins
  for(int i = 0; i < countof(m_streams); i++) 
  {
    for ( std::vector<stream>::iterator it = m_streams[i].begin(); it != m_streams[i].end(); it++ ) 
    {
      const WCHAR* name = CStreamList::ToString(i);
      
      CDSOutputPin* pPin = new CDSOutputPin(*it->streamInfo, name, this, this, &hr, container);
      if(SUCCEEDED(hr)) 
      {
        pPin->SetStreamId(it->pid);
        m_pPins.push_back(pPin);
        break;
      }
      else
        delete pPin;
    }
  }

  return S_OK;
fail:
  // Cleanup
  if(m_pDemuxer)
    SAFE_DELETE(m_pDemuxer);
  return E_FAIL;
}

STDMETHODIMP CDSDemuxerFilter::DeleteOutputs()
{
  CAutoLock lock(this);
  if(m_State != State_Stopped) 
    return VFW_E_NOT_STOPPED;

  /*if(m_pDemuxer)
    m_pDemuxer->Dispose();*/
  

  CAutoLock pinLock(&m_csPins);
  // Release pins
  std::vector<CDSOutputPin *>::iterator it;
  for(it = m_pPins.begin(); it != m_pPins.end(); it++) 
  {
    if(IPin* pPinTo = (*it)->GetConnected()) 
      pPinTo->Disconnect();
    (*it)->Disconnect();
    delete (*it);
  }
  m_pPins.clear();

  return S_OK;
}

bool CDSDemuxerFilter::IsAnyPinDrying()
{
  // MPC changes thread priority here
  // TODO: Investigate if that is needed
  std::vector<CDSOutputPin *>::iterator it;
  for(it = m_pPins.begin(); it != m_pPins.end(); it++)
  {
    if(!(*it)->IsDiscontinuous() && (*it)->QueueCount() < MIN_PACKETS_IN_QUEUE)
      return true;
  }
  return false;
}

// Worker Thread
DWORD CDSDemuxerFilter::ThreadProc()
{
  m_fFlushing = false;
  m_eEndFlush.Set();

  for(DWORD cmd = (DWORD)-1; ; cmd = GetRequest())
  {
    if(cmd == CMD_EXIT)
    {
      m_hThread = NULL;
      Reply(S_OK);
      return 0;
    }

    SetThreadPriority(m_hThread, THREAD_PRIORITY_NORMAL);

    m_rtStart = m_rtNewStart;
    m_rtStop = m_rtNewStop;

    DemuxSeek(m_rtStart);
    

    if(cmd != (DWORD)-1)
      Reply(S_OK);

    // Wait for the end of any flush
    m_eEndFlush.Wait();

    if(!m_fFlushing)
    {
      std::vector<CDSOutputPin *>::iterator it;
      for(it = m_pPins.begin(); it != m_pPins.end(); it++)
      {
        if ((*it)->IsConnected())
          (*it)->DeliverNewSegment(m_rtStart, m_rtStop, m_dRate);
      }
    }

    m_bDiscontinuitySent.clear();

    if (cmd == CMD_SEEK)
      END_SEEK_PERF("Seeking took")
    HRESULT hr = S_OK;
    while(SUCCEEDED(hr) && !CheckRequest(&cmd))
      hr = DemuxNextPacket();

    // If we didnt exit by request, deliver end-of-stream
    if(!CheckRequest(&cmd))
    {
      std::vector<CDSOutputPin *>::iterator it;
      for(it = m_pPins.begin(); it != m_pPins.end(); it++)
        (*it)->QueueEndOfStream();
    }

  }
  ASSERT(0); // we should only exit via CMD_EXIT

  m_hThread = NULL;
  return 0;
}

// Seek to the specified time stamp
// Based on DVDDemuxFFMPEG
void CDSDemuxerFilter::DemuxSeek(REFERENCE_TIME rtStart)
{
  int rt_sec;
  int64_t rt_dvd = rtStart / 10;
  rt_sec = DVD_TIME_TO_MSEC(rt_dvd);
  double start = DVD_NOPTS_VALUE;
  if (m_pDemuxer)
  {
    if (!m_pDemuxer->SeekTime(rt_sec, false, &start))
      CLog::Log(LOGERROR,"%s failed to seek",__FUNCTION__);
  }
  else
    CLog::Log(LOGERROR,"%s demuxer is closed",__FUNCTION__);
}

HRESULT CDSDemuxerFilter::DemuxNextPacket()
{
  HRESULT hr = S_OK;

  DemuxPacket *pPacket = NULL;
  CDemuxStream *pStream = NULL;
  // read a packet
  bool success = ReadPacket(pPacket, pStream);
  if ((pPacket && !pStream) || (pPacket && !GetOutputPin(pPacket->iStreamId)))
  {
    // empty packet, or no packet for an active stream, just skip
    hr = S_FALSE;
  }
  else if(success)
  {
    // all is well, deliver the thing
    Packet *p = new Packet();
    p->StreamId = (DWORD)pPacket->iStreamId;
    p->SetData(pPacket->pData, pPacket->iSize);

    REFERENCE_TIME rt = m_rtCurrent;
    REFERENCE_TIME duration = ((pPacket->duration > 0) ? (pPacket->duration * 10) : 1);
    if (pPacket->pts != DVD_NOPTS_VALUE)
      rt = (pPacket->pts * 10);  
    else if (pPacket->dts != DVD_NOPTS_VALUE)// if thats not set, use DTS
      rt = (pPacket->dts * 10);

    p->rtStart = rt;
    p->rtStop = p->rtStart + duration;
    p->bSyncPoint = (duration > 0) ? 1 : 0;
    p->bAppendable = !p->bSyncPoint;

    

    if (pStream->type == STREAM_SUBTITLE)
    {
      pPacket->duration = 1;
      p->rtStop = p->rtStart + 1;
    }

    hr = DeliverPacket(p);
  } else
    hr = E_FAIL;

  CDVDDemuxUtils::FreeDemuxPacket(pPacket);
  return hr;
}

bool CDSDemuxerFilter::ReadPacket(DemuxPacket*& DsPacket, CDemuxStream*& stream)
{
  if(m_pDemuxer)
    DsPacket = m_pDemuxer->Read();
  if (DsPacket)
  {
    stream = m_pDemuxer->GetStream(DsPacket->iStreamId);
    if (!stream)
    {
      CLog::Log(LOGERROR, "%s - Error demux DsPacket doesn't belong to a valid stream", __FUNCTION__);
      return false;
    }
    return true;
  }

  return false;
}

HRESULT CDSDemuxerFilter::DeliverPacket(Packet *pPacket)
{
  HRESULT hr = S_FALSE;

  CDSOutputPin* pPin = GetOutputPin(pPacket->StreamId);
  if(!pPin || !pPin->IsConnected()) 
  {
    delete pPacket;
    return S_FALSE;
  }

  if(pPacket->rtStart != INVALID_PACKET_TIME)
  {
    m_rtCurrent = pPacket->rtStart;

    pPacket->rtStart -= m_rtStart;
    pPacket->rtStop -= m_rtStart;

    ASSERT(pPacket->rtStart <= pPacket->rtStop);
  }

  if(m_bDiscontinuitySent.find(pPacket->StreamId) == m_bDiscontinuitySent.end())
    pPacket->bDiscontinuity = true;

  BOOL bDiscontinuity = pPacket->bDiscontinuity; 

  hr = pPin->QueuePacket(pPacket);

  // TODO track active pins

  if(bDiscontinuity)
    m_bDiscontinuitySent.insert(pPacket->StreamId);

  return hr;
}

// State Control
STDMETHODIMP CDSDemuxerFilter::Stop()
{
  CAutoLock cAutoLock(this);

  DeliverBeginFlush();
  CallWorker(CMD_EXIT);
  DeliverEndFlush();

  HRESULT hr;
  if(FAILED(hr = __super::Stop()))
    return hr;

  return S_OK;
}

STDMETHODIMP CDSDemuxerFilter::Pause()
{
  CAutoLock cAutoLock(this);

  FILTER_STATE fs = m_State;

  HRESULT hr;
  if(FAILED(hr = __super::Pause()))
    return hr;

  // The filter graph will set us to pause before running
  // So if we were stopped before, create the thread
  // Note that the splitter will always be running,
  // and even in pause mode fill up the buffers
  if(fs == State_Stopped)
    Create();

  return S_OK;
}

STDMETHODIMP CDSDemuxerFilter::Run(REFERENCE_TIME tStart)
{
  CAutoLock cAutoLock(this);

  HRESULT hr;
  if(FAILED(hr = __super::Run(tStart)))
    return hr;

  return S_OK;
}

// Flushing
void CDSDemuxerFilter::DeliverBeginFlush()
{
  m_fFlushing = true;

  // flush all pins
  std::vector<CDSOutputPin *>::iterator it;
  for(it = m_pPins.begin(); it != m_pPins.end(); it++)
    (*it)->DeliverBeginFlush();
}

void CDSDemuxerFilter::DeliverEndFlush()
{
  // flush all pins
  std::vector<CDSOutputPin *>::iterator it;
  for(it = m_pPins.begin(); it != m_pPins.end(); it++)
    (*it)->DeliverEndFlush();

  m_fFlushing = false;
  m_eEndFlush.Set();
}

// IMediaSeeking
STDMETHODIMP CDSDemuxerFilter::GetCapabilities(DWORD* pCapabilities)
{
  CheckPointer(pCapabilities, E_POINTER);

  *pCapabilities =
    AM_SEEKING_CanGetStopPos   |
    AM_SEEKING_CanGetDuration  |
    AM_SEEKING_CanSeekAbsolute |
    AM_SEEKING_CanSeekForwards |
    AM_SEEKING_CanSeekBackwards;

  return S_OK;
}

STDMETHODIMP CDSDemuxerFilter::CheckCapabilities(DWORD* pCapabilities)
{
  CheckPointer(pCapabilities, E_POINTER);
  // capabilities is empty, all is good
  if(*pCapabilities == 0) return S_OK;
  // read caps
  DWORD caps;
  GetCapabilities(&caps);

  // Store the caps that we wanted
  DWORD wantCaps = *pCapabilities;
  // Update pCapabilities with what we have
  *pCapabilities = caps & wantCaps;

  // if nothing matches, its a disaster!
  if(*pCapabilities == 0) return E_FAIL;
  // if all matches, its all good
  if(*pCapabilities == wantCaps) return S_OK;
  // otherwise, a partial match
  return S_FALSE;
}

STDMETHODIMP CDSDemuxerFilter::IsFormatSupported(const GUID* pFormat) {return !pFormat ? E_POINTER : *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;}
STDMETHODIMP CDSDemuxerFilter::QueryPreferredFormat(GUID* pFormat) {return GetTimeFormat(pFormat);}
STDMETHODIMP CDSDemuxerFilter::GetTimeFormat(GUID* pFormat) {return pFormat ? *pFormat = TIME_FORMAT_MEDIA_TIME, S_OK : E_POINTER;}
STDMETHODIMP CDSDemuxerFilter::IsUsingTimeFormat(const GUID* pFormat) {return IsFormatSupported(pFormat);}
STDMETHODIMP CDSDemuxerFilter::SetTimeFormat(const GUID* pFormat) {return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;}
STDMETHODIMP CDSDemuxerFilter::GetDuration(LONGLONG* pDuration) {CheckPointer(pDuration, E_POINTER); *pDuration = m_rtDuration; return S_OK;}
STDMETHODIMP CDSDemuxerFilter::GetStopPosition(LONGLONG* pStop) {CheckPointer(pStop, E_POINTER); *pStop = m_rtDuration; return S_OK;}
STDMETHODIMP CDSDemuxerFilter::GetCurrentPosition(LONGLONG* pCurrent)
{
  CheckPointer(pCurrent, E_NOTIMPL);
  if(pCurrent){ *pCurrent = m_rtCurrent;return S_OK;}
  
}
STDMETHODIMP CDSDemuxerFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat) {return E_NOTIMPL;}
STDMETHODIMP CDSDemuxerFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
  CAutoLock cAutoLock(this);
  START_SEEK_PERF
  if(!pCurrent && !pStop
    || (dwCurrentFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning 
    && (dwStopFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning)
      return S_OK;

  REFERENCE_TIME rtCurrent, rtStop;
  rtCurrent = m_rtCurrent,
  rtStop = m_rtStop;

  if(pCurrent)
  {
    switch(dwCurrentFlags&AM_SEEKING_PositioningBitsMask)
    {
    case AM_SEEKING_NoPositioning: break;
    case AM_SEEKING_AbsolutePositioning: rtCurrent = *pCurrent; break;
    case AM_SEEKING_RelativePositioning: rtCurrent = rtCurrent + *pCurrent; break;
    case AM_SEEKING_IncrementalPositioning: rtCurrent = rtCurrent + *pCurrent; break;
    }
  }

  if(pStop)
  {
    switch(dwStopFlags&AM_SEEKING_PositioningBitsMask)
    {
    case AM_SEEKING_NoPositioning: break;
    case AM_SEEKING_AbsolutePositioning: rtStop = *pStop; break;
    case AM_SEEKING_RelativePositioning: rtStop += *pStop; break;
    case AM_SEEKING_IncrementalPositioning: rtStop = rtCurrent + *pStop; break;
    }
  }

  if(m_rtCurrent == rtCurrent && m_rtStop == rtStop)
    return S_OK;

  m_rtNewStart = m_rtCurrent = rtCurrent;
  m_rtNewStop = rtStop;

  if(ThreadExists())
  {
    DeliverBeginFlush();
    CallWorker(CMD_SEEK);
    DeliverEndFlush();
  }

  return S_OK;
}
STDMETHODIMP CDSDemuxerFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
  if(pCurrent) *pCurrent = m_rtCurrent;
  if(pStop) *pStop = m_rtStop;
  return S_OK;
}
STDMETHODIMP CDSDemuxerFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
  if(pEarliest) *pEarliest = 0;
  return GetDuration(pLatest);
}
STDMETHODIMP CDSDemuxerFilter::SetRate(double dRate) {return dRate > 0 ? m_dRate = dRate, S_OK : E_INVALIDARG;}
STDMETHODIMP CDSDemuxerFilter::GetRate(double* pdRate) {return pdRate ? *pdRate = m_dRate, S_OK : E_POINTER;}
STDMETHODIMP CDSDemuxerFilter::GetPreroll(LONGLONG* pllPreroll) {return pllPreroll ? *pllPreroll = 0, S_OK : E_POINTER;}

STDMETHODIMP CDSDemuxerFilter::RenameOutputPin(DWORD TrackNumSrc, DWORD TrackNumDst, const AM_MEDIA_TYPE* pmt)
{
  CAutoLock lock(&m_csPins);

  CDSOutputPin* pPin = GetOutputPin(TrackNumSrc);
  if (pPin) 
  {
    if(IPin *pinTo = pPin->GetConnected())
    {
      if(pmt && FAILED(pinTo->QueryAccept(pmt)))
        return VFW_E_TYPE_NOT_ACCEPTED;
      pPin->SetStreamId(TrackNumDst);

      if(pmt)
        pPin->SetNewMediaType(*pmt);
    }
  }
  return E_FAIL;
}

// IAMStreamSelect
STDMETHODIMP CDSDemuxerFilter::Count(DWORD *pcStreams)
{
  CheckPointer(pcStreams, E_POINTER);

  *pcStreams = 0;
  for(int i = 0; i < countof(m_streams); i++)
    *pcStreams += m_streams[i].size();

  return S_OK;
}

STDMETHODIMP CDSDemuxerFilter::Enable(long lIndex, DWORD dwFlags)
{
  if(!(dwFlags & AMSTREAMSELECTENABLE_ENABLE))
    return E_NOTIMPL;

  for(int i = 0, j = 0; i < countof(m_streams); i++) 
  {
    int cnt = m_streams[i].size();

    if(lIndex >= j && lIndex < j+cnt) {
      long idx = (lIndex - j);

      stream& to = m_streams[i].at(idx);

      std::vector<stream>::iterator it;
      for(it = m_streams[i].begin(); it != m_streams[i].end(); it++) 
	  {
        if(!GetOutputPin(it->pid))
          continue;

        HRESULT hr;
        if(FAILED(hr = RenameOutputPin(*it, to, &to.streamInfo->mtype)))
          return hr;
        return S_OK;
      }
    }
    j += cnt;
  }
  return S_FALSE;
}

STDMETHODIMP CDSDemuxerFilter::Info(long lIndex, AM_MEDIA_TYPE **ppmt, DWORD *pdwFlags, LCID *plcid, DWORD *pdwGroup, WCHAR **ppszName, IUnknown **ppObject, IUnknown **ppUnk)
{
  for(int i = 0, j = 0; i < countof(m_streams); i++)
  {
    int cnt = m_streams[i].size();

    if(lIndex >= j && lIndex < j+cnt)
    {
      long idx = (lIndex - j);

      stream& s = m_streams[i][idx];

      if(ppmt) *ppmt = CreateMediaType(&s.streamInfo->mtype);
      if(pdwFlags) *pdwFlags = GetOutputPin(s) ? (AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE) : 0;
      if(plcid) *plcid = 0;
      if(pdwGroup) *pdwGroup = i;
      if(ppObject) *ppObject = NULL;
      if(ppUnk) *ppUnk = NULL;

      if(ppszName)
      {
        CStdStringW name = CStreamList::ToString(i);
        CStdStringW str;

          CDemuxStream *pStream = m_pDemuxer->GetStream(s.pid);
          std::string codecInfo;
          pStream->GetStreamInfo(codecInfo);
          CStdStringW codecInfoW(codecInfo.c_str());
          CStdString language = ISO6392ToLanguage(pStream->language);
          // TODO: make this nicer
          if (!language.IsEmpty())
            str.Format(L"%s - %s (%04x)", codecInfoW, language, s.pid); 
          else
            str.Format(L"%s (%04x)", codecInfoW, s.pid);

          if(plcid) *plcid = ISO6392ToLcid(pStream->language);

        *ppszName = (WCHAR*)CoTaskMemAlloc((str.GetLength()+1)*sizeof(WCHAR));
        if(*ppszName == NULL) return E_OUTOFMEMORY;

        wcscpy_s(*ppszName, str.GetLength()+1, str);
      }
    }
    j += cnt;
  }
  return S_OK;
}

// CStreamList
const WCHAR* CDSDemuxerFilter::CStreamList::ToString(int type)
{
  return 
    type == video ? L"Video" :
    type == audio ? L"Audio" :
    type == subpic ? L"Subtitle" :
    L"Unknown";
}

const CDSDemuxerFilter::stream* CDSDemuxerFilter::CStreamList::FindStream(DWORD pid)
{
  std::vector<stream>::iterator it;
  for ( it = begin(); it != end(); it++ )
  {
    if ((*it).pid == pid)
      return &(*it);
  }

  return NULL;
}

void CDSDemuxerFilter::CStreamList::Clear()
{
  std::vector<stream>::iterator it;
  for ( it = begin(); it != end(); it++ )
    delete (*it).streamInfo;
  __super::clear();
}
