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

#include "LAVFSplitter.h"
#include "LAVFOutputPin.h"

#include "BaseDemuxer.h"
#include "LAVFDemuxer.h"

#include <string>

CLAVFSplitter::CLAVFSplitter(LPUNKNOWN pUnk, HRESULT* phr)
    : CBaseFilter(NAME("lavf dshow source filter"), pUnk, this,  __uuidof(this), phr)
    , m_rtStart(0), m_rtStop(0), m_rtCurrent(0)
    , m_dRate(1.0)
    , m_pDemuxer(NULL)
{
  if (phr)
  {
    *phr = S_OK;
  }
}

CLAVFSplitter::~CLAVFSplitter()
{
  CAutoLock cAutoLock(this);

  CAMThread::CallWorker(CMD_EXIT);
  CAMThread::Close();

  m_State = State_Stopped;
  DeleteOutputs();

  if (m_pDemuxer)
  {
    m_pDemuxer->Release();
  }
  //SAFE_DELETE(m_pDemuxer);
}

STDMETHODIMP CLAVFSplitter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
  CheckPointer(ppv, E_POINTER);

  *ppv = NULL;

  if (m_pDemuxer && (riid == __uuidof(IKeyFrameInfo) || riid == IID_IAMExtendedSeeking))
  {
    return m_pDemuxer->QueryInterface(riid, ppv);
  }

  return
    QI(IFileSourceFilter)
    QI(IMediaSeeking)
    QI(IAMStreamSelect)
    __super::NonDelegatingQueryInterface(riid, ppv);
}

// CBaseSplitter
int CLAVFSplitter::GetPinCount()
{
  CAutoLock lock(this);

  return (int)m_pPins.size();
}

CBasePin *CLAVFSplitter::GetPin(int n)
{
  CAutoLock lock(this);

  if (n < 0 || n >= GetPinCount()) return NULL;
  return m_pPins[n];
}

CLAVFOutputPin *CLAVFSplitter::GetOutputPin(DWORD streamId)
{
  CAutoLock lock(&m_csPins);

  std::vector<CLAVFOutputPin *>::iterator it;
  for (it = m_pPins.begin(); it != m_pPins.end(); it++)
  {
    if ((*it)->GetStreamId() == streamId)
    {
      return *it;
    }
  }
  return NULL;
}

// IFileSourceFilter
STDMETHODIMP CLAVFSplitter::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE * pmt)
{
  CheckPointer(pszFileName, E_POINTER);

  m_fileName = std::wstring(pszFileName);

  HRESULT hr = S_OK;
  SAFE_DELETE(m_pDemuxer);
  m_pDemuxer = new CLAVFDemuxer(this);
  if (FAILED(hr = m_pDemuxer->Open(pszFileName)))
  {
    SAFE_DELETE(m_pDemuxer);
    return hr;
  }
  m_pDemuxer->AddRef();

  m_rtStart = m_rtNewStart = m_rtCurrent = 0;
  m_rtStop = m_rtNewStop = m_pDemuxer->GetDuration();

  // Try to create pins
  for (int i = 0; i < CBaseDemuxer::unknown; i++)
  {
    CBaseDemuxer::CStreamList *streams = m_pDemuxer->GetStreams((CBaseDemuxer::StreamType)i);
    std::deque<CBaseDemuxer::stream>::iterator it;
    for ( it = streams->begin(); it != streams->end(); it++ )
    {
      const WCHAR* name = CBaseDemuxer::CStreamList::ToStringW(i);

      std::vector<CMediaType> mts = it->streamInfo->mtypes;

      CLAVFOutputPin* pPin = new CLAVFOutputPin(mts, name, this, this, &hr, (CBaseDemuxer::StreamType)i, m_pDemuxer->GetContainerFormat());
      if (SUCCEEDED(hr))
      {
        pPin->SetStreamId(it->pid);
        m_pPins.push_back(pPin);
        m_pDemuxer->SetActiveStream((CBaseDemuxer::StreamType)i, it->pid);
        break;
      }
      else
      {
        delete pPin;
      }
    }
  }
  if (SUCCEEDED(hr))
  {
    return !m_pPins.empty();
  }
  else
  {
    return hr;
  }
}

// Get the currently loaded file
STDMETHODIMP CLAVFSplitter::GetCurFile(LPOLESTR *ppszFileName, AM_MEDIA_TYPE *pmt)
{
  CheckPointer(ppszFileName, E_POINTER);

  size_t strlen = m_fileName.length() + 1;
  *ppszFileName = (LPOLESTR)CoTaskMemAlloc(sizeof(wchar_t) * strlen);

  if (!(*ppszFileName))
    return E_OUTOFMEMORY;

  wcsncpy_s(*ppszFileName, strlen, m_fileName.c_str(), _TRUNCATE);
  return S_OK;
}

STDMETHODIMP CLAVFSplitter::DeleteOutputs()
{
  CAutoLock lock(this);
  if (m_State != State_Stopped) return VFW_E_NOT_STOPPED;

  CAutoLock pinLock(&m_csPins);
  // Release pins
  std::vector<CLAVFOutputPin *>::iterator it;
  for (it = m_pPins.begin(); it != m_pPins.end(); it++)
  {
    if (IPin* pPinTo = (*it)->GetConnected()) pPinTo->Disconnect();
    (*it)->Disconnect();
    delete (*it);
  }
  m_pPins.clear();

  return S_OK;
}

bool CLAVFSplitter::IsAnyPinDrying()
{
  // MPC changes thread priority here
  // TODO: Investigate if that is needed
  std::vector<CLAVFOutputPin *>::iterator it;
  for (it = m_pPins.begin(); it != m_pPins.end(); it++)
  {
    if (!(*it)->IsDiscontinuous() && (*it)->QueueCount() < MIN_PACKETS_IN_QUEUE)
    {
      return true;
    }
  }
  return false;
}

// Worker Thread
DWORD CLAVFSplitter::ThreadProc()
{
  CheckPointer(m_pDemuxer, 0);

  m_fFlushing = false;
  m_eEndFlush.Set();
  for (DWORD cmd = (DWORD) - 1; ; cmd = GetRequest())
  {
    if (cmd == CMD_EXIT)
    {
      m_hThread = NULL;
      Reply(S_OK);
      return 0;
    }

    SetThreadPriority(m_hThread, THREAD_PRIORITY_NORMAL);

    m_rtStart = m_rtNewStart;
    m_rtStop = m_rtNewStop;

    DemuxSeek(m_rtStart);

    if (cmd != (DWORD) - 1)
      Reply(S_OK);

    // Wait for the end of any flush
    m_eEndFlush.Wait();

    if (!m_fFlushing)
    {
      std::vector<CLAVFOutputPin *>::iterator it;
      for (it = m_pPins.begin(); it != m_pPins.end(); it++)
      {
        if ((*it)->IsConnected())
        {
          (*it)->DeliverNewSegment(m_rtStart, m_rtStop, m_dRate);
        }
      }
    }

    m_bDiscontinuitySent.clear();

    HRESULT hr = S_OK;
    while (SUCCEEDED(hr) && !CheckRequest(&cmd))
    {
      hr = DemuxNextPacket();
    }

    // If we didnt exit by request, deliver end-of-stream
    if (!CheckRequest(&cmd))
    {
      std::vector<CLAVFOutputPin *>::iterator it;
      for (it = m_pPins.begin(); it != m_pPins.end(); it++)
      {
        (*it)->QueueEndOfStream();
      }
    }

  }
  ASSERT(0); // we should only exit via CMD_EXIT

  m_hThread = NULL;
  return 0;
}

// Seek to the specified time stamp
// Based on DVDDemuxFFMPEG
HRESULT CLAVFSplitter::DemuxSeek(REFERENCE_TIME rtStart)
{
  if (rtStart < 0)
  {
    rtStart = 0;
  }

  return m_pDemuxer->Seek(rtStart);
}

// Demux the next packet and deliver it to the output pins
// Based on DVDDemuxFFMPEG
HRESULT CLAVFSplitter::DemuxNextPacket()
{
  Packet *pPacket;
  HRESULT hr = S_OK;
  hr = m_pDemuxer->GetNextPacket(&pPacket);
  // Only S_OK indicates we have a proper packet
  // S_FALSE is a "soft error", don't deliver the packet
  if (hr != S_OK)
  {
    return hr;
  }
  return DeliverPacket(pPacket);
}

HRESULT CLAVFSplitter::DeliverPacket(Packet *pPacket)
{
  HRESULT hr = S_FALSE;

  CLAVFOutputPin* pPin = GetOutputPin(pPacket->StreamId);
  if (!pPin || !pPin->IsConnected())
  {
    delete pPacket;
    return S_FALSE;
  }

  if (pPacket->rtStart != Packet::INVALID_TIME)
  {
    m_rtCurrent = pPacket->rtStart;

    pPacket->rtStart -= m_rtStart;
    pPacket->rtStop -= m_rtStart;

    ASSERT(pPacket->rtStart <= pPacket->rtStop);
  }

  if (m_bDiscontinuitySent.find(pPacket->StreamId) == m_bDiscontinuitySent.end())
  {
    pPacket->bDiscontinuity = TRUE;
  }

  BOOL bDiscontinuity = pPacket->bDiscontinuity;
  DWORD streamId = pPacket->StreamId;

  hr = pPin->QueuePacket(pPacket);

  // TODO track active pins

  if (bDiscontinuity)
  {
    m_bDiscontinuitySent.insert(streamId);
  }

  return hr;
}

// State Control
STDMETHODIMP CLAVFSplitter::Stop()
{
  CAutoLock cAutoLock(this);

  DeliverBeginFlush();
  CallWorker(CMD_EXIT);
  DeliverEndFlush();

  HRESULT hr;
  if (FAILED(hr = __super::Stop()))
  {
    return hr;
  }

  return S_OK;
}

STDMETHODIMP CLAVFSplitter::Pause()
{
  CAutoLock cAutoLock(this);

  FILTER_STATE fs = m_State;

  HRESULT hr;
  if (FAILED(hr = __super::Pause()))
  {
    return hr;
  }

  // The filter graph will set us to pause before running
  // So if we were stopped before, create the thread
  // Note that the splitter will always be running,
  // and even in pause mode fill up the buffers
  if (fs == State_Stopped)
  {
    Create();
  }

  return S_OK;
}

STDMETHODIMP CLAVFSplitter::Run(REFERENCE_TIME tStart)
{
  CAutoLock cAutoLock(this);

  HRESULT hr;
  if (FAILED(hr = __super::Run(tStart)))
  {
    return hr;
  }

  return S_OK;
}

// Flushing
void CLAVFSplitter::DeliverBeginFlush()
{
  m_fFlushing = true;

  // flush all pins
  std::vector<CLAVFOutputPin *>::iterator it;
  for (it = m_pPins.begin(); it != m_pPins.end(); it++)
  {
    (*it)->DeliverBeginFlush();
  }
}

void CLAVFSplitter::DeliverEndFlush()
{
  // flush all pins
  std::vector<CLAVFOutputPin *>::iterator it;
  for (it = m_pPins.begin(); it != m_pPins.end(); it++)
  {
    (*it)->DeliverEndFlush();
  }

  m_fFlushing = false;
  m_eEndFlush.Set();
}

// IMediaSeeking
STDMETHODIMP CLAVFSplitter::GetCapabilities(DWORD* pCapabilities)
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

STDMETHODIMP CLAVFSplitter::CheckCapabilities(DWORD* pCapabilities)
{
  CheckPointer(pCapabilities, E_POINTER);
  // capabilities is empty, all is good
  if (*pCapabilities == 0) return S_OK;
  // read caps
  DWORD caps;
  GetCapabilities(&caps);

  // Store the caps that we wanted
  DWORD wantCaps = *pCapabilities;
  // Update pCapabilities with what we have
  *pCapabilities = caps & wantCaps;

  // if nothing matches, its a disaster!
  if (*pCapabilities == 0) return E_FAIL;
  // if all matches, its all good
  if (*pCapabilities == wantCaps) return S_OK;
  // otherwise, a partial match
  return S_FALSE;
}

STDMETHODIMP CLAVFSplitter::IsFormatSupported(const GUID* pFormat)
{
  return !pFormat ? E_POINTER : *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}
STDMETHODIMP CLAVFSplitter::QueryPreferredFormat(GUID* pFormat)
{
  return GetTimeFormat(pFormat);
}
STDMETHODIMP CLAVFSplitter::GetTimeFormat(GUID* pFormat)
{
  return pFormat ? *pFormat = TIME_FORMAT_MEDIA_TIME, S_OK : E_POINTER;
}
STDMETHODIMP CLAVFSplitter::IsUsingTimeFormat(const GUID* pFormat)
{
  return IsFormatSupported(pFormat);
}
STDMETHODIMP CLAVFSplitter::SetTimeFormat(const GUID* pFormat)
{
  return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;
}
STDMETHODIMP CLAVFSplitter::GetDuration(LONGLONG* pDuration)
{
  CheckPointer(pDuration, E_POINTER);
  CheckPointer(m_pDemuxer, E_UNEXPECTED);
  *pDuration = m_pDemuxer->GetDuration();
  return S_OK;
}
STDMETHODIMP CLAVFSplitter::GetStopPosition(LONGLONG* pStop)
{
  return GetDuration(pStop);
}
STDMETHODIMP CLAVFSplitter::GetCurrentPosition(LONGLONG* pCurrent)
{
  return E_NOTIMPL;
}
STDMETHODIMP CLAVFSplitter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
  return E_NOTIMPL;
}
STDMETHODIMP CLAVFSplitter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
  CAutoLock cAutoLock(this);

  if (!pCurrent && !pStop
      || (dwCurrentFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning
      && (dwStopFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning)
  {
    return S_OK;
  }

  REFERENCE_TIME
  rtCurrent = m_rtCurrent,
              rtStop = m_rtStop;

  if (pCurrent)
  {
    switch (dwCurrentFlags&AM_SEEKING_PositioningBitsMask)
    {
    case AM_SEEKING_NoPositioning:
      break;
    case AM_SEEKING_AbsolutePositioning:
      rtCurrent = *pCurrent;
      break;
    case AM_SEEKING_RelativePositioning:
      rtCurrent = rtCurrent + *pCurrent;
      break;
    case AM_SEEKING_IncrementalPositioning:
      rtCurrent = rtCurrent + *pCurrent;
      break;
    }
  }

  if (pStop)
  {
    switch (dwStopFlags&AM_SEEKING_PositioningBitsMask)
    {
    case AM_SEEKING_NoPositioning:
      break;
    case AM_SEEKING_AbsolutePositioning:
      rtStop = *pStop;
      break;
    case AM_SEEKING_RelativePositioning:
      rtStop += *pStop;
      break;
    case AM_SEEKING_IncrementalPositioning:
      rtStop = rtCurrent + *pStop;
      break;
    }
  }

  if (m_rtCurrent == rtCurrent && m_rtStop == rtStop)
  {
    return S_OK;
  }

  m_rtNewStart = m_rtCurrent = rtCurrent;
  m_rtNewStop = rtStop;

  if (ThreadExists())
  {
    DeliverBeginFlush();
    CallWorker(CMD_SEEK);
    DeliverEndFlush();
  }

  return S_OK;
}
STDMETHODIMP CLAVFSplitter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
  if (pCurrent) *pCurrent = m_rtCurrent;
  if (pStop) *pStop = m_rtStop;
  return S_OK;
}
STDMETHODIMP CLAVFSplitter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
  if (pEarliest) *pEarliest = 0;
  return GetDuration(pLatest);
}
STDMETHODIMP CLAVFSplitter::SetRate(double dRate)
{
  return dRate > 0 ? m_dRate = dRate, S_OK : E_INVALIDARG;
}
STDMETHODIMP CLAVFSplitter::GetRate(double* pdRate)
{
  return pdRate ? *pdRate = m_dRate, S_OK : E_POINTER;
}
STDMETHODIMP CLAVFSplitter::GetPreroll(LONGLONG* pllPreroll)
{
  return pllPreroll ? *pllPreroll = 0, S_OK : E_POINTER;
}

STDMETHODIMP CLAVFSplitter::RenameOutputPin(DWORD TrackNumSrc, DWORD TrackNumDst, std::vector<CMediaType> pmts)
{
  CheckPointer(m_pDemuxer, E_UNEXPECTED);
  CLAVFOutputPin* pPin = GetOutputPin(TrackNumSrc);
  // Output Pin was found
  // Stop the Graph, remove the old filter, render the graph again, start it up again
  // This only works on pins that were connected before, or the filter graph could .. well, break
  if (pPin && pPin->IsConnected())
  {
    CAutoLock lock(this);
    HRESULT hr = S_OK;

    IMediaControl *pControl = NULL;
    hr = m_pGraph->QueryInterface(IID_IMediaControl, (void **) & pControl);

    FILTER_STATE oldState;
    // Get the graph state
    // If the graph is in transition, we'll get the next state, not the previous
    pControl->GetState(10, (OAFilterState *)&oldState);
    // Stop the filter graph
    pControl->Stop();
    // Update Output Pin
    pPin->SetStreamId(TrackNumDst);
    m_pDemuxer->SetActiveStream(pPin->GetPinType(), TrackNumDst);
    pPin->SetNewMediaTypes(pmts);

    // Audio Filters get their connected filter removed
    // This way we make sure we reconnect to the proper filter
    // Other filters just disconnect and try to reconnect later on
    BOOL doRender = FALSE;
    PIN_INFO pInfo;
    hr = pPin->GetConnected()->QueryPinInfo(&pInfo);
    if (pPin->IsAudioPin() && SUCCEEDED(hr) && pInfo.pFilter)
    {
      m_pGraph->RemoveFilter(pInfo.pFilter);
      doRender = TRUE;
    }
    else
    {
      unsigned int index = 0;
      for (unsigned int i = 0; i < pmts.size(); i++)
      {
        if (SUCCEEDED(hr = pPin->GetConnected()->QueryAccept(&pmts[i])))
        {
          index = i;
          break;
        }
      }
      if (FAILED(hr) || FAILED(hr = ReconnectPin(pPin, &pmts[index])))
      {
        m_pGraph->Disconnect(pPin->GetConnected());
        m_pGraph->Disconnect(pPin);
        doRender = TRUE;
      }
    }
    if (pInfo.pFilter)
    {
      pInfo.pFilter->Release();
    }

    // Use IGraphBuilder to rebuild the graph
    IGraphBuilder *pGraphBuilder = NULL;
    if (doRender && SUCCEEDED(hr = m_pGraph->QueryInterface(__uuidof(IGraphBuilder), (void **) & pGraphBuilder)))
    {
      // Instruct the GraphBuilder to connect us again
      hr = pGraphBuilder->Render(pPin);
      pGraphBuilder->Release();
    }

    if (SUCCEEDED(hr))
    {
      // Re-start the graph
      if (oldState == State_Paused)
      {
        hr = pControl->Pause();
      }
      else if (oldState == State_Running)
      {
        hr = pControl->Run();
      }
    }
    pControl->Release();

    return hr;
  }
  else if (pPin)
  {
    // In normal operations, this won't make much sense
    // However, in graphstudio it is now possible to change the stream before connecting
    pPin->SetStreamId(TrackNumDst);
    m_pDemuxer->SetActiveStream(pPin->GetPinType(), TrackNumDst);
    pPin->SetNewMediaTypes(pmts);
  }

  return E_FAIL;
}

// IAMStreamSelect
STDMETHODIMP CLAVFSplitter::Count(DWORD *pcStreams)
{
  CheckPointer(pcStreams, E_POINTER);
  CheckPointer(m_pDemuxer, E_UNEXPECTED);

  *pcStreams = 0;
  for (int i = 0; i < CBaseDemuxer::unknown; i++)
  {
    *pcStreams += (DWORD)m_pDemuxer->GetStreams((CBaseDemuxer::StreamType)i)->size();
  }

  return S_OK;
}

STDMETHODIMP CLAVFSplitter::Enable(long lIndex, DWORD dwFlags)
{
  CheckPointer(m_pDemuxer, E_UNEXPECTED);
  if (!(dwFlags & AMSTREAMSELECTENABLE_ENABLE))
  {
    return E_NOTIMPL;
  }

  for (int i = 0, j = 0; i < CBaseDemuxer::unknown; i++)
  {
    CBaseDemuxer::CStreamList *streams = m_pDemuxer->GetStreams((CBaseDemuxer::StreamType)i);
    int cnt = (int)streams->size();

    if (lIndex >= j && lIndex < j + cnt)
    {
      long idx = (lIndex - j);

      CBaseDemuxer::stream& to = streams->at(idx);

      std::deque<CBaseDemuxer::stream>::iterator it;
      for (it = streams->begin(); it != streams->end(); it++)
      {
        if (!GetOutputPin(it->pid))
        {
          continue;
        }

        HRESULT hr;
        if (FAILED(hr = RenameOutputPin(*it, to, to.streamInfo->mtypes)))
        {
          return hr;
        }
        return S_OK;
      }
      break;
    }
    j += cnt;
  }
  return S_FALSE;
}

STDMETHODIMP CLAVFSplitter::Info(long lIndex, AM_MEDIA_TYPE **ppmt, DWORD *pdwFlags, LCID *plcid, DWORD *pdwGroup, WCHAR **ppszName, IUnknown **ppObject, IUnknown **ppUnk)
{
  CheckPointer(m_pDemuxer, E_UNEXPECTED);
  for (int i = 0, j = 0; i < CBaseDemuxer::unknown; i++)
  {
    CBaseDemuxer::CStreamList *streams = m_pDemuxer->GetStreams((CBaseDemuxer::StreamType)i);
    int cnt = (int)streams->size();

    if (lIndex >= j && lIndex < j + cnt)
    {
      long idx = (lIndex - j);

      CBaseDemuxer::stream& s = streams->at(idx);

      if (ppmt) *ppmt = CreateMediaType(&s.streamInfo->mtypes[0]);
      if (pdwFlags) *pdwFlags = GetOutputPin(s) ? (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE) : 0;
      if (pdwGroup) *pdwGroup = i;
      if (ppObject) *ppObject = NULL;
      if (ppUnk) *ppUnk = NULL;

      // Special case for the "no subtitles" pin
      if (s.pid == NO_SUBTITLE_PID)
      {
        if (plcid) *plcid = LCID_NOSUBTITLES;
        if (ppszName)
        {
          WCHAR str[] = L"S: No subtitles";
          size_t len = wcslen(str) + 1;
          *ppszName = (WCHAR*)CoTaskMemAlloc(len * sizeof(WCHAR));
          wcsncpy_s(*ppszName, len, str, _TRUNCATE);
        }
      }
      else
      {
        // Populate stream name and language code
        m_pDemuxer->StreamInfo(s.pid, plcid, ppszName);
      }
      break;
    }
    j += cnt;
  }

  return S_OK;
}
