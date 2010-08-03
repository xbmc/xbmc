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
#include "DSDemuxerThread.h"
#include <list>

#include "DSDemuxerFilter.h"
#include "DShowUtil/DShowUtil.h"
#include "CharsetConverter.h"
#pragma warning(disable: 4355) //Remove warning about using "this" in base member initializer listclass
extern "C"
{
  #include "libavformat/avformat.h"
}

#define NO_SUBTITLE_PID USHRT_MAX    // Fake PID use for the "No subtitle" entry

//
// CDSDemuxerThread
//

CDSDemuxerThread::CDSDemuxerThread(CDSDemuxerFilter* pFilter ,LPWSTR pFileName)
  : m_pFilter(pFilter)
  , m_pStreamList(NULL)
  , m_filenameW(pFileName)
  , m_messenger("ds_demuxer")
{
  m_pDemuxer = NULL;
  m_pSubtitleDemuxer = NULL;
  m_pInputStream = NULL;

  m_hSeekProtection = CreateEvent(NULL, FALSE, TRUE, NULL);
  m_filenameA = DShowUtil::WToA(CStdStringW(pFileName));

  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, m_filenameA, "");
  if (!m_pInputStream)
  {
    CLog::Log(LOGERROR, "%s: Error creating stream for %s", __FUNCTION__, m_filenameA.c_str());
    assert(0);
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
      assert(0);
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when opening demuxer", __FUNCTION__);
    if (m_pDemuxer)
      delete m_pDemuxer;
    delete m_pInputStream;
    assert(0);
  }
  m_streamsCount = m_pDemuxer->GetNrOfStreams();
  m_playSpeed = DVD_PLAYSPEED_NORMAL;
}

CDSDemuxerThread::~CDSDemuxerThread()
{
  //BlockProc();
	CallWorker(CMD_STOP);
	Close();
	delete [] m_pStreamList;
	CloseHandle(m_hSeekProtection);
}

HRESULT CDSDemuxerThread::CreateOutputPin()
{
  HRESULT	hr = S_OK;
  CLog::DebugLog("%s",__FUNCTION__);
	delete [] m_pStreamList;
	m_pStreamList = new StreamList[m_streamsCount];
	if (m_pStreamList == NULL)
		return S_FALSE;
  const CDVDDemuxFFmpeg *pDemuxer = static_cast<const CDVDDemuxFFmpeg*>(m_pDemuxer);
  const char *containerFormat = pDemuxer->m_pFormatContext->iformat->name;
	for(int i=0; i < m_streamsCount; i++)
	{
		CDemuxStream* pStream = m_pDemuxer->GetStream(i);
    CDSStreamInfo hint(*pStream, true, containerFormat);
    
		LPOLESTR szPinName;
		CLog::DebugLog("%s Creating pin nbr %d", __FUNCTION__, i);
    
    
    int cch = lstrlenW(hint.PinNameW.c_str()) + 1;
    szPinName = new WCHAR[cch];
    CopyMemory(szPinName, hint.PinNameW.c_str(), cch * sizeof(WCHAR));
		// create a new outpin

		/*m_pStreamList[i].pin = new CDSOutputPin((CSource*)m_pFilter, &hr, szPinName, hint);*/
    m_pStreamList[i].pin = new CDSOutputPin((CSource*)m_pFilter, szPinName, hint, &hr);

		m_pStreamList[i].pin_index = (UINT)i;//pStream->iPhysicalId;
		
		if(FAILED(hr))
		{
			break;
		}
		CLog::DebugLog("%s %d created.", __FUNCTION__, i);
	}

	CLog::DebugLog("%s Done.",__FUNCTION__);

	return hr;
}

void CDSDemuxerThread::SeekTo(REFERENCE_TIME rt)
{
  __int64 seek_msec;
  seek_msec = DS_TIME_TO_MSEC(rt);
  m_messenger.Put(new CDVDMsgPlayerSeek((int)seek_msec, true, true, true));
}

void CDSDemuxerThread::SetPlaySpeed(int speed)
{
  if (speed != DVD_PLAYSPEED_PAUSE)
    CallWorker(CMD_WAKEUP);
  m_messenger.Put(new CDVDMsgInt(CDVDMsg::PLAYER_SETSPEED, speed));
  for (int i = 0;  i < m_streamsCount; i++)
  {
    m_pStreamList[i].pin->SetSpeed(speed);
  }
  //Is this needed for directshow?
  //SynchronizeDemuxer(100);
}

void CDSDemuxerThread::Stop()
{
  //TODO
  //Verify the the player is running before doing this
  

  // tell demuxer to abort
  if(m_pDemuxer)
    m_pDemuxer->Abort();

  if(m_pSubtitleDemuxer)
    m_pSubtitleDemuxer->Abort();

  for (INT i=0; i<m_streamsCount; i++)
  {
		m_pStreamList[i].pin->EndStream();
  }
    
}

bool CDSDemuxerThread::ReadPacket(DemuxPacket*& DsPacket, CDemuxStream*& stream)
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

//CAMThread

BOOL CDSDemuxerThread::Create(void)
{
  
  HRESULT hr = CreateOutputPin();
  
  for (int i=0; i<m_streamsCount; i++)
    m_pStreamList[i].pin->Reset();

  if (!CAMThread::Create())
    return FALSE;
  return SetThreadPriority(m_hThread, THREAD_PRIORITY_ABOVE_NORMAL);
  
}

void CDSDemuxerThread::HandleMessages()
{
  CDVDMsg* pMsg;
  
//TODO ADD
//chapter seeking
//stream switching
//demuxer reset
//maybe rate 
  while (m_messenger.Get(&pMsg, 0) == MSGQ_OK)
  {

    try
    {
      if (pMsg->IsType(CDVDMsg::PLAYER_SEEK) && m_messenger.GetPacketCount(CDVDMsg::PLAYER_SEEK)         == 0)
      {
        CDVDMsgPlayerSeek &msg(*((CDVDMsgPlayerSeek*)pMsg));

        double start = DVD_NOPTS_VALUE;
        int time = msg.GetTime();
        CLog::Log(LOGDEBUG, "demuxer seek to: %d", time);
        if (m_pDemuxer && m_pDemuxer->SeekTime(time, msg.GetBackward(), &start))
        {
          CLog::Log(LOGDEBUG, "demuxer seek to: %d, success", time);
          if(m_pSubtitleDemuxer)
          {
            if(!m_pSubtitleDemuxer->SeekTime(time, msg.GetBackward()))
              CLog::Log(LOGDEBUG, "failed to seek subtitle demuxer: %d, success", time);
          }
          for (int i = 0; i < m_streamsCount; i++)
          {
            m_pStreamList[i].pin->Flush();
          }
          //FlushBuffers(!msg.GetFlush(), start, msg.GetAccurate());
        }
        else
          CLog::Log(LOGWARNING, "error while seeking");
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
      { 
        m_playSpeed = static_cast<CDVDMsgInt*>(pMsg)->m_value;
      }
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s - Exception thrown when handling message", __FUNCTION__);
    }
    pMsg->Release();
  }

}

DWORD CDSDemuxerThread::ThreadProc()
{
  CLog::DebugLog("%s",__FUNCTION__);
	assert(m_pFilter != NULL);
  m_messenger.Init();
  while (1) 
  {
		DWORD aCmd;
		bool bStop = false;
		aCmd = GetRequest();
		switch(aCmd) {
		case CMD_STOP:
			bStop = true;
			Reply(NOERROR);
			break;
		case CMD_SLEEP:
			Reply(NOERROR);
			continue; // wait for the next command
			break;
		case CMD_WAKEUP:
			Reply(NOERROR);
			break;
		default:
			CLog::DebugLog("UNKNOWN command 0x%08x!!!", aCmd);
			Reply(S_FALSE);
		}
		if (bStop)
			break;
    while (1)
    {
 
      HandleMessages();
      if (m_playSpeed != DVD_PLAYSPEED_PAUSE)
      {
        for (int i=0; i<m_streamsCount; i++)
          m_pStreamList[i].pin->SetProcessingFlag();
      }

      //take a look at the message queue is full
      for (int i=0; i<m_streamsCount; i++)
      {
        if (!m_pStreamList[i].pin->AcceptsData() && m_pStreamList[i].pin->IsConnected())
        {
          Sleep(10);
          continue;
        }
      }
      // always yield to players if they have data
      for (int i=0; i<m_streamsCount; i++)
      {
        if (m_pStreamList[i].pin->m_messageQueue.GetDataSize() > 0 || m_pStreamList[i].pin->IsConnected())
        {
          Sleep(0);
          break;//exit for
        }
      }

      DemuxPacket* pPacket = NULL;
      CDemuxStream *pStream = NULL;

      ReadPacket(pPacket, pStream);

      if ((pPacket && !pStream))
      {
        /* probably a empty DsPacket, just free it and move on */
        CDVDDemuxUtils::FreeDemuxPacket(pPacket);
        continue;
      }
      if (!pPacket)
	    {
        CLog::DebugLog("%s no packet from demuxer assuming end of stream",__FUNCTION__);
	      /*end of stream detected*/
        

          if (!m_pInputStream->IsEOF())
            CLog::Log(LOGINFO, "%s - eof reading from demuxer", __FUNCTION__);
          continue;
        
				
			} 
      else 
      {
			  // find the OutputQueue corresponding to the track
				CDSOutputPin* thePin = GetOutputPinIndex(pPacket->iStreamId);

				// as it is blocking some call to stop the thread through ReaderEvent may produce a deadlock
				if(thePin->IsConnected() && thePin->IsProcessing())
			  {
            thePin->SendMessage(new CDVDMsgDemuxerPacket(pPacket));
				}
        if (pPacket->pts != DVD_NOPTS_VALUE)
          m_pFilter->SetCurrentPosition(pPacket->pts * 10);
        else if (pPacket->dts != DVD_NOPTS_VALUE)
          m_pFilter->SetCurrentPosition(pPacket->dts * 10);

			}
			if (CheckRequest(&aCmd))
				break;
		}
	}
  return 0;
}

CDSOutputPin *CDSDemuxerThread::GetOutputPinIndex(UINT index) const
{
	if (m_pStreamList == NULL)
		return NULL;

	for (INT i=0; i<m_streamsCount; i++) 
  {
		if (m_pStreamList[i].pin_index == index)
			return m_pStreamList[i].pin;
	}
	return NULL;
}