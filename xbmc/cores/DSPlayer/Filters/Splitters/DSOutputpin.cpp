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



#include "DSOutputpin.h"

#include "moreuuids.h"
#include "streams.h"
#include "dvdmedia.h"
#include "DShowUtil/DShowUtil.h"
#include "DVDPlayer/DVDClock.h"
#include "DSDemuxerFilter.h"
#pragma warning(disable: 4355)

//
// CDSOutputPin
//

CDSOutputPin::CDSOutputPin(CSource *pFilter, LPCWSTR pName, CDSStreamInfo& hints, HRESULT* phr)
 : CSourceStream(NAME("CDSOutputPin"), phr, pFilter, pName)
 , m_hints(hints)
 , m_messageQueue(DShowUtil::WToA(pName).c_str())
 , m_rtStart(0)
 , mPrevStartTime(0)
 , m_bDiscontinuity(false)
 , m_bIsProcessing(false) 
{
  /*for (std::vector<CMediaType>::iterator it = mts.begin(); it != mts.end(); it++)
  {
    m_mts.push_back(*it);
  }*/
  m_messageQueue.SetMaxDataSize(40 * 1024 * 1024);
  m_messageQueue.SetMaxTimeSize(8.0);
  m_messageQueue.Init();
  m_speed = DVD_PLAYSPEED_NORMAL;

  m_mt = hints.mtype;
  
	m_mts.push_back(m_mt);
}

CDSOutputPin::~CDSOutputPin()
{
  m_mts.clear();
}

HRESULT CDSOutputPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pAllocProps)
{
  CheckPointer(pAlloc,E_POINTER);
  CheckPointer(pAllocProps, E_POINTER);

  CAutoLock cAutoLock(m_pFilter->pStateLock());

  ALLOCATOR_PROPERTIES	Actual;
  HRESULT hr = NOERROR;

  pAllocProps->cBuffers = 8;
  
// compute buffer size
	switch(m_hints.type)
	{
	case STREAM_VIDEO:
		if (pAllocProps->cbBuffer < m_hints.height * 4 * m_hints.width * 4)
		{
			pAllocProps->cbBuffer = m_hints.height * 4 * m_hints.width * 4;
		}
		break;
	case STREAM_AUDIO:
	case STREAM_SUBTITLE:
		///< \todo check buffer size
		pAllocProps->cbBuffer = 0xFFFF;
		break;
	}
	
	pAllocProps->cbAlign = 4;

  if(m_mt.subtype == MEDIASUBTYPE_Vorbis && m_mt.formattype == FORMAT_VorbisFormat)
  {
    // oh great, the oggds vorbis decoder assumes there will be two at least, stupid thing...
    pAllocProps->cBuffers = std::max(pAllocProps->cBuffers, (long)2);
  }

  if(FAILED(hr = pAlloc->SetProperties(pAllocProps, &Actual))) 
    return hr;

  if(Actual.cbBuffer < pAllocProps->cbBuffer) return E_FAIL;
  ASSERT(Actual.cBuffers == pAllocProps->cBuffers);

    return NOERROR;
}

HRESULT CDSOutputPin::CheckMediaType(const CMediaType* pmt)
{
  for(unsigned int i = 0; i < m_mts.size(); i++)
  {
    if(*pmt == m_mts[i])
      return S_OK;
  }  
  

  return E_INVALIDARG;
}

HRESULT CDSOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
  CAutoLock cAutoLock(m_pFilter->pStateLock());

  if(iPosition < 0) 
    return E_INVALIDARG;

  if(iPosition >= m_mts.size()) 
    return VFW_S_NO_MORE_ITEMS;

  *pmt = m_mts[iPosition];

  return S_OK;
}

HRESULT CDSOutputPin::OnThreadStartPlay(void)
{
  m_bDiscontinuity = true;
  if (m_hints.type == STREAM_AUDIO)
  {
    SetThreadPriority(m_hThread, THREAD_PRIORITY_NORMAL + 1);
  }
    
	return NOERROR;
}

HRESULT CDSOutputPin::OnThreadDestroy(void)
{
  m_bIsProcessing = false;
	return NOERROR;
}

HRESULT CDSOutputPin::DeliverBeginFlush()
{
  m_bIsProcessing = false;
  Flush();
  HRESULT result = CBaseOutputPin::DeliverBeginFlush();
  CLog::Log(LOGINFO,"%s",__FUNCTION__);
  return result;
}

HRESULT CDSOutputPin::DeliverEndFlush()
{
  m_messageQueue.End();
  m_messageQueue.Init();
  HRESULT result = CBaseOutputPin::DeliverEndFlush();
  CLog::Log(LOGINFO,"%s",__FUNCTION__);
	return result;
}

void CDSOutputPin::EndStream()
{
  CLog::Log(LOGINFO,"%s",__FUNCTION__);
  if (m_speed > 0) 
    m_messageQueue.WaitUntilEmpty();
	
	m_messageQueue.Abort();
}

void CDSOutputPin::Reset()
{
  CLog::Log(LOGINFO,"%s",__FUNCTION__);
	m_messageQueue.Init();
}

void CDSOutputPin::SetSpeed(int speed)
{
  if(m_messageQueue.IsInited())
    m_messageQueue.Put( new CDVDMsgInt(CDVDMsg::PLAYER_SETSPEED, speed), 1 );
  else
    m_speed = speed;
}


void CDSOutputPin::Flush()
{
  /* flush using message as this get's called from dvdplayer thread */
  /* and any demux packet that has been taken out of queue need to */
  /* be disposed of before we flush */
  m_messageQueue.Flush();
  m_messageQueue.Put(new CDVDMsg(CDVDMsg::GENERAL_FLUSH), 1);
}

HRESULT CDSOutputPin::DoBufferProcessingLoop(void)
{
  HRESULT hr = __super::DoBufferProcessingLoop();
  // We will get here earlier when a stream switcher block the stream
	// We must stop to fill the queue
	// the pin thread is still active, it's just waiting for a new command
  m_bIsProcessing = false;
  CLog::Log(LOGINFO,"%s",__FUNCTION__);
  if (hr == S_OK)
  {
    Flush();
  }
  return hr;
}

/* this function is called from CSourceStream and its the main loop for develiring samples*/
HRESULT CDSOutputPin::FillBuffer(IMediaSample *pSample)
{
  //CLog::Log(LOGINFO,"%s",__FUNCTION__);
  CheckPointer(pSample,E_POINTER);
	BYTE *pData;
	long cbData;
	REFERENCE_TIME	tBlockDuration, tSmpStart, tSmpEnd;	
	LONGLONG aDuration;
  // Access the sample's data buffer
  

  CDVDMsg* pMsg;
  int iPriority = (m_speed == DVD_PLAYSPEED_PAUSE && IsConnected()) ? 1 : 0;
  
  int iQueueTimeOut = 1000;
  if (m_hints.type == STREAM_VIDEO)
  {
    iQueueTimeOut = (int)((m_hints.avgtimeperframe) / 1000);
  }
  else if (m_hints.type == STREAM_AUDIO)
  {
    //temporary timeout 1000 but still need to fix this
  }
  
  MsgQueueReturnCode ret = m_messageQueue.Get(&pMsg, iQueueTimeOut, iPriority);
  do
  {
    if (MSGQ_IS_ERROR(ret) || ret == MSGQ_ABORT)
    {
      CLog::Log(LOGERROR, "Got MSGQ_ABORT or MSGO_IS_ERROR return true");
      assert(0);
    }
    else if (ret == MSGQ_TIMEOUT)
    {
      // if we only wanted priority messages, this isn't a stall
      if( iPriority )
        continue;
      CLog::Log(LOGERROR, "%s Got MSGQ_TIMEOUT",__FUNCTION__);
      break;
        // if we only wanted priority messages, this isn't a stall
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH))
    {
      m_packets.clear();
    
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      m_speed = static_cast<CDVDMsgInt*>(pMsg)->m_value;
      //Duplicate called from demuxerthread
      //if (m_speed != DVD_PLAYSPEED_PAUSE)
      // SetProcessingFlag();
    }
    else if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      DemuxPacket* pPacket = ((CDVDMsgDemuxerPacket*)pMsg)->GetPacket();
      bool bPacketDrop     = ((CDVDMsgDemuxerPacket*)pMsg)->GetPacketDrop();
      
      //CDVDMsgDemuxerPacket* msg = (CDVDMsgDemuxerPacket*)m_packets.front().message->Acquire();
      //m_packets.pop_front();
      tBlockDuration = pPacket->duration * 10; //Sample duration
      tSmpStart = mPrevStartTime;
      BOOL syncpoint = FALSE;
    // Check for PTS first
      if (pPacket->pts != DVD_NOPTS_VALUE) 
        tSmpStart = (pPacket->pts * 10);
      else if (pPacket->dts != DVD_NOPTS_VALUE) // if thats not set, use DTS
        tSmpStart = (pPacket->dts * 10);
      else
        syncpoint = TRUE;
      tSmpEnd = tSmpStart + ((pPacket->duration > 0) ? (pPacket->duration * 10) : 1);
      
      tSmpStart -= m_rtStart;
	    tSmpEnd -= m_rtStart;
      cbData = pSample->GetSize();
      if (cbData != pPacket->iSize)
        pSample->SetActualDataLength(pPacket->iSize);
      pSample->GetPointer(&pData);

      pSample->SetTime(&tSmpStart, &tSmpEnd);
	    pSample->SetMediaTime(NULL,NULL);
      
      memcpy(pData, pPacket->pData, pPacket->iSize);
      
      pSample->SetSyncPoint(syncpoint);
      mPrevStartTime = tSmpEnd;
      if(m_bDiscontinuity)
	    {
        pSample->SetDiscontinuity(m_bDiscontinuity);
		    m_bDiscontinuity = false;
      }
      BOOL ispreroll = (tSmpStart < 0) ? 1 : 0;
      pSample->SetPreroll(ispreroll);
    }
    pMsg->Release();
    break;
  }
  while (1);
  

  
  return S_OK;
}
HRESULT CDSOutputPin::DeliverEndOfStream(void)
{
  return CSourceStream::DeliverEndOfStream();
}

STDMETHODIMP CDSOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
  CAutoLock cAutoLock(m_pFilter->pStateLock());
  CheckPointer(ppv, E_POINTER);
  if(riid == IID_IMediaSeeking)
    return m_pFilter->NonDelegatingQueryInterface(riid,ppv);

  return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
}  
