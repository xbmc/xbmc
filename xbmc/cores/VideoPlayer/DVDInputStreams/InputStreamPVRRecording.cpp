/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InputStreamPVRRecording.h"

#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/recordings/PVRRecordings.h"
#include "utils/log.h"

using namespace PVR;

CInputStreamPVRRecording::~CInputStreamPVRRecording()
{
  Close();
}

bool CInputStreamPVRRecording::OpenPVRStream()
{
  std::shared_ptr<CPVRRecording> recording = m_item.GetPVRRecordingInfoTag();
  if (!recording)
    recording = CServiceBroker::GetPVRManager().Recordings()->GetByPath(m_item.GetPath());

  if (!recording)
    CLog::LogF(LOGERROR, "Unable to obtain recording instance for recording {}", m_item.GetPath());

  if (recording && (GetClient().OpenRecordedStream(recording, m_streamId) == PVR_ERROR_NO_ERROR))
  {
    CLog::LogF(LOGDEBUG, "Opened recording stream {}", m_item.GetPath());
    return true;
  }
  return false;
}

void CInputStreamPVRRecording::ClosePVRStream()
{
  if (GetClient().CloseRecordedStream(m_streamId) == PVR_ERROR_NO_ERROR)
  {
    CLog::LogF(LOGDEBUG, "Closed recording stream {}", m_item.GetPath());
  }
}

int CInputStreamPVRRecording::ReadPVRStream(uint8_t* buf, int buf_size)
{
  int iRead = -1;
  GetClient().ReadRecordedStream(m_streamId, buf, buf_size, iRead);
  return iRead;
}

int64_t CInputStreamPVRRecording::SeekPVRStream(int64_t offset, int whence)
{
  int64_t ret = -1;
  GetClient().SeekRecordedStream(m_streamId, offset, whence, ret);
  return ret;
}

int64_t CInputStreamPVRRecording::GetPVRStreamLength()
{
  int64_t ret = -1;
  GetClient().GetRecordedStreamLength(m_streamId, ret);
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

bool CInputStreamPVRRecording::IsRealtimePVRStream()
{
  bool ret = false;
  GetClient().IsRecordedStreamRealTime(m_streamId, ret);
  return ret;
}

void CInputStreamPVRRecording::PausePVRStream(bool paused)
{
  GetClient().PauseRecordedStream(m_streamId, paused);
}

bool CInputStreamPVRRecording::GetPVRStreamTimes(Times& times)
{
  PVR_STREAM_TIMES streamTimes = {};
  if (GetClient().GetRecordedStreamTimes(m_streamId, &streamTimes) == PVR_ERROR_NO_ERROR)
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
