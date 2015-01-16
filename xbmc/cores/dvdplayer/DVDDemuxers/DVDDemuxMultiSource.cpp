/*
 *      Copyright (C) 2005-2015 Team XBMC
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

#include "DVDClock.h"
#include "DVDDemuxMultiSource.h"
#include "DVDDemuxUtils.h"
#include "DVDFactoryDemuxer.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "utils/log.h"


CDVDDemuxMultiSource::CDVDDemuxMultiSource()
{
}

CDVDDemuxMultiSource::~CDVDDemuxMultiSource()
{
  Dispose();
}

void CDVDDemuxMultiSource::Abort()
{
  for (auto iter : m_pDemuxers)
    iter->Abort();
}

void CDVDDemuxMultiSource::Dispose()
{
  while (!m_demuxerQueue.empty())
  {
    m_demuxerQueue.pop();
  }

  m_streamIdToDemuxerMap.clear();
  m_DemuxerToInputStreamMap.clear();
  m_pDemuxers.clear();
  m_currentDemuxer = NULL;
  m_pInput = NULL;

}

void CDVDDemuxMultiSource::Flush()
{
  for (auto iter : m_pDemuxers)
    iter->Flush();
}

int CDVDDemuxMultiSource::GetNrOfStreams()
{
  int streamsCount = 0;
  for (auto iter : m_pDemuxers)
    streamsCount += iter->GetNrOfStreams();

  return streamsCount;
}

CDemuxStream* CDVDDemuxMultiSource::GetStream(int iStreamId)
{
  auto iter = m_streamIdToDemuxerMap.find(iStreamId);
  if (iter != m_streamIdToDemuxerMap.end())
  {
    return iter->second->GetStream(iStreamId);
  }
  else
    return NULL;
}

void CDVDDemuxMultiSource::GetStreamCodecName(int iStreamId, std::string &strName)
{
  auto iter = m_streamIdToDemuxerMap.find(iStreamId);
  if (iter != m_streamIdToDemuxerMap.end())
  {
    iter->second->GetStreamCodecName(iStreamId, strName);
  }
};

int CDVDDemuxMultiSource::GetStreamLength()
{
  int length = 0;
  for (auto iter : m_pDemuxers)
  {
    length = std::max(length, iter->GetStreamLength());
  }

  return length;
}

bool CDVDDemuxMultiSource::Open(CDVDInputStream* pInput)
{
  if (!pInput || !pInput->IsStreamType(DVDSTREAM_TYPE_MULTIFILES))
    return false;

  m_pInput = dynamic_cast<CDVDInputStreamMultiSource*>(pInput);

  if (!m_pInput)
    return false;

  auto iter = m_pInput->m_pInputStreams.begin();
  while (iter != m_pInput->m_pInputStreams.end())
  {
    DemuxPtr demuxer = DemuxPtr(CDVDFactoryDemuxer::CreateDemuxer(iter->get()));
    if (!demuxer)
    {
      iter = m_pInput->m_pInputStreams.erase(iter);
    }
    else
    {
      unsigned int offset = m_streamIdToDemuxerMap.size();
      demuxer->RenumberStreamIds(offset);
      m_DemuxerStreamsOffset[demuxer] = offset;
      if (!UpdateStreamMap(demuxer))
      {
        iter = m_pInput->m_pInputStreams.erase(iter);
        m_DemuxerStreamsOffset.erase(demuxer);
        continue;
      }

      m_DemuxerToInputStreamMap[demuxer] = *iter;
      m_pDemuxers.push_back(demuxer);
      m_demuxerQueue.push(std::make_pair((double)DVD_NOPTS_VALUE, demuxer));
      ++iter;
    }
  }
  return !m_pDemuxers.empty();
}

bool CDVDDemuxMultiSource::RebuildStreamMap()
{
  auto iter = std::find(m_pDemuxers.begin(), m_pDemuxers.end(), m_currentDemuxer);
  m_DemuxerToInputStreamMap.erase(m_DemuxerToInputStreamMap.find(m_currentDemuxer));
  m_pDemuxers.erase(iter);
  m_currentDemuxer = NULL;

  // TODO: 
  // - rebuild m_DemuxerStreamsOffset
  // - rebuild m_streamIdToDemuxerMap

  return true;
}

void CDVDDemuxMultiSource::Reset()
{
  for (auto iter : m_pDemuxers)
    iter->Reset();
}

DemuxPacket* CDVDDemuxMultiSource::Read()
{
  if (m_demuxerQueue.empty())
    return NULL;

  m_currentDemuxer = m_demuxerQueue.top().second;
  m_demuxerQueue.pop();

  if (!m_currentDemuxer)
    return NULL;

  DemuxPacket* packet = m_currentDemuxer->Read();
  if (packet)
  {
    // new stream?
    if (m_streamIdToDemuxerMap.find(packet->iStreamId) == m_streamIdToDemuxerMap.end())
    {
      //UpdateStreamMap()
    }
  }
  else
  {
    packet = CDVDDemuxUtils::AllocateDemuxPacket(0);
    packet->iStreamId = DMX_SPECIALID_STREAMCHANGE;
    return packet;
  }

  m_demuxerQueue.push(std::make_pair(packet->dts != DVD_NOPTS_VALUE ? packet->dts : packet->pts, m_currentDemuxer));

  return packet;
}

bool CDVDDemuxMultiSource::SeekTime(int time, bool backwords, double* startpts)
{
  m_demuxerQueue = std::priority_queue < std::pair<double, DemuxPtr>, std::vector<std::pair<double, DemuxPtr>>, comparator >();
  for (auto iter : m_pDemuxers)
  {
    if (iter->SeekTime(time, false, startpts))
    {
      m_demuxerQueue.push(std::make_pair((double)DVD_NOPTS_VALUE, iter));
      CLog::Log(LOGDEBUG, "%s - starting demuxer from: %d", __FUNCTION__, time);
    }
    else
    {
      CLog::Log(LOGDEBUG, "%s - failed to start demuxing from: %d", __FUNCTION__, time);
    }
  }
  return true;
}

bool CDVDDemuxMultiSource::UpdateStreamMap(DemuxPtr demuxer)
{
  if (!demuxer.get())
    return false;

  unsigned int offset = 0;
  auto iter = m_DemuxerStreamsOffset.find(demuxer);
  if (iter != m_DemuxerStreamsOffset.end())
  {
    offset = iter->second;
  }
  else
  {
    return false;
  }

  size_t count = m_streamIdToDemuxerMap.size();
  for (int i = 0; i < demuxer->GetNrOfStreams(); ++i)
  {

    CDemuxStream* stream = demuxer->GetStream(offset + i);

    if (!stream || m_streamIdToDemuxerMap.find(stream->iId) != m_streamIdToDemuxerMap.end())
    {
      CLog::Log(LOGNOTICE, "%s - abnormal steam mapping found,"
                           "skipping demuxer for file %s.", __FUNCTION__, demuxer->GetFileName().c_str());
      return false;
    }
    m_streamIdToDemuxerMap[stream->iId] = demuxer;
  }

  return m_streamIdToDemuxerMap.size() > count;
}
