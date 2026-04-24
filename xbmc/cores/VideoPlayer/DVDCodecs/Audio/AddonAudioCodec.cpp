/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonAudioCodec.h"

#include "addons/addoninfo/AddonInfo.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecs.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/VideoPlayer/Interface/DemuxCrypto.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/StreamUtils.h"
#include "utils/log.h"

#include <cstdlib>

extern "C"
{
#include <libavcodec/defs.h>
}

namespace
{
AEDataFormat ConvAudioCodecFormat(AUDIOCODEC_FORMAT acFormat)
{
  switch (acFormat)
  {
    case AUDIOCODEC_FMT_U8:
      return AE_FMT_U8;
    case AUDIOCODEC_FMT_S16BE:
      return AE_FMT_S16BE;
    case AUDIOCODEC_FMT_S16LE:
      return AE_FMT_S16LE;
    case AUDIOCODEC_FMT_S16NE:
      return AE_FMT_S16NE;
    case AUDIOCODEC_FMT_S32BE:
      return AE_FMT_S32BE;
    case AUDIOCODEC_FMT_S32LE:
      return AE_FMT_S32LE;
    case AUDIOCODEC_FMT_S32NE:
      return AE_FMT_S32NE;
    case AUDIOCODEC_FMT_S24BE4:
      return AE_FMT_S24BE4;
    case AUDIOCODEC_FMT_S24LE4:
      return AE_FMT_S24LE4;
    case AUDIOCODEC_FMT_S24NE4:
      return AE_FMT_S24NE4;
    case AUDIOCODEC_FMT_S24NE4MSB:
      return AE_FMT_S24NE4MSB;
    case AUDIOCODEC_FMT_S24BE3:
      return AE_FMT_S24BE3;
    case AUDIOCODEC_FMT_S24LE3:
      return AE_FMT_S24LE3;
    case AUDIOCODEC_FMT_S24NE3:
      return AE_FMT_S24NE3;
    case AUDIOCODEC_FMT_DOUBLE:
      return AE_FMT_DOUBLE;
    case AUDIOCODEC_FMT_FLOAT:
      return AE_FMT_FLOAT;
    case AUDIOCODEC_FMT_RAW:
      return AE_FMT_RAW;
    case AUDIOCODEC_FMT_U8P:
      return AE_FMT_U8P;
    case AUDIOCODEC_FMT_S16NEP:
      return AE_FMT_S16NEP;
    case AUDIOCODEC_FMT_S32NEP:
      return AE_FMT_S32NEP;
    case AUDIOCODEC_FMT_S24NE4P:
      return AE_FMT_S24NE4P;
    case AUDIOCODEC_FMT_S24NE4MSBP:
      return AE_FMT_S24NE4MSBP;
    case AUDIOCODEC_FMT_S24NE3P:
      return AE_FMT_S24NE3P;
    case AUDIOCODEC_FMT_DOUBLEP:
      return AE_FMT_DOUBLEP;
    case AUDIOCODEC_FMT_FLOATP:
      return AE_FMT_FLOATP;
    default:
      CLog::LogF(LOGWARNING, "Audio format '{}' not valid", acFormat);
      return AE_FMT_INVALID;
  };
}
} // unnamed namespace

AEAudioFormat CAddonAudioCodec::GetFormat()
{
  return m_format;
}

CAddonAudioCodec::CAddonAudioCodec(CProcessInfo& processInfo,
                                   ADDON::AddonInfoPtr& addonInfo,
                                   KODI_HANDLE parentInstance)
  : CDVDAudioCodec(processInfo),
    IAddonInstanceHandler(
        ADDON_INSTANCE_AUDIOCODEC, addonInfo, ADDON::ADDON_INSTANCE_ID_UNUSED, parentInstance)
{
  m_ifc.audiocodec = new AddonInstance_AudioCodec;
  m_ifc.audiocodec->props = new AddonProps_AudioCodec();
  m_ifc.audiocodec->toAddon = new KodiToAddonFuncTable_AudioCodec();
  m_ifc.audiocodec->toKodi = new AddonToKodiFuncTable_AudioCodec();

  m_ifc.audiocodec->toKodi->kodiInstance = this;
  m_ifc.audiocodec->toKodi->get_frame_buffer = get_frame_buffer;
  m_ifc.audiocodec->toKodi->release_frame_buffer = release_frame_buffer;
  if (CreateInstance() != ADDON_STATUS_OK || !m_ifc.audiocodec->toAddon->open)
  {
    CLog::Log(LOGERROR, "CInputStreamAddon: Failed to create add-on instance for '{}'",
              addonInfo->ID());
    return;
  }
}

CAddonAudioCodec::~CAddonAudioCodec()
{
  //free remaining buffers
  Reset();

  DestroyInstance();

  // Delete "C" interface structures
  delete m_ifc.audiocodec->toAddon;
  delete m_ifc.audiocodec->toKodi;
  delete m_ifc.audiocodec->props;
  delete m_ifc.audiocodec;
}

bool CAddonAudioCodec::CopyToInitData(AUDIOCODEC_INITDATA& initData, CDVDStreamInfo& hints)
{
  initData.codecProfile = STREAMCODEC_PROFILE::CodecProfileNotNeeded;

  switch (hints.codec)
  {
    case AV_CODEC_ID_MP1:
      initData.codec = AUDIOCODEC_MP1;
      break;
    case AV_CODEC_ID_MP2:
      initData.codec = AUDIOCODEC_MP2;
      break;
    case AV_CODEC_ID_MP3:
      initData.codec = AUDIOCODEC_MP3;
      break;
    case AV_CODEC_ID_AAC:
      initData.codec = AUDIOCODEC_AAC;
      break;
    case AV_CODEC_ID_AAC_LATM:
      initData.codec = AUDIOCODEC_AAC_LATM;
      break;
    case AV_CODEC_ID_AC3:
      initData.codec = AUDIOCODEC_AC3;
      break;
    case AV_CODEC_ID_EAC3:
      initData.codec = AUDIOCODEC_EAC3;
      break;
    case AV_CODEC_ID_DTS:
      initData.codec = AUDIOCODEC_DTS;
      break;
    case AV_CODEC_ID_TRUEHD:
      initData.codec = AUDIOCODEC_TRUEHD;
      break;
    case AV_CODEC_ID_VORBIS:
      initData.codec = AUDIOCODEC_VORBIS;
      break;
    case AV_CODEC_ID_OPUS:
      initData.codec = AUDIOCODEC_OPUS;
      break;
    case AV_CODEC_ID_AC4:
      initData.codec = AUDIOCODEC_AC4;
      break;
    default:
      initData.codec = AUDIOCODEC_UNKNOWN;
      break;
  }

  if (hints.cryptoSession)
  {
    switch (hints.cryptoSession->keySystem)
    {
      case CRYPTO_SESSION_SYSTEM_NONE:
        initData.cryptoSession.keySystem = STREAM_CRYPTO_KEY_SYSTEM_NONE;
        break;
      case CRYPTO_SESSION_SYSTEM_WIDEVINE:
        initData.cryptoSession.keySystem = STREAM_CRYPTO_KEY_SYSTEM_WIDEVINE;
        break;
      case CRYPTO_SESSION_SYSTEM_PLAYREADY:
        initData.cryptoSession.keySystem = STREAM_CRYPTO_KEY_SYSTEM_PLAYREADY;
        break;
      case CRYPTO_SESSION_SYSTEM_WISEPLAY:
        initData.cryptoSession.keySystem = STREAM_CRYPTO_KEY_SYSTEM_WISEPLAY;
        break;
      case CRYPTO_SESSION_SYSTEM_CLEARKEY:
        initData.cryptoSession.keySystem = STREAM_CRYPTO_KEY_SYSTEM_CLEARKEY;
        break;
      default:
        return false;
    }

    strncpy(initData.cryptoSession.sessionId, hints.cryptoSession->sessionId.c_str(),
            sizeof(initData.cryptoSession.sessionId) - 1);
  }

  initData.extraData = hints.extradata.GetData();
  initData.extraDataSize = hints.extradata.GetSize();

  initData.sampleRate = hints.samplerate;
  m_format.m_sampleRate = hints.samplerate;
  initData.channels = hints.channels;
  m_audioChannels = hints.channels;
  initData.bitsPerSample = hints.bitspersample;

  // set data format from bits per sample as fallback
  switch (hints.bitspersample)
  {
    case 8:
      m_format.m_dataFormat = AE_FMT_U8;
      break;
    case 16:
      m_format.m_dataFormat = AE_FMT_S16NE;
      break;
    case 24:
      m_format.m_dataFormat = AE_FMT_S24NE3;
      break;
    case 32:
      m_format.m_dataFormat = AE_FMT_S32NE;
      break;
    default:
      m_format.m_dataFormat = AE_FMT_INVALID;
      break;
  }

  return true;
}

bool CAddonAudioCodec::Open(CDVDStreamInfo& hints, CDVDCodecOptions& options)
{
  if (!m_ifc.audiocodec->toAddon->open)
    return false;

  AUDIOCODEC_INITDATA initData;
  if (!CopyToInitData(initData, hints))
    return false;

  bool ret = m_ifc.audiocodec->toAddon->open(m_ifc.audiocodec, &initData);

  m_processInfo.SetAudioDecoderName(GetName());

  return ret;
}

bool CAddonAudioCodec::AddData(const DemuxPacket& packet)
{
  if (!m_ifc.audiocodec->toAddon->add_data)
    return false;

  return m_ifc.audiocodec->toAddon->add_data(m_ifc.audiocodec, &packet);
}

void CAddonAudioCodec::GetData(DVDAudioFrame& frame)
{
  if (!m_ifc.audiocodec->toAddon->get_frame)
    return;

  AUDIOCODEC_FRAME acFrame;

  switch (m_ifc.audiocodec->toAddon->get_frame(m_ifc.audiocodec, &acFrame))
  {
    case AUDIOCODEC_RETVAL::AC_NONE:
      return;
    case AUDIOCODEC_RETVAL::AC_ERROR:
      return;
    case AUDIOCODEC_RETVAL::AC_BUFFER:
      return;
    case AUDIOCODEC_RETVAL::AC_FRAME:
    {
      // Map addon frame to internal DVDAudioFrame. Many fields are filled by
      // the addon; here we perform basic mapping. Addons should provide
      // decodedData and decodedDataSize via GetFrameBuffer and set pts.
      frame.pts = static_cast<double>(acFrame.pts);
      frame.hasTimestamp = (acFrame.pts != DVD_NOPTS_VALUE);

      frame.format = m_format;

      const AEDataFormat format = ConvAudioCodecFormat(acFrame.audioFormat);
      if (format != AE_FMT_INVALID)
        frame.format.m_dataFormat = format;

      // compute framesize and nb_frames
      frame.framesize =
          (CAEUtil::DataFormatToBits(frame.format.m_dataFormat) >> 3) * m_audioChannels;
      if (frame.framesize > 0 && acFrame.decodedDataSize > 0)
        frame.nb_frames = static_cast<unsigned int>(acFrame.decodedDataSize / frame.framesize);
      else
        frame.nb_frames = 0;

      frame.framesOut = 0;
      frame.passthrough = false;
      frame.planes = AE_IS_PLANAR(frame.format.m_dataFormat) ? m_audioChannels : 1;
      frame.data[0] = acFrame.decodedData; //! @todo: support planar formats with more planes

      // compute duration
      if (frame.format.m_sampleRate)
        frame.duration =
            static_cast<double>(frame.nb_frames) * DVD_TIME_BASE / frame.format.m_sampleRate;
      else
        frame.duration = 0.0;

      frame.bits_per_sample = CAEUtil::DataFormatToBits(frame.format.m_dataFormat);
      frame.hasDownmix = false;
      frame.centerMixLevel = 0.0;
      frame.profile = 0;
      frame.matrix_encoding = AV_MATRIX_ENCODING_NONE;

      return;
    }
    case AUDIOCODEC_RETVAL::AC_EOF:
    {
      CLog::Log(LOGINFO, "CAddonAudioCodec: GetData: EOF");
      return;
    }
    default:
      return;
  }
}

std::string CAddonAudioCodec::GetName()
{
  if (m_ifc.audiocodec->toAddon->get_name)
    return m_ifc.audiocodec->toAddon->get_name(m_ifc.audiocodec);
  return "";
}

void CAddonAudioCodec::Reset()
{
  CLog::Log(LOGDEBUG, "CAddonAudioCodec: Reset");

  // Get the remaining frames out of the external decoder
  AUDIOCODEC_FRAME frame;
  frame.flags = AUDIOCODEC_FRAME_FLAG_DRAIN;

  AUDIOCODEC_RETVAL ret;
  while ((ret = m_ifc.audiocodec->toAddon->get_frame(m_ifc.audiocodec, &frame)) !=
         AUDIOCODEC_RETVAL::AC_EOF)
  {
    if (ret == AUDIOCODEC_RETVAL::AC_FRAME)
    {
      if (frame.audioBufferHandle)
        ReleaseFrameBuffer(frame.audioBufferHandle);
    }
  }
  if (m_ifc.audiocodec->toAddon->reset)
    m_ifc.audiocodec->toAddon->reset(m_ifc.audiocodec);
}

bool CAddonAudioCodec::GetFrameBuffer(AUDIOCODEC_FRAME& frame)
{
  CLog::Log(LOGDEBUG, "CAddonAudioCodec: GetFrameBuffer allocating {} bytes",
            frame.decodedDataSize);
  frame.decodedData = static_cast<uint8_t*>(malloc(frame.decodedDataSize));
  if (!frame.decodedData)
  {
    CLog::Log(LOGERROR, "CAddonAudioCodec::GetFrameBuffer Failed to allocate buffer");
    frame.audioBufferHandle = nullptr;
    return false;
  }
  frame.audioBufferHandle = frame.decodedData;

  return true;
}

void CAddonAudioCodec::ReleaseFrameBuffer(KODI_HANDLE audioBufferHandle)
{
  if (audioBufferHandle)
    free(audioBufferHandle);
}

/*********************     ADDON-TO-KODI    **********************/

bool CAddonAudioCodec::get_frame_buffer(void* kodiInstance, AUDIOCODEC_FRAME* frame)
{
  if (!kodiInstance)
    return false;

  return static_cast<CAddonAudioCodec*>(kodiInstance)->GetFrameBuffer(*frame);
}

void CAddonAudioCodec::release_frame_buffer(void* kodiInstance, KODI_HANDLE audioBufferHandle)
{
  if (!kodiInstance)
    return;

  static_cast<CAddonAudioCodec*>(kodiInstance)->ReleaseFrameBuffer(audioBufferHandle);
}
