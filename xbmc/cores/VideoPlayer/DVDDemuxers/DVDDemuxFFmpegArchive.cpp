/*
 *  Copyright (C) 2018 Arthur Liberman
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDDemuxFFmpegArchive.h"

#include "DVDDemuxUtils.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDInputStreamFFmpegArchive.h"
#include "cores/VideoPlayer/Interface/Addon/TimingConstants.h" // for DVD_NOPTS_VALUE
#include "threads/SingleLock.h"
#include "utils/log.h"

CDVDDemuxFFmpegArchive::CDVDDemuxFFmpegArchive() :
    m_bIsOpening(false), m_seekOffset(0)
{ }

bool CDVDDemuxFFmpegArchive::Open(std::shared_ptr<CDVDInputStream> pInput, bool streaminfo, bool fileinfo)
{
  m_bIsOpening = true;
  bool ret = CDVDDemuxFFmpeg::Open(pInput, streaminfo, fileinfo);
  m_bIsOpening = false;
  return ret;
}

bool CDVDDemuxFFmpegArchive::SeekTime(double time, bool backwards, double *startpts)
{
  if (!m_pInput || time < 0)
    return false;

  int whence = m_bIsOpening ? SEEK_CUR : SEEK_SET;
  int64_t seekResult = m_pInput->Seek(static_cast<int64_t>(time), whence);
  if (seekResult >= 0)
  {
    {
      CSingleLock lock(m_critSection);
      m_seekOffset = seekResult;
    }

    CLog::Log(LOGDEBUG, "Seek successful. m_seekOffset = %f, m_currentPts = %f, time = %f, backwards = %d, startptr = %f",
      m_seekOffset, m_currentPts, time, backwards, startpts ? *startpts : 0);
    return m_bIsOpening ? true : Reset();
  }

  CLog::Log(LOGDEBUG, "Seek failed. m_currentPts = %f, time = %f, backwards = %d, startptr = %f",
    m_currentPts, time, backwards, startpts ? *startpts : 0);
  return false;
}

DemuxPacket* CDVDDemuxFFmpegArchive::Read()
{
  DemuxPacket* pPacket = CDVDDemuxFFmpeg::Read();
  if (pPacket)
  {
    CSingleLock lock(m_critSection);
    pPacket->pts += m_seekOffset;
    pPacket->dts += m_seekOffset;
  }

  return pPacket;
}

void CDVDDemuxFFmpegArchive::UpdateCurrentPTS()
{
  CDVDDemuxFFmpeg::UpdateCurrentPTS();
  if (m_currentPts != DVD_NOPTS_VALUE)
    m_currentPts += m_seekOffset;
}