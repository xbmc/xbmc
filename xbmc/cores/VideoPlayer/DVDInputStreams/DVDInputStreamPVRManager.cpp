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
#include "filesystem/PVRFile.h"
#include "URL.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "utils/log.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "settings/Settings.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"

#include <assert.h>

using namespace XFILE;
using namespace PVR;

/************************************************************************
 * Description: Class constructor, initialize member variables
 *              public class is CDVDInputStream
 */
CDVDInputStreamPVRManager::CDVDInputStreamPVRManager(IVideoPlayer* pPlayer, CFileItem& fileitem)
  : CDVDInputStream(DVDSTREAM_TYPE_PVRMANAGER, fileitem)
{
  m_pPlayer = pPlayer;
  m_pFile = NULL;
  m_pRecordable = NULL;
  m_pLiveTV = NULL;
  m_pOtherStream = NULL;
  m_eof = true;
  m_ScanTimeout.Set(0);
  m_isOtherStreamHack = false;
  m_demuxActive = false;

  m_StreamProps = new PVR_STREAM_PROPERTIES;
  m_streamAudio = new CDemuxStreamAudio;
  m_streamVideo = new CDemuxStreamVideo;
  m_streamSubtitle = new CDemuxStreamSubtitle;
  m_streamTeletext = new CDemuxStreamTeletext;
  m_streamRadioRDS = new CDemuxStreamRadioRDS;
  m_streamDefault = new CDemuxStream;
}

/************************************************************************
 * Description: Class destructor
 */
CDVDInputStreamPVRManager::~CDVDInputStreamPVRManager()
{
  Close();

  delete m_StreamProps;
  delete m_streamAudio;
  delete m_streamVideo;
  delete m_streamSubtitle;
  delete m_streamTeletext;
  delete m_streamRadioRDS;
  delete m_streamDefault;
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
    return !m_pFile || m_eof;
}

bool CDVDInputStreamPVRManager::Open()
{
  /* Open PVR File for both cases, to have access to ILiveTVInterface and
   * IRecordable
   */
  m_pFile       = new CPVRFile;
  m_pLiveTV     = ((CPVRFile*)m_pFile)->GetLiveTV();
  m_pRecordable = ((CPVRFile*)m_pFile)->GetRecordable();

  if (!CDVDInputStream::Open())
    return false;

  CURL url(m_item.GetPath());
  if (!m_pFile->Open(url))
  {
    delete m_pFile;
    m_pFile = NULL;
    m_pLiveTV = NULL;
    m_pRecordable = NULL;
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
  std::string transFile = XFILE::CPVRFile::TranslatePVRFilename(m_item.GetPath());
  if(transFile.substr(0, 6) != "pvr://")
  {
    m_isOtherStreamHack = true;
    
    m_item.SetPath(transFile);
    m_pOtherStream = CDVDFactoryInputStream::CreateInputStream(m_pPlayer, m_item);
    if (!m_pOtherStream)
    {
      CLog::Log(LOGERROR, "CDVDInputStreamPVRManager::Open - unable to create input stream for [%s]", CURL::GetRedacted(transFile).c_str());
      return false;
    }

    if (!m_pOtherStream->Open())
    {
      CLog::Log(LOGERROR, "CDVDInputStreamPVRManager::Open - error opening [%s]", CURL::GetRedacted(transFile).c_str());
      delete m_pFile;
      m_pFile = NULL;
      m_pLiveTV = NULL;
      m_pRecordable = NULL;
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

// close file and reset everyting
void CDVDInputStreamPVRManager::Close()
{
  if (m_pOtherStream)
  {
    m_pOtherStream->Close();
    delete m_pOtherStream;
  }

  if (m_pFile)
  {
    m_pFile->Close();
    delete m_pFile;
  }

  CDVDInputStream::Close();

  m_pPlayer         = NULL;
  m_pFile           = NULL;
  m_pLiveTV         = NULL;
  m_pRecordable     = NULL;
  m_pOtherStream    = NULL;
  m_eof             = true;

  CLog::Log(LOGDEBUG, "CDVDInputStreamPVRManager::Close - stream closed");
}

int CDVDInputStreamPVRManager::Read(uint8_t* buf, int buf_size)
{
  if(!m_pFile) return -1;

  if (m_pOtherStream)
  {
    return m_pOtherStream->Read(buf, buf_size);
  }
  else
  {
    unsigned int ret = m_pFile->Read(buf, buf_size);

    /* we currently don't support non completing reads */
    if( ret == 0 ) m_eof = true;

    return (int)(ret & 0xFFFFFFFF);
  }
}

int64_t CDVDInputStreamPVRManager::Seek(int64_t offset, int whence)
{
  if (!m_pFile)
    return -1;

  if (m_pOtherStream)
  {
    return m_pOtherStream->Seek(offset, whence);
  }
  else
  {
    if (whence == SEEK_POSSIBLE)
      return m_pFile->IoControl(IOCTRL_SEEK_POSSIBLE, NULL);

    int64_t ret = m_pFile->Seek(offset, whence);

    /* if we succeed, we are not eof anymore */
    if( ret >= 0 ) m_eof = false;

    return ret;
  }
}

int64_t CDVDInputStreamPVRManager::GetLength()
{
  if(!m_pFile) return -1;

  if (m_pOtherStream)
    return m_pOtherStream->GetLength();
  else
    return m_pFile->GetLength();
}

int CDVDInputStreamPVRManager::GetTotalTime()
{
  if (m_pLiveTV)
    return m_pLiveTV->GetTotalTime();
  return 0;
}

int CDVDInputStreamPVRManager::GetTime()
{
  if (m_pLiveTV)
    return m_pLiveTV->GetStartTime();
  return 0;
}

bool CDVDInputStreamPVRManager::NextChannel(bool preview/* = false*/)
{
  PVR_CLIENT client;
  if (!preview && IsOtherStreamHack())
  {
    CPVRChannelPtr channel(g_PVRManager.GetCurrentChannel());
    CFileItemPtr item(g_PVRChannelGroups->Get(channel->IsRadio())->GetSelectedGroup()->GetByChannelUp(channel));
    if (item)
      return CloseAndOpen(item->GetPath().c_str());
  }
  else if (m_pLiveTV)
    return m_pLiveTV->NextChannel(preview);
  return false;
}

bool CDVDInputStreamPVRManager::PrevChannel(bool preview/* = false*/)
{
  PVR_CLIENT client;
  if (!preview && IsOtherStreamHack())
  {
    CPVRChannelPtr channel(g_PVRManager.GetCurrentChannel());
    CFileItemPtr item(g_PVRChannelGroups->Get(channel->IsRadio())->GetSelectedGroup()->GetByChannelDown(channel));
    if (item)
      return CloseAndOpen(item->GetPath().c_str());
  }
  else if (m_pLiveTV)
    return m_pLiveTV->PrevChannel(preview);
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
  else if (m_pLiveTV)
  {
    if (item->HasPVRChannelInfoTag())
      return m_pLiveTV->SelectChannelById(item->GetPVRChannelInfoTag()->ChannelID());
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
  else if (m_pLiveTV)
  {
    return m_pLiveTV->SelectChannelById(channel->ChannelID());
  }

  return false;
}

CPVRChannelPtr CDVDInputStreamPVRManager::GetSelectedChannel()
{
  return g_PVRManager.GetCurrentChannel();
}

bool CDVDInputStreamPVRManager::UpdateItem(CFileItem& item)
{
  if (m_pLiveTV)
    return m_pLiveTV->UpdateItem(item);
  return false;
}

CDVDInputStream::ENextStream CDVDInputStreamPVRManager::NextStream()
{
  if(!m_pFile)
    return NEXTSTREAM_NONE;

  m_eof = IsEOF();

  CDVDInputStream::ENextStream next;
  if (m_pOtherStream && ((next = m_pOtherStream->NextStream()) != NEXTSTREAM_NONE))
    return next;
  else if(m_pFile->SkipNext())
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
  if (m_pRecordable)
    return m_pRecordable->CanRecord();
  return false;
}

bool CDVDInputStreamPVRManager::IsRecording()
{
  if (m_pRecordable)
    return m_pRecordable->IsRecording();
  return false;
}

bool CDVDInputStreamPVRManager::Record(bool bOnOff)
{
  if (m_pRecordable)
    return m_pRecordable->Record(bOnOff);
  return false;
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
  if (!m_pOtherStream && g_PVRManager.IsStarted())
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
  }

  return pPacket;
}

CDemuxStream* CDVDInputStreamPVRManager::GetStream(int iStreamId)
{
  CDemuxStream *ret = m_streamDefault;
  m_streamDefault->type = STREAM_NONE;

  if (m_StreamProps->stream[iStreamId].iCodecType == XBMC_CODEC_TYPE_AUDIO)
  {
    m_streamAudio->iChannels       = m_StreamProps->stream[iStreamId].iChannels;
    m_streamAudio->iSampleRate     = m_StreamProps->stream[iStreamId].iSampleRate;
    m_streamAudio->iBlockAlign     = m_StreamProps->stream[iStreamId].iBlockAlign;
    m_streamAudio->iBitRate        = m_StreamProps->stream[iStreamId].iBitRate;
    m_streamAudio->iBitsPerSample  = m_StreamProps->stream[iStreamId].iBitsPerSample;

    ret = m_streamAudio;
  }
  else if (m_StreamProps->stream[iStreamId].iCodecType == XBMC_CODEC_TYPE_VIDEO)
  {
    m_streamVideo->iFpsScale       = m_StreamProps->stream[iStreamId].iFPSScale;
    m_streamVideo->iFpsRate        = m_StreamProps->stream[iStreamId].iFPSRate;
    m_streamVideo->iHeight         = m_StreamProps->stream[iStreamId].iHeight;
    m_streamVideo->iWidth          = m_StreamProps->stream[iStreamId].iWidth;
    m_streamVideo->fAspect         = m_StreamProps->stream[iStreamId].fAspect;
    m_streamVideo->stereo_mode     = "mono";

    ret = m_streamVideo;
  }
  else if (m_StreamProps->stream[iStreamId].iCodecId == AV_CODEC_ID_DVB_TELETEXT)
  {
    ret = m_streamTeletext;
  }
  else if (m_StreamProps->stream[iStreamId].iCodecType == XBMC_CODEC_TYPE_SUBTITLE)
  {
    if(m_StreamProps->stream[iStreamId].iIdentifier)
    {
      m_streamSubtitle->ExtraData = new uint8_t[4];
      m_streamSubtitle->ExtraSize = 4;
      m_streamSubtitle->ExtraData[0] = (m_StreamProps->stream[iStreamId].iIdentifier >> 8) & 0xff;
      m_streamSubtitle->ExtraData[1] = (m_StreamProps->stream[iStreamId].iIdentifier >> 0) & 0xff;
      m_streamSubtitle->ExtraData[2] = (m_StreamProps->stream[iStreamId].iIdentifier >> 24) & 0xff;
      m_streamSubtitle->ExtraData[3] = (m_StreamProps->stream[iStreamId].iIdentifier >> 16) & 0xff;
    }
    ret = m_streamSubtitle;
  }
  else if (m_StreamProps->stream[iStreamId].iCodecType == XBMC_CODEC_TYPE_RDS &&
           CSettings::GetInstance().GetBool("pvrplayback.enableradiords"))
  {
    ret = m_streamRadioRDS;
  }

  ret->codec = (AVCodecID)m_StreamProps->stream[iStreamId].iCodecId;
  ret->iId = iStreamId;
  ret->iPhysicalId = m_StreamProps->stream[iStreamId].iPhysicalId;
  ret->language[0] = m_StreamProps->stream[iStreamId].strLanguage[0];
  ret->language[1] = m_StreamProps->stream[iStreamId].strLanguage[1];
  ret->language[2] = m_StreamProps->stream[iStreamId].strLanguage[2];
  ret->language[3] = m_StreamProps->stream[iStreamId].strLanguage[3];
  ret->realtime = true;
  return ret;
}

int CDVDInputStreamPVRManager::GetNrOfStreams()
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
