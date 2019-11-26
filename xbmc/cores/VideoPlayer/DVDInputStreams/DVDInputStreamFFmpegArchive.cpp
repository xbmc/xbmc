/*
 *  Copyright (C) 2018 Arthur Liberman
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDInputStreamFFmpegArchive.h"
#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRStreamProperties.h"
#include "pvr/addons/PVRClient.h"
#include "utils/log.h"

CDVDInputStreamFFmpegArchive::CDVDInputStreamFFmpegArchive(const CFileItem& fileitem)
  : CDVDInputStreamFFmpeg(fileitem),
    m_client(CServiceBroker::GetPVRManager().GetClient(fileitem))
{ }

int64_t CDVDInputStreamFFmpegArchive::GetLength()
{
  int64_t ret = 0;
  if (m_client && m_client->GetLiveStreamLength(ret) != PVR_ERROR_NO_ERROR)
  {
    Times times = {0};
    if (GetTimes(times) && times.ptsEnd >= times.ptsBegin)
      ret = static_cast<int64_t>(times.ptsEnd - times.ptsBegin);
  }
  return ret;
}

bool CDVDInputStreamFFmpegArchive::GetTimes(Times &times)
{
  bool ret = false;
  PVR_STREAM_TIMES streamTimes = {0};
  if (m_client && m_client->GetStreamTimes(&streamTimes) == PVR_ERROR_NO_ERROR)
  {
    times.startTime = streamTimes.startTime;
    times.ptsStart = static_cast<double>(streamTimes.ptsStart);
    times.ptsBegin = static_cast<double>(streamTimes.ptsBegin);
    times.ptsEnd = static_cast<double>(streamTimes.ptsEnd);
    ret = true;
  }
  return ret;
}

int64_t CDVDInputStreamFFmpegArchive::Seek(int64_t offset, int whence)
{
  int64_t iPosition = -1;
  if (m_client)
    m_client->SeekLiveStream(offset, whence, iPosition);
  return iPosition;
}

CURL CDVDInputStreamFFmpegArchive::GetURL()
{
  if (m_client)
  {
    PVR::CPVRStreamProperties props;

    if (m_item.HasEPGInfoTag())
      m_client->GetEpgTagStreamProperties(m_item.GetEPGInfoTag(), props);
    else if (m_item.HasPVRChannelInfoTag())
      m_client->GetChannelStreamProperties(m_item.GetPVRChannelInfoTag(), props);

    if (props.size())
    {
      const std::string url = props.GetStreamURL();
      if (!url.empty())
        m_item.SetDynPath(url);

      const std::string mime = props.GetStreamMimeType();
      if (!mime.empty())
      {
        m_item.SetMimeType(mime);
        m_item.SetContentLookup(false);
      }

      for (const auto& prop : props)
        m_item.SetProperty(prop.first, prop.second);
    }
  }
  return CDVDInputStream::GetURL();
}

bool CDVDInputStreamFFmpegArchive::IsStreamType(DVDStreamType type) const
{
  return CDVDInputStream::IsStreamType(type) || type == DVDSTREAM_TYPE_PVR_ARCHIVE;
}
