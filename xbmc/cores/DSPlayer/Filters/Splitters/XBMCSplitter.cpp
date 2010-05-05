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


#include "XBMCSplitter.h"

#include <initguid.h>
#include <moreuuids.h>
#include "XBMCStreamVideo.h"
#include "XBMCStreamAudio.h"
#include "DShowUtil/DShowUtil.h"
#include "utils/Win32Exception.h"
#include "Util.h"
#include "FileSystem/StackDirectory.h"
#include "utils/TimeUtils.h"

#include "DVDPlayer/DVDDemuxers/DVDDemux.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxUtils.h"




//
// CXBMCSplitterFilter
//

CXBMCSplitterFilter::CXBMCSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
  :CSource(NAME("CXBMCSplitterFilter"), pUnk, __uuidof(this), phr),
  m_pAudioStream(GetOwner(), this, phr),
  m_pVideoStream(GetOwner(), this, phr),
  m_rtDuration(0), m_rtStart(0), m_rtStop(0), m_rtCurrent(0)
{	
  m_dwSeekingCaps = AM_SEEKING_CanGetDuration | AM_SEEKING_CanGetStopPos | 
                    AM_SEEKING_CanSeekForwards | AM_SEEKING_CanSeekBackwards | 
                    AM_SEEKING_CanSeekAbsolute;
  m_pDemuxer = NULL;
  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllAvFormat.Load())
    *phr = E_FAIL;
}

CXBMCSplitterFilter::~CXBMCSplitterFilter()
{

}

// ============================================================================
// CXBMCSplitterFilter IFileSourceFilter interface
// ============================================================================

STDMETHODIMP CXBMCSplitterFilter::Load(LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt)
{
  CLog::Log(LOGDEBUG,"%s",__FUNCTION__);
	CheckPointer(lpwszFileName, E_POINTER);	
	
	// lstrlenW is one of the few Unicode functions that works on win95
	int cch = lstrlenW(lpwszFileName) + 1;
	
	m_pFileName = new WCHAR[cch];
	
	if (m_pFileName!=NULL)
	{
		CopyMemory(m_pFileName, lpwszFileName, cch*sizeof(WCHAR));	
	} else {
		NOTE("CCAudioSource::Load filename is empty !");
		return VFW_E_CANNOT_RENDER;
	}
  TCHAR *lpszFileName=0;
	lpszFileName = new char[cch * 2];
	if (!lpszFileName) {
		NOTE("CXBMCSplitterFilter::Load new lpszFileName failed E_OUTOFMEMORY");
		return E_OUTOFMEMORY;
	}
	WideCharToMultiByte(GetACP(), 0, lpwszFileName, -1, lpszFileName, cch, NULL, NULL);
  CAutoLock cAutoLock(&m_cStateLock);
  m_dllAvFormat.av_register_all();
  OpenDemux(CStdString(lpszFileName));

}

STDMETHODIMP CXBMCSplitterFilter::Run(REFERENCE_TIME tStart)
{
  if (ThreadExists() == FALSE)
    Create();
  return S_OK;
}

STDMETHODIMP CXBMCSplitterFilter::Pause()
{
  CAutoLock cAutoLock(this);
  CLOG_FUNCTION
  FILTER_STATE fs = m_State;

  HRESULT hr;
  

  if(fs == State_Stopped)
  {
    Create();
  }
  if(FAILED(hr = __super::Pause()))
    return hr;

  return S_OK;
}

STDMETHODIMP CXBMCSplitterFilter::Stop()
{
	CLOG_FUNCTION
  
	

	return CSource::Stop();
}

// ============================================================================
// CXBMCSplitterFilter IMediaSeeking interface
// ============================================================================

HRESULT CXBMCSplitterFilter::ChangeStart()
{
	//m_pRunningThread->SeekToTimecode(m_rtStart, m_State == State_Stopped);
	return S_OK;
}

HRESULT CXBMCSplitterFilter::ChangeStop()
{
	return S_OK;
}

HRESULT CXBMCSplitterFilter::ChangeRate()
{
	return S_OK;
}

HRESULT CXBMCSplitterFilter::IsFormatSupported(const GUID * pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    // only seeking in time (REFERENCE_TIME units) is supported
    return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

HRESULT CXBMCSplitterFilter::QueryPreferredFormat(GUID *pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

HRESULT CXBMCSplitterFilter::SetTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    // nothing to set; just check that it's TIME_FORMAT_TIME
    return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : E_INVALIDARG;
}

HRESULT CXBMCSplitterFilter::IsUsingTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

HRESULT CXBMCSplitterFilter::GetTimeFormat(GUID *pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

HRESULT CXBMCSplitterFilter::GetDuration(LONGLONG *pDuration)
{
    CheckPointer(pDuration, E_POINTER);
    CAutoLock lock(&m_SeekLock);
  *pDuration = m_rtDuration;
    
  /*if (m_pFormatContext)
  {
    int64_t fmtdur = m_pFormatContext->duration;
    dur = (REFERENCE_TIME*)(fmtdur * 10);
  }*/
  return S_OK;
}

HRESULT CXBMCSplitterFilter::GetStopPosition(LONGLONG *pStop)
{
    CheckPointer(pStop, E_POINTER);
    CAutoLock lock(&m_SeekLock);
    *pStop = m_rtStop;
    return S_OK;
}

HRESULT CXBMCSplitterFilter::GetCurrentPosition(LONGLONG *pCurrent)
{
    // GetCurrentPosition is typically supported only in renderers and
    // not in source filters.
    return E_NOTIMPL;
}

HRESULT CXBMCSplitterFilter::GetCapabilities( DWORD * pCapabilities )
{
    CheckPointer(pCapabilities, E_POINTER);
    *pCapabilities = m_dwSeekingCaps;
    return S_OK;
}

HRESULT CXBMCSplitterFilter::CheckCapabilities( DWORD * pCapabilities )
{
    CheckPointer(pCapabilities, E_POINTER);

    // make sure all requested capabilities are in our mask
    return (~m_dwSeekingCaps & *pCapabilities) ? S_FALSE : S_OK;
}

HRESULT CXBMCSplitterFilter::ConvertTimeFormat( LONGLONG * pTarget, const GUID * pTargetFormat,
                           LONGLONG    Source, const GUID * pSourceFormat )
{
    CheckPointer(pTarget, E_POINTER);
    // format guids can be null to indicate current format

    // since we only support TIME_FORMAT_MEDIA_TIME, we don't really
    // offer any conversions.
    if(pTargetFormat == 0 || *pTargetFormat == TIME_FORMAT_MEDIA_TIME)
    {
        if(pSourceFormat == 0 || *pSourceFormat == TIME_FORMAT_MEDIA_TIME)
        {
            *pTarget = Source;
            return S_OK;
        }
    }

    return E_INVALIDARG;
}


HRESULT CXBMCSplitterFilter::SetPositions( LONGLONG * pCurrent,  DWORD CurrentFlags
                      , LONGLONG * pStop,  DWORD StopFlags )
{
    DWORD StopPosBits = StopFlags & AM_SEEKING_PositioningBitsMask;
    DWORD StartPosBits = CurrentFlags & AM_SEEKING_PositioningBitsMask;

    if(StopFlags) {
        CheckPointer(pStop, E_POINTER);

        // accept only relative, incremental, or absolute positioning
        if(StopPosBits != StopFlags) {
            return E_INVALIDARG;
        }
    }

    if(CurrentFlags) {
        CheckPointer(pCurrent, E_POINTER);
        if(StartPosBits != AM_SEEKING_AbsolutePositioning &&
           StartPosBits != AM_SEEKING_RelativePositioning) {
            return E_INVALIDARG;
        }
    }


    // scope for autolock
    {
        CAutoLock lock(&m_SeekLock);

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


HRESULT CXBMCSplitterFilter::GetPositions( LONGLONG * pCurrent, LONGLONG * pStop )
{
    if(pCurrent) {
        *pCurrent = m_rtStart;
    }
    if(pStop) {
        *pStop = m_rtStop;
    }

    return S_OK;;
}


HRESULT CXBMCSplitterFilter::GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest )
{
    if(pEarliest) {
        *pEarliest = 0;
    }
    if(pLatest) {
        CAutoLock lock(&m_SeekLock);
        *pLatest = m_rtDuration;
    }
    return S_OK;
}

HRESULT CXBMCSplitterFilter::SetRate( double dRate)
{
    {
        CAutoLock lock(&m_SeekLock);
        m_dRateSeeking = dRate;
    }
    return ChangeRate();
}

HRESULT CXBMCSplitterFilter::GetRate( double * pdRate)
{
    CheckPointer(pdRate, E_POINTER);
    CAutoLock lock(&m_SeekLock);
    *pdRate = m_dRateSeeking;
    return S_OK;
}

HRESULT CXBMCSplitterFilter::GetPreroll(LONGLONG *pPreroll)
{
    CheckPointer(pPreroll, E_POINTER);
    *pPreroll = 0;
    return S_OK;
}


bool CXBMCSplitterFilter::OpenDemux(CStdString pFile)
{
  CStdString strFile;
  if (CUtil::IsStack(pFile))
    strFile = XFILE::CStackDirectory::GetFirstStackedFile(pFile);
  else
    strFile = pFile;

  int nTime = CTimeUtils::GetTimeMS();
  CDVDInputStream *pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, pFile, "");
  if (!pInputStream)
  {
    CLog::Log(LOGERROR, "%s: Error creating stream for %s", __FUNCTION__, pFile.c_str());
    return false;
  }

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CLog::Log(LOGERROR, "InputStream: dvd streams not supported for thumb extraction, file: %s", strFile.c_str());
    delete pInputStream;
    return false;
  }

  if (!pInputStream->Open(strFile.c_str(), ""))
  {
    CLog::Log(LOGERROR, "InputStream: Error opening, %s", strFile.c_str());
    if (pInputStream)
      delete pInputStream;
    return false;
  }

  if(m_pDemuxer)
    SAFE_DELETE(m_pDemuxer);

  try
  {
    m_pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(pInputStream);
    if(!m_pDemuxer)
    {
      delete pInputStream;
      CLog::Log(LOGERROR, "%s - Error creating demuxer", __FUNCTION__);
      return false;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when opening demuxer", __FUNCTION__);
    if (m_pDemuxer)
      delete m_pDemuxer;
    delete pInputStream;
    return false;
  }
  
  for (int iStream=0; iStream < m_pDemuxer->GetNrOfStreams(); iStream++)
  {
    AddStream(iStream);
  }
}

void CXBMCSplitterFilter::AddStream(int streamindex)
{
  
  //if unknown STREAM_NONE
  //audio STREAM_AUDIO
  //video STREAM_VIDEO
  //data STREAM_DATA
  //subtitle STREAM_SUBTITLE
  //teletext STREAM_TELETEXT
  if (!m_pDemuxer)
    return;

  CDemuxStream* pStream = m_pDemuxer->GetStream(streamindex);
  
  CDSStreamInfo hint(*pStream, true);
  if (pStream->type == STREAM_VIDEO)
  {
    m_pVideoStream.SetStream(hint.mtype);
    m_pCurrentVideoStream = NULL;
    m_pCurrentVideoStream = (void*)pStream;
  }
  else if (pStream->type == STREAM_AUDIO)
  {
    m_pAudioStream.SetStream(hint.mtype);
    m_pCurrentAudioStream = NULL;
    m_pCurrentAudioStream = (void*)pStream;
    //m_pAudioStream.SetStream(strm);
  }


}

HRESULT CXBMCSplitterFilter::OnThreadCreate()
{ 
  //FIXME
  //mStream->setStartTimeAbs(mLastTime);
  CLOG_FUNCTION

    return NO_ERROR;
}

DWORD CXBMCSplitterFilter::ThreadProc()
{
  DWORD cmd = 0;
  SetThreadPriority(m_hThread, m_priority = THREAD_PRIORITY_NORMAL);
  m_rtStart = m_rtNewStart;
    m_rtStop = m_rtNewStop;
  while(!CheckRequest(NULL))//for(DWORD cmd = -1; ; cmd = GetRequest())
  {
    
    /*if(cmd == CMD_EXIT)
    {
      m_hThread = NULL;
      Reply(S_OK);
      return 0;
    }*/
		

    
    

  // if the queues are full, no need to read more
    if (!m_pVideoStream.QueueSize() < (1024*1024*5))
    {
      //Sleep(10);
      //continue;
    }


    DemuxPacket* pPacket = NULL;
    CDemuxStream *pStream = NULL;
    ReadPacket(pPacket, pStream);
    if (pPacket && !pStream)
    {
      /* probably a empty DsPacket, just free it and move on */
      CDVDDemuxUtils::FreeDemuxPacket(pPacket);
      continue;
    }
    // process the DsPacket
    ProcessPacket(pStream, pPacket);
  }
  return 0;
}

void CXBMCSplitterFilter::ProcessPacket(CDemuxStream* pStream, DemuxPacket* pPacket)
{
  //CAutoLock cAutoLock(this);
  
  CAutoLock cAutoLock(&m_cStateLock);
  if (pPacket)
  {
    if (pStream->type == STREAM_VIDEO)
    {
      std::auto_ptr<DsPacket> p(new DsPacket());
      
      //if(pPacket->dts != DVD_NOPTS_VALUE) m_CurrentVideo.dts = pPacket->dts;
      if(pPacket->dts != DVD_NOPTS_VALUE)
        p->rtStart = pPacket->dts * 10;
      else
        p->rtStart = m_rtStart;//this->m_rtCurrent 
      if ( p->rtStart < 0) p->rtStart = 0;
      //p->rtStart = m_rtStart;//(pPacket->pts * 10);
      p->rtStop = p->rtStart + (pPacket->duration * 10);
      p->resize(pPacket->iSize);
      memcpy(&p->at(0),pPacket->pData,pPacket->iSize);
      
      m_pVideoStream.QueuePacket(p);
    }
    else if(pStream->type == STREAM_AUDIO)
    {
      std::auto_ptr<DsPacket> p(new DsPacket());
      
      //if(pPacket->dts != DVD_NOPTS_VALUE) m_CurrentVideo.dts = pPacket->dts;
      if(pPacket->dts != DVD_NOPTS_VALUE)
        p->rtStart = pPacket->dts * 10;
      else
        p->rtStart = m_rtStart;//this->m_rtCurrent 
      if ( p->rtStart < 0) p->rtStart = 0;
      //p->rtStart = m_rtStart;//(pPacket->pts * 10);
      p->rtStop = p->rtStart + (pPacket->duration * 10);
      p->resize(pPacket->iSize);
      memcpy(&p->at(0),pPacket->pData,pPacket->iSize);
      
      m_pAudioStream.QueuePacket(p);
    //TODO
    }
    //pPacket->iStreamId
  }

}

bool CXBMCSplitterFilter::ReadPacket(DemuxPacket*& DsPacket, CDemuxStream*& stream)
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

void CXBMCSplitterFilter::UpdateCurrentPTS()
{
  m_iCurrentPts = DVD_NOPTS_VALUE;
  for(unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
  {
    AVStream *stream = m_pFormatContext->streams[i];
    if(stream && stream->cur_dts != (int64_t)AV_NOPTS_VALUE)
    {
      double ts = ConvertTimestamp(stream->cur_dts, stream->time_base.den, stream->time_base.num);
      if(m_iCurrentPts == DVD_NOPTS_VALUE || m_iCurrentPts > ts )
        m_iCurrentPts = ts;
    }
  }

}

void CXBMCSplitterFilter::Flush()
{
  if (m_pFormatContext)
    m_dllAvFormat.av_read_frame_flush(m_pFormatContext);

  m_iCurrentPts = DVD_NOPTS_VALUE;

}

double CXBMCSplitterFilter::ConvertTimestamp(int64_t pts, int den, int num)
{
  if (pts == (int64_t)AV_NOPTS_VALUE)
    return DVD_NOPTS_VALUE;

  // do calculations in floats as they can easily overflow otherwise
  // we don't care for having a completly exact timestamp anyway
  double timestamp = (double)pts * num  / den;
  double starttime = 0.0f;

  if (m_pFormatContext->start_time != (int64_t)AV_NOPTS_VALUE)
    starttime = (double)m_pFormatContext->start_time / AV_TIME_BASE;

  if(timestamp > starttime)
    timestamp -= starttime;
  else if( timestamp + 0.1f > starttime )
    timestamp = 0;

  return timestamp*DVD_TIME_BASE;
}