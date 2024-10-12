/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AESinkAUDIOTRACK.h"

#include "ServiceBroker.h"
#include "cores/AudioEngine/AESinkFactory.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"

#include "platform/android/activity/XBMCApp.h"

#include <numeric>

#include <androidjni/AudioFormat.h>
#include <androidjni/AudioManager.h>
#include <androidjni/AudioTrack.h>
#include <androidjni/Build.h>
#include <unistd.h>

// This is an alternative to the linear weighted delay smoothing
// advantages: only one history value needs to be stored
// in tests the linear weighted average smoother yield better results
//#define AT_USE_EXPONENTIAL_AVERAGING 1

using namespace jni;

using namespace std::chrono_literals;

// those are empirical values while the HD buffer
// is the max TrueHD package
const unsigned int MAX_RAW_AUDIO_BUFFER_HD = 61440;
const unsigned int MAX_RAW_AUDIO_BUFFER = 16384;
const unsigned int MOVING_AVERAGE_MAX_MEMBERS = 3;
const uint64_t UINT64_LOWER_BYTES = 0x00000000FFFFFFFF;
const uint64_t UINT64_UPPER_BYTES = 0xFFFFFFFF00000000;

static const AEChannel KnownChannels[] = {AE_CH_FL, AE_CH_FR,   AE_CH_FC,   AE_CH_LFE,
                                          AE_CH_SL, AE_CH_SR,   AE_CH_BL,   AE_CH_BR,
                                          AE_CH_BC, AE_CH_BLOC, AE_CH_BROC, AE_CH_NULL};

static int AEStreamFormatToATFormat(const CAEStreamInfo::DataType& dt)
{
  switch (dt)
  {
    case CAEStreamInfo::STREAM_TYPE_AC3:
      return CJNIAudioFormat::ENCODING_AC3;
    case CAEStreamInfo::STREAM_TYPE_DTS_512:
    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
      return CJNIAudioFormat::ENCODING_DTS;
    case CAEStreamInfo::STREAM_TYPE_DTSHD:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_MA:
      return CJNIAudioFormat::ENCODING_DTS_HD;
    case CAEStreamInfo::STREAM_TYPE_EAC3:
      return CJNIAudioFormat::ENCODING_E_AC3;
    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
      return CJNIAudioFormat::ENCODING_DOLBY_TRUEHD;
    default:
      return CJNIAudioFormat::ENCODING_PCM_16BIT;
  }
}

static AEChannel AUDIOTRACKChannelToAEChannel(int atChannel)
{
  AEChannel aeChannel;

  /* cannot use switch since CJNIAudioFormat is populated at runtime */

       if (atChannel == CJNIAudioFormat::CHANNEL_OUT_FRONT_LEFT)            aeChannel = AE_CH_FL;
  else if (atChannel == CJNIAudioFormat::CHANNEL_OUT_FRONT_RIGHT)           aeChannel = AE_CH_FR;
  else if (atChannel == CJNIAudioFormat::CHANNEL_OUT_FRONT_CENTER)          aeChannel = AE_CH_FC;
  else if (atChannel == CJNIAudioFormat::CHANNEL_OUT_LOW_FREQUENCY)         aeChannel = AE_CH_LFE;
  else if (atChannel == CJNIAudioFormat::CHANNEL_OUT_BACK_LEFT)             aeChannel = AE_CH_BL;
  else if (atChannel == CJNIAudioFormat::CHANNEL_OUT_BACK_RIGHT)            aeChannel = AE_CH_BR;
  else if (atChannel == CJNIAudioFormat::CHANNEL_OUT_SIDE_LEFT)             aeChannel = AE_CH_SL;
  else if (atChannel == CJNIAudioFormat::CHANNEL_OUT_SIDE_RIGHT)            aeChannel = AE_CH_SR;
  else if (atChannel == CJNIAudioFormat::CHANNEL_OUT_FRONT_LEFT_OF_CENTER)  aeChannel = AE_CH_FLOC;
  else if (atChannel == CJNIAudioFormat::CHANNEL_OUT_FRONT_RIGHT_OF_CENTER) aeChannel = AE_CH_FROC;
  else if (atChannel == CJNIAudioFormat::CHANNEL_OUT_BACK_CENTER)           aeChannel = AE_CH_BC;
  else                                                                      aeChannel = AE_CH_UNKNOWN1;

  return aeChannel;
}

static CAEChannelInfo AUDIOTRACKChannelMaskToAEChannelMap(int atMask)
{
  CAEChannelInfo info;

  int mask = 0x1;
  for (unsigned int i = 0; i < sizeof(int32_t) * 8; i++)
  {
    if (atMask & mask)
      info += AUDIOTRACKChannelToAEChannel(mask);
    mask <<= 1;
  }

  return info;
}

static int AEChannelMapToAUDIOTRACKChannelMask(CAEChannelInfo info)
{
  info.ResolveChannels(CAEChannelInfo(KnownChannels));

  // Sadly Android is quite limited these days with supported formats
  // Therefore only distinguish between Stereo, 5.1 and 7.1
  // simply by the number of speakers.

  if (info.Count() > 6)
    return CJNIAudioFormat::CHANNEL_OUT_7POINT1_SURROUND;

  if (info.Count() > 2)
    return CJNIAudioFormat::CHANNEL_OUT_5POINT1;

  return CJNIAudioFormat::CHANNEL_OUT_STEREO;
}

jni::CJNIAudioTrack *CAESinkAUDIOTRACK::CreateAudioTrack(int stream, int sampleRate, int channelMask, int encoding, int bufferSize)
{
  jni::CJNIAudioTrack *jniAt = NULL;

  try
  {
    CJNIAudioAttributesBuilder attrBuilder;
    attrBuilder.setUsage(CJNIAudioAttributes::USAGE_MEDIA);
    attrBuilder.setContentType(CJNIAudioAttributes::CONTENT_TYPE_MUSIC);

    CJNIAudioFormatBuilder fmtBuilder;
    fmtBuilder.setChannelMask(channelMask);
    fmtBuilder.setEncoding(encoding);
    fmtBuilder.setSampleRate(sampleRate);

    jniAt = new CJNIAudioTrack(attrBuilder.build(),
                               fmtBuilder.build(),
                               bufferSize,
                               CJNIAudioTrack::MODE_STREAM,
                               CJNIAudioManager::AUDIO_SESSION_ID_GENERATE);
  }
  catch (const std::invalid_argument& e)
  {
    CLog::Log(LOGINFO, "AESinkAUDIOTRACK - AudioTrack creation (channelMask {:#08x}): {}",
              channelMask, e.what());
  }

  return jniAt;
}

int CAESinkAUDIOTRACK::AudioTrackWrite(char* audioData, int offsetInBytes, int sizeInBytes)
{
  int     written = 0;
  if (m_jniAudioFormat == CJNIAudioFormat::ENCODING_PCM_FLOAT)
  {
    if (m_floatbuf.size() != (sizeInBytes - offsetInBytes) / sizeof(float))
      m_floatbuf.resize((sizeInBytes - offsetInBytes) / sizeof(float));
    memcpy(m_floatbuf.data(), audioData + offsetInBytes, sizeInBytes - offsetInBytes);
    written = m_at_jni->write(m_floatbuf, 0, (sizeInBytes - offsetInBytes) / sizeof(float),
                              CJNIAudioTrack::WRITE_BLOCKING);
    written *= sizeof(float);
  }
  else if (m_jniAudioFormat == CJNIAudioFormat::ENCODING_IEC61937)
  {
    if (m_shortbuf.size() != (sizeInBytes - offsetInBytes) / sizeof(int16_t))
      m_shortbuf.resize((sizeInBytes - offsetInBytes) / sizeof(int16_t));
    memcpy(m_shortbuf.data(), audioData + offsetInBytes, sizeInBytes - offsetInBytes);
    written = m_at_jni->write(m_shortbuf, 0, (sizeInBytes - offsetInBytes) / sizeof(int16_t),
                              CJNIAudioTrack::WRITE_BLOCKING);
    written *= sizeof(uint16_t);
  }
  else
  {
    if (static_cast<int>(m_charbuf.size()) != (sizeInBytes - offsetInBytes))
      m_charbuf.resize(sizeInBytes - offsetInBytes);
    memcpy(m_charbuf.data(), audioData + offsetInBytes, sizeInBytes - offsetInBytes);
    written =
        m_at_jni->write(m_charbuf, 0, sizeInBytes - offsetInBytes, CJNIAudioTrack::WRITE_BLOCKING);
  }

  return written;
}

int CAESinkAUDIOTRACK::AudioTrackWrite(char* audioData, int sizeInBytes, int64_t timestamp)
{
  int     written = 0;
  std::vector<char> buf;
  buf.reserve(sizeInBytes);
  memcpy(buf.data(), audioData, sizeInBytes);

  CJNIByteBuffer bytebuf = CJNIByteBuffer::wrap(buf);
  written = m_at_jni->write(bytebuf.get_raw(), sizeInBytes, CJNIAudioTrack::WRITE_BLOCKING, timestamp);

  return written;
}

CAEDeviceInfo CAESinkAUDIOTRACK::m_info;
CAEDeviceInfo CAESinkAUDIOTRACK::m_info_iec;
CAEDeviceInfo CAESinkAUDIOTRACK::m_info_raw;
bool CAESinkAUDIOTRACK::m_hasIEC = false;
std::set<unsigned int> CAESinkAUDIOTRACK::m_sink_sampleRates;
bool CAESinkAUDIOTRACK::m_sinkSupportsFloat = false;
bool CAESinkAUDIOTRACK::m_sinkSupportsMultiChannelFloat = false;
bool CAESinkAUDIOTRACK::m_passthrough_use_eac3 = false;

////////////////////////////////////////////////////////////////////////////////////////////
CAESinkAUDIOTRACK::CAESinkAUDIOTRACK()
{
  m_alignedS16 = NULL;
  m_sink_frameSize = 0;
  m_encoding = CJNIAudioFormat::ENCODING_PCM_16BIT;
  m_audiotrackbuffer_sec = 0.0;
  m_audiotrackbuffer_sec_orig = 0.0;
  m_at_jni = NULL;
  m_duration_written = 0;
  m_headPos = 0;
  m_stuckCounter = 0;
  m_headPosOld = 0;
  m_timestampPos = 0;
  m_sink_sampleRate = 0;
  m_passthrough = false;
  m_min_buffer_size = 0;
}

CAESinkAUDIOTRACK::~CAESinkAUDIOTRACK()
{
  Deinitialize();
}

bool CAESinkAUDIOTRACK::VerifySinkConfiguration(int sampleRate,
                                                int channelMask,
                                                int encoding,
                                                bool isRaw)
{
  int minBufferSize = CJNIAudioTrack::getMinBufferSize(sampleRate, channelMask, encoding);
  bool supported = (minBufferSize > 0);

  // make sure to have enough buffer as minimum might not be enough to open
  if (!isRaw)
    minBufferSize *= 2;

  if (supported)
  {
    jni::CJNIAudioTrack* jniAt = CreateAudioTrack(CJNIAudioManager::STREAM_MUSIC, sampleRate,
                                                  channelMask, encoding, minBufferSize);
    supported = (jniAt && jniAt->getState() == CJNIAudioTrack::STATE_INITIALIZED);
    if (supported)
    {
      jniAt->pause();
      jniAt->flush();
    }

    if (jniAt)
    {
      jniAt->release();
      delete jniAt;
    }
  }
  CLog::Log(LOGDEBUG, "VerifySinkConfiguration samplerate: {} mask: {} encoding: {} success: {}",
            sampleRate, channelMask, encoding, supported ? "true" : "false");
  return supported;
}


bool CAESinkAUDIOTRACK::IsSupported(int sampleRateInHz, int channelConfig, int encoding)
{
  int ret = CJNIAudioTrack::getMinBufferSize( sampleRateInHz, channelConfig, encoding);
  return (ret > 0);
}

bool CAESinkAUDIOTRACK::Initialize(AEAudioFormat &format, std::string &device)
{

  // try to recover used device
  if (!m_hasIEC)
    m_info = m_info_raw;
  else if (device == "Default" && !m_info.m_wantsIECPassthrough)
    m_info = m_info_raw;
  else if (device == "AudioTrack (RAW)")
    m_info = m_info_raw;
  else
    m_info = m_info_iec;

  m_format      = format;
  m_headPos = 0;
  m_stuckCounter = 0;
  m_headPosOld = 0;
  m_timestampPos = 0;
  m_linearmovingaverage.clear();
  m_pause_ms = 0.0;
  CLog::Log(LOGDEBUG,
            "CAESinkAUDIOTRACK::Initialize requested: sampleRate {}; format: {}; channels: {}",
            format.m_sampleRate, CAEUtil::DataFormatToStr(format.m_dataFormat),
            format.m_channelLayout.Count());

  int stream = CJNIAudioManager::STREAM_MUSIC;
  m_encoding = CJNIAudioFormat::ENCODING_PCM_16BIT;

  // If the device supports EAC3 passthrough, but not basic AC3 patthrough, send it as EAC3 (which is AC3 compatible) instead
  if (!m_info.m_wantsIECPassthrough)
  {
    if ((m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_AC3) && m_passthrough_use_eac3)
      m_format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_EAC3;
  }

  uint32_t distance = UINT32_MAX; // max upper distance, update at least ones to use one of our samplerates
  for (auto& s : m_sink_sampleRates)
  {
     // prefer best match or alternatively something that divides nicely and
     // is not too far away
     uint32_t d = std::abs((int)m_format.m_sampleRate - (int)s) + 8 * (s > m_format.m_sampleRate ? (s % m_format.m_sampleRate) : (m_format.m_sampleRate % s));
     if (d < distance)
     {
       m_sink_sampleRate = s;
       distance = d;
       CLog::Log(LOGDEBUG, "Updated SampleRate: {} Distance: {}", m_sink_sampleRate, d);
     }
  }

  if (m_format.m_dataFormat == AE_FMT_RAW)
  {
    m_passthrough = true;
    m_encoding = AEStreamFormatToATFormat(m_format.m_streamInfo.m_type);
    m_format.m_channelLayout = AE_CH_LAYOUT_2_0;

    if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTSHD_MA)
    {
      // we keep the 48 khz sample rate, reason: Androids packer only packs DTS Core
      // even if we ask for DTS-HD-MA it seems.
      m_format.m_channelLayout = AE_CH_LAYOUT_7_1;
    }

    if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD)
    {
      m_format.m_channelLayout = AE_CH_LAYOUT_7_1;
      m_sink_sampleRate = 192000;
    }

    // EAC3 needs real samplerate not the modulation
    if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_EAC3)
      m_sink_sampleRate = m_format.m_streamInfo.m_sampleRate;

    if (m_info.m_wantsIECPassthrough)
    {
      m_format.m_dataFormat     = AE_FMT_S16LE;
      if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTSHD ||
          m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTSHD_MA ||
          m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD)
        m_sink_sampleRate = 192000;

      // new Android N format
      if (CJNIAudioFormat::ENCODING_IEC61937 != -1)
      {
        m_encoding = CJNIAudioFormat::ENCODING_IEC61937;
        // this will be sent tunneled, therefore the IEC path needs e.g.
        // 4 * m_format.m_streamInfo.m_sampleRate
        if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_EAC3)
          m_sink_sampleRate = m_format.m_sampleRate;
      }

      // we are running on an old android version
      // that does neither know AC3, DTS or whatever
      // we will fallback to 16BIT passthrough
      if (m_encoding == -1)
      {
        m_format.m_channelLayout = AE_CH_LAYOUT_2_0;
        m_format.m_sampleRate     = m_sink_sampleRate;
        m_encoding = CJNIAudioFormat::ENCODING_PCM_16BIT;
        CLog::Log(LOGDEBUG, "Fallback to PCM passthrough mode - this might not work!");
      }
    }
  }
  else
  {
    m_passthrough = false;
    m_format.m_sampleRate     = m_sink_sampleRate;
    // this is temporarily opt-in via advancedsettings only
    const bool allowMultiFloat =
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_AllowMultiChannelFloat;
    if (allowMultiFloat && m_sinkSupportsMultiChannelFloat)
    {
      m_encoding = CJNIAudioFormat::ENCODING_PCM_FLOAT;
      m_format.m_dataFormat     = AE_FMT_FLOAT;
    }
    else if (m_sinkSupportsFloat && m_format.m_channelLayout.Count() == 2)
    {
      m_encoding = CJNIAudioFormat::ENCODING_PCM_FLOAT;
      m_format.m_dataFormat     = AE_FMT_FLOAT;
    }
    else
    {
      m_encoding = CJNIAudioFormat::ENCODING_PCM_16BIT;
      m_format.m_dataFormat     = AE_FMT_S16LE;
    }
  }

  m_superviseAudioDelay =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_superviseAudioDelay;

  int atChannelMask = AEChannelMapToAUDIOTRACKChannelMask(m_format.m_channelLayout);
  m_format.m_channelLayout  = AUDIOTRACKChannelMaskToAEChannelMap(atChannelMask);
  if (m_encoding == CJNIAudioFormat::ENCODING_IEC61937)
  {
    // keep above channel output if we do IEC61937 and got DTSHD or TrueHD by AudioEngine
    if (m_format.m_streamInfo.m_type != CAEStreamInfo::STREAM_TYPE_DTSHD_MA && m_format.m_streamInfo.m_type != CAEStreamInfo::STREAM_TYPE_TRUEHD)
      atChannelMask = CJNIAudioFormat::CHANNEL_OUT_STEREO;
  }

  bool retried = false;
  while (!m_at_jni)
  {
    CLog::Log(LOGINFO, "Trying to open: samplerate: {}, channelMask: {}, encoding: {}",
              m_sink_sampleRate, atChannelMask, m_encoding);
    int min_buffer = CJNIAudioTrack::getMinBufferSize(m_sink_sampleRate,
                                                         atChannelMask,
                                                         m_encoding);

    if (min_buffer < 0)
    {
      CLog::Log(LOGERROR,
                "Minimum Buffer Size was: {} - disable passthrough (?) your hw does not support it",
                min_buffer);
      return false;
    }

    m_min_buffer_size = (unsigned int) min_buffer;
    CLog::Log(LOGINFO, "Minimum size we need for stream: {} Bytes", m_min_buffer_size);
    double rawlength_in_seconds = 0.0;
    int multiplier = 1;
    unsigned int ac3FrameSize = 1;
    if (m_passthrough && !m_info.m_wantsIECPassthrough)
    {
      switch (m_format.m_streamInfo.m_type)
      {
        case CAEStreamInfo::STREAM_TYPE_TRUEHD:
          m_min_buffer_size = MAX_RAW_AUDIO_BUFFER_HD;
          m_format.m_frames = m_min_buffer_size;
          rawlength_in_seconds = 8 * m_format.m_streamInfo.GetDuration() / 1000; // on average
          break;
        case CAEStreamInfo::STREAM_TYPE_DTSHD_MA:
        case CAEStreamInfo::STREAM_TYPE_DTSHD:
          // normal frame is max  2012 bytes + 2764 sub frame
          m_min_buffer_size = 66432; //according to the buffer model of ISO/IEC13818-1
          m_format.m_frames = m_min_buffer_size;
          rawlength_in_seconds = 8 * m_format.m_streamInfo.GetDuration() / 1000; // average value
          break;
        case CAEStreamInfo::STREAM_TYPE_DTS_512:
        case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
          // max 2012 bytes
          // depending on sample rate between 156 ms and 312 ms
          m_min_buffer_size = 16 * 2012;
          m_format.m_frames = m_min_buffer_size;
          rawlength_in_seconds = 16 * m_format.m_streamInfo.GetDuration() / 1000;
          break;
        case CAEStreamInfo::STREAM_TYPE_DTS_1024:
        case CAEStreamInfo::STREAM_TYPE_DTS_2048:
          m_min_buffer_size = 8 * 5462;
          m_format.m_frames = m_min_buffer_size;
          rawlength_in_seconds = 8 * m_format.m_streamInfo.GetDuration() / 1000;
          break;
        case CAEStreamInfo::STREAM_TYPE_AC3:
          ac3FrameSize = m_format.m_streamInfo.m_frameSize;
          if (ac3FrameSize == 0)
            ac3FrameSize = 1536; // fallback if not set, e.g. Transcoding
          m_min_buffer_size = std::max(m_min_buffer_size * 3, ac3FrameSize * 8);
          m_format.m_frames = m_min_buffer_size;
          multiplier = m_min_buffer_size / ac3FrameSize; // int division is wanted
          rawlength_in_seconds = multiplier * m_format.m_streamInfo.GetDuration() / 1000;
          break;
        // EAC3 is currently not supported
        case CAEStreamInfo::STREAM_TYPE_EAC3:
          m_min_buffer_size = 2 * 10752; // least common multiple of 1792 and 1536
          m_format.m_frames = m_min_buffer_size; // needs testing
          rawlength_in_seconds = 8 * m_format.m_streamInfo.GetDuration() / 1000;
          break;
        default:
          m_min_buffer_size = MAX_RAW_AUDIO_BUFFER;
          m_format.m_frames = m_min_buffer_size;
          rawlength_in_seconds = 0.4;
          break;
      }

      CLog::Log(LOGDEBUG, "Opening Passthrough RAW Format: {} Sink SampleRate: {}",
                CAEUtil::StreamTypeToStr(m_format.m_streamInfo.m_type), m_sink_sampleRate);
      m_format.m_frameSize = 1;
      m_sink_frameSize = m_format.m_frameSize;
    }
    else
    {
      m_format.m_frameSize = m_format.m_channelLayout.Count() * (CAEUtil::DataFormatToBits(m_format.m_dataFormat) / 8);
      m_sink_frameSize = m_format.m_frameSize;
      // aim at max 200 ms buffer and 50 ms periods but at least two periods of min_buffer
      // make sure periods are actually not smaller than 32 ms (32, cause 32 * 2 = 64)
      // but also lower than 64 ms
      // which is large enough to not cause CPU hogging in case 32 ms periods are used
      m_audiotrackbuffer_sec =
          static_cast<double>(m_min_buffer_size) / (m_sink_frameSize * m_sink_sampleRate);
      // the period calculation starts
      // after the buffer division to get even division results
      int c = 1;
      if (m_audiotrackbuffer_sec > 0.25)
      {
        CLog::Log(LOGWARNING,
                  "Audiobuffer is already very large {:f} ms - Reducing to a sensible value",
                  1000.0 * m_audiotrackbuffer_sec);
        int buffer_frames = m_sink_sampleRate / 4; // 250 ms
        m_min_buffer_size = buffer_frames * m_sink_frameSize;
        c = 5; // 50 ms
      }
      // update potential new buffertime
      m_audiotrackbuffer_sec =
          static_cast<double>(m_min_buffer_size) / (m_sink_frameSize * m_sink_sampleRate);
      constexpr double max_time = 0.064;
      constexpr double min_time = 0.032;
      constexpr double target_duration = 0.128;

      while (m_audiotrackbuffer_sec < target_duration)
      {
        m_min_buffer_size += min_buffer;
        c++;
        m_audiotrackbuffer_sec =
            static_cast<double>(m_min_buffer_size) / (m_sink_frameSize * m_sink_sampleRate);
      }
      unsigned int period_size = m_min_buffer_size / c;
      double period_time =
          static_cast<double>(period_size) / (m_sink_frameSize * m_sink_sampleRate);

      // This will result in minimum 32 ms
      while (period_time >= max_time)
      {
        period_time /= 2;
        period_size /= 2;
      }
      // If the audio track API gave us very low values increase them
      // In this case the first loop would not have been run at all
      while (period_time < min_time)
      {
        period_size *= 2;
        period_time *= 2;
      }
      m_format.m_frames = static_cast<int>(period_size / m_format.m_frameSize);

      CLog::Log(LOGINFO,
                "Audiotrack buffer params are: period time = {:.3f} ms, period size = "
                "{} bytes, num periods = {}",
                period_time * 1000, period_size, m_min_buffer_size / period_size);
    }

    if (m_passthrough && !m_info.m_wantsIECPassthrough)
      m_audiotrackbuffer_sec = rawlength_in_seconds;

    CLog::Log(LOGINFO,
              "Created Audiotrackbuffer with playing time of {:f} ms min buffer size: {} bytes",
              m_audiotrackbuffer_sec * 1000, m_min_buffer_size);

    m_audiotrackbuffer_sec_orig = m_audiotrackbuffer_sec;

    m_jniAudioFormat = m_encoding;
    m_at_jni = CreateAudioTrack(stream, m_sink_sampleRate, atChannelMask,
                                m_encoding, m_min_buffer_size);

    if (!IsInitialized())
    {
      if (!m_passthrough)
      {
        if (atChannelMask != CJNIAudioFormat::CHANNEL_OUT_STEREO &&
            atChannelMask != CJNIAudioFormat::CHANNEL_OUT_5POINT1)
        {
          atChannelMask = CJNIAudioFormat::CHANNEL_OUT_5POINT1;
          CLog::Log(LOGDEBUG, "AESinkAUDIOTRACK - Retrying multichannel playback with a 5.1 layout");
          continue;
        }
        else if (atChannelMask != CJNIAudioFormat::CHANNEL_OUT_STEREO)
        {
          atChannelMask = CJNIAudioFormat::CHANNEL_OUT_STEREO;
          CLog::Log(LOGDEBUG, "AESinkAUDIOTRACK - Retrying with a stereo layout");
          continue;
        }
      }
      else
      {
        if (!retried)
        {
          retried = true;
          CLog::Log(LOGWARNING, "AESinkAUDIOTRACK - Unable to open PT device - will retry once");
          // Seems that some devices don't properly implement pause + flush, which during seek
          // might open the device too fast - let's retry
          usleep(200 * 1000);
          continue;
        }
      }
      CLog::Log(LOGERROR, "AESinkAUDIOTRACK - Unable to create AudioTrack");
      Deinitialize();
      return false;
    }
    const char* method = m_passthrough ? (m_info.m_wantsIECPassthrough ? "IEC (PT)" : "RAW (PT)") : "PCM";
    CLog::Log(LOGINFO,
              "CAESinkAUDIOTRACK::Initializing with: m_sampleRate: {} format: {} (AE) method: {} "
              "stream-type: {} min_buffer_size: {} m_frames: {} m_frameSize: {} channels: {}",
              m_sink_sampleRate, CAEUtil::DataFormatToStr(m_format.m_dataFormat), method,
              m_passthrough ? CAEUtil::StreamTypeToStr(m_format.m_streamInfo.m_type) : "PCM-STREAM",
              m_min_buffer_size, m_format.m_frames, m_format.m_frameSize,
              m_format.m_channelLayout.Count());
  }
  format = m_format;

  return true;
}

void CAESinkAUDIOTRACK::Deinitialize()
{
  CLog::Log(LOGDEBUG, "CAESinkAUDIOTRACK::Deinitialize");

  if (!m_at_jni)
    return;

  if (IsInitialized())
  {
    m_at_jni->pause();
    m_at_jni->flush();
  }
  m_at_jni->release();

  m_duration_written = 0;
  m_headPos = 0;
  m_headPosOld = 0;
  m_timestampPos = 0;
  m_stampTimer.SetExpired();

  m_linearmovingaverage.clear();

  delete m_at_jni;
  m_at_jni = NULL;
  m_delay = 0.0;
  m_hw_delay = 0.0;
}

bool CAESinkAUDIOTRACK::IsInitialized()
{
  return (m_at_jni && m_at_jni->getState() == CJNIAudioTrack::STATE_INITIALIZED);
}

void CAESinkAUDIOTRACK::GetDelay(AEDelayStatus& status)
{
  if (!m_at_jni)
  {
    status.SetDelay(0);
    return;
  }

  bool usesAdvancedLogging = CServiceBroker::GetLogging().CanLogComponent(LOGAUDIO);
  // In their infinite wisdom, Google decided to make getPlaybackHeadPosition
  // return a 32bit "int" that you should "interpret as unsigned."  As such,
  // for wrap safety, we need to do all ops on it in 32bit integer math.

  uint32_t head_pos = (uint32_t)m_at_jni->getPlaybackHeadPosition();

  // Wraparound
  if ((uint32_t)(m_headPos & UINT64_LOWER_BYTES) > head_pos) // need to compute wraparound
    m_headPos += (1ULL << 32); // add wraparound, e.g. 0x0000 FFFF FFFF -> 0x0001 FFFF FFFF
  // clear lower 32 bit values, e.g. 0x0001 FFFF FFFF -> 0x0001 0000 0000
  // and add head_pos which wrapped around, e.g. 0x0001 0000 0000 -> 0x0001 0000 0004
  m_headPos = (m_headPos & UINT64_UPPER_BYTES) | (uint64_t)head_pos;
  // check if sink is stuck
  if (m_headPos == m_headPosOld)
    m_stuckCounter++;
  else
  {
    m_stuckCounter = 0;
    m_headPosOld = m_headPos;
  }

  double gone = static_cast<double>(m_headPos) / m_sink_sampleRate;

  // if sink is run dry without buffer time written anymore
  if (gone > m_duration_written)
    gone = m_duration_written;

  double delay = m_duration_written - gone;

  if (m_stampTimer.IsTimePast())
  {
    if (!m_at_jni->getTimestamp(m_timestamp))
    {
      CLog::Log(LOGDEBUG, "Could not acquire timestamp");
      m_stampTimer.Set(100ms);
    }
    else
    {
      // check if frameposition is valid and nano timer less than 50 ms outdated
      if (m_timestamp.get_framePosition() > 0 &&
          (CurrentHostCounter() - m_timestamp.get_nanoTime()) < 50 * 1000 * 1000)
        m_stampTimer.Set(1000ms);
      else
        m_stampTimer.Set(100ms);
    }
  }
  if (usesAdvancedLogging)
    CLog::Log(LOGINFO, "RAW Head-Position {}", m_headPos);
  // check if last value was received less than 2 seconds ago
  if (m_timestamp.get_framePosition() > 0 &&
      (CurrentHostCounter() - m_timestamp.get_nanoTime()) < 2 * 1000 * 1000 * 1000)
  {
    if (usesAdvancedLogging)
    {
      CLog::Log(LOGINFO, "Framecounter: {} Time: {} Current-Time: {}",
                (m_timestamp.get_framePosition() & UINT64_LOWER_BYTES), m_timestamp.get_nanoTime(),
                CurrentHostCounter());
    }
    uint64_t delta = static_cast<uint64_t>(CurrentHostCounter() - m_timestamp.get_nanoTime());
    uint64_t stamphead =
        static_cast<uint64_t>(m_timestamp.get_framePosition() & UINT64_LOWER_BYTES) +
        delta * m_sink_sampleRate / 1000000000.0;
    // wrap around
    // e.g. 0xFFFFFFFFFFFF0123 -> 0x0000000000002478
    // because we only query each second the simple smaller comparison won't suffice
    // as delay can fluctuate minimally
    if (stamphead < m_timestampPos && (m_timestampPos - stamphead) > 0x7FFFFFFFFFFFFFFFULL)
    {
      uint64_t stamp = m_timestampPos;
      stamp += (1ULL << 32);
      stamphead = (stamp & UINT64_UPPER_BYTES) | stamphead;
      CLog::Log(LOGDEBUG, "Wraparound happened old: {} new: {}", m_timestampPos, stamphead);
    }
    m_timestampPos = stamphead;

    double playtime = m_timestampPos / static_cast<double>(m_sink_sampleRate);

    if (usesAdvancedLogging)
    {
      CLog::Log(LOGINFO,
                "Delay - Timestamp: {} (ms) delta: {} (ms) playtime: {} (ms) Duration: {} ms",
                1000.0 * (m_duration_written - playtime), delta / 1000000.0, playtime * 1000,
                m_duration_written * 1000);
      CLog::Log(LOGINFO, "Head-Position {} Timestamp Position {} Delay-Offset: {} ms", m_headPos,
                m_timestampPos,
                1000.0 * (static_cast<int64_t>(m_headPos - m_timestampPos)) / m_sink_sampleRate);
    }
    double hw_delay = m_duration_written - playtime;
    // correct by subtracting above measured delay, if lower delay gets automatically reduced
    hw_delay -= delay;
    // sometimes at the beginning of the stream m_timestampPos is more accurate and ahead of
    // m_headPos - don't use the computed value then and wait
    if (hw_delay > -1.0 && hw_delay < 1.0)
      m_hw_delay = hw_delay;
    else
      m_hw_delay = 0.0;
    if (usesAdvancedLogging)
    {
      CLog::Log(LOGINFO, "HW-Delay (1): {} ms", hw_delay * 1000);
    }
  }

  delay += m_hw_delay;

  if (usesAdvancedLogging)
  {
    CLog::Log(LOGINFO, "Combined Delay: {} ms", delay * 1000);
  }
  if (delay < 0.0)
    delay = 0.0;

  // the RAW hack for simulating pause bursts should not come
  // into the way of hw delay
  if (m_pause_ms > m_audiotrackbuffer_sec * 1000.0)
    m_pause_ms = m_audiotrackbuffer_sec * 1000.0;

  if (m_pause_ms > 0.0)
  {
    if (delay < m_audiotrackbuffer_sec)
      delay = m_audiotrackbuffer_sec;
    else
      m_audiotrackbuffer_sec = delay;
  }
  const double d = GetMovingAverageDelay(delay);

  // Audiotrack is caching more than we thought it would
  if (d > m_audiotrackbuffer_sec)
    m_audiotrackbuffer_sec = d;

  // track delay in local member
  m_delay = d;
  if (usesAdvancedLogging)
  {
    CLog::Log(LOGINFO, "Delay Current: {:f} ms", d * 1000);
    if (m_pause_ms > 0.0)
      CLog::Log(LOGINFO, "Delay faked due to pause delay: {:f} ms", m_pause_ms);
  }
  status.SetDelay(d);
}

double CAESinkAUDIOTRACK::GetLatency()
{
  return 0.0;
}

double CAESinkAUDIOTRACK::GetCacheTotal()
{
  // total amount that the audio sink can buffer in units of seconds
  return m_audiotrackbuffer_sec;
}

// this method is supposed to block until all frames are written to the device buffer
// when it returns ActiveAESink will take the next buffer out of a queue
unsigned int CAESinkAUDIOTRACK::AddPackets(uint8_t **data, unsigned int frames, unsigned int offset)
{
  if (!IsInitialized())
    return INT_MAX;

  // If the sink did not move twice the buffer size in time it was opened
  // take action. Some sinks open with e.g. 128 ms nicely but under the
  // hood need a bit more samples to start moving on sink start.
  // Simple equation: N x stime packages in ms > 2 configured audiotrack_buffer in ms
  // will result in the error condition triggering.

  const bool isRawPt = m_passthrough && !m_info.m_wantsIECPassthrough;
  if (!isRawPt)
  {
    const double max_stuck_delay_ms = m_audiotrackbuffer_sec_orig * 2000.0;
    const double stime_ms = 1000.0 * frames / m_format.m_sampleRate;

    if (m_superviseAudioDelay && (m_stuckCounter * stime_ms > max_stuck_delay_ms))
    {
      CLog::Log(LOGERROR, "Sink got stuck with {:f} ms - ask AE for reopening", max_stuck_delay_ms);
      usleep(max_stuck_delay_ms * 1000);
      return INT_MAX;
    }
  }

  // for debugging only - can be removed if everything is really stable
  uint64_t startTime = CurrentHostCounter();

  uint8_t *buffer = data[0]+offset*m_format.m_frameSize;
  uint8_t *out_buf = buffer;
  int size = frames * m_format.m_frameSize;

  // write as many frames of audio as we can fit into our internal buffer.
  int written = 0;
  int loop_written = 0;
  if (frames)
  {
    if (m_at_jni->getPlayState() != CJNIAudioTrack::PLAYSTATE_PLAYING)
      m_at_jni->play();

    bool retried = false;
    int size_left = size;
    while (written < size)
    {
      loop_written = AudioTrackWrite((char*)out_buf, 0, size_left);
      written += loop_written;
      size_left -= loop_written;

      if (loop_written < 0)
      {
        CLog::Log(LOGERROR, "CAESinkAUDIOTRACK::AddPackets write returned error:  {}",
                  loop_written);
        return INT_MAX;
      }

      // if we could not add any data - sleep a bit and retry
      if (loop_written == 0)
      {
        if (!retried)
        {
          retried = true;
          double sleep_time = 0;
          if (m_passthrough && !m_info.m_wantsIECPassthrough)
          {
            sleep_time = m_format.m_streamInfo.GetDuration();
            usleep(sleep_time * 1000);
          }
          else
          {
            sleep_time = 1000.0 * m_format.m_frames / m_format.m_sampleRate;
            usleep(sleep_time * 1000);
          }
          bool playing = m_at_jni->getPlayState() == CJNIAudioTrack::PLAYSTATE_PLAYING;
          CLog::Log(LOGDEBUG, "Retried to write onto the sink - slept: {:f} playing: {}",
                    sleep_time, playing ? "yes" : "no");
          continue;
        }
        else
        {
          CLog::Log(LOGDEBUG, "Repeatedly tried to write onto the sink - giving up");
          break;
        }
      }
      retried = false; // at least one time there was more than zero data written
      if (m_passthrough && !m_info.m_wantsIECPassthrough)
      {
        if (written == size)
          m_duration_written += m_format.m_streamInfo.GetDuration() / 1000;
        else
        {
          CLog::Log(LOGDEBUG, "Error writing full package to sink, left: {}", size_left);
          // Let AE wait some ms to come back
          unsigned int written_frames = (unsigned int) (written/m_format.m_frameSize);
          return written_frames;
        }
      }
      else
      {
        double duration =
            (static_cast<double>(loop_written) / m_format.m_frameSize) / m_format.m_sampleRate;
        m_duration_written += duration;
      }

      // just try again to care for fragmentation
      if (written < size)
        out_buf = out_buf + loop_written;

      loop_written = 0;
    }
  }
  unsigned int written_frames = static_cast<unsigned int>(written / m_format.m_frameSize);
  double time_to_add_ms = 1000.0 * (CurrentHostCounter() - startTime) / CurrentHostFrequency();
  // Get Back in Sync with faked pause bursts
  if (m_passthrough && !m_info.m_wantsIECPassthrough)
  {
    if (m_pause_ms > 0.0)
    {
      // Idea here is: Slowly correct the wrong buffer so that AE should not realize
      // but do not underrun while doing so
      double extra_sleep_ms = m_format.m_streamInfo.GetDuration() / 2.0 - time_to_add_ms;
      if (extra_sleep_ms > 0)
      {
        CLog::Log(LOGDEBUG, "Sleeping for {:f}", extra_sleep_ms);
        m_pause_ms -= extra_sleep_ms;
        usleep(extra_sleep_ms * 1000);
      }
      if (m_pause_ms <= 0.0)
      {
        m_pause_ms = 0.0;
        extra_sleep_ms = 0.0;
        CLog::Log(LOGDEBUG, "Resetting pause bursts as buffer level was reached! (1)");
      }
    }
  }
  else
  {
    // waiting should only be done if sink is not run dry
    double period_time = m_format.m_frames / static_cast<double>(m_sink_sampleRate);
    if (m_delay >= (m_audiotrackbuffer_sec - period_time))
    {
      double time_should_ms = 1000.0 * written_frames / m_format.m_sampleRate;
      double time_off = time_should_ms - time_to_add_ms;
      if (time_off > 0)
        usleep(time_off * 500); // sleep half the error on average away
    }
  }
  return written_frames;
}

void CAESinkAUDIOTRACK::AddPause(unsigned int millis)
{
  if (!m_at_jni)
    return;

  // just sleep out the frames
  if (m_at_jni->getPlayState() != CJNIAudioTrack::PLAYSTATE_PAUSED)
    m_at_jni->pause();

  // This is a mixture to get it right between
  // blocking, sleeping roughly and GetDelay smoothing
  // In short: Shit in, shit out
  usleep(millis * 1000);
  m_pause_ms += millis;
}

void CAESinkAUDIOTRACK::Drain()
{
  if (!m_at_jni)
    return;

  CLog::Log(LOGDEBUG, "Draining Audio");
  if (IsInitialized())
  {
    m_at_jni->stop();
    // stay ready
    m_at_jni->pause();
  }
  m_duration_written = 0;
  m_headPos = 0;
  m_stuckCounter = 0;
  m_timestampPos = 0;
  m_linearmovingaverage.clear();
  m_stampTimer.SetExpired();
  m_pause_ms = 0.0;
}

void CAESinkAUDIOTRACK::Register()
{
  AE::AESinkRegEntry entry;
  entry.sinkName = "AUDIOTRACK";
  entry.createFunc = CAESinkAUDIOTRACK::Create;
  entry.enumerateFunc = CAESinkAUDIOTRACK::EnumerateDevicesEx;
  AE::CAESinkFactory::RegisterSink(entry);
}

std::unique_ptr<IAESink> CAESinkAUDIOTRACK::Create(std::string& device,
                                                   AEAudioFormat& desiredFormat)
{
  auto sink = std::make_unique<CAESinkAUDIOTRACK>();
  if (sink->Initialize(desiredFormat, device))
    return sink;

  return {};
}

void CAESinkAUDIOTRACK::EnumerateDevicesEx(AEDeviceInfoList &list, bool force)
{
  // Clear everything
  m_info.m_channels.Reset();
  m_info.m_dataFormats.clear();
  m_info.m_sampleRates.clear();
  m_info.m_streamTypes.clear();
  m_sink_sampleRates.clear();

  m_info.m_deviceType = AE_DEVTYPE_PCM;
  m_info.m_deviceName = "AudioTrack (IEC)";
  m_info.m_displayName = "AudioTrack (IEC)";
  m_info.m_displayNameExtra = "Kodi IEC packer (recommended)";

  // Query IEC capabilities
  bool isRaw = false;
  if (CJNIAudioFormat::ENCODING_IEC61937 != -1)
  {
    UpdateAvailablePCMCapabilities();
    UpdateAvailablePassthroughCapabilities(isRaw);

    if (!m_info.m_streamTypes.empty())
    {
      m_info_iec = m_info;
      list.push_back(m_info_iec);
      m_hasIEC = true;
    }
  }

  // Query RAW capabilities
  isRaw = true;
  m_info.m_deviceName = "AudioTrack (RAW)";
  m_info.m_displayName = "AudioTrack (RAW)";
  m_info.m_displayNameExtra = "Android IEC packer";
  UpdateAvailablePCMCapabilities();
  UpdateAvailablePassthroughCapabilities(isRaw);
  m_info_raw = m_info;

  // no need to display two PCM sinks - as they are the same
  if (!list.empty())
    m_info_raw.m_onlyPassthrough = true;

  list.push_back(m_info_raw);
}

void CAESinkAUDIOTRACK::UpdateAvailablePassthroughCapabilities(bool isRaw)
{
  m_info.m_deviceType = AE_DEVTYPE_HDMI;
  m_info.m_wantsIECPassthrough = false;
  m_info.m_dataFormats.push_back(AE_FMT_RAW);
  m_info.m_streamTypes.clear();
  bool supports_192khz = m_sink_sampleRates.find(192000) != m_sink_sampleRates.end();

  if (isRaw)
  {
    bool canDoAC3 = false;

    if (CJNIAudioFormat::ENCODING_AC3 != -1)
    {
      if (VerifySinkConfiguration(48000, CJNIAudioFormat::CHANNEL_OUT_STEREO,
                                  CJNIAudioFormat::ENCODING_AC3, true))
      {
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_AC3);
        CLog::Log(LOGDEBUG, "Firmware implements AC3 RAW");

        canDoAC3 = true;
      }
    }

    // EAC3 working on shield, broken on FireTV
    if (CJNIAudioFormat::ENCODING_E_AC3 != -1)
    {
      if (VerifySinkConfiguration(48000, CJNIAudioFormat::CHANNEL_OUT_STEREO,
                                  CJNIAudioFormat::ENCODING_E_AC3, true))
      {
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_EAC3);
        CLog::Log(LOGDEBUG, "Firmware implements EAC3 RAW");

        if (!canDoAC3)
          m_passthrough_use_eac3 = true;
      }
    }

    if (CJNIAudioFormat::ENCODING_DTS != -1)
    {
      if (VerifySinkConfiguration(48000, CJNIAudioFormat::CHANNEL_OUT_STEREO,
                                  CJNIAudioFormat::ENCODING_DTS, true))
      {
        CLog::Log(LOGDEBUG, "Firmware implements DTS RAW");
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD_CORE);
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_1024);
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_2048);
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_512);
      }
    }

    if (CJNIAudioFormat::ENCODING_DTS_HD != -1)
    {
      if (VerifySinkConfiguration(48000, AEChannelMapToAUDIOTRACKChannelMask(AE_CH_LAYOUT_7_1),
                                  CJNIAudioFormat::ENCODING_DTS_HD, true))
      {
        CLog::Log(LOGDEBUG, "Firmware implements DTS-HD RAW");
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD);
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD_MA);
      }
    }
    if (CJNIAudioFormat::ENCODING_DOLBY_TRUEHD != -1 && supports_192khz)
    {
      if (VerifySinkConfiguration(192000, AEChannelMapToAUDIOTRACKChannelMask(AE_CH_LAYOUT_7_1),
                                  CJNIAudioFormat::ENCODING_DOLBY_TRUEHD, true))
      {
        CLog::Log(LOGDEBUG, "Firmware implements TrueHD RAW");
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_TRUEHD);
      }
    }
  }
  else
  {
    // Android v24 and backports can do real IEC API
    if (CJNIAudioFormat::ENCODING_IEC61937 != -1)
    {
      // check if we support opening an IEC sink at all:
      bool supports_iec = VerifySinkConfiguration(48000, CJNIAudioFormat::CHANNEL_OUT_STEREO,
                                                  CJNIAudioFormat::ENCODING_IEC61937);
      if (supports_iec)
      {
        m_info.m_wantsIECPassthrough = true;
        m_info.m_streamTypes.clear();
        m_info.m_dataFormats.push_back(AE_FMT_RAW);
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_AC3);
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD_CORE);
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_1024);
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_2048);
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_512);
        CLog::Log(LOGDEBUG, "AESinkAUDIOTrack: Using IEC PT mode: {}",
                  CJNIAudioFormat::ENCODING_IEC61937);
        CLog::Log(LOGDEBUG, "AC3 and DTS via IEC61937 is supported");
        if (supports_192khz)
        {
          // Check for IEC 2 channel 192 khz PT DTS-HD-HR and E-AC3
          if (VerifySinkConfiguration(192000, CJNIAudioFormat::CHANNEL_OUT_STEREO,
                                      CJNIAudioFormat::ENCODING_IEC61937))
          {
            m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_EAC3);
            m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD);
            CLog::Log(LOGDEBUG, "E-AC3 and DTSHD-HR via IEC61937 is supported");
          }
          // Check for IEC 8 channel 192 khz PT DTS-HD-MA and TrueHD
          int atChannelMask = AEChannelMapToAUDIOTRACKChannelMask(AE_CH_LAYOUT_7_1);
          if (VerifySinkConfiguration(192000, atChannelMask, CJNIAudioFormat::ENCODING_IEC61937))
          {
            m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD_MA);
            m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_TRUEHD);
            CLog::Log(LOGDEBUG, "DTSHD-MA and TrueHD via IEC61937 is supported");
          }
        }
      }
    }
  }
}

void CAESinkAUDIOTRACK::UpdateAvailablePCMCapabilities()
{
  m_info.m_channels = KnownChannels;

  // default fallback format
  m_info.m_dataFormats.push_back(AE_FMT_S16LE);
  unsigned int native_sampleRate = CJNIAudioTrack::getNativeOutputSampleRate(CJNIAudioManager::STREAM_MUSIC);
  m_sink_sampleRates.insert(native_sampleRate);

  int encoding = CJNIAudioFormat::ENCODING_PCM_16BIT;
  m_sinkSupportsFloat = VerifySinkConfiguration(native_sampleRate, CJNIAudioFormat::CHANNEL_OUT_STEREO, CJNIAudioFormat::ENCODING_PCM_FLOAT);

  m_sinkSupportsMultiChannelFloat =
      VerifySinkConfiguration(native_sampleRate, CJNIAudioFormat::CHANNEL_OUT_7POINT1_SURROUND,
                              CJNIAudioFormat::ENCODING_PCM_FLOAT);

  if (m_sinkSupportsFloat)
  {
    encoding = CJNIAudioFormat::ENCODING_PCM_FLOAT;
    m_info.m_dataFormats.push_back(AE_FMT_FLOAT);
    CLog::Log(LOGINFO, "Float is supported");
  }
  if (m_sinkSupportsMultiChannelFloat)
  {
    CLog::Log(LOGINFO, "Multi channel Float is supported");
  }

  int test_sample[] = { 32000, 44100, 48000, 88200, 96000, 176400, 192000 };
  int test_sample_sz = sizeof(test_sample) / sizeof(int);

  for (int i = 0; i < test_sample_sz; ++i)
  {
    if (IsSupported(test_sample[i], CJNIAudioFormat::CHANNEL_OUT_STEREO, encoding))
    {
      m_sink_sampleRates.insert(test_sample[i]);
      CLog::Log(LOGDEBUG, "AESinkAUDIOTRACK - {} supported", test_sample[i]);
    }
  }
  std::copy(m_sink_sampleRates.begin(), m_sink_sampleRates.end(), std::back_inserter(m_info.m_sampleRates));
}

double CAESinkAUDIOTRACK::GetMovingAverageDelay(double newestdelay)
{
#if defined AT_USE_EXPONENTIAL_AVERAGING
  double old = 0.0;
  if (m_linearmovingaverage.empty()) // just for creating one space in list
    m_linearmovingaverage.push_back(newestdelay);
  else
    old = m_linearmovingaverage.front();

  const double alpha = 0.3;
  const double beta = 0.7;

  double d = alpha * newestdelay + beta * old;
  m_linearmovingaverage.at(0) = d;

  return d;
#endif
  m_linearmovingaverage.push_back(newestdelay);

  // new values are in the back, old values are in the front
  // oldest value is removed if elements > MOVING_AVERAGE_MAX_MEMBERS
  // removing first element of a vector sucks - I know that
  // but hey - 10 elements - not 1 million
  size_t size = m_linearmovingaverage.size();
  if (size > MOVING_AVERAGE_MAX_MEMBERS)
  {
    m_linearmovingaverage.pop_front();
    size--;
  }
  double sum = std::accumulate(m_linearmovingaverage.begin(), m_linearmovingaverage.end(), 0.0);

  return sum / size;
}

