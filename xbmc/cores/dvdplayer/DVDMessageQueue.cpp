
#include "../../stdafx.h"
#include "DVDMessageQueue.h"
#include "DVDDemuxers/DVDDemuxUtils.h"

CDVDMessageQueue::CDVDMessageQueue()
{
  m_pFirstMessage = NULL;
  m_pLastMessage  = NULL;
  m_iDataSize     = 0;
  m_bAbortRequest = false;
  m_bInitialized  = false;
  
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
      
      CDVDMessage::FreeMessageData(msg->msg, msg->pData);
      
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


MsgQueueReturnCode CDVDMessageQueue::Put(DVDMsg msg, void* data, unsigned int data_size)
{
  if (!m_bInitialized)
  {
    CLog::Log(LOGFATAL, "CDVDMessageQueue::Put MSGQ_NOT_INITIALIZED");
    return MSGQ_NOT_INITIALIZED;
  }
  if (!msg)
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
  
  msgItem->msg = msg;
  msgItem->pData = data;
  msgItem->dataSize = data_size;
  msgItem->pNext = NULL;
  
  EnterCriticalSection(&m_critSection);

  if (!m_pFirstMessage) m_pFirstMessage = msgItem;
  else m_pLastMessage->pNext = msgItem;
  
  m_pLastMessage = msgItem;

  m_iDataSize += data_size;

  SetEvent(m_hEvent); // inform waiter for new packet

  LeaveCriticalSection(&m_critSection);
  
  return MSGQ_OK;
}

MsgQueueReturnCode CDVDMessageQueue::Get(DVDMsg* msg, unsigned int iTimeoutInMilliSeconds, DVDMsgData* pMsgData, unsigned int* data_size)
{
  *msg = 0;
  if (pMsgData) *pMsgData = NULL;
  if (data_size) *data_size = 0;
  
  DVDMessageListItem* msgItem;
  int ret;

  if (!m_bInitialized)
  {
    CLog::Log(LOGFATAL, "CDVDMessageQueue::Put MSGQ_NOT_INITIALIZED");
    return MSGQ_NOT_INITIALIZED;
  }

  EnterCriticalSection(&m_critSection);

  while (!m_bAbortRequest)
  {
    msgItem = m_pFirstMessage;
    if (msgItem)
    {
      m_pFirstMessage = msgItem->pNext;
      
      if (!m_pFirstMessage) m_pLastMessage = NULL;

      m_iDataSize -= msgItem->dataSize;

      *msg = msgItem->msg;
      if (pMsgData)   *pMsgData = msgItem->pData;
      if (data_size)  *data_size = msgItem->dataSize;
      
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
      if (WaitForSingleObjectEx(m_hEvent, iTimeoutInMilliSeconds, false) == WAIT_TIMEOUT)
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
