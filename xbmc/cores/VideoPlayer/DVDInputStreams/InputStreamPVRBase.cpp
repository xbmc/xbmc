/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InputStreamPVRBase.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

CInputStreamPVRBase::CInputStreamPVRBase(IVideoPlayer* pPlayer, const CFileItem& fileitem)
  : CDVDInputStream(DVDSTREAM_TYPE_PVRMANAGER, fileitem),
    m_StreamProps(new PVR_STREAM_PROPERTIES()),
    m_client(CServiceBroker::GetPVRManager().GetClient(fileitem))
{
  if (!m_client)
    CLog::Log(LOGERROR,
              "CInputStreamPVRBase - {} - unable to obtain pvr addon instance for item '{}'",
              __FUNCTION__, fileitem.GetPath());
}

CInputStreamPVRBase::~CInputStreamPVRBase()
{
  m_streamMap.clear();
}

bool CInputStreamPVRBase::IsEOF()
{
  return m_eof;
}

bool CInputStreamPVRBase::Open()
{
  if (CDVDInputStream::Open() && OpenPVRStream())
  {
    m_eof = false;
    m_StreamProps->iStreamCount = 0;
    return true;
  }
  else
  {
    return false;
  }
}

void CInputStreamPVRBase::Close()
{
  ClosePVRStream();
  CDVDInputStream::Close();
  m_eof = true;
}

int CInputStreamPVRBase::Read(uint8_t* buf, int buf_size)
{
  int ret = ReadPVRStream(buf, buf_size);

  // we currently don't support non completing reads
  if (ret == 0)
    m_eof = true;
  else if (ret < -1)
    ret = -1;

  return ret;
}

int64_t CInputStreamPVRBase::Seek(int64_t offset, int whence)
{
  if (whence == SEEK_POSSIBLE)
    return CanSeek() ? 1 : 0;

  int64_t ret = SeekPVRStream(offset, whence);

  // if we succeed, we are not eof anymore
  if (ret >= 0)
    m_eof = false;

  return ret;
}

int64_t CInputStreamPVRBase::GetLength()
{
  return GetPVRStreamLength();
}

int CInputStreamPVRBase::GetBlockSize()
{
  int ret = -1;

  if (m_client)
    m_client->GetStreamReadChunkSize(ret);

  return ret;
}

bool CInputStreamPVRBase::GetTimes(Times &times)
{
  PVR_STREAM_TIMES streamTimes = {};
  if (m_client && m_client->GetStreamTimes(&streamTimes) == PVR_ERROR_NO_ERROR)
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

CDVDInputStream::ENextStream CInputStreamPVRBase::NextStream()
{
  return NextPVRStream();
}

bool CInputStreamPVRBase::CanPause()
{
  return CanPausePVRStream();
}

bool CInputStreamPVRBase::CanSeek()
{
  return CanSeekPVRStream();
}

void CInputStreamPVRBase::Pause(bool bPaused)
{
  if (m_client)
    m_client->PauseStream(bPaused);
}

bool CInputStreamPVRBase::IsRealtime()
{
  bool ret = false;

  if (m_client)
    m_client->IsRealTimeStream(ret);

  return ret;
}

bool CInputStreamPVRBase::OpenDemux()
{
  if (m_client)
  {
    m_client->GetStreamProperties(m_StreamProps.get());
    UpdateStreamMap();
    return true;
  }
  else
  {
    return false;
  }
}

DemuxPacket* CInputStreamPVRBase::ReadDemux()
{
  if (!m_client)
    return nullptr;

  DemuxPacket* pPacket = nullptr;
  m_client->DemuxRead(pPacket);
  if (!pPacket)
  {
    return nullptr;
  }
  else if (pPacket->iStreamId == DMX_SPECIALID_STREAMINFO)
  {
    m_client->GetStreamProperties(m_StreamProps.get());
    return pPacket;
  }
  else if (pPacket->iStreamId == DMX_SPECIALID_STREAMCHANGE)
  {
    m_client->GetStreamProperties(m_StreamProps.get());
    UpdateStreamMap();
  }

  return pPacket;
}

CDemuxStream* CInputStreamPVRBase::GetStream(int iStreamId) const
{
  const auto stream = m_streamMap.find(iStreamId);
  if (stream != m_streamMap.end())
    return stream->second.get();
  else
    return nullptr;
}

std::vector<CDemuxStream*> CInputStreamPVRBase::GetStreams() const
{
  std::vector<CDemuxStream*> streams;

  streams.reserve(m_streamMap.size());
  for (const auto& st : m_streamMap)
    streams.emplace_back(st.second.get());

  return streams;
}

int CInputStreamPVRBase::GetNrOfStreams() const
{
  return m_StreamProps->iStreamCount;
}

void CInputStreamPVRBase::SetSpeed(int Speed)
{
  if (m_client)
    m_client->SetSpeed(Speed);
}

void CInputStreamPVRBase::FillBuffer(bool mode)
{
  if (m_client)
    m_client->FillBuffer(mode);
}

bool CInputStreamPVRBase::SeekTime(double timems, bool backwards, double *startpts)
{
  if (m_client)
    return m_client->SeekTime(timems, backwards, startpts) == PVR_ERROR_NO_ERROR;
  else
    return false;
}

void CInputStreamPVRBase::AbortDemux()
{
  if (m_client)
    m_client->DemuxAbort();
}

void CInputStreamPVRBase::FlushDemux()
{
  if (m_client)
    m_client->DemuxFlush();
}

std::shared_ptr<CDemuxStream> CInputStreamPVRBase::GetStreamInternal(int iStreamId)
{
  const auto stream = m_streamMap.find(iStreamId);
  if (stream != m_streamMap.end())
    return stream->second;
  else
    return nullptr;
}

void CInputStreamPVRBase::UpdateStreamMap()
{
  std::map<int, std::shared_ptr<CDemuxStream>> newStreamMap;

  int num = GetNrOfStreams();
  for (int i = 0; i < num; ++i)
  {
    PVR_STREAM_PROPERTIES::PVR_STREAM stream = m_StreamProps->stream[i];

    std::shared_ptr<CDemuxStream> dStream = GetStreamInternal(stream.iPID);

    if (stream.iCodecType == PVR_CODEC_TYPE_AUDIO)
    {
      std::shared_ptr<CDemuxStreamAudio> streamAudio;

      if (dStream)
        streamAudio = std::dynamic_pointer_cast<CDemuxStreamAudio>(dStream);
      if (!streamAudio)
        streamAudio = std::make_shared<CDemuxStreamAudio>();

      streamAudio->iChannels = stream.iChannels;
      streamAudio->iSampleRate = stream.iSampleRate;
      streamAudio->iBlockAlign = stream.iBlockAlign;
      streamAudio->iBitRate = stream.iBitRate;
      streamAudio->iBitsPerSample = stream.iBitsPerSample;

      dStream = streamAudio;
    }
    else if (stream.iCodecType == PVR_CODEC_TYPE_VIDEO)
    {
      std::shared_ptr<CDemuxStreamVideo> streamVideo;

      if (dStream)
        streamVideo = std::dynamic_pointer_cast<CDemuxStreamVideo>(dStream);
      if (!streamVideo)
        streamVideo = std::make_shared<CDemuxStreamVideo>();

      streamVideo->iFpsScale = stream.iFPSScale;
      streamVideo->iFpsRate = stream.iFPSRate;
      streamVideo->iHeight = stream.iHeight;
      streamVideo->iWidth = stream.iWidth;
      streamVideo->fAspect = static_cast<double>(stream.fAspect);

      dStream = streamVideo;
    }
    else if (stream.iCodecId == AV_CODEC_ID_DVB_TELETEXT)
    {
      std::shared_ptr<CDemuxStreamTeletext> streamTeletext;

      if (dStream)
        streamTeletext = std::dynamic_pointer_cast<CDemuxStreamTeletext>(dStream);
      if (!streamTeletext)
        streamTeletext = std::make_shared<CDemuxStreamTeletext>();

      dStream = streamTeletext;
    }
    else if (stream.iCodecType == PVR_CODEC_TYPE_SUBTITLE)
    {
      std::shared_ptr<CDemuxStreamSubtitle> streamSubtitle;

      if (dStream)
        streamSubtitle = std::dynamic_pointer_cast<CDemuxStreamSubtitle>(dStream);
      if (!streamSubtitle)
        streamSubtitle = std::make_shared<CDemuxStreamSubtitle>();

      if (stream.iSubtitleInfo)
      {
        streamSubtitle->extraData = FFmpegExtraData(4);
        streamSubtitle->extraData.GetData()[0] = (stream.iSubtitleInfo >> 8) & 0xff;
        streamSubtitle->extraData.GetData()[1] = (stream.iSubtitleInfo >> 0) & 0xff;
        streamSubtitle->extraData.GetData()[2] = (stream.iSubtitleInfo >> 24) & 0xff;
        streamSubtitle->extraData.GetData()[3] = (stream.iSubtitleInfo >> 16) & 0xff;
      }
      dStream = streamSubtitle;
    }
    else if (stream.iCodecType == PVR_CODEC_TYPE_RDS &&
             CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                 "pvrplayback.enableradiords"))
    {
      std::shared_ptr<CDemuxStreamRadioRDS> streamRadioRDS;

      if (dStream)
        streamRadioRDS = std::dynamic_pointer_cast<CDemuxStreamRadioRDS>(dStream);
      if (!streamRadioRDS)
        streamRadioRDS = std::make_shared<CDemuxStreamRadioRDS>();

      dStream = streamRadioRDS;
    }
    else if (stream.iCodecType == PVR_CODEC_TYPE_ID3)
    {
      std::shared_ptr<CDemuxStreamAudioID3> streamAudioID3;

      if (dStream)
        streamAudioID3 = std::dynamic_pointer_cast<CDemuxStreamAudioID3>(dStream);
      if (!streamAudioID3)
        streamAudioID3 = std::make_shared<CDemuxStreamAudioID3>();

      dStream = std::move(streamAudioID3);
    }
    else
      dStream = std::make_shared<CDemuxStream>();

    dStream->codec = (AVCodecID)stream.iCodecId;
    dStream->uniqueId = stream.iPID;
    dStream->language = stream.strLanguage;

    newStreamMap[stream.iPID] = dStream;
  }

  m_streamMap = newStreamMap;
}
