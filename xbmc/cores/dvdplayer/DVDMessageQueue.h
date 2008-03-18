
#pragma once

#include "DVDMessage.h"

typedef struct stDVDMessageListItem
{
  CDVDMsg* pMsg;
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
  MSGQ_OUT_OF_MEMORY    = -4
};

#define MSGQ_IS_ERROR(c)    (c < 0)

class CDVDMessageQueue
{
public:
  CDVDMessageQueue();
  virtual ~CDVDMessageQueue();
  
  void  Init();
  void  Flush();
  void  Abort();
  void  End();

  MsgQueueReturnCode Put(CDVDMsg* pMsg);
 
  /**
   * msg,       message type from DVDMessage.h
   * timeout,   timeout in msec
   */
  MsgQueueReturnCode Get(CDVDMsg** pMsg, unsigned int iTimeoutInMilliSeconds);

  
  int GetDataSize() const               { return m_iDataSize; }
  unsigned GetPacketCount(CDVDMsg::Message type);
  bool RecievedAbortRequest()           { return m_bAbortRequest; }
  void WaitUntilEmpty()                 { while (m_pFirstMessage && !m_bAbortRequest) Sleep(1); }
  
  // non messagequeue related functions
  bool IsFull() const                   { return (m_iDataSize >= m_iMaxDataSize); }
  void SetMaxDataSize(int iMaxDataSize) { m_iMaxDataSize = iMaxDataSize; }
  int GetMaxDataSize() const            { return m_iMaxDataSize; }
  bool IsInited() const                 { return m_bInitialized; }
private:

  HANDLE m_hEvent;
  mutable CRITICAL_SECTION m_critSection;
  
  DVDMessageListItem* m_pFirstMessage;
  DVDMessageListItem* m_pLastMessage;
  
  bool m_bAbortRequest;
  bool m_bInitialized;
  bool m_bCaching;

  int m_iDataSize;
  int m_iMaxDataSize;
};

