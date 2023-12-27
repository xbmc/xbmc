/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InputStreamAddon.h"

#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/VideoCodec.h"
#include "cores/FFmpeg.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "cores/VideoPlayer/Interface/DemuxCrypto.h"
#include "cores/VideoPlayer/Interface/InputStreamConstants.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "windowing/Resolution.h"

#include <memory>

CInputStreamProvider::CInputStreamProvider(const ADDON::AddonInfoPtr& addonInfo,
                                           KODI_HANDLE parentInstance)
  : m_addonInfo(addonInfo), m_parentInstance(parentInstance)
{
}

void CInputStreamProvider::GetAddonInstance(INSTANCE_TYPE instance_type,
                                            ADDON::AddonInfoPtr& addonInfo,
                                            KODI_HANDLE& parentInstance)
{
  if (instance_type == ADDON::IAddonProvider::INSTANCE_VIDEOCODEC)
  {
    addonInfo = m_addonInfo;
    parentInstance = m_parentInstance;
  }
}

/*****************************************************************************************************************/

using namespace ADDON;

CInputStreamAddon::CInputStreamAddon(const AddonInfoPtr& addonInfo,
                                     IVideoPlayer* player,
                                     const CFileItem& fileitem,
                                     const std::string& instanceId)
  : IAddonInstanceHandler(
        ADDON_INSTANCE_INPUTSTREAM, addonInfo, ADDON_INSTANCE_ID_UNUSED, nullptr, instanceId),
    CDVDInputStream(DVDSTREAM_TYPE_ADDON, fileitem),
    m_player(player)
{
  std::string listitemprops =
      addonInfo->Type(AddonType::INPUTSTREAM)->GetValue("@listitemprops").asString();
  std::string name(addonInfo->ID());

  m_fileItemProps = StringUtils::Tokenize(listitemprops, "|");
  for (auto &key : m_fileItemProps)
  {
    StringUtils::Trim(key);
    key = name + "." + key;
  }
  m_caps = {};
}

CInputStreamAddon::~CInputStreamAddon()
{
  Close();
}

bool CInputStreamAddon::Supports(const AddonInfoPtr& addonInfo, const CFileItem& fileitem)
{
  /// @todo Error for users to show deprecation, can be removed in Kodi 20
  CVariant oldAddonProp = fileitem.GetProperty("inputstreamaddon");
  if (!oldAddonProp.isNull())
  {
    CLog::Log(LOGERROR,
              "CInputStreamAddon::{} - 'inputstreamaddon' has been deprecated, "
              "please use `#KODIPROP:inputstream={}` instead",
              __func__, oldAddonProp.asString());
  }

  // check if a specific inputstream addon is requested
  CVariant addon = fileitem.GetProperty(STREAM_PROPERTY_INPUTSTREAM);
  if (!addon.isNull())
    return (addon.asString() == addonInfo->ID());

  // check protocols
  std::string protocol = CURL(fileitem.GetDynPath()).GetProtocol();
  if (!protocol.empty())
  {
    std::string protocols =
        addonInfo->Type(AddonType::INPUTSTREAM)->GetValue("@protocols").asString();
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
    std::string extensions =
        addonInfo->Type(AddonType::INPUTSTREAM)->GetValue("@extension").asString();
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
  // Create "C" interface structures, used as own parts to prevent API problems on update
  m_ifc.inputstream = new AddonInstance_InputStream;
  m_ifc.inputstream->props = new AddonProps_InputStream();
  m_ifc.inputstream->toAddon = new KodiToAddonFuncTable_InputStream();
  m_ifc.inputstream->toKodi = new AddonToKodiFuncTable_InputStream();

  m_ifc.inputstream->toKodi->kodiInstance = this;
  m_ifc.inputstream->toKodi->free_demux_packet = cb_free_demux_packet;
  m_ifc.inputstream->toKodi->allocate_demux_packet = cb_allocate_demux_packet;
  m_ifc.inputstream->toKodi->allocate_encrypted_demux_packet = cb_allocate_encrypted_demux_packet;
  /*
  // Way to include part on new API version
  if (Addon()->GetTypeVersionDll(ADDON_TYPE::ADDON_INSTANCE_INPUTSTREAM) >= AddonVersion("3.0.0")) // Set the version to your new
  {

  }
  */
  if (CreateInstance() != ADDON_STATUS_OK || !m_ifc.inputstream->toAddon->open)
    return false;

  INPUTSTREAM_PROPERTY props = {};
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

    if (props.m_nCountInfoValues >= STREAM_MAX_PROPERTY_COUNT)
    {
      CLog::Log(LOGERROR,
                "CInputStreamAddon::{} - Hit max count of stream properties, "
                "have {}, actual count: {}",
                __func__, STREAM_MAX_PROPERTY_COUNT, propsMap.size());
      break;
    }
  }

  props.m_strURL = m_item.GetDynPath().c_str();
  props.m_mimeType = m_item.GetMimeType().c_str();

  std::string libFolder = URIUtils::GetDirectory(Addon()->Path());
  std::string profileFolder = CSpecialProtocol::TranslatePath(Addon()->Profile());
  props.m_libFolder = libFolder.c_str();
  props.m_profileFolder = profileFolder.c_str();

  DetectScreenResolution();

  bool ret = m_ifc.inputstream->toAddon->open(m_ifc.inputstream, &props);
  if (ret)
  {
    m_caps = {};
    m_ifc.inputstream->toAddon->get_capabilities(m_ifc.inputstream, &m_caps);

    m_subAddonProvider = std::make_shared<CInputStreamProvider>(
        GetAddonInfo(), m_ifc.inputstream->toAddon->addonInstance);
  }
  return ret;
}

void CInputStreamAddon::Close()
{
  if (m_ifc.inputstream->toAddon->close)
    m_ifc.inputstream->toAddon->close(m_ifc.inputstream);
  DestroyInstance();

  // Delete "C" interface structures
  delete m_ifc.inputstream->toAddon;
  delete m_ifc.inputstream->toKodi;
  delete m_ifc.inputstream->props;
  delete m_ifc.inputstream;
  m_ifc.inputstream = nullptr;
}

bool CInputStreamAddon::IsEOF()
{
  return false;
}

int CInputStreamAddon::Read(uint8_t* buf, int buf_size)
{
  if (!m_ifc.inputstream->toAddon->read_stream)
    return -1;

  return m_ifc.inputstream->toAddon->read_stream(m_ifc.inputstream, buf, buf_size);
}

int64_t CInputStreamAddon::Seek(int64_t offset, int whence)
{
  if (!m_ifc.inputstream->toAddon->seek_stream)
    return -1;

  return m_ifc.inputstream->toAddon->seek_stream(m_ifc.inputstream, offset, whence);
}

int64_t CInputStreamAddon::GetLength()
{
  if (!m_ifc.inputstream->toAddon->length_stream)
    return -1;

  return m_ifc.inputstream->toAddon->length_stream(m_ifc.inputstream);
}

int CInputStreamAddon::GetBlockSize()
{
  if (!m_ifc.inputstream->toAddon->block_size_stream)
    return 0;

  return m_ifc.inputstream->toAddon->block_size_stream(m_ifc.inputstream);
}

bool CInputStreamAddon::CanSeek()
{
  return (m_caps.m_mask & INPUTSTREAM_SUPPORTS_SEEK) != 0;
}

bool CInputStreamAddon::CanPause()
{
  return (m_caps.m_mask & INPUTSTREAM_SUPPORTS_PAUSE) != 0;
}

// IDisplayTime
CDVDInputStream::IDisplayTime* CInputStreamAddon::GetIDisplayTime()
{
  if ((m_caps.m_mask & INPUTSTREAM_SUPPORTS_IDISPLAYTIME) == 0)
    return nullptr;

  return this;
}

int CInputStreamAddon::GetTotalTime()
{
  if (!m_ifc.inputstream->toAddon->get_total_time)
    return 0;

  return m_ifc.inputstream->toAddon->get_total_time(m_ifc.inputstream);
}

int CInputStreamAddon::GetTime()
{
  if (!m_ifc.inputstream->toAddon->get_time)
    return 0;

  return m_ifc.inputstream->toAddon->get_time(m_ifc.inputstream);
}

// ITime
CDVDInputStream::ITimes* CInputStreamAddon::GetITimes()
{
  // Check if screen resolution is changed during playback
  // e.g. window resized and callback to add-on
  DetectScreenResolution();

  if ((m_caps.m_mask & INPUTSTREAM_SUPPORTS_ITIME) == 0)
    return nullptr;

  return this;
}

bool CInputStreamAddon::GetTimes(Times &times)
{
  if (!m_ifc.inputstream->toAddon->get_times)
    return false;

  INPUTSTREAM_TIMES i_times;

  if (m_ifc.inputstream->toAddon->get_times(m_ifc.inputstream, &i_times))
  {
    times.ptsBegin = i_times.ptsBegin;
    times.ptsEnd = i_times.ptsEnd;
    times.ptsStart = i_times.ptsStart;
    times.startTime = i_times.startTime;
    return true;
  }
  return false;
}

// IPosTime
CDVDInputStream::IPosTime* CInputStreamAddon::GetIPosTime()
{
  if ((m_caps.m_mask & INPUTSTREAM_SUPPORTS_IPOSTIME) == 0)
    return nullptr;

  return this;
}

bool CInputStreamAddon::PosTime(int ms)
{
  if (!m_ifc.inputstream->toAddon->pos_time)
    return false;

  return m_ifc.inputstream->toAddon->pos_time(m_ifc.inputstream, ms);
}

// IDemux
CDVDInputStream::IDemux* CInputStreamAddon::GetIDemux()
{
  if ((m_caps.m_mask & INPUTSTREAM_SUPPORTS_IDEMUX) == 0)
    return nullptr;

  return this;
}

bool CInputStreamAddon::OpenDemux()
{
  if ((m_caps.m_mask & INPUTSTREAM_SUPPORTS_IDEMUX) != 0)
    return true;
  else
    return false;
}

DemuxPacket* CInputStreamAddon::ReadDemux()
{
  if (!m_ifc.inputstream->toAddon->demux_read)
    return nullptr;

  return reinterpret_cast<DemuxPacket*>(m_ifc.inputstream->toAddon->demux_read(m_ifc.inputstream));
}

std::vector<CDemuxStream*> CInputStreamAddon::GetStreams() const
{
  std::vector<CDemuxStream*> streams;

  INPUTSTREAM_IDS streamIDs = {};
  bool ret = m_ifc.inputstream->toAddon->get_stream_ids(m_ifc.inputstream, &streamIDs);
  if (!ret || streamIDs.m_streamCount > INPUTSTREAM_MAX_STREAM_COUNT)
    return streams;

  for (unsigned int i = 0; i < streamIDs.m_streamCount; ++i)
    if (CDemuxStream* stream = GetStream(streamIDs.m_streamIds[i]))
      streams.push_back(stream);

  return streams;
}

CDemuxStream* CInputStreamAddon::GetStream(int streamId) const
{
  INPUTSTREAM_INFO stream{};
  KODI_HANDLE demuxStream = nullptr;
  bool ret = m_ifc.inputstream->toAddon->get_stream(m_ifc.inputstream, streamId, &stream,
                                                    &demuxStream, cb_get_stream_transfer);
  if (!ret || stream.m_streamType == INPUTSTREAM_TYPE_NONE)
    return nullptr;

  return static_cast<CDemuxStream*>(demuxStream);
}

KODI_HANDLE CInputStreamAddon::cb_get_stream_transfer(KODI_HANDLE handle,
                                                      int streamId,
                                                      INPUTSTREAM_INFO* stream)
{
  CInputStreamAddon* thisClass = static_cast<CInputStreamAddon*>(handle);
  if (!thisClass || !stream)
    return nullptr;

  std::string codecName(stream->m_codecName);
  const AVCodec* codec = nullptr;

  if (stream->m_streamType != INPUTSTREAM_TYPE_TELETEXT &&
      stream->m_streamType != INPUTSTREAM_TYPE_RDS && stream->m_streamType != INPUTSTREAM_TYPE_ID3)
  {
    StringUtils::ToLower(codecName);
    codec = avcodec_find_decoder_by_name(codecName.c_str());
    if (!codec)
      return nullptr;
  }

  CDemuxStream* demuxStream;

  if (stream->m_streamType == INPUTSTREAM_TYPE_AUDIO)
  {
    CDemuxStreamAudio *audioStream = new CDemuxStreamAudio();

    audioStream->iChannels = stream->m_Channels;
    audioStream->iSampleRate = stream->m_SampleRate;
    audioStream->iBlockAlign = stream->m_BlockAlign;
    audioStream->iBitRate = stream->m_BitRate;
    audioStream->iBitsPerSample = stream->m_BitsPerSample;
    audioStream->profile = ConvertAudioCodecProfile(stream->m_codecProfile);
    demuxStream = audioStream;
  }
  else if (stream->m_streamType == INPUTSTREAM_TYPE_VIDEO)
  {
    CDemuxStreamVideo *videoStream = new CDemuxStreamVideo();

    videoStream->iFpsScale = stream->m_FpsScale;
    videoStream->iFpsRate = stream->m_FpsRate;
    videoStream->iWidth = stream->m_Width;
    videoStream->iHeight = stream->m_Height;
    videoStream->fAspect = static_cast<double>(stream->m_Aspect);
    videoStream->iBitRate = stream->m_BitRate;
    videoStream->profile = ConvertVideoCodecProfile(stream->m_codecProfile);

    /*! Added on API version 2.0.8 */
    //@{
    videoStream->colorSpace = static_cast<AVColorSpace>(stream->m_colorSpace);
    videoStream->colorRange = static_cast<AVColorRange>(stream->m_colorRange);
    //@}

    /*! Added on API version 2.0.9 */
    //@{
    videoStream->colorPrimaries = static_cast<AVColorPrimaries>(stream->m_colorPrimaries);
    videoStream->colorTransferCharacteristic =
        static_cast<AVColorTransferCharacteristic>(stream->m_colorTransferCharacteristic);

    if (stream->m_masteringMetadata)
    {
      videoStream->masteringMetaData = std::make_shared<AVMasteringDisplayMetadata>();
      videoStream->masteringMetaData->display_primaries[0][0] =
          av_d2q(stream->m_masteringMetadata->primary_r_chromaticity_x, INT_MAX);
      videoStream->masteringMetaData->display_primaries[0][1] =
          av_d2q(stream->m_masteringMetadata->primary_r_chromaticity_y, INT_MAX);
      videoStream->masteringMetaData->display_primaries[1][0] =
          av_d2q(stream->m_masteringMetadata->primary_g_chromaticity_x, INT_MAX);
      videoStream->masteringMetaData->display_primaries[1][1] =
          av_d2q(stream->m_masteringMetadata->primary_g_chromaticity_y, INT_MAX);
      videoStream->masteringMetaData->display_primaries[2][0] =
          av_d2q(stream->m_masteringMetadata->primary_b_chromaticity_x, INT_MAX);
      videoStream->masteringMetaData->display_primaries[2][1] =
          av_d2q(stream->m_masteringMetadata->primary_b_chromaticity_y, INT_MAX);
      videoStream->masteringMetaData->white_point[0] =
          av_d2q(stream->m_masteringMetadata->white_point_chromaticity_x, INT_MAX);
      videoStream->masteringMetaData->white_point[1] =
          av_d2q(stream->m_masteringMetadata->white_point_chromaticity_y, INT_MAX);
      videoStream->masteringMetaData->min_luminance =
          av_d2q(stream->m_masteringMetadata->luminance_min, INT_MAX);
      videoStream->masteringMetaData->max_luminance =
          av_d2q(stream->m_masteringMetadata->luminance_max, INT_MAX);
      videoStream->masteringMetaData->has_luminance =
          videoStream->masteringMetaData->has_primaries = 1;
    }

    if (stream->m_contentLightMetadata)
    {
      videoStream->contentLightMetaData = std::make_shared<AVContentLightMetadata>();
      videoStream->contentLightMetaData->MaxCLL =
          static_cast<unsigned>(stream->m_contentLightMetadata->max_cll);
      videoStream->contentLightMetaData->MaxFALL =
          static_cast<unsigned>(stream->m_contentLightMetadata->max_fall);
    }
    //@}

    /*
    // Way to include part on new API version
    if (Addon()->GetTypeVersionDll(ADDON_TYPE::ADDON_INSTANCE_INPUTSTREAM) >= AddonVersion("3.0.0")) // Set the version to your new
    {

    }
    */

    demuxStream = videoStream;
  }
  else if (stream->m_streamType == INPUTSTREAM_TYPE_SUBTITLE)
  {
    CDemuxStreamSubtitle *subtitleStream = new CDemuxStreamSubtitle();
    demuxStream = subtitleStream;
  }
  else if (stream->m_streamType == INPUTSTREAM_TYPE_TELETEXT)
  {
    CDemuxStreamTeletext* teletextStream = new CDemuxStreamTeletext();
    demuxStream = teletextStream;
  }
  else if (stream->m_streamType == INPUTSTREAM_TYPE_RDS)
  {
    CDemuxStreamRadioRDS* rdsStream = new CDemuxStreamRadioRDS();
    demuxStream = rdsStream;
  }
  else if (stream->m_streamType == INPUTSTREAM_TYPE_ID3)
  {
    CDemuxStreamAudioID3* id3Stream = new CDemuxStreamAudioID3();
    demuxStream = id3Stream;
  }
  else
    return nullptr;

  demuxStream->name = stream->m_name;
  if (codec)
    demuxStream->codec = codec->id;
  else
    demuxStream->codec = AV_CODEC_ID_DVB_TELETEXT;
  demuxStream->codecName = stream->m_codecInternalName;
  demuxStream->uniqueId = streamId;
  demuxStream->flags = static_cast<StreamFlags>(stream->m_flags);
  demuxStream->language = stream->m_language;

  if (thisClass->GetAddonInfo()->DependencyVersion(ADDON_INSTANCE_VERSION_INPUTSTREAM_XML_ID) >=
      CAddonVersion("2.0.8"))
  {
    demuxStream->codec_fourcc = stream->m_codecFourCC;
  }

  if (stream->m_ExtraData && stream->m_ExtraSize)
  {
    demuxStream->extraData = FFmpegExtraData(stream->m_ExtraData, stream->m_ExtraSize);
  }

  if (stream->m_cryptoSession.keySystem != STREAM_CRYPTO_KEY_SYSTEM_NONE &&
      stream->m_cryptoSession.keySystem < STREAM_CRYPTO_KEY_SYSTEM_COUNT)
  {
    static const CryptoSessionSystem map[] = {
        CRYPTO_SESSION_SYSTEM_NONE,      CRYPTO_SESSION_SYSTEM_WIDEVINE,
        CRYPTO_SESSION_SYSTEM_PLAYREADY, CRYPTO_SESSION_SYSTEM_WISEPLAY,
        CRYPTO_SESSION_SYSTEM_CLEARKEY,
    };
    demuxStream->cryptoSession = std::make_shared<DemuxCryptoSession>(
        map[stream->m_cryptoSession.keySystem], stream->m_cryptoSession.sessionId,
        stream->m_cryptoSession.flags);

    if ((stream->m_features & INPUTSTREAM_FEATURE_DECODE) != 0)
      demuxStream->externalInterfaces = thisClass->m_subAddonProvider;
  }

  // Tie the lifetime of the stream to the CInputStreamAddon
  thisClass->m_streams.emplace_back(demuxStream);

  return demuxStream;
}

void CInputStreamAddon::EnableStream(int streamId, bool enable)
{
  if (!m_ifc.inputstream->toAddon->enable_stream)
    return;

  m_ifc.inputstream->toAddon->enable_stream(m_ifc.inputstream, streamId, enable);
}

bool CInputStreamAddon::OpenStream(int streamId)
{
  if (!m_ifc.inputstream->toAddon->open_stream)
    return false;

  return m_ifc.inputstream->toAddon->open_stream(m_ifc.inputstream, streamId);
}

int CInputStreamAddon::GetNrOfStreams() const
{
  return m_streamCount;
}

void CInputStreamAddon::SetSpeed(int speed)
{
  if (!m_ifc.inputstream->toAddon->demux_set_speed)
    return;

  m_ifc.inputstream->toAddon->demux_set_speed(m_ifc.inputstream, speed);
}

bool CInputStreamAddon::SeekTime(double time, bool backward, double* startpts)
{
  if (!m_ifc.inputstream->toAddon->demux_seek_time)
    return false;

  if ((m_caps.m_mask & INPUTSTREAM_SUPPORTS_IPOSTIME) != 0)
  {
    if (!PosTime(static_cast<int>(time)))
      return false;

    FlushDemux();

    if(startpts)
      *startpts = DVD_NOPTS_VALUE;
    return true;
  }

  return m_ifc.inputstream->toAddon->demux_seek_time(m_ifc.inputstream, time, backward, startpts);
}

void CInputStreamAddon::AbortDemux()
{
  if (m_ifc.inputstream->toAddon->demux_abort)
    m_ifc.inputstream->toAddon->demux_abort(m_ifc.inputstream);
}

void CInputStreamAddon::FlushDemux()
{
  if (m_ifc.inputstream->toAddon->demux_flush)
    m_ifc.inputstream->toAddon->demux_flush(m_ifc.inputstream);
}

void CInputStreamAddon::SetVideoResolution(unsigned int width,
                                           unsigned int height,
                                           unsigned int maxWidth,
                                           unsigned int maxHeight)
{
  if (m_ifc.inputstream->toAddon->set_video_resolution)
    m_ifc.inputstream->toAddon->set_video_resolution(m_ifc.inputstream, width, height, maxWidth,
                                                     maxHeight);
}

bool CInputStreamAddon::IsRealtime()
{
  if (m_ifc.inputstream->toAddon->is_real_time_stream)
    return m_ifc.inputstream->toAddon->is_real_time_stream(m_ifc.inputstream);
  return false;
}


// IChapter
CDVDInputStream::IChapter* CInputStreamAddon::GetIChapter()
{
  if ((m_caps.m_mask & INPUTSTREAM_SUPPORTS_ICHAPTER) == 0)
    return nullptr;

  return this;
}

int CInputStreamAddon::GetChapter()
{
  if (m_ifc.inputstream->toAddon->get_chapter)
    return m_ifc.inputstream->toAddon->get_chapter(m_ifc.inputstream);

  return -1;
}

int CInputStreamAddon::GetChapterCount()
{
  if (m_ifc.inputstream->toAddon->get_chapter_count)
    return m_ifc.inputstream->toAddon->get_chapter_count(m_ifc.inputstream);

  return 0;
}

void CInputStreamAddon::GetChapterName(std::string& name, int ch)
{
  name.clear();
  if (m_ifc.inputstream->toAddon->get_chapter_name)
  {
    const char* res = m_ifc.inputstream->toAddon->get_chapter_name(m_ifc.inputstream, ch);
    if (res)
      name = res;
  }
}

int64_t CInputStreamAddon::GetChapterPos(int ch)
{
  if (m_ifc.inputstream->toAddon->get_chapter_pos)
    return m_ifc.inputstream->toAddon->get_chapter_pos(m_ifc.inputstream, ch);

  return 0;
}

bool CInputStreamAddon::SeekChapter(int ch)
{
  if (m_ifc.inputstream->toAddon->seek_chapter)
    return m_ifc.inputstream->toAddon->seek_chapter(m_ifc.inputstream, ch);

  return false;
}

int CInputStreamAddon::ConvertVideoCodecProfile(STREAMCODEC_PROFILE profile)
{
  switch (profile)
  {
  case H264CodecProfileBaseline:
    return FF_PROFILE_H264_BASELINE;
  case H264CodecProfileMain:
    return FF_PROFILE_H264_MAIN;
  case H264CodecProfileExtended:
    return FF_PROFILE_H264_EXTENDED;
  case H264CodecProfileHigh:
    return FF_PROFILE_H264_HIGH;
  case H264CodecProfileHigh10:
    return FF_PROFILE_H264_HIGH_10;
  case H264CodecProfileHigh422:
    return FF_PROFILE_H264_HIGH_422;
  case H264CodecProfileHigh444Predictive:
    return FF_PROFILE_H264_HIGH_444_PREDICTIVE;
  case VP9CodecProfile0:
    return FF_PROFILE_VP9_0;
  case VP9CodecProfile1:
    return FF_PROFILE_VP9_1;
  case VP9CodecProfile2:
    return FF_PROFILE_VP9_2;
  case VP9CodecProfile3:
    return FF_PROFILE_VP9_3;
  case AV1CodecProfileMain:
    return FF_PROFILE_AV1_MAIN;
  case AV1CodecProfileHigh:
    return FF_PROFILE_AV1_HIGH;
  case AV1CodecProfileProfessional:
    return FF_PROFILE_AV1_PROFESSIONAL;
  default:
    return FF_PROFILE_UNKNOWN;
  }
}

int CInputStreamAddon::ConvertAudioCodecProfile(STREAMCODEC_PROFILE profile)
{
  switch (profile)
  {
    case AACCodecProfileMAIN:
      return FF_PROFILE_AAC_MAIN;
    case AACCodecProfileLOW:
      return FF_PROFILE_AAC_LOW;
    case AACCodecProfileSSR:
      return FF_PROFILE_AAC_SSR;
    case AACCodecProfileLTP:
      return FF_PROFILE_AAC_LTP;
    case AACCodecProfileHE:
      return FF_PROFILE_AAC_HE;
    case AACCodecProfileHEV2:
      return FF_PROFILE_AAC_HE_V2;
    case AACCodecProfileLD:
      return FF_PROFILE_AAC_LD;
    case AACCodecProfileELD:
      return FF_PROFILE_AAC_ELD;
    case MPEG2AACCodecProfileLOW:
      return FF_PROFILE_MPEG2_AAC_LOW;
    case MPEG2AACCodecProfileHE:
      return FF_PROFILE_MPEG2_AAC_HE;
    case DTSCodecProfile:
      return FF_PROFILE_DTS;
    case DTSCodecProfileES:
      return FF_PROFILE_DTS_ES;
    case DTSCodecProfile9624:
      return FF_PROFILE_DTS_96_24;
    case DTSCodecProfileHDHRA:
      return FF_PROFILE_DTS_HD_HRA;
    case DTSCodecProfileHDMA:
      return FF_PROFILE_DTS_HD_MA;
    case DTSCodecProfileHDExpress:
      return FF_PROFILE_DTS_EXPRESS;
    case DTSCodecProfileHDMAX:
      //! @todo: with ffmpeg >= 6.1 set the appropriate profile
      return FF_PROFILE_UNKNOWN; // FF_PROFILE_DTS_HD_MA_X
    case DTSCodecProfileHDMAIMAX:
      //! @todo: with ffmpeg >= 6.1 set the appropriate profile
      return FF_PROFILE_UNKNOWN; // FF_PROFILE_DTS_HD_MA_X_IMAX
    case DDPlusCodecProfileAtmos:
      //! @todo: with ffmpeg >= 6.1 set the appropriate profile
      return FF_PROFILE_UNKNOWN; // FF_PROFILE_EAC3_DDP_ATMOS
    default:
      return FF_PROFILE_UNKNOWN;
  }
}

void CInputStreamAddon::DetectScreenResolution()
{
  unsigned int videoWidth{1280};
  unsigned int videoHeight{720};
  if (m_player)
  {
    m_player->GetVideoResolution(videoWidth, videoHeight);
  }
  if (m_currentVideoWidth != videoWidth || m_currentVideoHeight != videoHeight)
  {
    unsigned int maxWidth{videoWidth};
    unsigned int maxHeight{videoHeight};
    // For Adaptive stream technology is needed to know the screen resolution
    // one parameter used to fit stream resolution to screen resolution.
    // Currently we provide current GUI resolution, but if Adjust refresh rate
    // is enabled the GUI resolution is no longer relevant, Adjust refresh rate
    // will change screen resolution based on whitelist (if any) and only after
    // that have the video stream in the demuxer, therefore will fail because
    // the addon has as reference the GUI resolution.
    // So we have to provide the max resolution info before the playback take place
    // in order to allow addon to provide in the demuxer the best stream resolution
    // that can fit the supported screen resolution (changed when playback start).
    CResolutionUtils::GetMaxAllowedScreenResolution(maxWidth, maxHeight);

    SetVideoResolution(videoWidth, videoHeight, maxWidth, maxHeight);

    m_currentVideoWidth = videoWidth;
    m_currentVideoHeight = videoHeight;
  }
}

/*!
 * Callbacks from add-on to kodi
 */
//@{
DEMUX_PACKET* CInputStreamAddon::cb_allocate_demux_packet(void* kodiInstance, int data_size)
{
  return CDVDDemuxUtils::AllocateDemuxPacket(data_size);
}

DEMUX_PACKET* CInputStreamAddon::cb_allocate_encrypted_demux_packet(
    void* kodiInstance, unsigned int dataSize, unsigned int encryptedSubsampleCount)
{
  return CDVDDemuxUtils::AllocateDemuxPacket(dataSize, encryptedSubsampleCount);
}

void CInputStreamAddon::cb_free_demux_packet(void* kodiInstance, DEMUX_PACKET* packet)
{
  CDVDDemuxUtils::FreeDemuxPacket(static_cast<DemuxPacket*>(packet));
}

//@}
