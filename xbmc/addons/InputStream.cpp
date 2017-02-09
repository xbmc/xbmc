/*
 *      Copyright (C) 2016 Team Kodi
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
#include "InputStream.h"
#include "URL.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"
#include "cores/VideoPlayer/DVDDemuxers/DemuxCrypto.h"
#include "threads/SingleLock.h"
#include "utils/RegExp.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

namespace ADDON
{

CInputStream::CInputStream(AddonInfoPtr addonInfo, IVideoPlayer* player)
  : CAddonDll(addonInfo),
    m_player(player)
{
  std::string listitemprops = AddonInfo()->Type(ADDON_INPUTSTREAM)->GetValue("@listitemprops").asString();
  std::string extensions = AddonInfo()->Type(ADDON_INPUTSTREAM)->GetValue("@extension").asString();
  std::string protocols = AddonInfo()->Type(ADDON_INPUTSTREAM)->GetValue("@protocols").asString();
  std::string name(AddonInfo()->ID());

  m_fileItemProps = StringUtils::Tokenize(listitemprops, "|");
  for (auto &key : m_fileItemProps)
  {
    StringUtils::Trim(key);
    key = name + "." + key;
  }

  m_extensionsList = StringUtils::Tokenize(extensions, "|");
  for (auto &ext : m_extensionsList)
  {
    StringUtils::Trim(ext);
  }

  m_protocolsList = StringUtils::Tokenize(protocols, "|");
  for (auto &ext : m_protocolsList)
  {
    StringUtils::Trim(ext);
  }
}

bool CInputStream::Supports(const CFileItem &fileitem)
{
  // check if a specific inputstream addon is requested
  CVariant addon = fileitem.GetProperty("inputstreamaddon");
  if (!addon.isNull())
  {
    if (addon.asString() != AddonInfo()->ID())
      return false;
    else
      return true;
  }

  // check protocols
  std::string protocol = fileitem.GetURL().GetProtocol();
  if (!protocol.empty())
  {
    if (std::find(m_protocolsList.begin(),
                  m_protocolsList.end(), protocol) != m_protocolsList.end())
      return true;
  }

  std::string filetype = fileitem.GetURL().GetFileType();
  if (!filetype.empty())
  {
    if (std::find(m_extensionsList.begin(),
                  m_extensionsList.end(), filetype) != m_extensionsList.end())
      return true;
  }

  return false;
}

bool CInputStream::Open(CFileItem &fileitem)
{
  if (CAddonDll::Create(ADDON_INSTANCE_INPUTSTREAM, &m_struct, &m_struct.props) != ADDON_STATUS_OK)
    return false;

  unsigned int videoWidth = 720;
  unsigned int videoHeight = 578;
  if (m_player)
    m_player->GetVideoResolution(videoWidth, videoHeight);
  SetVideoResolution(videoWidth, videoHeight);

  INPUTSTREAM props;
  std::map<std::string, std::string> propsMap;
  for (auto &key : m_fileItemProps)
  {
    if (fileitem.GetProperty(key).isNull())
      continue;
    propsMap[key] = fileitem.GetProperty(key).asString();
  }

  props.m_nCountInfoValues = 0;
  for (auto &pair : propsMap)
  {
    props.m_ListItemProperties[props.m_nCountInfoValues].m_strKey = pair.first.c_str();
    props.m_ListItemProperties[props.m_nCountInfoValues].m_strValue = pair.second.c_str();
    props.m_nCountInfoValues++;
  }

  props.m_strURL = fileitem.GetPath().c_str();
  
  std::string libFolder = URIUtils::GetDirectory(m_parentLib);
  std::string profileFolder = CSpecialProtocol::TranslatePath(Profile());
  props.m_libFolder = libFolder.c_str();
  props.m_profileFolder = profileFolder.c_str();

  bool ret = m_struct.toAddon.Open(m_addonInstance, props);
  if (ret)
  {
    memset(&m_caps, 0, sizeof(m_caps));
    m_struct.toAddon.GetCapabilities(m_addonInstance, &m_caps);
  }

  UpdateStreams();
  return ret;
}

void CInputStream::Close()
{
  m_struct.toAddon.Close(m_addonInstance);
}

// IDisplayTime
int CInputStream::GetTotalTime()
{
  return m_struct.toAddon.GetTotalTime(m_addonInstance);
}

int CInputStream::GetTime()
{
  return m_struct.toAddon.GetTime(m_addonInstance);
}

// IPosTime
bool CInputStream::PosTime(int ms)
{
  return m_struct.toAddon.PosTime(m_addonInstance, ms);
}

// IDemux
void CInputStream::UpdateStreams()
{
  DisposeStreams();

  INPUTSTREAM_IDS streamIDs = m_struct.toAddon.GetStreamIds(m_addonInstance);
  if (streamIDs.m_streamCount > INPUTSTREAM_IDS::MAX_STREAM_COUNT)
  {
    DisposeStreams();
    return;
  }

  for (unsigned int i=0; i<streamIDs.m_streamCount; i++)
  {
    INPUTSTREAM_INFO stream = m_struct.toAddon.GetStream(m_addonInstance, streamIDs.m_streamIds[i]);
    if (stream.m_streamType == INPUTSTREAM_INFO::TYPE_NONE)
      continue;

    std::string codecName(stream.m_codecName);
    StringUtils::ToLower(codecName);
    AVCodec *codec = avcodec_find_decoder_by_name(codecName.c_str());
    if (!codec)
      continue;

    CDemuxStream *demuxStream;

    if (stream.m_streamType == INPUTSTREAM_INFO::TYPE_AUDIO)
    {
      CDemuxStreamAudio *audioStream = new CDemuxStreamAudio();

      audioStream->iChannels = stream.m_Channels;
      audioStream->iSampleRate = stream.m_SampleRate;
      audioStream->iBlockAlign = stream.m_BlockAlign;
      audioStream->iBitRate = stream.m_BitRate;
      audioStream->iBitsPerSample = stream.m_BitsPerSample;
      demuxStream = audioStream;
    }
    else if (stream.m_streamType == INPUTSTREAM_INFO::TYPE_VIDEO)
    {
      CDemuxStreamVideo *videoStream = new CDemuxStreamVideo();

      videoStream->iFpsScale = stream.m_FpsScale;
      videoStream->iFpsRate = stream.m_FpsRate;
      videoStream->iWidth = stream.m_Width;
      videoStream->iHeight = stream.m_Height;
      videoStream->fAspect = stream.m_Aspect;
      videoStream->stereo_mode = "mono";
      videoStream->iBitRate = stream.m_BitRate;
      demuxStream = videoStream;
    }
    else if (stream.m_streamType == INPUTSTREAM_INFO::TYPE_SUBTITLE)
    {
      //! @todo needs identifier in INPUTSTREAM_INFO
      continue;
    }
    else
      continue;

    demuxStream->codec = codec->id;
    demuxStream->bandwidth = stream.m_Bandwidth;
    demuxStream->codecName = stream.m_codecInternalName;
    demuxStream->uniqueId = streamIDs.m_streamIds[i];
    demuxStream->language[0] = stream.m_language[0];
    demuxStream->language[1] = stream.m_language[1];
    demuxStream->language[2] = stream.m_language[2];
    demuxStream->language[3] = stream.m_language[3];

    if (stream.m_ExtraData && stream.m_ExtraSize)
    {
      demuxStream->ExtraData = new uint8_t[stream.m_ExtraSize];
      demuxStream->ExtraSize = stream.m_ExtraSize;
      for (unsigned int j=0; j<stream.m_ExtraSize; j++)
        demuxStream->ExtraData[j] = stream.m_ExtraData[j];
    }

    if (stream.m_CryptoKeySystem != INPUTSTREAM_INFO::CRYPTO_KEY_SYSTEM_NONE &&
      stream.m_CryptoKeySystem < INPUTSTREAM_INFO::CRYPTO_KEY_SYSTEM_COUNT)
    {
      static const CryptoSessionSystem map[] =
      {
        CRYPTO_SESSION_SYSTEM_NONE,
        CRYPTO_SESSION_SYSTEM_WIDEVINE,
        CRYPTO_SESSION_SYSTEM_PLAYREADY
      };
      demuxStream->cryptoSession = std::shared_ptr<DemuxCryptoSession>(new DemuxCryptoSession(
        map[stream.m_CryptoKeySystem], stream.m_CryptoSessionIdSize, stream.m_CryptoSessionId));
    }

    m_streams[demuxStream->uniqueId] = demuxStream;
  }
}

void CInputStream::DisposeStreams()
{
  for (auto &stream : m_streams)
    delete stream.second;
  m_streams.clear();
}

int CInputStream::GetNrOfStreams() const
{
  return m_streams.size();
}

CDemuxStream* CInputStream::GetStream(int iStreamId)
{
  std::map<int, CDemuxStream*>::iterator it = m_streams.find(iStreamId);
  if (it != m_streams.end())
    return it->second;

  return nullptr;
}

std::vector<CDemuxStream*> CInputStream::GetStreams() const
{
  std::vector<CDemuxStream*> streams;

  for (auto &stream : m_streams)
  {
    streams.push_back(stream.second);
  }

  return streams;
}

void CInputStream::EnableStream(int iStreamId, bool enable)
{
  std::map<int, CDemuxStream*>::iterator it = m_streams.find(iStreamId);
  if (it == m_streams.end())
    return;

  m_struct.toAddon.EnableStream(m_addonInstance, it->second->uniqueId, enable);
}

DemuxPacket* CInputStream::ReadDemux()
{
  DemuxPacket* pPacket = m_struct.toAddon.DemuxRead(m_addonInstance);

  if (!pPacket)
  {
    return nullptr;
  }
  else if (pPacket->iStreamId == DMX_SPECIALID_STREAMINFO)
  {
    UpdateStreams();
  }
  else if (pPacket->iStreamId == DMX_SPECIALID_STREAMCHANGE)
  {
    UpdateStreams();
  }

  return pPacket;
}

bool CInputStream::SeekTime(double time, bool backward, double* startpts)
{
  return m_struct.toAddon.DemuxSeekTime(m_addonInstance, time, backward, startpts);
}

void CInputStream::AbortDemux()
{
  m_struct.toAddon.DemuxAbort(m_addonInstance);
}

void CInputStream::FlushDemux()
{
  m_struct.toAddon.DemuxFlush(m_addonInstance);
}

void CInputStream::SetSpeed(int iSpeed)
{
  m_struct.toAddon.DemuxSetSpeed(m_addonInstance, iSpeed);
}

int CInputStream::ReadStream(uint8_t* buf, unsigned int size)
{
  return m_struct.toAddon.ReadStream(m_addonInstance, buf, size);
}

int64_t CInputStream::SeekStream(int64_t offset, int whence)
{
  return m_struct.toAddon.SeekStream(m_addonInstance, offset, whence);
}

int64_t CInputStream::PositionStream()
{
  return m_struct.toAddon.PositionStream(m_addonInstance);
}

int64_t CInputStream::LengthStream()
{
  return m_struct.toAddon.LengthStream(m_addonInstance);
}

void CInputStream::PauseStream(double time)
{
  m_struct.toAddon.PauseStream(m_addonInstance, time);
}

bool CInputStream::IsRealTimeStream()
{
  return m_struct.toAddon.IsRealTimeStream(m_addonInstance);
}

void CInputStream::SetVideoResolution(int width, int height)
{
  m_struct.toAddon.SetVideoResolution(m_addonInstance, width, height);
}

} /*namespace ADDON*/

