/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDMessageQueue.h"
#include "DVDDemuxers/DVDDemuxPacket.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "DVDClock.h"
#include "math.h"

CDVDMessageQueue::CDVDMessageQueue(const std::string &owner) : m_hEvent(true), m_owner(owner)
{
  m_iDataSize     = 0;
  m_bAbortRequest = false;
  m_bInitialized = false;

  m_TimeBack = DVD_NOPTS_VALUE;
  m_TimeFront = DVD_NOPTS_VALUE;
  m_TimeSize = 1.0 / 4.0; /* 4 seconds */
  m_iMaxDataSize = 0;
}

CDVDMessageQueue::~CDVDMessageQueue()
{
  // remove all remaining messages
  Flush(CDVDMsg::NONE);
}

void CDVDMessageQueue::Init()
{
  m_iDataSize = 0;
  m_bAbortRequest = false;
  m_bInitialized = true;
  m_TimeBack = DVD_NOPTS_VALUE;
  m_TimeFront = DVD_NOPTS_VALUE;
}

void CDVDMessageQueue::Flush(CDVDMsg::Message type)
{
  CSingleLock lock(m_section);

  m_messages.remove_if([type](const DVDMessageListItem &item){
    return type == CDVDMsg::NONE || item.message->IsType(type);
  });

  m_prioMessages.remove_if([type](const DVDMessageListItem &item){
    return type == CDVDMsg::NONE || item.message->IsType(type);
  });

  if (type == CDVDMsg::DEMUXER_PACKET ||  type == CDVDMsg::NONE)
  {
    m_iDataSize = 0;
    m_TimeBack = DVD_NOPTS_VALUE;
    m_TimeFront = DVD_NOPTS_VALUE;
  }
}

void CDVDMessageQueue::Abort()
{
  CSingleLock lock(m_section);

  m_bAbortRequest = true;

  // inform waiter for abort action
  m_hEvent.Set();
}

void CDVDMessageQueue::End()
{
  CSingleLock lock(m_section);

  Flush(CDVDMsg::NONE);

  m_bInitialized = false;
  m_iDataSize = 0;
  m_bAbortRequest = false;
}

MsgQueueReturnCode CDVDMessageQueue::Put(CDVDMsg* pMsg, int priority, bool front)
{
  CSingleLock lock(m_section);

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

  if (priority > 0)
  {
    int prio = priority;
    if (!front)
      prio++;

    auto it = std::find_if(m_prioMessages.begin(), m_prioMessages.end(),
                           [prio](const DVDMessageListItem &item){
                             return prio <= item.priority;
                           });
    m_prioMessages.emplace(it, pMsg, priority);
  }
  else
  {
    if (front)
      m_messages.emplace_front(pMsg, priority);
    else
      m_messages.emplace_back(pMsg, priority);
  }

  if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET) && priority == 0)
  {
    DemuxPacket* packet = ((CDVDMsgDemuxerPacket*)pMsg)->GetPacket();
    if (packet)
    {
      m_iDataSize += packet->iSize;
      if (packet->dts != DVD_NOPTS_VALUE)
        m_TimeFront = packet->dts;
      else if (packet->pts != DVD_NOPTS_VALUE)
        m_TimeFront = packet->pts;

      if (m_TimeBack == DVD_NOPTS_VALUE)
        m_TimeBack = m_TimeFront;
    }
  }

  pMsg->Release();

  // inform waiter for new packet
  m_hEvent.Set();

  return MSGQ_OK;
}

MsgQueueReturnCode CDVDMessageQueue::Get(CDVDMsg** pMsg, unsigned int iTimeoutInMilliSeconds, int &priority)
{
  CSingleLock lock(m_section);

  *pMsg = NULL;

  int ret = 0;

  if (!m_bInitialized)
  {
    CLog::Log(LOGFATAL, "CDVDMessageQueue(%s)::Get MSGQ_NOT_INITIALIZED", m_owner.c_str());
    return MSGQ_NOT_INITIALIZED;
  }

  while (!m_bAbortRequest)
  {
    std::list<DVDMessageListItem> &msgs = (priority > 0 || !m_prioMessages.empty()) ? m_prioMessages : m_messages;

    if (!msgs.empty() && msgs.back().priority >= priority)
    {
      DVDMessageListItem& item(msgs.back());
      priority = item.priority;

      if (item.message->IsType(CDVDMsg::DEMUXER_PACKET) && item.priority == 0)
      {
        DemuxPacket* packet = ((CDVDMsgDemuxerPacket*)item.message)->GetPacket();
        if (packet)
        {
          m_iDataSize -= packet->iSize;
          if (packet->dts != DVD_NOPTS_VALUE)
            m_TimeBack = packet->dts;
          else if (packet->pts != DVD_NOPTS_VALUE)
            m_TimeBack = packet->pts;
        }
      }

      *pMsg = item.message->Acquire();
      msgs.pop_back();

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
      m_hEvent.Reset();
      lock.Leave();

      // wait for a new message
      if (!m_hEvent.WaitMSec(iTimeoutInMilliSeconds))
        return MSGQ_TIMEOUT;

      lock.Enter();
    }
  }

  if (m_bAbortRequest)
    return MSGQ_ABORT;

  return (MsgQueueReturnCode)ret;
}

unsigned CDVDMessageQueue::GetPacketCount(CDVDMsg::Message type)
{
  CSingleLock lock(m_section);

  if (!m_bInitialized)
    return 0;

  unsigned count = 0;
  for (const auto &item : m_messages)
  {
    if(item.message->IsType(type))
      count++;
  }
  for (const auto &item : m_prioMessages)
  {
    if(item.message->IsType(type))
      count++;
  }

  return count;
}

void CDVDMessageQueue::WaitUntilEmpty()
{
  CLog::Log(LOGNOTICE, "CDVDMessageQueue(%s)::WaitUntilEmpty", m_owner.c_str());
  CDVDMsgGeneralSynchronize* msg = new CDVDMsgGeneralSynchronize(40000, 0);
  Put(msg->Acquire());
  msg->Wait(m_bAbortRequest, 0);
  msg->Release();
}

int CDVDMessageQueue::GetLevel() const
{
  CSingleLock lock(m_section);

  if (m_iDataSize > m_iMaxDataSize)
    return 100;
  if (m_iDataSize == 0)
    return 0;

  if (IsDataBased())
    return std::min(100, 100 * m_iDataSize / m_iMaxDataSize);

  int level = std::min(100.0, ceil(100.0 * m_TimeSize * (m_TimeFront - m_TimeBack) / DVD_TIME_BASE ));

  // if we added lots of packets with NOPTS, make sure that the queue is not signalled empty
  if (level == 0 && m_iDataSize != 0)
  {
    CLog::Log(LOGDEBUG, "CDVDMessageQueue::GetLevel() - can't determine level");
    return 1;
  }

  return level;
}

int CDVDMessageQueue::GetTimeSize() const
{
  CSingleLock lock(m_section);

  if (IsDataBased())
    return 0;
  else
    return (int)((m_TimeFront - m_TimeBack) / DVD_TIME_BASE);
}

bool CDVDMessageQueue::IsDataBased() const
{
  return (m_TimeBack == DVD_NOPTS_VALUE  ||
          m_TimeFront == DVD_NOPTS_VALUE ||
          m_TimeFront <= m_TimeBack);
}
