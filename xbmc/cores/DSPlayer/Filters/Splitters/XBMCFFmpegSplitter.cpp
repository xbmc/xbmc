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

#include <mmreg.h>
#include "XBMCFFmpegSplitter.h"
#include <list>

#include "DVDPlayer/DVDDemuxers/DVDDemux.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "DVDPlayer/DVDClock.h"
extern "C"
{
  #include "libavformat/avformat.h"
}

#define NO_SUBTITLE_PID USHRT_MAX		// Fake PID use for the "No subtitle" entry

//
// CXBMCFFmpegSplitter
//

CXBMCFFmpegSplitter::CXBMCFFmpegSplitter(LPUNKNOWN pUnk, HRESULT* phr)
  : CBaseSplitterFilter(NAME("CXBMCFFmpegSplitter"), pUnk, phr, __uuidof(this))
  , m_timeformat(TIME_FORMAT_MEDIA_TIME)
{
  m_pDemuxer = NULL;
  m_pInputStream = NULL;

}

CXBMCFFmpegSplitter::~CXBMCFFmpegSplitter()
{
}

STDMETHODIMP CXBMCFFmpegSplitter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
  CheckPointer(ppv, E_POINTER);
  return
    QI(IAMStreamSelect)
    __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CXBMCFFmpegSplitter::CreateOutputs(IAsyncReader* pAsyncReader)
{
  CheckPointer(pAsyncReader, E_POINTER);

  HRESULT hr = E_FAIL;
  m_filenameA = DShowUtil::WToA(CStdStringW(m_fn));

  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, m_filenameA, "");
  if (!m_pInputStream)
  {
    CLog::Log(LOGERROR, "%s: Error creating stream for %s", __FUNCTION__, m_filenameA.c_str());
    return E_FAIL;
  }
  m_pInputStream->Open(m_filenameA.c_str(),"");
  if(m_pDemuxer)
    SAFE_DELETE(m_pDemuxer);
  try
  {
    m_pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(m_pInputStream);
    if(!m_pDemuxer)
    {
      delete m_pInputStream;
      CLog::Log(LOGERROR, "%s - Error creating demuxer", __FUNCTION__);
      return E_FAIL;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when opening demuxer", __FUNCTION__);
    if (m_pDemuxer)
      delete m_pDemuxer;
    delete m_pInputStream;
    return E_FAIL;
  }

  m_rtNewStart = m_rtCurrent = 0;
  m_rtNewStop = m_rtStop = m_rtDuration = (DVD_MSEC_TO_TIME(m_pDemuxer->GetStreamLength()) * 10);//directshow basetime is 100 nanosec

  for(int i = 0; i < countof(m_streams); i++)
  {
    m_streams[i].clear();
  }

  bool fHasIndex = false;
  const CDVDDemuxFFmpeg *pDemuxer = static_cast<const CDVDDemuxFFmpeg*>(m_pDemuxer);

  /*stream s;
  s.pid = NO_SUBTITLE_PID;
  s.streamInfo = new CDSStreamInfo();
  s.streamInfo->mtype.InitMediaType();
  s.streamInfo->mtype.majortype = MEDIATYPE_Subtitle;
  s.streamInfo->mtype.subtype = MEDIASUBTYPE_NULL;
  s.streamInfo->mtype.formattype = FORMAT_SubtitleInfo;

  SUBTITLEINFO* psi = (SUBTITLEINFO *)s.streamInfo->mtype.AllocFormatBuffer(sizeof(SUBTITLEINFO));
  if (psi)
  {
    memset(psi, 0, sizeof(SUBTITLEINFO));
    strncpy_s(psi->IsoLang, "---", 3);
  }
  m_streams[SUBPIC].push_back(s);*/

  const char *containerFormat = pDemuxer->m_pFormatContext->iformat->name;
  for (int iStream=0; iStream < m_pDemuxer->GetNrOfStreams(); iStream++)
  {
    CDemuxStream *pStream = m_pDemuxer->GetStream(iStream);
    CDSStreamInfo *hint = new CDSStreamInfo(*pStream, true, containerFormat);

    stream s;
    s.streamInfo = hint;
    s.pid = iStream;

    switch(pStream->type) 
    {
      case STREAM_VIDEO:
        m_streams[VIDEO].push_back(s);
        break;
      case STREAM_AUDIO:
        m_streams[AUDIO].push_back(s);
        break;
      case STREAM_SUBTITLE:
        m_streams[SUBPIC].push_back(s);
        break;
      default:
        // unsupported stream
        delete hint;
        break;
      }
  }

  /*if(m_streams[SUBPIC].size() == 1) {
    m_streams[SUBPIC].clear();
  }*/

  for(int i = 0; i < countof(m_streams); i++)
  {
    vector<stream>::const_iterator it;
    for(it = m_streams[i].begin(); it != m_streams[i].end(); ++it) {
      CMediaType mt;
      vector<CMediaType> mts;
      mts.push_back(it->streamInfo->mtype);

      CStdStringW name = CStreamList::ToString(i);

      boost::shared_ptr<CBaseSplitterOutputPin> pPinOut(DNew CXBMCFFmpegOutputPin(mts, name, this, this, &hr));
      if(S_OK == AddOutputPin(it->pid, pPinOut))
        break;
    }
  }

  return m_pOutputs.size() > 0 ? S_OK : E_FAIL;
}

bool CXBMCFFmpegSplitter::DemuxInit()
{
  if(!m_pDemuxer) 
    return false;

  m_nOpenProgress = 100;
  return true;
}

void CXBMCFFmpegSplitter::DemuxSeek(REFERENCE_TIME rt)
{
  int rt_sec;
  int64_t rt_dvd = rt / 10;
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

bool CXBMCFFmpegSplitter::DemuxLoop()
{
  HRESULT hr = S_OK;
  
  while(SUCCEEDED(hr) && !CheckRequest(NULL))
  {
    hr = DemuxNextPacket();
  }

  return true;
}

HRESULT CXBMCFFmpegSplitter::DemuxNextPacket()
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
    boost::shared_ptr<Packet> p(new Packet(pPacket->iSize));
    p->TrackNumber = (DWORD)pPacket->iStreamId;
    memcpy(p->pInputBuffer, pPacket->pData, pPacket->iSize);

    REFERENCE_TIME rt = m_rtCurrent;
    // Check for PTS first
    if (pPacket->pts != DVD_NOPTS_VALUE) {
      rt = (pPacket->pts * 10);
      // if thats not set, use DTS
    } else if (pPacket->dts != DVD_NOPTS_VALUE) {
      rt = (pPacket->dts * 10);
    }

    // VC1 is mean like that!
    if (pStream->codec == CODEC_ID_VC1 && pPacket->dts != DVD_NOPTS_VALUE) {
      rt = (pPacket->dts * 10);
    }

    p->rtStart = rt;

    p->bSyncPoint = (pPacket->duration > 0) ? 1 : 0;
    //p->bAppendable = !p->bSyncPoint;

    p->rtStop = p->rtStart + ((pPacket->duration > 0) ? (pPacket->duration * 10) : 1);

    if (pStream->type == STREAM_SUBTITLE)
    {
      pPacket->duration = 1;
      p->rtStop = p->rtStart + 1;
    }

    hr = DeliverPacket(p);
  } else {
    hr = E_FAIL;
  }

  CDVDDemuxUtils::FreeDemuxPacket(pPacket);
  return hr;
}

bool CXBMCFFmpegSplitter::ReadPacket(DemuxPacket*& DsPacket, CDemuxStream*& stream)
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

// IMediaSeeking

STDMETHODIMP CXBMCFFmpegSplitter::GetDuration(LONGLONG* pDuration)
{
  CheckPointer(pDuration, E_POINTER);
  CheckPointer(m_pDemuxer, VFW_E_NOT_CONNECTED);

  if(m_timeformat == TIME_FORMAT_FRAME)
  {
    *pDuration = DVD_MSEC_TO_TIME(m_pDemuxer->GetStreamLength()) * 10;
    return S_OK;
  }

  return __super::GetDuration(pDuration);
}

//

STDMETHODIMP CXBMCFFmpegSplitter::IsFormatSupported(const GUID* pFormat)
{
  CheckPointer(pFormat, E_POINTER);
  HRESULT hr = __super::IsFormatSupported(pFormat);
  if(S_OK == hr) return hr;
  return *pFormat == TIME_FORMAT_FRAME ? S_OK : S_FALSE;
}

STDMETHODIMP CXBMCFFmpegSplitter::GetTimeFormat(GUID* pFormat)
{
  CheckPointer(pFormat, E_POINTER);
  *pFormat = m_timeformat;
  return S_OK;
}

STDMETHODIMP CXBMCFFmpegSplitter::IsUsingTimeFormat(const GUID* pFormat)
{
  CheckPointer(pFormat, E_POINTER);
  return *pFormat == m_timeformat ? S_OK : S_FALSE;
}

STDMETHODIMP CXBMCFFmpegSplitter::SetTimeFormat(const GUID* pFormat)
{
  CheckPointer(pFormat, E_POINTER);
  if(S_OK != IsFormatSupported(pFormat)) return E_FAIL;
  m_timeformat = *pFormat;
  return S_OK;
}

STDMETHODIMP CXBMCFFmpegSplitter::GetStopPosition(LONGLONG* pStop)
{
  CheckPointer(pStop, E_POINTER);
  if(FAILED(__super::GetStopPosition(pStop))) return E_FAIL;
  if(m_timeformat == TIME_FORMAT_MEDIA_TIME) return S_OK;
  LONGLONG rt = *pStop;
  if(FAILED(ConvertTimeFormat(pStop, &TIME_FORMAT_FRAME, rt, &TIME_FORMAT_MEDIA_TIME))) return E_FAIL;
  return S_OK;
}

STDMETHODIMP CXBMCFFmpegSplitter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
  return E_NOTIMPL;
  CheckPointer(pTarget, E_POINTER);
  //TODO

  return E_FAIL;
}

STDMETHODIMP CXBMCFFmpegSplitter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
  HRESULT hr;
  if(FAILED(hr = __super::GetPositions(pCurrent, pStop)) || m_timeformat != TIME_FORMAT_FRAME)
    return hr;

  if(pCurrent)
    if(FAILED(ConvertTimeFormat(pCurrent, &TIME_FORMAT_FRAME, *pCurrent, &TIME_FORMAT_MEDIA_TIME))) return E_FAIL;
  if(pStop)
    if(FAILED(ConvertTimeFormat(pStop, &TIME_FORMAT_FRAME, *pStop, &TIME_FORMAT_MEDIA_TIME))) return E_FAIL;

  return S_OK;
}

HRESULT CXBMCFFmpegSplitter::SetPositionsInternal(void* id, LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{

  if(m_timeformat != TIME_FORMAT_FRAME)
  {
    return __super::SetPositionsInternal(id, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
  } 


  if(!pCurrent && !pStop
    || (dwCurrentFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning 
    && (dwStopFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning)
    return S_OK;

  REFERENCE_TIME 
    rtCurrent = m_rtCurrent,
    rtStop = m_rtStop;

  if((dwCurrentFlags&AM_SEEKING_PositioningBitsMask)
    && FAILED(ConvertTimeFormat(&rtCurrent, &TIME_FORMAT_FRAME, rtCurrent, &TIME_FORMAT_MEDIA_TIME))) 
    return E_FAIL;
  if((dwStopFlags&AM_SEEKING_PositioningBitsMask)
    && FAILED(ConvertTimeFormat(&rtStop, &TIME_FORMAT_FRAME, rtStop, &TIME_FORMAT_MEDIA_TIME)))
    return E_FAIL;

  if(pCurrent)
    switch(dwCurrentFlags&AM_SEEKING_PositioningBitsMask)
  {
    case AM_SEEKING_NoPositioning: 
      break;
    case AM_SEEKING_AbsolutePositioning: 
      rtCurrent = *pCurrent; 
      break;
    case AM_SEEKING_RelativePositioning: 
      rtCurrent = rtCurrent + *pCurrent; break;
    case AM_SEEKING_IncrementalPositioning: 
      rtCurrent = rtCurrent + *pCurrent; break;
  }

  if(pStop)
  {
    switch(dwStopFlags&AM_SEEKING_PositioningBitsMask)
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

  if((dwCurrentFlags&AM_SEEKING_PositioningBitsMask)
    && pCurrent)
    if(FAILED(ConvertTimeFormat(pCurrent, &TIME_FORMAT_MEDIA_TIME, rtCurrent, &TIME_FORMAT_FRAME))) return E_FAIL;
  if((dwStopFlags&AM_SEEKING_PositioningBitsMask)
    && pStop)
    if(FAILED(ConvertTimeFormat(pStop, &TIME_FORMAT_MEDIA_TIME, rtStop, &TIME_FORMAT_FRAME))) return E_FAIL;

  return __super::SetPositionsInternal(id, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

HRESULT CXBMCFFmpegSplitter::Count(DWORD *pcStreams)
{
  CheckPointer(pcStreams, E_POINTER);

  *pcStreams = 0;

  for(int i = 0; i < countof(m_streams); i++)
    (*pcStreams) += m_streams[i].size();

  return S_OK;
}

HRESULT CXBMCFFmpegSplitter::Enable(long lIndex, DWORD dwFlags)
{
  if(!(dwFlags & AMSTREAMSELECTENABLE_ENABLE))
    return E_NOTIMPL;

  for(int i = 0, j = 0; i < countof(m_streams); i++)
  {
    int cnt = m_streams[i].size();

    if(lIndex >= j && lIndex < j+cnt)
    {
      long idx = (lIndex - j);

      stream& to = m_streams[i][idx];

      vector<stream>::iterator it;
      for(it = m_streams[i].begin(); it != m_streams[i].end(); ++it) {
        if(!GetOutputPin(it->pid)) continue;
        if(it->pid == to.pid) return S_FALSE;

        HRESULT hr;
        if(FAILED(hr = RenameOutputPin(it->pid, to.pid, &to.streamInfo->mtype)))
          return hr;

        return S_OK;
      }
    }

    j += cnt;
  }

  return S_FALSE;
}

HRESULT CXBMCFFmpegSplitter::Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk)
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

        if (i == CXBMCFFmpegSplitter::SUBPIC && s.pid == NO_SUBTITLE_PID)
        {
          str		= _T("No subtitles");
          *plcid	= LCID_NOSUBTITLES;
        } else {
          CDemuxStream *pStream = m_pDemuxer->GetStream(s.pid);
          std::string codecInfo;
          pStream->GetStreamInfo(codecInfo);
          CStdStringW codecInfoW(codecInfo.c_str());
          CStdString language = DShowUtil::ISO6392ToLanguage(pStream->language);
          // TODO: make this nicer
          if (!language.IsEmpty()) {
            str.Format(L"%s - %s (%04x)", codecInfoW, language, s.pid); 
          } else {
            str.Format(L"%s (%04x)", codecInfoW, s.pid);
          }
          if(plcid) *plcid = DShowUtil::ISO6392ToLcid(pStream->language);
        }

        *ppszName = (WCHAR*)CoTaskMemAlloc((str.GetLength()+1)*sizeof(WCHAR));
        if(*ppszName == NULL) return E_OUTOFMEMORY;

        wcscpy_s(*ppszName, str.GetLength()+1, str);
      }
    }
    j += cnt;
  }
  return S_OK;
}


//
// CXBMCFFmpegSourceFilter
//

CXBMCFFmpegSourceFilter::CXBMCFFmpegSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
  : CXBMCFFmpegSplitter(pUnk, phr)
{
  m_clsid = __uuidof(this);
  m_pInput.reset();

}

