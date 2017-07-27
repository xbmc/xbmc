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
#include "addons/binary-addons/BinaryAddonBase.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/VideoPlayer/DVDDemuxers/DemuxCrypto.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecs.h"
#include "cores/VideoPlayer/Process/VideoBuffer.h"
#include "cores/VideoPlayer/TimingConstants.h"
#include "utils/log.h"
#include "settings/AdvancedSettings.h"

using namespace kodi::addon;

CAddonVideoCodec::CAddonVideoCodec(CProcessInfo &processInfo, ADDON::BinaryAddonBasePtr& addonInfo, kodi::addon::IAddonInstance* parentInstance)
  : CDVDVideoCodec(processInfo),
    IAddonInstanceHandler(ADDON_INSTANCE_VIDEOCODEC, addonInfo, parentInstance)
  , m_codecFlags(0)
  , m_displayAspect(0.0f)
{
  m_struct = { { 0 } };
  m_struct.toKodi.kodiInstance = this;
  m_struct.toKodi.get_frame_buffer = get_frame_buffer;
  m_struct.toKodi.release_frame_buffer = release_frame_buffer;
  if (CreateInstance(&m_struct) != ADDON_STATUS_OK || !m_struct.toAddon.open)
  {
    CLog::Log(LOGERROR, "CInputStreamAddon: Failed to create add-on instance for '%s'", addonInfo->ID().c_str());
    return;
  }
  m_processInfo.SetVideoDecoderName(GetName(), false);
}

CAddonVideoCodec::~CAddonVideoCodec()
{
  //free remaining buffers
  Reset();

  DestroyInstance();
}

bool CAddonVideoCodec::CopyToInitData(VIDEOCODEC_INITDATA &initData, CDVDStreamInfo &hints)
{
  initData.codecProfile = STREAMCODEC_PROFILE::CodecProfileNotNeeded;
  switch (hints.codec)
  {
  case AV_CODEC_ID_H264:
    initData.codec = VIDEOCODEC_INITDATA::CodecH264;
    switch (hints.profile)
    {
    case 0:
    case FF_PROFILE_UNKNOWN:
      initData.codecProfile = STREAMCODEC_PROFILE::CodecProfileUnknown;
      break;
    case FF_PROFILE_H264_BASELINE:
      initData.codecProfile = STREAMCODEC_PROFILE::H264CodecProfileBaseline;
      break;
    case FF_PROFILE_H264_MAIN:
      initData.codecProfile = STREAMCODEC_PROFILE::H264CodecProfileMain;
      break;
    case FF_PROFILE_H264_EXTENDED:
      initData.codecProfile = STREAMCODEC_PROFILE::H264CodecProfileExtended;
      break;
    case FF_PROFILE_H264_HIGH:
      initData.codecProfile = STREAMCODEC_PROFILE::H264CodecProfileHigh;
      break;
    case FF_PROFILE_H264_HIGH_10:
      initData.codecProfile = STREAMCODEC_PROFILE::H264CodecProfileHigh10;
      break;
    case FF_PROFILE_H264_HIGH_422:
      initData.codecProfile = STREAMCODEC_PROFILE::H264CodecProfileHigh422;
      break;
    case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
      initData.codecProfile = STREAMCODEC_PROFILE::H264CodecProfileHigh444Predictive;
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

  m_displayAspect = (hints.aspect > 0.0 && !hints.forced_aspect) ? static_cast<float>(hints.aspect) : 0.0f;
  m_width = hints.width;
  m_height = hints.height;

  m_processInfo.SetVideoDimensions(hints.width, hints.height);
  m_processInfo.SetVideoDAR(m_displayAspect);
  if (hints.fpsscale)
    m_processInfo.SetVideoFps(static_cast<float>(hints.fpsrate) / hints.fpsscale);

  return true;
}

bool CAddonVideoCodec::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (!m_struct.toAddon.open)
    return false;

  unsigned int nformats(0);
  m_formats[nformats++] = VideoFormatYV12;
  m_formats[nformats] = UnknownVideoFormat;

  VIDEOCODEC_INITDATA initData;
  if (!CopyToInitData(initData, hints))
    return false;

  return m_struct.toAddon.open(&m_struct, &initData);
}

bool CAddonVideoCodec::Reconfigure(CDVDStreamInfo &hints)
{
  if (!m_struct.toAddon.reconfigure)
    return false;

  VIDEOCODEC_INITDATA initData;
  if (!CopyToInitData(initData, hints))
    return false;

  return m_struct.toAddon.reconfigure(&m_struct, &initData);
}

bool CAddonVideoCodec::AddData(const DemuxPacket &packet)
{
  if (!m_struct.toAddon.add_data)
    return false;

  return m_struct.toAddon.add_data(&m_struct, &packet);
}

CDVDVideoCodec::VCReturn CAddonVideoCodec::GetPicture(VideoPicture* pVideoPicture)
{
  if (!m_struct.toAddon.get_picture)
    return CDVDVideoCodec::VC_ERROR;

  VIDEOCODEC_PICTURE picture;
  picture.flags = (m_codecFlags & DVD_CODEC_CTRL_DRAIN) ? VIDEOCODEC_PICTURE::FLAG_DRAIN : 0;

  switch (m_struct.toAddon.get_picture(&m_struct, &picture))
  {
  case VIDEOCODEC_RETVAL::VC_NONE:
    return CDVDVideoCodec::VC_NONE;
  case VIDEOCODEC_RETVAL::VC_ERROR:
    return CDVDVideoCodec::VC_ERROR;
  case VIDEOCODEC_RETVAL::VC_BUFFER:
    return CDVDVideoCodec::VC_BUFFER;
  case VIDEOCODEC_RETVAL::VC_PICTURE:
    pVideoPicture->iWidth = picture.width;
    pVideoPicture->iHeight = picture.height;
    pVideoPicture->pts = static_cast<double>(picture.pts);
    pVideoPicture->dts = DVD_NOPTS_VALUE;
    pVideoPicture->color_range = 0;
    pVideoPicture->color_matrix = 4;
    pVideoPicture->iFlags = 0;
    if (m_codecFlags & DVD_CODEC_CTRL_DROP)
      pVideoPicture->iFlags |= DVP_FLAG_DROPPED;

    if (pVideoPicture->videoBuffer)
      pVideoPicture->videoBuffer->Release();

    pVideoPicture->videoBuffer = static_cast<CVideoBuffer*>(picture.buffer);

    int strides[YuvImage::MAX_PLANES], planeOffsets[YuvImage::MAX_PLANES];
    for (int i = 0; i<YuvImage::MAX_PLANES; ++i)
      strides[i] = picture.stride[i];
    for (int i = 0; i<YuvImage::MAX_PLANES; ++i)
      planeOffsets[i] = picture.planeOffsets[i];

    pVideoPicture->videoBuffer->SetDimensions(picture.width, picture.height, strides, planeOffsets);

    pVideoPicture->iDisplayWidth = pVideoPicture->iWidth;
    pVideoPicture->iDisplayHeight = pVideoPicture->iHeight;
    if (m_displayAspect > 0.0)
    {
      pVideoPicture->iDisplayWidth = ((int)lrint(pVideoPicture->iHeight * m_displayAspect)) & ~3;
      if (pVideoPicture->iDisplayWidth > pVideoPicture->iWidth)
      {
        pVideoPicture->iDisplayWidth = pVideoPicture->iWidth;
        pVideoPicture->iDisplayHeight = ((int)lrint(pVideoPicture->iWidth / m_displayAspect)) & ~3;
      }
    }

    if (g_advancedSettings.CanLogComponent(LOGVIDEO))
      CLog::Log(LOGDEBUG, "CAddonVideoCodec: GetPicture::VC_PICTURE with pts %llu %dx%d (%dx%d) %f %p:%d offset:%d,%d,%d, stride:%d,%d,%d", picture.pts, pVideoPicture->iWidth, pVideoPicture->iHeight, pVideoPicture->iDisplayWidth, pVideoPicture->iDisplayHeight, m_displayAspect,
          picture.decodedData, picture.decodedDataSize, picture.planeOffsets[0], picture.planeOffsets[1], picture.planeOffsets[2], picture.stride[0], picture.stride[1], picture.stride[2]);

    if (picture.width != m_width || picture.height != m_height)
    {
      m_width = picture.width;
      m_height = picture.height;
      m_processInfo.SetVideoDimensions(m_width, m_height);
    }

    return CDVDVideoCodec::VC_PICTURE;
  case VIDEOCODEC_RETVAL::VC_EOF:
    return CDVDVideoCodec::VC_EOF;
  default:
    return CDVDVideoCodec::VC_ERROR;
  }
}

const char* CAddonVideoCodec::GetName()
{
  if (m_struct.toAddon.get_name)
    return m_struct.toAddon.get_name(&m_struct);
  return "";
}

void CAddonVideoCodec::Reset()
{
  CVideoBuffer *videoBuffer;

  CLog::Log(LOGDEBUG, "CAddonVideoCodec: Reset");

  // Get the remaining pictures out of the external decoder
  VIDEOCODEC_PICTURE picture;
  picture.flags = VIDEOCODEC_PICTURE::FLAG_DRAIN;

  VIDEOCODEC_RETVAL ret;
  while ((ret = m_struct.toAddon.get_picture(&m_struct, &picture)) != VIDEOCODEC_RETVAL::VC_EOF)
  {
    if (ret == VIDEOCODEC_RETVAL::VC_PICTURE)
    {
      videoBuffer = static_cast<CVideoBuffer*>(picture.buffer);
      if (videoBuffer)
        videoBuffer->Release();
    }
  }
  if (m_struct.toAddon.reset)
    m_struct.toAddon.reset(&m_struct);
}

bool CAddonVideoCodec::GetFrameBuffer(VIDEOCODEC_PICTURE &picture)
{
  CVideoBuffer *videoBuffer = m_processInfo.GetVideoBufferManager().Get(AV_PIX_FMT_YUV420P, picture.decodedDataSize);
  if (!videoBuffer)
  {
    CLog::Log(LOGERROR,"CAddonVideoCodec::GetFrameBuffer Failed to allocate buffer");
    return false;
  }
  picture.decodedData = videoBuffer->GetMemPtr();
  picture.buffer = videoBuffer;

  return true;
}

void CAddonVideoCodec::ReleaseFrameBuffer(void *buffer)
{
  if (buffer)
    static_cast<CVideoBuffer*>(buffer)->Release();
}

/*********************     ADDON-TO-KODI    **********************/

bool CAddonVideoCodec::get_frame_buffer(void* kodiInstance, VIDEOCODEC_PICTURE *picture)
{
  if (!kodiInstance)
    return false;

  return static_cast<CAddonVideoCodec*>(kodiInstance)->GetFrameBuffer(*picture);
}

void CAddonVideoCodec::release_frame_buffer(void* kodiInstance, void *buffer)
{
  if (!kodiInstance)
    return;

  static_cast<CAddonVideoCodec*>(kodiInstance)->ReleaseFrameBuffer(buffer);
}
