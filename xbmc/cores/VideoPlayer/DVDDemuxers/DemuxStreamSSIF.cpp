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

#include "DemuxStreamSSIF.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "DVDDemuxUtils.h"
#include "utils/log.h"

//#define DEBUG_VERBOSE
#define MVC_QUEUE_SIZE 100

DemuxPacket* CDemuxStreamSSIF::AddPacket(DemuxPacket* &srcPkt)
{
  if (srcPkt->iStreamId != m_h264StreamId &&
      srcPkt->iStreamId != m_mvcStreamId)
    return srcPkt;

  if (srcPkt->iStreamId == m_h264StreamId)
  {
    if (m_bluRay && !m_bluRay->HasExtention())
      return srcPkt;
#if defined(DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, ">>> MVC add h264 packet: pts: {:.3f} dts: {:.3f}", srcPkt->pts*1e-6, srcPkt->dts*1e-6);
#endif
    m_H264queue.push(srcPkt);
  }
  else if (srcPkt->iStreamId == m_mvcStreamId)
  {
    AddMVCExtPacket(srcPkt);
  }

  return GetMVCPacket();
}

void CDemuxStreamSSIF::Flush()
{
  while (!m_H264queue.empty())
  {
    CDVDDemuxUtils::FreeDemuxPacket(m_H264queue.front());
    m_H264queue.pop();
  }
  while (!m_MVCqueue.empty())
  {
    CDVDDemuxUtils::FreeDemuxPacket(m_MVCqueue.front());
    m_MVCqueue.pop();
  }
}

DemuxPacket* CDemuxStreamSSIF::MergePacket(DemuxPacket* &srcPkt, DemuxPacket* &appendPkt)
{
  DemuxPacket* newpkt = nullptr;
  newpkt = CDVDDemuxUtils::AllocateDemuxPacket(srcPkt->iSize + appendPkt->iSize);
  newpkt->iSize = srcPkt->iSize + appendPkt->iSize;

  newpkt->pts = srcPkt->pts;
  newpkt->dts = srcPkt->dts;
  newpkt->duration = srcPkt->duration;
  newpkt->iGroupId = srcPkt->iGroupId;
  newpkt->iStreamId = srcPkt->iStreamId;
  memcpy(newpkt->pData, srcPkt->pData, srcPkt->iSize);
  memcpy(newpkt->pData + srcPkt->iSize, appendPkt->pData, appendPkt->iSize);

  CDVDDemuxUtils::FreeDemuxPacket(srcPkt);
  srcPkt = nullptr;
  CDVDDemuxUtils::FreeDemuxPacket(appendPkt);
  appendPkt = nullptr;

  return newpkt;
}

DemuxPacket* CDemuxStreamSSIF::GetMVCPacket()
{
  // if input is a bluray fill mvc queue before processing
  if (m_bluRay && m_MVCqueue.empty() && !m_H264queue.empty())
    FillMVCQueue(m_H264queue.front()->dts);

  // Here, we recreate a h264 MVC packet from the base one + buffered MVC NALU's
  while (!m_H264queue.empty() && !m_MVCqueue.empty())
  {
    DemuxPacket* h264pkt = m_H264queue.front();
    double tsH264 = (h264pkt->dts != DVD_NOPTS_VALUE ? h264pkt->dts : h264pkt->pts);
    DemuxPacket* mvcpkt = m_MVCqueue.front();
    double tsMVC = (mvcpkt->dts != DVD_NOPTS_VALUE ? mvcpkt->dts : mvcpkt->pts);

    if (tsH264 == tsMVC)
    {
      m_H264queue.pop();
      m_MVCqueue.pop();

      while (!m_H264queue.empty())
      {
        DemuxPacket* pkt = m_H264queue.front();
        double ts = (pkt->dts != DVD_NOPTS_VALUE ? pkt->dts : pkt->pts);
        if (ts == DVD_NOPTS_VALUE)
        {
#if defined(DEBUG_VERBOSE)
          CLog::Log(LOGDEBUG, ">>> MVC merge h264 fragment: {:6}+{:6}, pts({:.3f}/{:.3f}) dts({:.3f}/{:.3f})", h264pkt->iSize, pkt->iSize, h264pkt->pts*1e-6, pkt->pts*1e-6, h264pkt->dts*1e-6, pkt->dts*1e-6);
#endif
          h264pkt = MergePacket(h264pkt, pkt);
          m_H264queue.pop();
        }
        else
          break;
      }
      while (!m_MVCqueue.empty())
      {
        DemuxPacket* pkt = m_MVCqueue.front();
        double ts = (pkt->dts != DVD_NOPTS_VALUE ? pkt->dts : pkt->pts);
        if (ts == DVD_NOPTS_VALUE)
        {
#if defined(DEBUG_VERBOSE)
          CLog::Log(LOGDEBUG, ">>> MVC merge mvc fragment: {:6}+{:6}, pts({:.3f}/{:.3f}) dts({:.3f}/{:.3f})", mvcpkt->iSize, pkt->iSize, mvcpkt->pts*1e-6, pkt->pts*1e-6, mvcpkt->dts*1e-6, pkt->dts*1e-6);
#endif
          mvcpkt = MergePacket(mvcpkt, pkt);
          m_MVCqueue.pop();
        }
        else
          break;
      }

#if defined(DEBUG_VERBOSE)
      CLog::Log(LOGDEBUG, ">>> MVC merge packet: {:6}+{:6}, pts({:.3f}/{:.3f}) dts({:.3f}/{:.3f})", h264pkt->iSize, mvcpkt->iSize, h264pkt->pts*1e-6, mvcpkt->pts*1e-6, h264pkt->dts*1e-6, mvcpkt->dts*1e-6);
#endif
      return MergePacket(h264pkt, mvcpkt);
    }
    else if (tsH264 > tsMVC)
    {
#if defined(DEBUG_VERBOSE)
      CLog::Log(LOGDEBUG, ">>> MVC discard  mvc: {:6}, pts({:.3f}) dts({:.3f})", mvcpkt->iSize, mvcpkt->pts*1e-6, mvcpkt->dts*1e-6);
#endif
      CDVDDemuxUtils::FreeDemuxPacket(mvcpkt);
      m_MVCqueue.pop();
    }
    else
    {
#if defined(DEBUG_VERBOSE)
      CLog::Log(LOGDEBUG, ">>> MVC discard h264: {:6}, pts({:.3f}) dts({:.3f})", h264pkt->iSize, h264pkt->pts*1e-6, h264pkt->dts*1e-6);
#endif
      CDVDDemuxUtils::FreeDemuxPacket(h264pkt);
      m_H264queue.pop();
    }
  }

#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, ">>> MVC waiting. MVC({}) H264({})", m_MVCqueue.size(), m_H264queue.size());
#endif
  return CDVDDemuxUtils::AllocateDemuxPacket(0);
}

void CDemuxStreamSSIF::AddMVCExtPacket(DemuxPacket* &mvcExtPkt)
{
#if defined(DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, ">>> MVC add mvc  packet: pts: {:.3f} dts: {:.3f}", mvcExtPkt->pts*1e-6, mvcExtPkt->dts*1e-6);
#endif
  m_MVCqueue.push(mvcExtPkt);
}

bool CDemuxStreamSSIF::FillMVCQueue(double dtsBase)
{
  if (!m_bluRay)
    return false;

  CDVDDemux* demux = m_bluRay->GetExtentionDemux();
  DemuxPacket* mvc;
  while ((m_MVCqueue.size() < MVC_QUEUE_SIZE) && (mvc = demux->Read()))
  {
    if (dtsBase == DVD_NOPTS_VALUE || mvc->dts == DVD_NOPTS_VALUE)
    {
      // do nothing, can't compare timestamps when they are not set
    }
    else if (mvc->dts < dtsBase)
    {
#if defined(DEBUG_VERBOSE)
      CLog::Log(LOGDEBUG, ">>> MVC drop mvc: {:6}, pts({:.3f}) dts({:.3f})", mvc->iSize, mvc->pts*1e-6, mvc->dts*1e-6);
#endif
      CDVDDemuxUtils::FreeDemuxPacket(mvc);
      continue;
    }
    AddMVCExtPacket(mvc);
  };
  if (m_MVCqueue.size() != MVC_QUEUE_SIZE)
    m_bluRay->OpenNextStream();

  return true;
}
