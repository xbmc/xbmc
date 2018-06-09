/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
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
