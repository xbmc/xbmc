/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "threads/SystemClock.h"
#include "DVDMessage.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDStreamInfo.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "threads/CriticalSection.h"
#include "threads/Condition.h"
#include "threads/SystemClock.h"
#include "utils/MathUtils.h"

class CDVDMsgGeneralSynchronizePriv
{
public:
  CDVDMsgGeneralSynchronizePriv(unsigned int timeout, unsigned int sources)
    : sources(sources ? sources : SYNCSOURCE_ALL)
    , reached(0)
    , timeout(timeout)
  {}
  unsigned int                   sources;
  unsigned int                   reached;
  CCriticalSection               section;
  XbmcThreads::ConditionVariable condition;
  XbmcThreads::EndTime           timeout;
};

/**
 * CDVDMsgGeneralSynchronize --- GENERAL_SYNCRONIZR
 */
CDVDMsgGeneralSynchronize::CDVDMsgGeneralSynchronize(unsigned int timeout, unsigned int sources) : CDVDMsg(GENERAL_SYNCHRONIZE)
  , m_p(new CDVDMsgGeneralSynchronizePriv(timeout, sources))
{
}

CDVDMsgGeneralSynchronize::~CDVDMsgGeneralSynchronize()
{
  delete m_p;
}

bool CDVDMsgGeneralSynchronize::Wait(unsigned long milliseconds, unsigned int source)
{
  if(source == 0)
    source = SYNCSOURCE_OWNER;

  /* if we are not requested to wait on this object just return, reference count will be decremented */
  if (!(m_p->sources & source))
    return true;

  CSingleLock lock(m_p->section);

  XbmcThreads::EndTime timeout(milliseconds);

  m_p->reached |= source & m_p->sources;

  while( (long)MathUtils::bitcount(m_p->reached) < GetNrOfReferences() )
  {
    milliseconds = std::min(m_p->timeout.MillisLeft(), timeout.MillisLeft());
    if(m_p->condition.wait(lock, milliseconds))
      continue;
    if(m_p->timeout.IsTimePast())
      return true;  /* global timeout, we are done */
    if(timeout.IsTimePast())
      return false; /* request timeout, should be retried */
  }
  return true;
}

void CDVDMsgGeneralSynchronize::Wait(volatile bool *abort, unsigned int source)
{
  while(!Wait(100, source))
  {
    if(abort && *abort)
      return;
  }
}

long CDVDMsgGeneralSynchronize::Release()
{
  CSingleLock lock(m_p->section);
  long count = --m_refs;
  m_p->condition.notifyAll();
  lock.Leave();
  if (count == 0)
    delete this;
  return count;
}

/**
 * CDVDMsgDemuxerPacket --- DEMUXER_PACKET
 */
CDVDMsgDemuxerPacket::CDVDMsgDemuxerPacket(DemuxPacket* packet, bool drop) : CDVDMsg(DEMUXER_PACKET)
{
  m_packet = packet;
  m_drop   = drop;
}

CDVDMsgDemuxerPacket::~CDVDMsgDemuxerPacket()
{
  if (m_packet)
    CDVDDemuxUtils::FreeDemuxPacket(m_packet);
}
