/*
 *      Copyright (C) 2017 Team Kodi
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

#include "AddonVideoCodec.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/VideoPlayer/DVDDemuxers/DemuxCrypto.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecs.h"
#include "utils/log.h"

using namespace ADDON;

CAddonVideoCodec::CAddonVideoCodec(CProcessInfo &processInfo, ADDON::AddonInfoPtr& addonInfo, kodi::addon::IAddonInstance* parentInstance)
  : CDVDVideoCodec(processInfo),
  IAddonInstanceHandler(ADDON::ADDON_VIDEOCODEC, addonInfo),
  m_parentInstance(parentInstance)
{
  memset(&m_struct, 0, sizeof(m_struct));
  m_struct.toKodi.kodiInstance = this;
  if (!CreateInstance(ADDON_INSTANCE_VIDEOCODEC, UUID(), &m_struct, reinterpret_cast<KODI_HANDLE*>(&m_addonInstance), m_parentInstance) || !m_struct.toAddon.Open)
  {
    CLog::Log(LOGERROR, "CAddonVideoCodec: Failed to create add-on instance for '%s'", addonInfo->ID().c_str());
    return;
  }
}

CAddonVideoCodec::~CAddonVideoCodec()
{
  DestroyInstance(UUID());
}

bool CAddonVideoCodec::CopyToInitData(VIDEOCODEC_INITDATA &initData, CDVDStreamInfo &hints)
{
  initData.codecProfile = VIDEOCODEC_INITDATA::CodecProfileNotNeeded;
  switch (hints.codec)
  {
  case AV_CODEC_ID_H264:
    initData.codec = VIDEOCODEC_INITDATA::CodecH264;
    switch (hints.profile)
    {
    case 0:
      initData.codecProfile = VIDEOCODEC_INITDATA::CodecProfileUnknown;
      break;
    case FF_PROFILE_H264_BASELINE:
      initData.codecProfile = VIDEOCODEC_INITDATA::H264CodecProfileBaseline;
      break;
    case FF_PROFILE_H264_MAIN:
      initData.codecProfile = VIDEOCODEC_INITDATA::H264CodecProfileMain;
      break;
    case FF_PROFILE_H264_EXTENDED:
      initData.codecProfile = VIDEOCODEC_INITDATA::H264CodecProfileExtended;
      break;
    case FF_PROFILE_H264_HIGH:
      initData.codecProfile = VIDEOCODEC_INITDATA::H264CodecProfileHigh;
      break;
    case FF_PROFILE_H264_HIGH_10:
      initData.codecProfile = VIDEOCODEC_INITDATA::H264CodecProfileHigh10;
      break;
    case FF_PROFILE_H264_HIGH_422:
      initData.codecProfile = VIDEOCODEC_INITDATA::H264CodecProfileHigh422;
      break;
    case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
      initData.codecProfile = VIDEOCODEC_INITDATA::H264CodecProfileHigh444Predictive;
      break;
    default:
      return false;
    }
    break;
  case AV_CODEC_ID_VP8:
    initData.codec = VIDEOCODEC_INITDATA::CodecVp8;
    break;
  case AV_CODEC_ID_VP9:
    initData.codec = VIDEOCODEC_INITDATA::CodecVp9;
    break;
  default:
    return false;
  }
  if (hints.cryptoSession)
  {
    switch (hints.cryptoSession->keySystem)
    {
    case CRYPTO_SESSION_SYSTEM_NONE:
      initData.cryptoInfo.m_CryptoKeySystem = CRYPTO_INFO::CRYPTO_KEY_SYSTEM_NONE;
      break;
    case CRYPTO_SESSION_SYSTEM_WIDEVINE:
      initData.cryptoInfo.m_CryptoKeySystem = CRYPTO_INFO::CRYPTO_KEY_SYSTEM_WIDEVINE;
      break;
    case CRYPTO_SESSION_SYSTEM_PLAYREADY:
      initData.cryptoInfo.m_CryptoKeySystem = CRYPTO_INFO::CRYPTO_KEY_SYSTEM_PLAYREADY;
      break;
    default:
      return false;
    }
    initData.cryptoInfo.m_CryptoSessionIdSize = hints.cryptoSession->sessionIdSize;
    //We assume that we need this sessionid only for the directly following call
    initData.cryptoInfo.m_CryptoSessionId = hints.cryptoSession->sessionId;
  }

  initData.extraData = reinterpret_cast<const uint8_t*>(hints.extradata);
  initData.extraDataSize = hints.extrasize;
  initData.width = hints.width;
  initData.height = hints.height;
  initData.videoFormats = m_formats;

  return true;
}

bool CAddonVideoCodec::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (!m_struct.toAddon.Open)
    return false;

  unsigned int nformats(0);
  for (auto fmt : options.m_formats)
    if (fmt == RENDER_FMT_YUV420P)
    {
      m_formats[nformats++] = VideoFormatYV12;
      break;
    }
  m_formats[nformats] = UnknownVideoFormat;

  if (nformats == 0)
    return false;

  VIDEOCODEC_INITDATA initData;
  if (!CopyToInitData(initData, hints))
    return false;

  return m_struct.toAddon.Open(m_addonInstance, initData);
}

bool CAddonVideoCodec::Reconfigure(CDVDStreamInfo &hints)
{
  if (!m_struct.toAddon.Reconfigure)
    return false;

  VIDEOCODEC_INITDATA initData;
  if (!CopyToInitData(initData, hints))
    return false;

  return m_struct.toAddon.Reconfigure(m_addonInstance, initData);
}

bool CAddonVideoCodec::AddData(const DemuxPacket &packet)
{
  if (!m_struct.toAddon.AddData)
    return false;

  return m_struct.toAddon.AddData(m_addonInstance, packet);
}

CDVDVideoCodec::VCReturn CAddonVideoCodec::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if (!m_struct.toAddon.GetPicture)
    return CDVDVideoCodec::VC_ERROR;

  VIDEOCODEC_PICTURE picture;
  switch (m_struct.toAddon.GetPicture(m_addonInstance, picture))
  {
  case VC_NONE:
    return CDVDVideoCodec::VC_NONE;
  case VC_ERROR:
    return CDVDVideoCodec::VC_ERROR;
  case VC_BUFFER:
    return CDVDVideoCodec::VC_BUFFER;
  case VC_PICTURE:
    //TODO: copy data from picture to pDvdVideoPicture
  default:
    return CDVDVideoCodec::VC_ERROR;
  }
}

const char* CAddonVideoCodec::GetName()
{
  if (m_struct.toAddon.GetName)
    return m_struct.toAddon.GetName(m_addonInstance);
  return "";
}

void CAddonVideoCodec::Reset()
{
  if (!m_struct.toAddon.Reset)
    return;

  m_struct.toAddon.Reset(m_addonInstance);
}

