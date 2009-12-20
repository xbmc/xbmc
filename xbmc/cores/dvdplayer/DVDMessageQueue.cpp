/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "DVDMessageQueue.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "utils/log.h"

using namespace std;

CDVDMessageQueue::CDVDMessageQueue(const string &owner)
{
  m_owner = owner;
  m_iDataSize     = 0;
  m_bAbortRequest = false;
  m_bInitialized  = false;
  m_bCaching      = false;
  m_bEmptied      = true;

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
  m_iDataSize     = 0;
  m_bAbortRequest = false;
  m_bEmptied      = true;
  m_bInitialized  = true;
}

void CDVDMessageQueue::Flush(CDVDMsg::Message type)
{
  EnterCriticalSection(&m_critSection);

  for(SList::iterator it = m_list.begin(); it != m_list.end();)
  {
    if (it->pMsg->IsType(type) ||  type == CDVDMsg::NONE)
    {
      it->pMsg->Release();
      it = m_list.erase(it);
    }
    else
      it++;
  }

  if (type == CDVDMsg::DEMUXER_PACKET ||  type == CDVDMsg::NONE)
  {
    m_iDataSize = 0;
    m_bEmptied = true;
  }

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
  m_iDataSize     = 0;
  m_bAbortRequest = false;

  LeaveCriticalSection(&m_critSection);
}


MsgQueueReturnCode CDVDMessageQueue::Put(CDVDMsg* pMsg, int priority)
{
  if (!m_bInitialized)
  {
    CLog::Log(LOGWARNING, "CDVDMessageQueue(%s)::Put MSGQ_NOT_INITIALIZED", m_owner.c_str());
    pMsg->Release();
    return MSGQ_NOT_INITIALIZED;
  }
  if (!pMsg)
  {
    CLog::Log(LOGFATAL, "CDVDMessageQueue(%s)::Put MSGQ_INVALID_MSG", m_owner.c_str());
    return MSGQ_INVALID_MSG;
  }

  DVDMessageListItem item;

  item.pMsg = pMsg;
  item.priority = priority;

  EnterCriticalSection(&m_critSection);


  SList::iterator it = m_list.begin();
  while(it != m_list.end())
  {
    if(priority <= it->priority)
      break;
    it++;
  }
  m_list.insert(it, item);


  if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
  {
    CDVDMsgDemuxerPacket* pMsgDemuxerPacket = (CDVDMsgDemuxerPacket*)pMsg;
    m_iDataSize += pMsgDemuxerPacket->GetPacketSize();
  }

  SetEvent(m_hEvent); // inform waiter for new packet

  LeaveCriticalSection(&m_critSection);

  return MSGQ_OK;
}

MsgQueueReturnCode CDVDMessageQueue::Get(CDVDMsg** pMsg, unsigned int iTimeoutInMilliSeconds, int priority)
{
  *pMsg = NULL;

  int ret = 0;

  if (!m_bInitialized)
  {
    CLog::Log(LOGFATAL, "CDVDMessageQueue(%s)::Get MSGQ_NOT_INITIALIZED", m_owner.c_str());
    return MSGQ_NOT_INITIALIZED;
  }

  EnterCriticalSection(&m_critSection);

  while (!m_bAbortRequest)
  {
    if(m_list.size() && m_list.back().priority >= priority && !m_bCaching)
    {
      DVDMessageListItem item(m_list.back());

      if (item.pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
      {
        CDVDMsgDemuxerPacket* pMsgDemuxerPacket = (CDVDMsgDemuxerPacket*)item.pMsg;
        m_iDataSize -= pMsgDemuxerPacket->GetPacketSize();
        if(m_iDataSize == 0)
        {
          if(!m_bEmptied && m_owner != "teletext") // Prevent log flooding
            CLog::Log(LOGWARNING, "CDVDMessageQueue(%s)::Get - retrieved last data packet of queue", m_owner.c_str());
          m_bEmptied = true;
        }
        else
          m_bEmptied = false;
      }

      *pMsg = item.pMsg;
      m_list.pop_back();

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
  for(SList::iterator it = m_list.begin(); it != m_list.end();it++)
  {
    if(it->pMsg->IsType(type))
      count++;
  }

  LeaveCriticalSection(&m_critSection);
  return count;
}

void CDVDMessageQueue::WaitUntilEmpty()
{
    CLog::Log(LOGNOTICE, "CDVDMessageQueue(%s)::WaitUntilEmpty", m_owner.c_str());
    CDVDMsgGeneralSynchronize* msg = new CDVDMsgGeneralSynchronize(40000, 0);
    Put(msg->Acquire());
    msg->Wait(&m_bAbortRequest, 0);
    msg->Release();
}
