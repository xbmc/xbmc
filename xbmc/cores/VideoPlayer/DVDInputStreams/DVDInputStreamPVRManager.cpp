/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#include "DVDFactoryInputStream.h"
#include "DVDInputStreamPVRManager.h"
#include "DVDDemuxers/DVDDemuxPacket.h"
#include "URL.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "pvr/recordings/PVRRecordings.h"
#include "settings/Settings.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"

#include <assert.h>

using namespace XFILE;
using namespace PVR;

/************************************************************************
 * Description: Class constructor, initialize member variables
 *              public class is CDVDInputStream
 */
CDVDInputStreamPVRManager::CDVDInputStreamPVRManager(IVideoPlayer* pPlayer, const CFileItem& fileitem)
  : CDVDInputStream(DVDSTREAM_TYPE_PVRMANAGER, fileitem)
{
  m_pPlayer = pPlayer;
  m_pOtherStream = nullptr;
  m_eof = true;
  m_ScanTimeout.Set(0);
  m_isOtherStreamHack = false;
  m_demuxActive = false;

  m_StreamProps = new PVR_STREAM_PROPERTIES;
}

/************************************************************************
 * Description: Class destructor
 */
CDVDInputStreamPVRManager::~CDVDInputStreamPVRManager()
{
  Close();

  m_streamMap.clear();
  delete m_StreamProps;
}

void CDVDInputStreamPVRManager::ResetScanTimeout(unsigned int iTimeoutMs)
{
  m_ScanTimeout.Set(iTimeoutMs);
}

bool CDVDInputStreamPVRManager::IsEOF()
{
  // don't mark as eof while within the scan timeout
  if (!m_ScanTimeout.IsTimePast())
    return false;

  if (m_pOtherStream)
    return m_pOtherStream->IsEOF();
  else
    return m_eof;
}

bool CDVDInputStreamPVRManager::Open()
{
  if (!CDVDInputStream::Open())
    return false;

  CURL url(m_item.GetPath());

  std::string strURL = url.Get();

  if (StringUtils::StartsWith(strURL, "pvr://channels/tv/") ||
      StringUtils::StartsWith(strURL, "pvr://channels/radio/"))
  {
    CFileItemPtr tag = g_PVRChannelGroups->GetByPath(strURL);
    if (tag && tag->HasPVRChannelInfoTag())
    {
      if (!g_PVRManager.OpenLiveStream(*tag))
        return false;

      m_isRecording = false;
      CLog::Log(LOGDEBUG, "CDVDInputStreamPVRManager - %s - playback has started on filename %s", __FUNCTION__, strURL.c_str());
    }
    else
    {
      CLog::Log(LOGERROR, "CDVDInputStreamPVRManager - %s - channel not found with filename %s", __FUNCTION__, strURL.c_str());
      return false;
    }
  }
  else if (CPVRRecordingsPath(strURL).IsActive())
  {
    CFileItemPtr tag = g_PVRRecordings->GetByPath(strURL);
    if (tag && tag->HasPVRRecordingInfoTag())
    {
      if (!g_PVRManager.OpenRecordedStream(tag->GetPVRRecordingInfoTag()))
        return false;

      m_isRecording = true;
      CLog::Log(LOGDEBUG, "%s - playback has started on recording %s (%s)", __FUNCTION__, strURL.c_str(), tag->GetPVRRecordingInfoTag()->m_strIconPath.c_str());
    }
    else
    {
      CLog::Log(LOGERROR, "CDVDInputStreamPVRManager - Recording not found with filename %s", strURL.c_str());
      return false;
    }
  }
  else if (CPVRRecordingsPath(strURL).IsDeleted())
  {
    CLog::Log(LOGNOTICE, "CDVDInputStreamPVRManager - Playback of deleted recordings is not possible (%s)", strURL.c_str());
    return false;
  }
  else
  {
    CLog::Log(LOGERROR, "%s - invalid path specified %s", __FUNCTION__, strURL.c_str());
    return false;
  }

  m_eof = false;

  /*
   * Translate the "pvr://....." entry.
   * The PVR Client can use http or whatever else is supported by VideoPlayer.
   * to access streams.
   * If after translation the file protocol is still "pvr://" use this class
   * to read the stream data over the CPVRFile class and the PVR Library itself.
   * Otherwise call CreateInputStream again with the translated filename and looks again
   * for the right protocol stream handler and swap every call to this input stream
   * handler.
   */
  m_isOtherStreamHack = false;
  std::string transFile = ThisIsAHack(m_item.GetPath());
  if(transFile.substr(0, 6) != "pvr://")
  {
    m_isOtherStreamHack = true;
    
    m_item.SetPath(transFile);
    m_item.SetMimeTypeForInternetFile();

    m_pOtherStream = CDVDFactoryInputStream::CreateInputStream(m_pPlayer, m_item);
    if (!m_pOtherStream)
    {
      CLog::Log(LOGERROR, "CDVDInputStreamPVRManager::Open - unable to create input stream for [%s]", CURL::GetRedacted(transFile).c_str());
      return false;
    }

    if (!m_pOtherStream->Open())
    {
      CLog::Log(LOGERROR, "CDVDInputStreamPVRManager::Open - error opening [%s]", CURL::GetRedacted(transFile).c_str());
      delete m_pOtherStream;
      m_pOtherStream = NULL;
      return false;
    }
  }
  else
  {
    if (URIUtils::IsPVRChannel(url.Get()))
    {
      std::shared_ptr<CPVRClient> client;
      if (g_PVRClients->GetPlayingClient(client) &&
          client->HandlesDemuxing())
        m_demuxActive = true;
    }
  }

  ResetScanTimeout((unsigned int) CSettings::GetInstance().GetInt(CSettings::SETTING_PVRPLAYBACK_SCANTIME) * 1000);
  CLog::Log(LOGDEBUG, "CDVDInputStreamPVRManager::Open - stream opened: %s", CURL::GetRedacted(transFile).c_str());

  m_StreamProps->iStreamCount = 0;
  return true;
}

std::string CDVDInputStreamPVRManager::ThisIsAHack(const std::string& pathFile)
{
  std::string FileName = pathFile;
  if (FileName.substr(0, 14) == "pvr://channels")
  {
    CFileItemPtr channel = g_PVRChannelGroups->GetByPath(FileName);
    if (channel && channel->HasPVRChannelInfoTag())
    {
      std::string stream = channel->GetPVRChannelInfoTag()->StreamURL();
      if(!stream.empty())
      {
        if (stream.compare(6, 7, "stream/") == 0)
        {
          // pvr://stream
          // This function was added to retrieve the stream URL for this item
          // Is is used for the MediaPortal (ffmpeg) PVR addon
          // see PVRManager.cpp
          return g_PVRClients->GetStreamURL(channel->GetPVRChannelInfoTag());
        }
        else
        {
          return stream;
        }
      }
    }
  }
  return FileName;
}

// close file and reset everything
void CDVDInputStreamPVRManager::Close()
{
  if (m_pOtherStream)
  {
    m_pOtherStream->Close();
    delete m_pOtherStream;
  }

  g_PVRManager.CloseStream();

  CDVDInputStream::Close();

  m_pPlayer         = NULL;
  m_pOtherStream    = NULL;
  m_eof             = true;

  CLog::Log(LOGDEBUG, "CDVDInputStreamPVRManager::Close - stream closed");
}

int CDVDInputStreamPVRManager::Read(uint8_t* buf, int buf_size)
{
  if (m_pOtherStream)
  {
    return m_pOtherStream->Read(buf, buf_size);
  }
  else
  {
    int ret = g_PVRClients->ReadStream((BYTE*)buf, buf_size);
    if (ret < 0)
      ret = -1;

    /* we currently don't support non completing reads */
    if( ret == 0 )
      m_eof = true;

    return ret;
  }
}

int64_t CDVDInputStreamPVRManager::Seek(int64_t offset, int whence)
{
  if (m_pOtherStream)
  {
    return m_pOtherStream->Seek(offset, whence);
  }
  else
  {
    if (whence == SEEK_POSSIBLE)
    {
      if (g_PVRClients->CanSeekStream())
        return 1;
      else
        return 0;
    }

    int64_t ret = g_PVRClients->SeekStream(offset, whence);

    // if we succeed, we are not eof anymore
    if( ret >= 0 )
      m_eof = false;

    return ret;
  }
}

int64_t CDVDInputStreamPVRManager::GetLength()
{
  if (m_pOtherStream)
    return m_pOtherStream->GetLength();
  else
    return g_PVRClients->GetStreamLength();
}

int CDVDInputStreamPVRManager::GetTotalTime()
{
  if (!m_isRecording)
    return g_PVRManager.GetTotalTime();
  return 0;
}

int CDVDInputStreamPVRManager::GetTime()
{
  if (!m_isRecording)
    return g_PVRManager.GetStartTime();
  return 0;
}

bool CDVDInputStreamPVRManager::NextChannel(bool preview/* = false*/)
{
  PVR_CLIENT client;
  unsigned int newchannel;
  if (!preview && IsOtherStreamHack())
  {
    CPVRChannelPtr channel(g_PVRManager.GetCurrentChannel());
    CFileItemPtr item(g_PVRChannelGroups->Get(channel->IsRadio())->GetSelectedGroup()->GetByChannelUp(channel));
    if (item)
      return CloseAndOpen(item->GetPath().c_str());
  }
  else if (!m_isRecording)
    return g_PVRManager.ChannelUp(&newchannel, preview);
  return false;
}

bool CDVDInputStreamPVRManager::PrevChannel(bool preview/* = false*/)
{
  PVR_CLIENT client;
  unsigned int newchannel;
  if (!preview && IsOtherStreamHack())
  {
    CPVRChannelPtr channel(g_PVRManager.GetCurrentChannel());
    CFileItemPtr item(g_PVRChannelGroups->Get(channel->IsRadio())->GetSelectedGroup()->GetByChannelDown(channel));
    if (item)
      return CloseAndOpen(item->GetPath().c_str());
  }
  else if (!m_isRecording)
    return g_PVRManager.ChannelDown(&newchannel, preview);
  return false;
}

bool CDVDInputStreamPVRManager::SelectChannelByNumber(unsigned int iChannelNumber)
{
  PVR_CLIENT client;
  CPVRChannelPtr currentChannel(g_PVRManager.GetCurrentChannel());
  CFileItemPtr item(g_PVRChannelGroups->Get(currentChannel->IsRadio())->GetSelectedGroup()->GetByChannelNumber(iChannelNumber));
  if (!item)
    return false;

  if (IsOtherStreamHack())
  {
    return CloseAndOpen(item->GetPath().c_str());
  }
  else if (!m_isRecording)
  {
    if (item->HasPVRChannelInfoTag())
      return g_PVRManager.ChannelSwitchById(item->GetPVRChannelInfoTag()->ChannelID());
  }

  return false;
}

bool CDVDInputStreamPVRManager::SelectChannel(const CPVRChannelPtr &channel)
{
  assert(channel.get());

  PVR_CLIENT client;
  if (IsOtherStreamHack())
  {
    CFileItem item(channel);
    return CloseAndOpen(item.GetPath().c_str());
  }
  else if (!m_isRecording)
  {
    return g_PVRManager.ChannelSwitchById(channel->ChannelID());
  }

  return false;
}

CPVRChannelPtr CDVDInputStreamPVRManager::GetSelectedChannel()
{
  return g_PVRManager.GetCurrentChannel();
}

bool CDVDInputStreamPVRManager::UpdateItem(CFileItem& item)
{
  return g_PVRManager.UpdateItem(item);
}

CDVDInputStream::ENextStream CDVDInputStreamPVRManager::NextStream()
{
  m_eof = IsEOF();

  CDVDInputStream::ENextStream next;
  if (m_pOtherStream && ((next = m_pOtherStream->NextStream()) != NEXTSTREAM_NONE))
    return next;
  else if(!m_isRecording)
  {
    if (m_eof)
      return NEXTSTREAM_OPEN;
    else
      return NEXTSTREAM_RETRY;
  }
  return NEXTSTREAM_NONE;
}

bool CDVDInputStreamPVRManager::CanRecord()
{
  if (!m_isRecording)
    return g_PVRClients->CanRecordInstantly();
  return false;
}

bool CDVDInputStreamPVRManager::IsRecording()
{
  return g_PVRClients->IsRecordingOnPlayingChannel();
}

void CDVDInputStreamPVRManager::Record(bool bOnOff)
{
  g_PVRManager.StartRecordingOnPlayingChannel(bOnOff);
}

bool CDVDInputStreamPVRManager::CanPause()
{
  return g_PVRClients->CanPauseStream();
}

bool CDVDInputStreamPVRManager::CanSeek()
{
  return g_PVRClients->CanSeekStream();
}

void CDVDInputStreamPVRManager::Pause(bool bPaused)
{
  g_PVRClients->PauseStream(bPaused);
}

std::string CDVDInputStreamPVRManager::GetInputFormat()
{
  if (!m_pOtherStream)
    return g_PVRClients->GetCurrentInputFormat();
  return "";
}

bool CDVDInputStreamPVRManager::CloseAndOpen(const char* strFile)
{
  Close();

  m_item.SetPath(strFile);
  if (Open())
  {
    return true;
  }

  return false;
}

bool CDVDInputStreamPVRManager::IsOtherStreamHack(void)
{
  return m_isOtherStreamHack;
}

bool CDVDInputStreamPVRManager::IsRealtime()
{
  return g_PVRClients->IsRealTimeStream();
}

inline CDVDInputStream::IDemux* CDVDInputStreamPVRManager::GetIDemux()
{
  if (m_demuxActive)
    return this;
  else
    return nullptr;
}

bool CDVDInputStreamPVRManager::OpenDemux()
{
  PVR_CLIENT client;
  if (!g_PVRClients->GetPlayingClient(client))
  {
    return false;
  }

  client->GetStreamProperties(m_StreamProps);
  UpdateStreamMap();
  return true;
}

DemuxPacket* CDVDInputStreamPVRManager::ReadDemux()
{
  PVR_CLIENT client;
  if (!g_PVRClients->GetPlayingClient(client))
  {
    return nullptr;
  }

  DemuxPacket* pPacket = client->DemuxRead();
  if (!pPacket)
  {
    return nullptr;
  }
  else if (pPacket->iStreamId == DMX_SPECIALID_STREAMINFO)
  {
    client->GetStreamProperties(m_StreamProps);
    return pPacket;
  }
  else if (pPacket->iStreamId == DMX_SPECIALID_STREAMCHANGE)
  {
    client->GetStreamProperties(m_StreamProps);
    UpdateStreamMap();
  }

  return pPacket;
}

CDemuxStream* CDVDInputStreamPVRManager::GetStream(int iStreamId) const
{
  auto stream = m_streamMap.find(iStreamId);
  if (stream != m_streamMap.end())
  {
    return stream->second.get();
  }
  else
    return nullptr;
}

std::vector<CDemuxStream*> CDVDInputStreamPVRManager::GetStreams() const
{
  std::vector<CDemuxStream*> streams;

  for (auto& st : m_streamMap)
  {
    streams.push_back(st.second.get());
  }

  return streams;
}

int CDVDInputStreamPVRManager::GetNrOfStreams() const
{
  return m_StreamProps->iStreamCount;
}

void CDVDInputStreamPVRManager::SetSpeed(int Speed)
{
  PVR_CLIENT client;
  if (g_PVRClients->GetPlayingClient(client))
  {
    client->SetSpeed(Speed);
  }
}

bool CDVDInputStreamPVRManager::SeekTime(int timems, bool backwards, double *startpts)
{
  PVR_CLIENT client;
  if (g_PVRClients->GetPlayingClient(client))
  {
    return client->SeekTime(timems, backwards, startpts);
  }
  return false;
}

void CDVDInputStreamPVRManager::AbortDemux()
{
  PVR_CLIENT client;
  if (g_PVRClients->GetPlayingClient(client))
  {
    client->DemuxAbort();
  }
}

void CDVDInputStreamPVRManager::FlushDemux()
{
  PVR_CLIENT client;
  if (g_PVRClients->GetPlayingClient(client))
  {
    client->DemuxFlush();
  }
}

std::shared_ptr<CDemuxStream> CDVDInputStreamPVRManager::GetStreamInternal(int iStreamId)
{
  auto stream = m_streamMap.find(iStreamId);
  if (stream != m_streamMap.end())
  {
    return stream->second;
  }
  else
    return nullptr;
}

void CDVDInputStreamPVRManager::UpdateStreamMap()
{
  std::map<int, std::shared_ptr<CDemuxStream>> m_newStreamMap;

  int num = GetNrOfStreams();
  for (int i = 0; i < num; ++i)
  {
    PVR_STREAM_PROPERTIES::PVR_STREAM stream = m_StreamProps->stream[i];

    std::shared_ptr<CDemuxStream> dStream = GetStreamInternal(stream.iPID);

    if (stream.iCodecType == XBMC_CODEC_TYPE_AUDIO)
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
    else if (stream.iCodecType == XBMC_CODEC_TYPE_VIDEO)
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
      streamVideo->fAspect = stream.fAspect;
      streamVideo->stereo_mode = "mono";

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
    else if (stream.iCodecType == XBMC_CODEC_TYPE_SUBTITLE)
    {
      std::shared_ptr<CDemuxStreamSubtitle> streamSubtitle;

      if (dStream)
        streamSubtitle = std::dynamic_pointer_cast<CDemuxStreamSubtitle>(dStream);
      if (!streamSubtitle)
        streamSubtitle = std::make_shared<CDemuxStreamSubtitle>();

      if (stream.iSubtitleInfo)
      {
        streamSubtitle->ExtraData = new uint8_t[4];
        streamSubtitle->ExtraSize = 4;
        streamSubtitle->ExtraData[0] = (stream.iSubtitleInfo >> 8) & 0xff;
        streamSubtitle->ExtraData[1] = (stream.iSubtitleInfo >> 0) & 0xff;
        streamSubtitle->ExtraData[2] = (stream.iSubtitleInfo >> 24) & 0xff;
        streamSubtitle->ExtraData[3] = (stream.iSubtitleInfo >> 16) & 0xff;
      }
      dStream = streamSubtitle;
    }
    else if (stream.iCodecType == XBMC_CODEC_TYPE_RDS &&
      CSettings::GetInstance().GetBool("pvrplayback.enableradiords"))
    {
      std::shared_ptr<CDemuxStreamRadioRDS> streamRadioRDS;

      if (dStream)
        streamRadioRDS = std::dynamic_pointer_cast<CDemuxStreamRadioRDS>(dStream);
      if (!streamRadioRDS)
        streamRadioRDS = std::make_shared<CDemuxStreamRadioRDS>();

      dStream = streamRadioRDS;
    }
    else
      dStream = std::make_shared<CDemuxStream>();

    dStream->codec = (AVCodecID)stream.iCodecId;
    dStream->uniqueId = stream.iPID;
    dStream->language[0] = stream.strLanguage[0];
    dStream->language[1] = stream.strLanguage[1];
    dStream->language[2] = stream.strLanguage[2];
    dStream->language[3] = stream.strLanguage[3];
    dStream->realtime = true;

    m_newStreamMap[stream.iPID] = dStream;
  }

  m_streamMap = m_newStreamMap;
}
