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

#include "InputStreamPVRRecording.h"

#include "addons/PVRClient.h"
#include "utils/log.h"

CInputStreamPVRRecording::CInputStreamPVRRecording(IVideoPlayer* pPlayer, const CFileItem& fileitem)
  : CInputStreamPVRBase(pPlayer, fileitem)
{
}

CInputStreamPVRRecording::~CInputStreamPVRRecording()
{
  Close();
}

bool CInputStreamPVRRecording::OpenPVRStream()
{
  if (m_client && (m_client->OpenRecordedStream(m_item.GetPVRRecordingInfoTag()) == PVR_ERROR_NO_ERROR))
  {
    CLog::Log(LOGDEBUG, "CInputStreamPVRRecording - %s - opened recording stream %s", __FUNCTION__, m_item.GetPath().c_str());
    return true;
  }
  return false;
}

void CInputStreamPVRRecording::ClosePVRStream()
{
  if (m_client && (m_client->CloseRecordedStream() == PVR_ERROR_NO_ERROR))
  {
    CLog::Log(LOGDEBUG, "CInputStreamPVRRecording - %s - closed recording stream %s", __FUNCTION__, m_item.GetPath().c_str());
  }
}

int CInputStreamPVRRecording::ReadPVRStream(uint8_t* buf, int buf_size)
{
  int iRead = -1;

  if (m_client)
    m_client->ReadRecordedStream(buf, buf_size, iRead);

  return iRead;
}

int64_t CInputStreamPVRRecording::SeekPVRStream(int64_t offset, int whence)
{
  int64_t ret = -1;

  if (m_client)
    m_client->SeekRecordedStream(offset, whence, ret);

  return ret;
}

int64_t CInputStreamPVRRecording::GetPVRStreamLength()
{
  int64_t ret = -1;

  if (m_client)
    m_client->GetRecordedStreamLength(ret);

  return ret;
}

CDVDInputStream::ENextStream CInputStreamPVRRecording::NextPVRStream()
{
  return NEXTSTREAM_NONE;
}

bool CInputStreamPVRRecording::CanPausePVRStream()
{
  return true;
}

bool CInputStreamPVRRecording::CanSeekPVRStream()
{
  return true;
}
