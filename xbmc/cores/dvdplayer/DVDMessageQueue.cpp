
#include "stdafx.h"
#include "DVDMessageQueue.h"
#include "DVDDemuxers/DVDDemuxUtils.h"

CDVDMessageQueue::CDVDMessageQueue()
{
  m_pFirstMessage = NULL;
  m_pLastMessage  = NULL;
  m_iDataSize     = 0;
  m_bAbortRequest = false;
  m_bInitialized  = false;
  m_bCaching      = false;
  
  InitializeCriticalSection(&m_critSection);
  m_hEvent = CreateEvent(NULL, true, false, NULL);
}

CDVDMessageQueue::~CDVDMessageQueue()
{
  // remove all remaining messages
  Flush();
  
  DeleteCriticalSection(&m_critSection);
  CloseHandle(m_hEvent);
}

void CDVDMessageQueue::Init()
{
  m_pFirstMessage = NULL;
  m_pLastMessage  = NULL;
  m_iDataSize     = 0;
  m_bAbortRequest = false;
  
  m_bInitialized  = true;
}

void CDVDMessageQueue::Flush()
{
  EnterCriticalSection(&m_critSection);

  if (m_bInitialized)
  {
    DVDMessageListItem *msg = m_pFirstMessage;
    DVDMessageListItem *tmp_msg = NULL;
    while (msg != NULL)
    {
      tmp_msg = msg->pNext;
      
      msg->pMsg->Release();
      delete msg;
      
      msg = tmp_msg;
    }
  }
  
  m_pLastMessage = NULL;
  m_pFirstMessage = NULL;
  m_iDataSize = 0;

  LeaveCriticalSection(&m_critSection);
}

void CDVDMessageQueue::Abort()
{
  EnterCriticalSection(&m_critSection);

  m_bAbortRequest = true;

  SetEvent(m_hEvent); // inform waiter for abort action

  LeaveCriticalSection(&m_critSection);
}

void CDVDMessageQueue::End()
{
  Flush();
  
  EnterCriticalSection(&m_critSection);
  
  m_bInitialized  = false;
  m_pFirstMessage = NULL;
  m_pLastMessage  = NULL;
  m_iDataSize     = 0;
  m_bAbortRequest = false;
  
  LeaveCriticalSection(&m_critSection);
}


MsgQueueReturnCode CDVDMessageQueue::Put(CDVDMsg* pMsg)
{
  if (!m_bInitialized)
  {
    CLog::Log(LOGWARNING, "CDVDMessageQueue::Put MSGQ_NOT_INITIALIZED");
    pMsg->Release();
    return MSGQ_NOT_INITIALIZED;
  }
  if (!pMsg)
  {
    CLog::Log(LOGFATAL, "CDVDMessageQueue::Put MSGQ_INVALID_MSG");
    return MSGQ_INVALID_MSG;
  }
  
  DVDMessageListItem* msgItem = new DVDMessageListItem;

  if (!msgItem)
  {
    CLog::Log(LOGFATAL, "CDVDMessageQueue::Put MSGQ_OUT_OF_MEMORY");
    return MSGQ_OUT_OF_MEMORY;
  }
  
  msgItem->pMsg = pMsg;
  msgItem->pNext = NULL;
  
  EnterCriticalSection(&m_critSection);

  if (!m_pFirstMessage) m_pFirstMessage = msgItem;
  else m_pLastMessage->pNext = msgItem;
  
  m_pLastMessage = msgItem;

  if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
  {
    CDVDMsgDemuxerPacket* pMsgDemuxerPacket = (CDVDMsgDemuxerPacket*)pMsg;
    m_iDataSize += pMsgDemuxerPacket->GetPacketSize();
  }
  
  SetEvent(m_hEvent); // inform waiter for new packet

  LeaveCriticalSection(&m_critSection);
  
  return MSGQ_OK;
}

MsgQueueReturnCode CDVDMessageQueue::Get(CDVDMsg** pMsg, unsigned int iTimeoutInMilliSeconds)
{
  *pMsg = NULL;
  
  DVDMessageListItem* msgItem;
  int ret;

  if (!m_bInitialized)
  {
    CLog::Log(LOGFATAL, "CDVDMessageQueue::Get MSGQ_NOT_INITIALIZED");
    return MSGQ_NOT_INITIALIZED;
  }

  EnterCriticalSection(&m_critSection);

  while (!m_bAbortRequest)
  {
    msgItem = m_pFirstMessage;
    if (msgItem && !m_bCaching)
    {
      m_pFirstMessage = msgItem->pNext;
      
      if (!m_pFirstMessage) m_pLastMessage = NULL;

      if (msgItem->pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
      {
        CDVDMsgDemuxerPacket* pMsgDemuxerPacket = (CDVDMsgDemuxerPacket*)msgItem->pMsg;
        m_iDataSize -= pMsgDemuxerPacket->GetPacketSize();
      }

      *pMsg = msgItem->pMsg;
      
      delete msgItem; // free the list item we allocated in ::Put()
      
      ret = MSGQ_OK;
      break;
    }
    else if (!iTimeoutInMilliSeconds)
    {
      ret = MSGQ_TIMEOUT;
      break;
    }
    else
    {
      ResetEvent(m_hEvent);
      LeaveCriticalSection(&m_critSection);
      
      // wait for a new message
      if (WaitForSingleObject(m_hEvent, iTimeoutInMilliSeconds) == WAIT_TIMEOUT)
      {
        // just return here directly, we have already left critical section
        return MSGQ_TIMEOUT;
      }
      EnterCriticalSection(&m_critSection);
    }
  }
  LeaveCriticalSection(&m_critSection);
  
  if (m_bAbortRequest) return MSGQ_ABORT;
  
  return (MsgQueueReturnCode)ret;
}


unsigned CDVDMessageQueue::GetPacketCount(CDVDMsg::Message type) 
{    
  if (!m_bInitialized)
    return 0;

  EnterCriticalSection(&m_critSection);
  
  unsigned count = 0;
  DVDMessageListItem* msgItem = m_pFirstMessage;
  while(msgItem)
  {
    if( msgItem->pMsg->IsType(type) )
      count++;
    msgItem = msgItem->pNext;
  }
  
  LeaveCriticalSection(&m_critSection);
  return count;
}

