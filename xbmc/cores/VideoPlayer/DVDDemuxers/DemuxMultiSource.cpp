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

#include "TimingConstants.h"
#include "DemuxMultiSource.h"
#include "DVDDemuxUtils.h"
#include "DVDFactoryDemuxer.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "utils/log.h"
#include "Util.h"


CDemuxMultiSource::CDemuxMultiSource()
{
}

CDemuxMultiSource::~CDemuxMultiSource()
{
  Dispose();
}

void CDemuxMultiSource::Abort()
{
  for (auto& iter : m_demuxerMap)
    iter.second->Abort();
}

void CDemuxMultiSource::Dispose()
{
  while (!m_demuxerQueue.empty())
  {
    m_demuxerQueue.pop();
  }

  m_demuxerMap.clear();
  m_DemuxerToInputStreamMap.clear();
  m_pInput = NULL;

}

void CDemuxMultiSource::EnableStream(int64_t demuxerId, int id, bool enable)
{
  auto iter = m_demuxerMap.find(demuxerId);
  if (iter != m_demuxerMap.end())
  {
    DemuxPtr demuxer = iter->second;
    demuxer->EnableStream(demuxerId, id, enable);
  }
}

void CDemuxMultiSource::Flush()
{
  for (auto& iter : m_demuxerMap)
    iter.second->Flush();
}

int CDemuxMultiSource::GetNrOfStreams() const
{
  int streamsCount = 0;
  for (auto& iter : m_demuxerMap)
    streamsCount += iter.second->GetNrOfStreams();

  return streamsCount;
}

CDemuxStream* CDemuxMultiSource::GetStream(int64_t demuxerId, int iStreamId) const
{
  auto iter = m_demuxerMap.find(demuxerId);
  if (iter != m_demuxerMap.end())
  {
    return iter->second->GetStream(demuxerId, iStreamId);
  }
  else
    return NULL;
}

std::vector<CDemuxStream*> CDemuxMultiSource::GetStreams() const
{
  std::vector<CDemuxStream*> streams;

  for (auto& iter : m_demuxerMap)
  {
    for (auto& stream : iter.second->GetStreams())
    {
      streams.push_back(stream);
    }
  }
  return streams;
}

std::string CDemuxMultiSource::GetStreamCodecName(int64_t demuxerId, int iStreamId)
{
  auto iter = m_demuxerMap.find(demuxerId);
  if (iter != m_demuxerMap.end())
  {
    return iter->second->GetStreamCodecName(demuxerId, iStreamId);
  }
  else
    return "";
};

int CDemuxMultiSource::GetStreamLength()
{
  int length = 0;
  for (auto& iter : m_demuxerMap)
  {
    length = std::max(length, iter.second->GetStreamLength());
  }

  return length;
}

bool CDemuxMultiSource::Open(CDVDInputStream* pInput)
{
  if (!pInput)
    return false;

  m_pInput = dynamic_cast<InputStreamMultiStreams*>(pInput);

  if (!m_pInput)
    return false;

  auto iter = m_pInput->m_InputStreams.begin();
  while (iter != m_pInput->m_InputStreams.end())
  {
    DemuxPtr demuxer = DemuxPtr(CDVDFactoryDemuxer::CreateDemuxer(iter->get()));
    if (!demuxer)
    {
      iter = m_pInput->m_InputStreams.erase(iter);
    }
    else
    {
      SetMissingStreamDetails(demuxer);

      m_demuxerMap[demuxer->GetDemuxerId()] = demuxer;
      m_DemuxerToInputStreamMap[demuxer] = *iter;
      m_demuxerQueue.push(std::make_pair((double)DVD_NOPTS_VALUE, demuxer));
      ++iter;
    }
  }
  return !m_demuxerMap.empty();
}

void CDemuxMultiSource::Reset()
{
  for (auto& iter : m_demuxerMap)
    iter.second->Reset();
}

DemuxPacket* CDemuxMultiSource::Read()
{
  if (m_demuxerQueue.empty())
    return NULL;

  DemuxPtr currentDemuxer = m_demuxerQueue.top().second;
  m_demuxerQueue.pop();

  if (!currentDemuxer)
    return NULL;

  DemuxPacket* packet = currentDemuxer->Read();
  if (packet)
  {
    double readTime = 0;
    if (packet->dts != DVD_NOPTS_VALUE)
      readTime = packet->dts;
    else
      readTime = packet->pts;
    m_demuxerQueue.push(std::make_pair(readTime, currentDemuxer));
  }
  else
  {
    auto input = m_DemuxerToInputStreamMap.find(currentDemuxer);
    if (input != m_DemuxerToInputStreamMap.end())
    {
      if (input->second->IsEOF())
      {
        CLog::Log(LOGDEBUG, "%s - Demuxer for file %s is at eof, removed it from the queue",
          __FUNCTION__, CURL::GetRedacted(currentDemuxer->GetFileName()).c_str());
      }
      else    //maybe add an error counter?
        m_demuxerQueue.push(std::make_pair((double)DVD_NOPTS_VALUE, currentDemuxer));
    }
  }

  return packet;
}

bool CDemuxMultiSource::SeekTime(double time, bool backwards, double* startpts)
{
  DemuxQueue demuxerQueue = DemuxQueue();
  bool ret = false;
  for (auto& iter : m_demuxerMap)
  {
    if (iter.second->SeekTime(time, false, startpts))
    {
      demuxerQueue.push(std::make_pair(*startpts, iter.second));
      CLog::Log(LOGDEBUG, "%s - starting demuxer from: %f", __FUNCTION__, time);
      ret = true;
    }
    else
    {
      CLog::Log(LOGDEBUG, "%s - failed to start demuxing from: %f", __FUNCTION__, time);
    }
  }
  m_demuxerQueue = demuxerQueue;
  return ret;
}

void CDemuxMultiSource::SetMissingStreamDetails(DemuxPtr demuxer)
{
  std::string baseFileName = m_pInput->GetFileName();
  std::string fileName = demuxer->GetFileName();
  for (auto& stream : demuxer->GetStreams())
  {
    ExternalStreamInfo info = CUtil::GetExternalStreamDetailsFromFilename(baseFileName, fileName);

    if (stream->flags == CDemuxStream::FLAG_NONE)
    {
      stream->flags = static_cast<CDemuxStream::EFlags>(info.flag);
    }
    if (stream->language[0] == '\0')
    {
      size_t len = info.language.size();
      for (size_t i = 0; i < 3; ++i)
      {
        if (i < len)
        {
          stream->language[i] = info.language.at(i);
        }
      }
    }
  }
}
