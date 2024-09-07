/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InputStreamPVRMedia.h"

#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/media/PVRMedia.h"
#include "utils/log.h"

using namespace PVR;

CInputStreamPVRMedia::CInputStreamPVRMedia(IVideoPlayer* pPlayer, const CFileItem& fileitem)
  : CInputStreamPVRBase(pPlayer, fileitem)
{
}

CInputStreamPVRMedia::~CInputStreamPVRMedia()
{
  Close();
}

bool CInputStreamPVRMedia::OpenPVRStream()
{
  std::shared_ptr<CPVRMediaTag> mediaTag = m_item.GetPVRMediaInfoTag();
  if (!mediaTag)
    mediaTag = CServiceBroker::GetPVRManager().Media()->GetByPath(m_item.GetPath());

  if (!mediaTag)
    CLog::Log(LOGERROR,
              "CInputStreamPVRMedia - %s - unable to obtain mediaTag instance for mediaTag %s",
              __FUNCTION__, m_item.GetPath().c_str());

  if (mediaTag && m_client && (m_client->OpenMediaStream(mediaTag) == PVR_ERROR_NO_ERROR))
  {
    CLog::Log(LOGDEBUG, "CInputStreamPVRMedia - %s - opened mediaTag stream %s", __FUNCTION__,
              m_item.GetPath().c_str());
    return true;
  }

  return false;
}

void CInputStreamPVRMedia::ClosePVRStream()
{
  if (m_client && (m_client->CloseMediaStream() == PVR_ERROR_NO_ERROR))
  {
    CLog::Log(LOGDEBUG, "CInputStreamPVRMedia - %s - closed mediaTag stream %s", __FUNCTION__,
              m_item.GetPath().c_str());
  }
}

int CInputStreamPVRMedia::ReadPVRStream(uint8_t* buf, int buf_size)
{
  int iRead = -1;

  if (m_client)
    m_client->ReadMediaStream(buf, buf_size, iRead);

  return iRead;
}

int64_t CInputStreamPVRMedia::SeekPVRStream(int64_t offset, int whence)
{
  int64_t ret = -1;

  if (m_client)
    m_client->SeekMediaStream(offset, whence, ret);

  return ret;
}

int64_t CInputStreamPVRMedia::GetPVRStreamLength()
{
  int64_t ret = -1;

  if (m_client)
    m_client->GetMediaStreamLength(ret);

  return ret;
}

CDVDInputStream::ENextStream CInputStreamPVRMedia::NextPVRStream()
{
  return NEXTSTREAM_NONE;
}

bool CInputStreamPVRMedia::CanPausePVRStream()
{
  return true;
}

bool CInputStreamPVRMedia::CanSeekPVRStream()
{
  return true;
}
