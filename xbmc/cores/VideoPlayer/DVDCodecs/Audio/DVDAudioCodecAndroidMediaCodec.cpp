/*
 *  Copyright (C) 2016 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// http://developer.android.com/reference/android/media/MediaCodec.html
//
// Android MediaCodec class can be used to access low-level media codec,
// i.e. encoder/decoder components. (android.media.MediaCodec). Requires
// SDK16+ which is 4.1 Jellybean and above.
//

#include "DVDAudioCodecAndroidMediaCodec.h"

#include "DVDAudioCodecFFmpeg.h"
#include "DVDAudioCodecPassthrough.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "ServiceBroker.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/VideoPlayer/Interface/DemuxCrypto.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <cassert>
#include <stdexcept>

#include <androidjni/ByteBuffer.h>
#include <androidjni/MediaCodec.h>
#include <androidjni/MediaCodecCryptoInfo.h>
#include <androidjni/MediaCodecInfo.h>
#include <androidjni/MediaCodecList.h>
#include <androidjni/MediaCrypto.h>
#include <androidjni/MediaFormat.h>
#include <androidjni/Surface.h>
#include <androidjni/UUID.h>

static const AEChannel KnownChannels[] = { AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_SL, AE_CH_SR, AE_CH_BL, AE_CH_BR, AE_CH_BC, AE_CH_BLOC, AE_CH_BROC, AE_CH_NULL };

static bool IsDownmixDecoder(const std::string &name)
{
  static const char *downmixDecoders[] = {
    "OMX.dolby",
    // End of list
    NULL
  };
  for (const char **ptr = downmixDecoders; *ptr; ptr++)
  {
    if (!StringUtils::CompareNoCase(*ptr, name, strlen(*ptr)))
      return true;
  }
  return false;
}

static bool IsDecoderWhitelisted(const std::string &name)
{
  static const char *whitelistDecoders[] = {
    // End of list
    NULL
  };
  for (const char **ptr = whitelistDecoders; *ptr; ptr++)
  {
    if (!StringUtils::CompareNoCase(*ptr, name, strlen(*ptr)))
      return true;
  }
  return false;
}

/****************************/

CDVDAudioCodecAndroidMediaCodec::CDVDAudioCodecAndroidMediaCodec(CProcessInfo& processInfo)
  : CDVDAudioCodec(processInfo), m_formatname("mediacodec"), m_buffer(NULL)
{
}

CDVDAudioCodecAndroidMediaCodec::~CDVDAudioCodecAndroidMediaCodec()
{
  Dispose();
}

std::unique_ptr<CDVDAudioCodec> CDVDAudioCodecAndroidMediaCodec::Create(CProcessInfo& processInfo)
{
  return std::make_unique<CDVDAudioCodecAndroidMediaCodec>(processInfo);
}

bool CDVDAudioCodecAndroidMediaCodec::Register()
{
  CDVDFactoryCodec::RegisterHWAudioCodec("mediacodec_dec", &CDVDAudioCodecAndroidMediaCodec::Create);
  return true;
}

bool CDVDAudioCodecAndroidMediaCodec::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  m_hints = hints;

  CLog::Log(LOGDEBUG,
            "CDVDAudioCodecAndroidMediaCodec::Open codec({}), profile({}), tag({}), extrasize({})",
            hints.codec, hints.profile, hints.codec_tag, hints.extradata.GetSize());

  // First check if passthrough decoder is supported
  CAEStreamInfo::DataType ptStreamType = CAEStreamInfo::STREAM_TYPE_NULL;
  for (const auto &key : options.m_keys)
    if (key.m_name == "ptstreamtype")
    {
      ptStreamType = static_cast<CAEStreamInfo::DataType>(atoi(key.m_value.c_str()));
      break;
    }

  if (ptStreamType != CAEStreamInfo::STREAM_TYPE_NULL)
  {
    //Look if the PT decoder can be opened
    m_decryptCodec = std::shared_ptr<CDVDAudioCodec>(new CDVDAudioCodecPassthrough(m_processInfo, ptStreamType));
    if (m_decryptCodec->Open(hints, options))
      goto PROCESSDECODER;
  }

  switch(m_hints.codec)
  {
    case AV_CODEC_ID_AAC:
    case AV_CODEC_ID_AAC_LATM:
      if (!m_hints.extradata)
      {
        CLog::Log(LOGINFO, "CDVDAudioCodecAndroidMediaCodec: extradata required for aac decoder!");
        return false;
      }

      m_mime = "audio/mp4a-latm";
      m_formatname = "amc-aac";
      break;

    case AV_CODEC_ID_MP2:
      m_mime = "audio/mpeg-L2";
      m_formatname = "amc-mp2";
      break;

    case AV_CODEC_ID_MP3:
      m_mime = "audio/mpeg";
      m_formatname = "amc-mp3";
      break;

    case AV_CODEC_ID_VORBIS:
      m_mime = "audio/vorbis";
      m_formatname = "amc-ogg";

      //TODO
      return false;

      break;

    case AV_CODEC_ID_WMAPRO:
      m_mime = "audio/wmapro";
      m_formatname = "amc-wma";

      //TODO
      return false;

      break;

    case AV_CODEC_ID_WMAV1:
    case AV_CODEC_ID_WMAV2:
      m_mime = "audio/x-ms-wma";
      m_formatname = "amc-wma";
      //TODO
      return false;

      break;

    case AV_CODEC_ID_AC3:
      m_mime = "audio/ac3";
      m_formatname = "amc-ac3";
      break;

    case AV_CODEC_ID_EAC3:
      m_mime = "audio/eac3";
      m_formatname = "amc-eac3";
      break;

    default:
      CLog::Log(LOGINFO, "CDVDAudioCodecAndroidMediaCodec: Unknown hints.codec({})", hints.codec);
      return false;
      break;
  }

  {
    //StereoDownmixAllowed is true if the user has selected 2.0 Audio channels in settings
    bool stereoDownmixAllowed = CServiceBroker::GetActiveAE()->HasStereoAudioChannelCount();
    const std::vector<CJNIMediaCodecInfo> codecInfos =
        CJNIMediaCodecList(CJNIMediaCodecList::REGULAR_CODECS).getCodecInfos();
    std::vector<std::string> mimeTypes;

    for (const CJNIMediaCodecInfo& codec_info : codecInfos)
    {
      if (codec_info.isEncoder())
        continue;

      std::string codecName = codec_info.getName();

      if (!IsDecoderWhitelisted(codecName))
        continue;

      if (m_hints.channels > 2 && !stereoDownmixAllowed && IsDownmixDecoder(codecName))
        continue;

      mimeTypes = codec_info.getSupportedTypes();
      if (std::find(mimeTypes.begin(), mimeTypes.end(), m_mime) != mimeTypes.end())
      {
        m_codec = std::shared_ptr<CJNIMediaCodec>(new CJNIMediaCodec(CJNIMediaCodec::createByCodecName(codecName)));
        if (xbmc_jnienv()->ExceptionCheck())
        {
          xbmc_jnienv()->ExceptionDescribe();
          xbmc_jnienv()->ExceptionClear();
          m_codec = NULL;
          continue;
        }
        CLog::Log(LOGINFO, "CDVDAudioCodecAndroidMediaCodec: Selected audio decoder: {}",
                  codecName);
        break;
      }
    }
  }

PROCESSDECODER:

  if (m_crypto)
    delete m_crypto;

  if (m_hints.cryptoSession)
  {
    CLog::Log(LOGDEBUG, "CDVDAudioCodecAndroidMediaCodec::Open Initializing MediaCrypto");

    CJNIUUID uuid(jni::jhobject(NULL));
    if (m_hints.cryptoSession->keySystem == CRYPTO_SESSION_SYSTEM_WIDEVINE)
      uuid = CJNIUUID(0xEDEF8BA979D64ACEULL, 0xA3C827DCD51D21EDULL);
    else if (m_hints.cryptoSession->keySystem == CRYPTO_SESSION_SYSTEM_PLAYREADY)
      uuid = CJNIUUID(0x9A04F07998404286ULL, 0xAB92E65BE0885F95ULL);
    else
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec::Open Unsupported crypto-keysystem:{}",
                m_hints.cryptoSession->keySystem);
      return false;
    }

    m_crypto =
        new CJNIMediaCrypto(uuid, std::vector<char>(m_hints.cryptoSession->sessionId.begin(),
                                                    m_hints.cryptoSession->sessionId.end()));

    if (xbmc_jnienv()->ExceptionCheck())
    {
      CLog::Log(LOGERROR, "MediaCrypto::ExceptionCheck: <init>");
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
      return false;
    }
  }
  else
    m_crypto = new CJNIMediaCrypto(jni::jhobject(NULL));

  if (!m_codec)
  {
    if (m_hints.cryptoSession)
    {
      m_mime = "audio/raw";

      // Workaround for old Android devices
      // Prefer the Google raw decoder over the MediaTek one
      const std::vector<CJNIMediaCodecInfo> codecInfos =
          CJNIMediaCodecList(CJNIMediaCodecList::REGULAR_CODECS).getCodecInfos();

      bool mtk_raw_decoder = false;
      bool google_raw_decoder = false;

      for (const CJNIMediaCodecInfo& codec_info : codecInfos)
      {
        if (codec_info.isEncoder())
          continue;

        if (codec_info.getName() == "OMX.MTK.AUDIO.DECODER.RAW")
          mtk_raw_decoder = true;
        if (codec_info.getName() == "OMX.google.raw.decoder")
          google_raw_decoder = true;
      }

      if (CJNIBase::GetSDKVersion() <= 27 && mtk_raw_decoder && google_raw_decoder)
      {
        CLog::Log(LOGDEBUG, "CDVDAudioCodecAndroidMediaCodec::Open Prefer the Google raw decoder "
                            "over the MediaTek one");
        m_codec = std::shared_ptr<CJNIMediaCodec>(
            new CJNIMediaCodec(CJNIMediaCodec::createByCodecName("OMX.google.raw.decoder")));
      }
      else
      {
        CLog::Log(
            LOGDEBUG,
            "CDVDAudioCodecAndroidMediaCodec::Open Use the raw decoder proposed by the platform");
        m_codec = std::shared_ptr<CJNIMediaCodec>(
            new CJNIMediaCodec(CJNIMediaCodec::createDecoderByType(m_mime)));
      }
      if (xbmc_jnienv()->ExceptionCheck())
      {
        xbmc_jnienv()->ExceptionDescribe();
        xbmc_jnienv()->ExceptionClear();
        CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec::Open Failed creating raw decoder");
        return false;
      }
      if (!m_decryptCodec)
      {
        CDVDStreamInfo ffhints = hints;
        ffhints.cryptoSession = nullptr;

        m_decryptCodec = std::shared_ptr<CDVDAudioCodec>(new CDVDAudioCodecFFmpeg(m_processInfo));
        if (!m_decryptCodec->Open(ffhints, options))
        {
          CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec::Open() Failed opening FFmpeg decoder");
          return false;
        }
      }
      CLog::Log(LOGINFO, "CDVDAudioCodecAndroidMediaCodec Use raw decoder and decode using {}",
                m_decryptCodec->GetName());
    }
    else
    {
      CLog::Log(LOGINFO, "CDVDAudioCodecAndroidMediaCodec::Open() Use default handling for non encrypted stream");
      return false;
    }
  }

  if (!ConfigureMediaCodec())
  {
    m_codec.reset();
    return false;
  }

  CLog::Log(LOGINFO, "CDVDAudioCodecAndroidMediaCodec Open Android MediaCodec {}", m_formatname);

  m_opened = true;
  m_codecIsFed = false;
  m_currentPts = DVD_NOPTS_VALUE;
  return m_opened;
}

std::string CDVDAudioCodecAndroidMediaCodec::GetName()
{
  if (m_decryptCodec)
    return "amc-raw/" + m_decryptCodec->GetName();
  return m_formatname;
};

void CDVDAudioCodecAndroidMediaCodec::Dispose()
{
  if (!m_opened)
    return;

  m_opened = false;

  if (m_codec)
  {
    m_codec->stop();
    m_codec->release();
    m_codec.reset();
    if (xbmc_jnienv()->ExceptionCheck())
    {
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
    }
  }

  if (m_crypto)
  {
    delete m_crypto;
    m_crypto = nullptr;
  }
  m_decryptCodec = nullptr;
}

bool CDVDAudioCodecAndroidMediaCodec::AddData(const DemuxPacket &packet)
{
  CLog::Log(LOGDEBUG, LOGAUDIO,
            "CDVDAudioCodecAndroidMediaCodec::AddData dts:{:0.4f} pts:{:0.4f} size({})", packet.dts,
            packet.pts, packet.iSize);

  if (packet.pData)
  {
    // try to fetch an input buffer
    int64_t timeout_us = 5000;
    int index = m_codec->dequeueInputBuffer(timeout_us);
    if (xbmc_jnienv()->ExceptionCheck())
    {
      std::string err = CJNIBase::ExceptionToString();
      CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec::AddData ExceptionCheck: {}", err);
    }
    else if (index >= 0)
    {
      CJNIByteBuffer buffer = m_codec->getInputBuffer(index);
      int size = buffer.capacity();

      if (xbmc_jnienv()->ExceptionCheck())
      {
        CLog::Log(LOGERROR, "CDVDMediaCodecInfo::AddData getInputBuffers ExceptionCheck");
        xbmc_jnienv()->ExceptionDescribe();
        xbmc_jnienv()->ExceptionClear();
      }

      if (packet.iSize > size)
      {
        CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec::AddData, iSize({}) > size({})",
                  packet.iSize, size);
        return packet.iSize;
      }
      // fetch a pointer to the ByteBuffer backing store
      uint8_t *dst_ptr = (uint8_t*)xbmc_jnienv()->GetDirectBufferAddress(buffer.get_raw());

      if (dst_ptr)
      {
        // Codec specifics
        switch(m_hints.codec)
        {
          default:
            memcpy(dst_ptr, packet.pData, packet.iSize);
            break;
        }
      }
      else
        return false;

      CJNIMediaCodecCryptoInfo *cryptoInfo(0);
      if (!!m_crypto->get_raw() && packet.cryptoInfo)
      {
        if (CJNIBase::GetSDKVersion() < 25 &&
            packet.cryptoInfo->mode == CJNIMediaCodec::CRYPTO_MODE_AES_CBC)
        {
          CLog::LogF(LOGERROR, "Device API does not support CBCS decryption");
          return false;
        }
        cryptoInfo = new CJNIMediaCodecCryptoInfo();
        cryptoInfo->set(
            packet.cryptoInfo->numSubSamples,
            std::vector<int>(packet.cryptoInfo->clearBytes,
                             packet.cryptoInfo->clearBytes + packet.cryptoInfo->numSubSamples),
            std::vector<int>(packet.cryptoInfo->cipherBytes,
                             packet.cryptoInfo->cipherBytes + packet.cryptoInfo->numSubSamples),
            std::vector<char>(std::begin(packet.cryptoInfo->kid), std::end(packet.cryptoInfo->kid)),
            std::vector<char>(std::begin(packet.cryptoInfo->iv), std::end(packet.cryptoInfo->iv)),
            packet.cryptoInfo->mode == CJNIMediaCodec::CRYPTO_MODE_AES_CBC
                ? CJNIMediaCodec::CRYPTO_MODE_AES_CBC
                : CJNIMediaCodec::CRYPTO_MODE_AES_CTR);

        CJNIMediaCodecCryptoInfoPattern cryptoInfoPattern(packet.cryptoInfo->cryptBlocks,
                                                          packet.cryptoInfo->skipBlocks);
        cryptoInfo->setPattern(cryptoInfoPattern);
      }

      int flags = 0;
      int offset = 0;
      int64_t presentationTimeUs = static_cast<int64_t>(packet.pts);

      if (!cryptoInfo)
        m_codec->queueInputBuffer(index, offset, packet.iSize, presentationTimeUs, flags);
      else
      {
        m_codec->queueSecureInputBuffer(index, offset, *cryptoInfo, presentationTimeUs, flags);
        delete cryptoInfo;
      }

      // clear any jni exceptions, jni gets upset if we do not.
      if (xbmc_jnienv()->ExceptionCheck())
      {
        CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec::Decode ExceptionCheck");
        xbmc_jnienv()->ExceptionDescribe();
        xbmc_jnienv()->ExceptionClear();
      }
      m_codecIsFed = true;
    }
  }

  if (m_decryptCodec)
  {
    DemuxPacket newPkt;
    newPkt.iSize = GetData(&newPkt.pData);
    newPkt.pts = m_currentPts;
    newPkt.iStreamId = packet.iStreamId;
    newPkt.demuxerId = packet.demuxerId;
    newPkt.iGroupId = packet.iGroupId;
    newPkt.pSideData = packet.pSideData;
    newPkt.duration = packet.duration;
    newPkt.dispTime = packet.dispTime;
    newPkt.recoveryPoint = packet.recoveryPoint;
    if (!packet.pData || newPkt.iSize)
      m_decryptCodec->AddData(newPkt);
  }
  else
  {
    m_format.m_dataFormat = GetDataFormat();
    m_format.m_channelLayout = GetChannelMap();
    m_format.m_sampleRate = GetSampleRate();
    m_format.m_frameSize = m_format.m_channelLayout.Count() * CAEUtil::DataFormatToBits(m_format.m_dataFormat) >> 3;
  }
  return true;
}

void CDVDAudioCodecAndroidMediaCodec::Reset()
{
  if (!m_opened)
    return;

  if (m_codec && m_codecIsFed)
  {
    // now we can flush the actual MediaCodec object
    m_codec->flush();
    if (xbmc_jnienv()->ExceptionCheck())
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec::Reset ExceptionCheck");
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
    }
  }
  m_codecIsFed = false;

  if (m_decryptCodec)
    m_decryptCodec->Reset();

  m_currentPts = DVD_NOPTS_VALUE;
}

AEAudioFormat CDVDAudioCodecAndroidMediaCodec::GetFormat()
{
  if (m_decryptCodec)
    return m_decryptCodec->GetFormat();

  return m_format;
}

CAEChannelInfo CDVDAudioCodecAndroidMediaCodec::GetChannelMap()
{
  CAEChannelInfo chaninfo;

  for (int i=0; i<m_channels; ++i)
    chaninfo += KnownChannels[i];

  return chaninfo;
}

bool CDVDAudioCodecAndroidMediaCodec::ConfigureMediaCodec(void)
{
  // setup a MediaFormat to match the audio content,
  // used by codec during configure
  CJNIMediaFormat mediaformat(
      CJNIMediaFormat::createAudioFormat(m_mime, m_hints.samplerate, m_hints.channels));

  if (!m_decryptCodec)
  {
    // handle codec extradata
    if (m_hints.extradata)
    {
      size_t size = m_hints.extradata.GetSize();
      void* src_ptr = m_hints.extradata.GetData();
      // Allocate a byte buffer via allocateDirect in java instead of NewDirectByteBuffer,
      // since the latter doesn't allocate storage of its own, and we don't know how long
      // the codec uses the buffer.
      CJNIByteBuffer bytebuffer = CJNIByteBuffer::allocateDirect(size);
      void *dts_ptr = xbmc_jnienv()->GetDirectBufferAddress(bytebuffer.get_raw());
      memcpy(dts_ptr, src_ptr, size);
      // codec will automatically handle buffers as extradata
      // using entries with keys "csd-0", "csd-1", etc.
      mediaformat.setByteBuffer("csd-0", bytebuffer);
    }
    else if (m_hints.codec == AV_CODEC_ID_AAC || m_hints.codec == AV_CODEC_ID_AAC_LATM)
    {
      mediaformat.setInteger(CJNIMediaFormat::KEY_IS_ADTS, 1);
    }
  }

  // configure and start the codec.
  // use the MediaFormat that we have setup.
  // use a null MediaCrypto, our content is not encrypted.
  // use a null Surface
  int flags = 0;
  CJNISurface surface(jni::jhobject(NULL));
  m_codec->configure(mediaformat, surface, *m_crypto, flags);

  // always, check/clear jni exceptions.
  if (xbmc_jnienv()->ExceptionCheck())
  {
    CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec::ExceptionCheck: configure");
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
    return false;
  }

  m_codec->start();
  // always, check/clear jni exceptions.
  if (xbmc_jnienv()->ExceptionCheck())
  {
    CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec::ExceptionCheck: start");
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
    return false;
  }

  // There is no guarantee we'll get an INFO_OUTPUT_FORMAT_CHANGED (up to Android 4.3)
  // Configure the output with defaults
  if (!m_decryptCodec)
    ConfigureOutputFormat(&mediaformat);

  return true;
}

void CDVDAudioCodecAndroidMediaCodec::GetData(DVDAudioFrame &frame)
{
  if (m_decryptCodec)
  {
    m_decryptCodec->GetData(frame);
    return;
  }

  frame.passthrough = false;
  frame.nb_frames = 0;
  frame.framesOut = 0;
  frame.format.m_dataFormat = m_format.m_dataFormat;
  frame.format.m_channelLayout = m_format.m_channelLayout;
  frame.framesize = (CAEUtil::DataFormatToBits(frame.format.m_dataFormat) >> 3) * frame.format.m_channelLayout.Count();

  if (frame.framesize == 0)
    return;

  if (!m_codecIsFed)
    return;

  frame.nb_frames = GetData(frame.data)/frame.framesize;
  frame.planes = AE_IS_PLANAR(frame.format.m_dataFormat) ? frame.format.m_channelLayout.Count() : 1;
  frame.bits_per_sample = CAEUtil::DataFormatToBits(frame.format.m_dataFormat);
  frame.format.m_sampleRate = m_format.m_sampleRate;
  frame.pts = m_currentPts;
  m_currentPts = DVD_NOPTS_VALUE;
  frame.matrix_encoding = GetMatrixEncoding();
  frame.audio_service_type = GetAudioServiceType();
  frame.profile = GetProfile();
  // compute duration.
  if (frame.format.m_sampleRate)
    frame.duration = ((double)frame.nb_frames * DVD_TIME_BASE) / frame.format.m_sampleRate;
  else
    frame.duration = 0.0;
  if (frame.nb_frames > 0 && CServiceBroker::GetLogging().CanLogComponent(LOGAUDIO))
    CLog::Log(LOGDEBUG, "MediaCodecAudio::GetData: frames:{} pts: {:0.4f}", frame.nb_frames,
              frame.pts);
}

int CDVDAudioCodecAndroidMediaCodec::GetData(uint8_t** dst)
{
  m_bufferUsed = 0;

  int64_t timeout_us = 10000;
  CJNIMediaCodecBufferInfo bufferInfo;
  int index = m_codec->dequeueOutputBuffer(bufferInfo, timeout_us);
  if (xbmc_jnienv()->ExceptionCheck())
  {
    std::string err = CJNIBase::ExceptionToString();
    CLog::Log(LOGERROR,
              "CDVDAudioCodecAndroidMediaCodec::GetData ExceptionCheck: dequeueOutputBuffer: {}",
              err);
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
    return 0;
  }
  if (index >= 0)
  {
    CJNIByteBuffer buffer = m_codec->getOutputBuffer(index);
    if (xbmc_jnienv()->ExceptionCheck())
    {
      CLog::Log(LOGERROR,
                "CDVDAudioCodecAndroidMediaCodec::GetData ExceptionCheck: getOutputBuffer({})",
                index);
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
      return 0;
    }

    int flags = bufferInfo.flags();
    if (flags & CJNIMediaCodec::BUFFER_FLAG_SYNC_FRAME)
      CLog::Log(LOGDEBUG, "CDVDAudioCodecAndroidMediaCodec:: BUFFER_FLAG_SYNC_FRAME");

    if (flags & CJNIMediaCodec::BUFFER_FLAG_CODEC_CONFIG)
      CLog::Log(LOGDEBUG, "CDVDAudioCodecAndroidMediaCodec:: BUFFER_FLAG_CODEC_CONFIG");

    if (flags & CJNIMediaCodec::BUFFER_FLAG_END_OF_STREAM)
    {
      CLog::Log(LOGDEBUG, "CDVDAudioCodecAndroidMediaCodec:: BUFFER_FLAG_END_OF_STREAM");
      m_codec->releaseOutputBuffer(index, false);
      if (xbmc_jnienv()->ExceptionCheck())
      {
        CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec::GetData ExceptionCheck: releaseOutputBuffer");
        xbmc_jnienv()->ExceptionDescribe();
        xbmc_jnienv()->ExceptionClear();
        return 0;
      }
      return 0;
    }

    int size = bufferInfo.size();
    int offset = bufferInfo.offset();

    if (!buffer.isDirect())
      CLog::Log(LOGWARNING, "CDVDAudioCodecAndroidMediaCodec:: buffer.isDirect == false");

    if (size && buffer.capacity())
    {
      uint8_t *src_ptr = (uint8_t*)xbmc_jnienv()->GetDirectBufferAddress(buffer.get_raw());
      src_ptr += offset;

      if (size > m_bufferSize)
      {
        m_bufferSize = size;
        m_buffer = (uint8_t*)realloc(m_buffer, m_bufferSize);
        if (m_buffer == nullptr)
          throw std::runtime_error("Failed to realloc memory, insufficient memory available");
      }

      memcpy(m_buffer, src_ptr, size);
      m_bufferUsed = size;
    }
    else
      return 0;

    m_codec->releaseOutputBuffer(index, false);
    if (xbmc_jnienv()->ExceptionCheck())
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec::GetData ExceptionCheck: releaseOutputBuffer");
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
    }

    CLog::Log(LOGDEBUG, LOGAUDIO, "CDVDAudioCodecAndroidMediaCodec::GetData index({}), size({})",
              index, m_bufferUsed);

    m_currentPts = bufferInfo.presentationTimeUs() == (int64_t)DVD_NOPTS_VALUE ? DVD_NOPTS_VALUE :  bufferInfo.presentationTimeUs();

    // always, check/clear jni exceptions.
    if (xbmc_jnienv()->ExceptionCheck())
      xbmc_jnienv()->ExceptionClear();
  }
  else if (index == CJNIMediaCodec::INFO_OUTPUT_BUFFERS_CHANGED)
  {
    CLog::Log(LOGDEBUG, "CDVDAudioCodecAndroidMediaCodec:: GetData OUTPUT_BUFFERS_CHANGED");
  }
  else if (index == CJNIMediaCodec::INFO_OUTPUT_FORMAT_CHANGED)
  {
    CJNIMediaFormat mediaformat = m_codec->getOutputFormat();
    if (xbmc_jnienv()->ExceptionCheck())
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec::GetData(INFO_OUTPUT_FORMAT_CHANGED) ExceptionCheck: getOutputBuffers");
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
    }
    ConfigureOutputFormat(&mediaformat);
  }
  else if (index == CJNIMediaCodec::INFO_TRY_AGAIN_LATER)
  {
    // normal dequeueOutputBuffer timeout, ignore it.
    m_bufferUsed = 0;
  }
  else
  {
    // we should never get here
    CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec::GetData unknown index({})", index);
  }

  *dst     = m_buffer;
  return m_bufferUsed;
}

void CDVDAudioCodecAndroidMediaCodec::ConfigureOutputFormat(CJNIMediaFormat* mediaformat)
{
  m_samplerate = 0;
  m_channels = 0;

  if (mediaformat->containsKey(CJNIMediaFormat::KEY_SAMPLE_RATE))
    m_samplerate = mediaformat->getInteger(CJNIMediaFormat::KEY_SAMPLE_RATE);
  if (mediaformat->containsKey(CJNIMediaFormat::KEY_CHANNEL_COUNT))
    m_channels = mediaformat->getInteger(CJNIMediaFormat::KEY_CHANNEL_COUNT);

  CLog::Log(LOGDEBUG,
            "CDVDAudioCodecAndroidMediaCodec::ConfigureOutputFormat "
            "sample_rate({}), channel_count({})",
            m_samplerate, m_channels);

  // clear any jni exceptions
  if (xbmc_jnienv()->ExceptionCheck())
  {
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
  }
}
