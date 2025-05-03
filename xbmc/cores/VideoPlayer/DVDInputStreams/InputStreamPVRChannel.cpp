/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InputStreamPVRChannel.h"

#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "utils/log.h"

using namespace PVR;

CInputStreamPVRChannel::~CInputStreamPVRChannel()
{
  Close();
}

CDVDInputStream::IDemux* CInputStreamPVRChannel::GetIDemux()
{
  if (m_bDemuxActive)
    return this;

  return CInputStreamPVRBase::GetIDemux();
}

bool CInputStreamPVRChannel::OpenPVRStream()
{
  std::shared_ptr<CPVRChannel> channel = m_item.GetPVRChannelInfoTag();
  if (!channel)
    channel = CServiceBroker::GetPVRManager().ChannelGroups()->GetByPath(m_item.GetPath());

  if (!channel)
    CLog::LogF(LOGERROR, "Unable to obtain channel instance for channel {}", m_item.GetPath());

  if (channel && (GetClient().OpenLiveStream(channel) == PVR_ERROR_NO_ERROR))
  {
    m_bDemuxActive = GetClient().GetClientCapabilities().HandlesDemuxing();
    CLog::LogF(LOGDEBUG, "Opened channel stream {}", m_item.GetPath());
    return true;
  }
  return false;
}

void CInputStreamPVRChannel::ClosePVRStream()
{
  if (GetClient().CloseLiveStream() == PVR_ERROR_NO_ERROR)
  {
    m_bDemuxActive = false;
    CLog::LogF(LOGDEBUG, "Closed channel stream {}", m_item.GetPath());
  }
}

int CInputStreamPVRChannel::ReadPVRStream(uint8_t* buf, int buf_size)
{
  int ret = -1;
  GetClient().ReadLiveStream(buf, buf_size, ret);
  return ret;
}

int64_t CInputStreamPVRChannel::SeekPVRStream(int64_t offset, int whence)
{
  int64_t ret = -1;
  GetClient().SeekLiveStream(offset, whence, ret);
  return ret;
}

int64_t CInputStreamPVRChannel::GetPVRStreamLength()
{
  int64_t ret = -1;
  GetClient().GetLiveStreamLength(ret);
  return ret;
}

CDVDInputStream::ENextStream CInputStreamPVRChannel::NextPVRStream()
{
  return IsEOF() ? NEXTSTREAM_OPEN : NEXTSTREAM_RETRY;
}

bool CInputStreamPVRChannel::CanPausePVRStream()
{
  bool ret = false;
  GetClient().CanPauseStream(ret);
  return ret;
}

bool CInputStreamPVRChannel::CanSeekPVRStream()
{
  bool ret = false;
  GetClient().CanSeekStream(ret);
  return ret;
}

bool CInputStreamPVRChannel::IsRealtimePVRStream()
{
  bool ret = false;
  GetClient().IsRealTimeStream(ret);
  return ret;
}

void CInputStreamPVRChannel::PausePVRStream(bool paused)
{
  GetClient().PauseStream(paused);
}

bool CInputStreamPVRChannel::GetPVRStreamTimes(Times& times)
{
  PVR_STREAM_TIMES streamTimes = {};
  if (GetClient().GetStreamTimes(&streamTimes) == PVR_ERROR_NO_ERROR)
  {
    times.startTime = streamTimes.startTime;
    times.ptsStart = streamTimes.ptsStart;
    times.ptsBegin = streamTimes.ptsBegin;
    times.ptsEnd = streamTimes.ptsEnd;
    return true;
  }
  else
  {
    return false;
  }
}
