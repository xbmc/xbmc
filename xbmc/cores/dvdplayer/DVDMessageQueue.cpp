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

void CDVDMessageQueue::Flush(CDVDMsg::Message type)
{
  EnterCriticalSection(&m_critSection);

  if (m_bInitialized)
  {
    DVDMessageListItem first;
    first.pNext = m_pFirstMessage;

    DVDMessageListItem *pLast = &first;
    DVDMessageListItem *pCurr; 
    while ((pCurr = pLast->pNext))
    {
      if (pCurr->pMsg->IsType(type) ||  type == CDVDMsg::NONE)
      {
        pLast->pNext = pCurr->pNext;
        pCurr->pMsg->Release();
        delete pCurr;
      }
      else
        pLast = pCurr;
    }

    m_pFirstMessage = first.pNext;
    if(pLast == &first)
      m_pLastMessage = NULL;
    else
      m_pLastMessage = pLast;
  }

  if (type == CDVDMsg::DEMUXER_PACKET ||  type == CDVDMsg::NONE)
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


MsgQueueReturnCode CDVDMessageQueue::Put(CDVDMsg* pMsg, int priority)
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
  msgItem->priority = priority;

  EnterCriticalSection(&m_critSection);

  if(!m_pLastMessage || m_pLastMessage && m_pLastMessage->priority >= priority)
  {
    /* quick path to just add at the end */
    if (!m_pFirstMessage) m_pFirstMessage = msgItem;
    else m_pLastMessage->pNext = msgItem;
    m_pLastMessage = msgItem;
  }
  else
  {
    /* add in prio order */
    DVDMessageListItem* pCurr = m_pFirstMessage;
    DVDMessageListItem* pLast = NULL;

    while (pCurr && pCurr->priority >= priority)
    {
      pLast = pCurr;
      pCurr = pCurr->pNext;
    }

    msgItem->pNext = pCurr;
    if (pLast)
      pLast->pNext = msgItem;
    if (msgItem->pNext == NULL)
      m_pLastMessage = msgItem;
    if (msgItem->pNext == m_pFirstMessage)
      m_pFirstMessage = msgItem;
  }

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
  
  DVDMessageListItem* msgItem;
  int ret = 0;

  if (!m_bInitialized)
  {
    CLog::Log(LOGFATAL, "CDVDMessageQueue::Get MSGQ_NOT_INITIALIZED");
    return MSGQ_NOT_INITIALIZED;
  }

  EnterCriticalSection(&m_critSection);

  while (!m_bAbortRequest)
  {
    msgItem = m_pFirstMessage;
    if (msgItem && msgItem->priority >= priority)
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

