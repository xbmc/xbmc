/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InputStreamPVRChannel.h"

#include "addons/PVRClient.h"
#include "utils/log.h"

CInputStreamPVRChannel::CInputStreamPVRChannel(IVideoPlayer* pPlayer, const CFileItem& fileitem)
  : CInputStreamPVRBase(pPlayer, fileitem),
    m_bDemuxActive(false)
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
  if (m_client && (m_client->OpenLiveStream(m_item.GetPVRChannelInfoTag()) == PVR_ERROR_NO_ERROR))
  {
    m_bDemuxActive = m_client->GetClientCapabilities().HandlesDemuxing();
    CLog::Log(LOGDEBUG, "CInputStreamPVRChannel - %s - opened channel stream %s", __FUNCTION__, m_item.GetPath().c_str());
    return true;
  }
  return false;
}

void CInputStreamPVRChannel::ClosePVRStream()
{
  if (m_client && (m_client->CloseLiveStream() == PVR_ERROR_NO_ERROR))
  {
    m_bDemuxActive = false;
    CLog::Log(LOGDEBUG, "CInputStreamPVRChannel - %s - closed channel stream %s", __FUNCTION__, m_item.GetPath().c_str());
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
