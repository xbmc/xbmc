
#pragma once

//#include "..\DVDDemuxers\DVDDemux.h"
#include "DVDMessage.h"

typedef struct stDVDMessageListItem
{
  DVDMsg msg;
  DVDMsgData pData;
  unsigned int dataSize;
  struct stDVDMessageListItem *pNext;
}
DVDMessageListItem;

enum MsgQueueReturnCode
{
  MSGQ_OK               = 1,
  MSGQ_TIMEOUT          = 0,
  MSGQ_ABORT            = -1, // negative for legacy, not an error actually
  MSGQ_NOT_INITIALIZED  = -2,
  MSGQ_INVALID_MSG      = -3,
  MSGQ_OUT_OF_MEMORY    = -4,
};

#define MSGQ_IS_ERROR(c)    (c < 0)

class CDVDMessageQueue
{
public:
  CDVDMessageQueue();
  ~CDVDMessageQueue();
  
  void  Init();
  void  Flush();
  void  Abort();
  void  End();

  MsgQueueReturnCode  Put(DVDMsg msg, void* data, unsigned int data_size);
  MsgQueueReturnCode  Put(DVDMsg msg, void* data)             { return Put(msg, data, 0); }
  MsgQueueReturnCode  Put(DVDMsg msg)                         { return Put(msg, NULL, 0); }
 
  /**
   * msg,       message type from DVDMessae.h
   * timeout,   timeout in msec
   * data,      pointer that recieves any additional data
   * data_size, pointer that recieves any additional data size
   */
  MsgQueueReturnCode  Get(DVDMsg* msg, unsigned int iTimeoutInMilliSeconds, DVDMsgData* pMsgData, unsigned int* data_size);
  
  MsgQueueReturnCode  Get(DVDMsg* msg, unsigned int iTimeoutInMilliSeconds, DVDMsgData* pMsgData)
  {
    return Get(msg, iTimeoutInMilliSeconds, pMsgData, NULL);
  }
  
  MsgQueueReturnCode  Get(DVDMsg* msg, unsigned int iTimeoutInMilliSeconds)
  {
    DVDMsgData msg_data;
    
    MsgQueueReturnCode ret = Get(msg, iTimeoutInMilliSeconds, &msg_data, NULL);
    
    // safety, this function shouldn't be called at all when data can be expected.
    // But it can come in handy
    if (msg_data)
    {
      CDVDMessage::FreeMessageData(*msg, msg_data);
    }
    
    return ret;
  }
  
  int GetDataSize()                     { return m_iDataSize; }
  bool RecievedAbortRequest()           { return m_bAbortRequest; }
  void WaitUntilEmpty()                 { while (m_pFirstMessage) Sleep(1); }
  
  // non messagequeue related functions
  bool IsFull()                         { return (m_iDataSize >= m_iMaxDataSize); }
  void SetMaxDataSize(int iMaxDataSize) { m_iMaxDataSize = iMaxDataSize; }
  int GetMaxDataSize()                  { return m_iMaxDataSize; }
  
private:

  HANDLE m_hEvent;
  CRITICAL_SECTION m_critSection;
  
  DVDMessageListItem* m_pFirstMessage;
  DVDMessageListItem* m_pLastMessage;
  
  bool m_bAbortRequest;
  bool m_bInitialized;
  
  int m_iDataSize;
  int m_iMaxDataSize;
};
