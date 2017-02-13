/*
 *      Copyright (C) 2015 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "InputStreamAddon.h"

#include "FileItem.h"
#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/VideoCodec.h"
#include "cores/VideoPlayer/DVDClock.h"
#include "cores/VideoPlayer/DVDDemuxers/DemuxCrypto.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

CInputStreamProvider::CInputStreamProvider(ADDON::AddonInfoPtr addonInfo, kodi::addon::IAddonInstance* parentInstance)
  : m_addonInfo(addonInfo),
  m_parentInstance(parentInstance)
{

}

void CInputStreamProvider::getAddonInstance(INSTANCE_TYPE instance_type, ADDON::AddonInfoPtr& addonInfo, kodi::addon::IAddonInstance*& parentInstance)
{
  if (instance_type == ADDON::CAddonProvider::INSTANCE_VIDEOCODEC)
  {
    addonInfo = m_addonInfo;
    parentInstance = m_parentInstance;
  }
  return;
}

/*****************************************************************************************************************/

using namespace ADDON;

CInputStreamAddon::CInputStreamAddon(ADDON::AddonInfoPtr addonInfo, IVideoPlayer* player, const CFileItem& fileitem)
  : CDVDInputStream(DVDSTREAM_TYPE_ADDON, fileitem)
  , IAddonInstanceHandler(ADDON_INPUTSTREAM, addonInfo)
  , m_player(player)
{
  std::string listitemprops = addonInfo->Type(ADDON_INPUTSTREAM)->GetValue("@listitemprops").asString();
  std::string name(addonInfo->ID());

  m_fileItemProps = StringUtils::Tokenize(listitemprops, "|");
  for (auto &key : m_fileItemProps)
  {
    StringUtils::Trim(key);
    key = name + "." + key;
  }
  m_caps.m_mask = 0;
  memset(&m_struct, 0, sizeof(m_struct));
}

CInputStreamAddon::~CInputStreamAddon()
{
  Close();
}

bool CInputStreamAddon::Supports(AddonInfoPtr& addonInfo, const CFileItem &fileitem)
{
  // check if a specific inputstream addon is requested
  CVariant addon = fileitem.GetProperty("inputstreamaddon");
  if (!addon.isNull())
  {
    if (addon.asString() != addonInfo->ID())
      return false;
    else
      return true;
  }
  
  // check protocols
  std::string protocol = fileitem.GetURL().GetProtocol();
  if (!protocol.empty())
  {
    std::string protocols = addonInfo->Type(ADDON_INPUTSTREAM)->GetValue("@protocols").asString();
    if (!protocols.empty())
    {
      std::vector<std::string> protocolsList = StringUtils::Tokenize(protocols, "|");
      for (auto& value : protocolsList)
      {
        StringUtils::Trim(value);
        if (value == protocol)
          return true;
      }
    }
  }

  std::string filetype = fileitem.GetURL().GetFileType();
  if (!filetype.empty())
  {
    std::string extensions = addonInfo->Type(ADDON_INPUTSTREAM)->GetValue("@extension").asString();
    if (!extensions.empty())
    {
      std::vector<std::string> extensionsList = StringUtils::Tokenize(extensions, "|");
      for (auto& value : extensionsList)
      {
        StringUtils::Trim(value);
        if (value == filetype)
          return true;
      }
    }
  }

  return false;
}

bool CInputStreamAddon::Open()
{
  m_struct.toKodi.kodiInstance = this;
  m_struct.toKodi.FreeDemuxPacket = InputStreamFreeDemuxPacket;
  m_struct.toKodi.AllocateDemuxPacket = InputStreamAllocateDemuxPacket;
  m_struct.toKodi.AllocateEncryptedDemuxPacket = InputStreamAllocateEncryptedDemuxPacket;
  if (!CreateInstance(ADDON_INSTANCE_INPUTSTREAM, UUID(), &m_struct, reinterpret_cast<KODI_HANDLE*>(&m_addonInstance)) || !m_struct.toAddon.Open)
    return false;

  INPUTSTREAM props;
  std::map<std::string, std::string> propsMap;
  for (auto &key : m_fileItemProps)
  {
    if (m_item.GetProperty(key).isNull())
      continue;
    propsMap[key] = m_item.GetProperty(key).asString();
  }

  props.m_nCountInfoValues = 0;
  for (auto &pair : propsMap)
  {
    props.m_ListItemProperties[props.m_nCountInfoValues].m_strKey = pair.first.c_str();
    props.m_ListItemProperties[props.m_nCountInfoValues].m_strValue = pair.second.c_str();
    props.m_nCountInfoValues++;
  }

  props.m_strURL = m_item.GetPath().c_str();
  
  std::string libFolder = URIUtils::GetDirectory(Addon()->MainLibPath());
  std::string profileFolder = CSpecialProtocol::TranslatePath(Addon()->Profile());
  props.m_libFolder = libFolder.c_str();
  props.m_profileFolder = profileFolder.c_str();

  bool ret = m_struct.toAddon.Open(m_addonInstance, props);
  if (ret)
  {
    unsigned int videoWidth = 1280;
    unsigned int videoHeight = 720;
    if (m_player)
      m_player->GetVideoResolution(videoWidth, videoHeight);
    SetVideoResolution(videoWidth, videoHeight);

    memset(&m_caps, 0, sizeof(m_caps));
    m_struct.toAddon.GetCapabilities(m_addonInstance, &m_caps);
  }

  UpdateStreams();
  return ret;
}

void CInputStreamAddon::Close()
{
  if (m_struct.toAddon.Close)
    m_struct.toAddon.Close(m_addonInstance);
  DestroyInstance(UUID());
  memset(&m_struct, 0, sizeof(m_struct));
}

bool CInputStreamAddon::IsEOF()
{
  return false;
}

int CInputStreamAddon::Read(uint8_t* buf, int buf_size)
{
  if (!m_struct.toAddon.ReadStream)
    return -1;

  return m_struct.toAddon.ReadStream(m_addonInstance, buf, buf_size);
}

int64_t CInputStreamAddon::Seek(int64_t offset, int whence)
{
  if (!m_struct.toAddon.SeekStream)
    return -1;

  return m_struct.toAddon.SeekStream(m_addonInstance, offset, whence);
}

int64_t CInputStreamAddon::PositionStream()
{
  if (!m_struct.toAddon.PositionStream)
    return -1;

  return m_struct.toAddon.PositionStream(m_addonInstance);
}

int64_t CInputStreamAddon::GetLength()
{
  if (!m_struct.toAddon.LengthStream)
    return -1;

  return m_struct.toAddon.LengthStream(m_addonInstance);
}

bool CInputStreamAddon::Pause(double dTime)
{
  if (!m_struct.toAddon.PauseStream)
    return false;

  m_struct.toAddon.PauseStream(m_addonInstance, dTime);
  return true;
}

bool CInputStreamAddon::CanSeek()
{
  return (m_caps.m_mask & INPUTSTREAM_CAPABILITIES::SUPPORTSSEEK) != 0;
}

bool CInputStreamAddon::CanPause()
{
  return (m_caps.m_mask & INPUTSTREAM_CAPABILITIES::SUPPORTSPAUSE) != 0;
}

// IDisplayTime
CDVDInputStream::IDisplayTime* CInputStreamAddon::GetIDisplayTime()
{
  if ((m_caps.m_mask & INPUTSTREAM_CAPABILITIES::SUPPORTSIDISPLAYTIME) == 0)
    return nullptr;

  return this;
}

int CInputStreamAddon::GetTotalTime()
{
  if (!m_struct.toAddon.GetTotalTime)
    return 0;

  return m_struct.toAddon.GetTotalTime(m_addonInstance);
}

int CInputStreamAddon::GetTime()
{
  if (!m_struct.toAddon.GetTime)
    return 0;

  return m_struct.toAddon.GetTime(m_addonInstance);
}

// IPosTime
CDVDInputStream::IPosTime* CInputStreamAddon::GetIPosTime()
{
  if ((m_caps.m_mask & INPUTSTREAM_CAPABILITIES::SUPPORTSIPOSTIME) == 0)
    return nullptr;

  return this;
}

bool CInputStreamAddon::PosTime(int ms)
{
  if (!m_struct.toAddon.PosTime)
    return false;

  return m_struct.toAddon.PosTime(m_addonInstance, ms);
}

// IDemux
CDVDInputStream::IDemux* CInputStreamAddon::GetIDemux()
{
  if ((m_caps.m_mask & INPUTSTREAM_CAPABILITIES::SUPPORTSIDEMUX) == 0)
    return nullptr;

  return this;
}

bool CInputStreamAddon::OpenDemux()
{
  if ((m_caps.m_mask & INPUTSTREAM_CAPABILITIES::SUPPORTSIDEMUX) != 0)
    return true;
  else
    return false;
}

DemuxPacket* CInputStreamAddon::ReadDemux()
{
  if (!m_struct.toAddon.DemuxRead)
    return nullptr;

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

std::vector<CDemuxStream*> CInputStreamAddon::GetStreams() const
{
  std::vector<CDemuxStream*> streams;

  for (auto& stream : m_streams)
    streams.push_back(stream.second);

  return streams;
}

CDemuxStream* CInputStreamAddon::GetStream(int iStreamId) const
{
  std::map<int, CDemuxStream*>::const_iterator it = m_streams.find(iStreamId);
  if (it == m_streams.end())
    return nullptr;

  return it->second;
}

void CInputStreamAddon::EnableStream(int iStreamId, bool enable)
{
  if (!m_struct.toAddon.EnableStream)
    return;

  std::map<int, CDemuxStream*>::iterator it = m_streams.find(iStreamId);
  if (it == m_streams.end())
    return;

  m_struct.toAddon.EnableStream(m_addonInstance, it->second->uniqueId, enable);
}

int CInputStreamAddon::GetNrOfStreams() const
{
  return m_streams.size();
}

void CInputStreamAddon::SetSpeed(int iSpeed)
{
  if (!m_struct.toAddon.DemuxSetSpeed)
    return;

  m_struct.toAddon.DemuxSetSpeed(m_addonInstance, iSpeed);
}

bool CInputStreamAddon::SeekTime(double time, bool backward, double* startpts)
{
  if (!m_struct.toAddon.DemuxSeekTime)
    return false;

  if ((m_caps.m_mask & INPUTSTREAM_CAPABILITIES::SUPPORTSIPOSTIME) != 0)
  {
    if (!PosTime(time))
      return false;

    FlushDemux();

    if(startpts)
      *startpts = DVD_NOPTS_VALUE;
    return true;
  }

  return m_struct.toAddon.DemuxSeekTime(m_addonInstance, time, backward, startpts);
}

void CInputStreamAddon::AbortDemux()
{
  if (!m_struct.toAddon.DemuxAbort)
    return;

  m_struct.toAddon.DemuxAbort(m_addonInstance);
}

void CInputStreamAddon::FlushDemux()
{
  if (m_struct.toAddon.DemuxFlush)
    return;

  m_struct.toAddon.DemuxFlush(m_addonInstance);
}

void CInputStreamAddon::SetVideoResolution(int width, int height)
{
  if (m_struct.toAddon.SetVideoResolution)
    m_struct.toAddon.SetVideoResolution(m_addonInstance, width, height);
}

bool CInputStreamAddon::IsRealTimeStream()
{
  if (m_struct.toAddon.IsRealTimeStream)
    return m_struct.toAddon.IsRealTimeStream(m_addonInstance);
  return false;
}

// IDemux
void CInputStreamAddon::UpdateStreams()
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

    if (stream.m_cryptoInfo.m_CryptoKeySystem != CRYPTO_INFO::CRYPTO_KEY_SYSTEM_NONE &&
      stream.m_cryptoInfo.m_CryptoKeySystem < CRYPTO_INFO::CRYPTO_KEY_SYSTEM_COUNT)
    {
      static const CryptoSessionSystem map[] =
      {
        CRYPTO_SESSION_SYSTEM_NONE,
        CRYPTO_SESSION_SYSTEM_WIDEVINE,
        CRYPTO_SESSION_SYSTEM_PLAYREADY
      };
      demuxStream->cryptoSession = std::shared_ptr<DemuxCryptoSession>(new DemuxCryptoSession(
        map[stream.m_cryptoInfo.m_CryptoKeySystem], stream.m_cryptoInfo.m_CryptoSessionIdSize, stream.m_cryptoInfo.m_CryptoSessionId));

      if ((m_caps.m_mask & INPUTSTREAM_CAPABILITIES::SUPPORTSDECODE) != 0)
      {
        if (!m_subAddonProvider)
          m_subAddonProvider = std::shared_ptr<CInputStreamProvider>(new CInputStreamProvider(AddonInfo(), m_addonInstance));

        demuxStream->externalInterfaces = m_subAddonProvider;
      }
    }

    m_streams[demuxStream->uniqueId] = demuxStream;
  }
}

void CInputStreamAddon::DisposeStreams()
{
  for (auto &stream : m_streams)
    delete stream.second;
  m_streams.clear();
}

/*!
  * Callbacks from add-on to kodi
  */
//@{
void CInputStreamAddon::InputStreamFreeDemuxPacket(void* kodiInstanceBase, DemuxPacket* pPacket)
{
  CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

DemuxPacket* CInputStreamAddon::InputStreamAllocateDemuxPacket(void* kodiInstanceBase, int iDataSize)
{
  return CDVDDemuxUtils::AllocateDemuxPacket(iDataSize);
}

DemuxPacket* CInputStreamAddon::InputStreamAllocateEncryptedDemuxPacket(void* kodiInstanceBase, unsigned int iDataSize, unsigned int encryptedSubsampleCount)
{
  return CDVDDemuxUtils::AllocateDemuxPacket(iDataSize, encryptedSubsampleCount);
}

//@}
