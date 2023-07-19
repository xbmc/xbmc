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

CInputStreamPVRChannel::CInputStreamPVRChannel(IVideoPlayer* pPlayer, const CFileItem& fileitem)
  : CInputStreamPVRBase(pPlayer, fileitem)
{
}

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
    CLog::Log(LOGERROR,
              "CInputStreamPVRChannel - {} - unable to obtain channel instance for channel {}",
              __FUNCTION__, m_item.GetPath());

  if (channel && m_client && (m_client->OpenLiveStream(channel) == PVR_ERROR_NO_ERROR))
  {
    m_bDemuxActive = m_client->GetClientCapabilities().HandlesDemuxing();
    CLog::Log(LOGDEBUG, "CInputStreamPVRChannel - {} - opened channel stream {}", __FUNCTION__,
              m_item.GetPath());
    return true;
  }
  return false;
}

void CInputStreamPVRChannel::ClosePVRStream()
{
  if (m_client && (m_client->CloseLiveStream() == PVR_ERROR_NO_ERROR))
  {
    m_bDemuxActive = false;
    CLog::Log(LOGDEBUG, "CInputStreamPVRChannel - {} - closed channel stream {}", __FUNCTION__,
              m_item.GetPath());
  }
}

int CInputStreamPVRChannel::ReadPVRStream(uint8_t* buf, int buf_size)
{
  int ret = -1;

  if (m_client)
    m_client->ReadLiveStream(buf, buf_size, ret);

  return ret;
}

int64_t CInputStreamPVRChannel::SeekPVRStream(int64_t offset, int whence)
{
  int64_t ret = -1;

  if (m_client)
    m_client->SeekLiveStream(offset, whence, ret);

  return ret;
}

int64_t CInputStreamPVRChannel::GetPVRStreamLength()
{
  int64_t ret = -1;

  if (m_client)
    m_client->GetLiveStreamLength(ret);

  return ret;
}

CDVDInputStream::ENextStream CInputStreamPVRChannel::NextPVRStream()
{
  if (m_eof)
    return NEXTSTREAM_OPEN;

  return NEXTSTREAM_RETRY;
}

bool CInputStreamPVRChannel::CanPausePVRStream()
{
  bool ret = false;

  if (m_client)
    m_client->CanPauseStream(ret);

  return ret;
}

bool CInputStreamPVRChannel::CanSeekPVRStream()
{
  bool ret = false;

  if (m_client)
    m_client->CanSeekStream(ret);

  return ret;
}
