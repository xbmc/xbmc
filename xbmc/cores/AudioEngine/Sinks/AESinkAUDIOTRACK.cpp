 /*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include "AESinkAUDIOTRACK.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/AudioEngine/Utils/AERingBuffer.h"
#include "cores/AudioEngine/Utils/AEPackIEC61937.h"
#include "android/activity/XBMCApp.h"
#include "settings/Settings.h"
#if defined(HAS_LIBAMCODEC)
#include "utils/AMLUtils.h"
#endif
#include "utils/log.h"
#include "utils/StringUtils.h"

#include "android/jni/AudioFormat.h"
#include "android/jni/AudioManager.h"
#include "android/jni/AudioTrack.h"
#include "android/jni/Build.h"

//#define DEBUG_VERBOSE 1

using namespace jni;

/*
 * ADT-1 on L preview as of 2014-10 downmixes all non-5.1/7.1 content
 * to stereo, so use 7.1 or 5.1 for all multichannel content for now to
 * avoid that (except passthrough).
 * If other devices surface that support other multichannel layouts,
 * this should be disabled or adapted accordingly.
 */
#define LIMIT_TO_STEREO_AND_5POINT1_AND_7POINT1 1

#define TRUEHD_UNIT 960

static const AEChannel KnownChannels[] = { AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_SL, AE_CH_SR, AE_CH_BL, AE_CH_BR, AE_CH_BC, AE_CH_BLOC, AE_CH_BROC, AE_CH_NULL };

static bool Has71Support()
{
  /* Android 5.0 introduced side channels */
  return CJNIAudioManager::GetSDKVersion() >= 21;
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

static int AEChannelToAUDIOTRACKChannel(AEChannel aeChannel)
{
  int atChannel;
  switch (aeChannel)
  {
    case AE_CH_FL:    atChannel = CJNIAudioFormat::CHANNEL_OUT_FRONT_LEFT; break;
    case AE_CH_FR:    atChannel = CJNIAudioFormat::CHANNEL_OUT_FRONT_RIGHT; break;
    case AE_CH_FC:    atChannel = CJNIAudioFormat::CHANNEL_OUT_FRONT_CENTER; break;
    case AE_CH_LFE:   atChannel = CJNIAudioFormat::CHANNEL_OUT_LOW_FREQUENCY; break;
    case AE_CH_BL:    atChannel = CJNIAudioFormat::CHANNEL_OUT_BACK_LEFT; break;
    case AE_CH_BR:    atChannel = CJNIAudioFormat::CHANNEL_OUT_BACK_RIGHT; break;
    case AE_CH_SL:    atChannel = CJNIAudioFormat::CHANNEL_OUT_SIDE_LEFT; break;
    case AE_CH_SR:    atChannel = CJNIAudioFormat::CHANNEL_OUT_SIDE_RIGHT; break;
    case AE_CH_BC:    atChannel = CJNIAudioFormat::CHANNEL_OUT_BACK_CENTER; break;
    case AE_CH_FLOC:  atChannel = CJNIAudioFormat::CHANNEL_OUT_FRONT_LEFT_OF_CENTER; break;
    case AE_CH_FROC:  atChannel = CJNIAudioFormat::CHANNEL_OUT_FRONT_RIGHT_OF_CENTER; break;
    default:          atChannel = CJNIAudioFormat::CHANNEL_INVALID; break;
  }
  return atChannel;
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
#ifdef LIMIT_TO_STEREO_AND_5POINT1_AND_7POINT1
  if (info.Count() > 6 && Has71Support())
    return CJNIAudioFormat::CHANNEL_OUT_5POINT1
         | CJNIAudioFormat::CHANNEL_OUT_SIDE_LEFT
         | CJNIAudioFormat::CHANNEL_OUT_SIDE_RIGHT;
  else if (info.Count() > 2)
    return CJNIAudioFormat::CHANNEL_OUT_5POINT1;
  else
    return CJNIAudioFormat::CHANNEL_OUT_STEREO;
#endif

  info.ResolveChannels(KnownChannels);

  int atMask = 0;

  for (unsigned int i = 0; i < info.Count(); i++)
    atMask |= AEChannelToAUDIOTRACKChannel(info[i]);

  return atMask;
}

static jni::CJNIAudioTrack *CreateAudioTrack(int stream, int sampleRate, int channelMask, int encoding, int bufferSize)
{
  jni::CJNIAudioTrack *jniAt = NULL;

  try
  {
    jniAt = new CJNIAudioTrack(stream,
                               sampleRate,
                               channelMask,
                               encoding,
                               bufferSize,
                               CJNIAudioTrack::MODE_STREAM);
  }
  catch (const std::invalid_argument& e)
  {
    CLog::Log(LOGINFO, "AESinkAUDIOTRACK - AudioTrack creation (channelMask 0x%08x): %s", channelMask, e.what());
  }

  return jniAt;
}


CAEDeviceInfo CAESinkAUDIOTRACK::m_info;
std::set<unsigned int> CAESinkAUDIOTRACK::m_sink_sampleRates;

////////////////////////////////////////////////////////////////////////////////////////////
CAESinkAUDIOTRACK::CAESinkAUDIOTRACK()
{
  m_alignedS16 = NULL;
  m_sink_frameSize = 0;
  m_audiotrackbuffer_sec = 0.0;
  m_at_jni = NULL;
  m_frames_written = 0;
  m_lastHeadPosition = 0;
  m_ptOffset = 0;
}

CAESinkAUDIOTRACK::~CAESinkAUDIOTRACK()
{
  Deinitialize();
}

bool CAESinkAUDIOTRACK::IsSupported(int sampleRateInHz, int channelConfig, int encoding)
{
  int ret = CJNIAudioTrack::getMinBufferSize( sampleRateInHz, channelConfig, encoding);
  return (ret > 0);
}

bool CAESinkAUDIOTRACK::Initialize(AEAudioFormat &format, std::string &device)
{
  m_format      = format;
  m_volume      = -1;
  m_silenceframes = 0;

  CLog::Log(LOGDEBUG, "CAESinkAUDIOTRACK::Initialize requested: sampleRate %u; format: %s; channels: %d", format.m_sampleRate, CAEUtil::DataFormatToStr(format.m_dataFormat), format.m_channelLayout.Count());

  int stream = CJNIAudioManager::STREAM_MUSIC;

  // Get equal or lower supported sample rate
  std::set<unsigned int>::iterator s = m_sink_sampleRates.upper_bound(m_format.m_sampleRate);
  if (--s != m_sink_sampleRates.begin())
    m_sink_sampleRate = *s;
  else
    m_sink_sampleRate = CJNIAudioTrack::getNativeOutputSampleRate(CJNIAudioManager::STREAM_MUSIC);

  if (AE_IS_RAW(m_format.m_dataFormat) && !CXBMCApp::IsHeadsetPlugged())
  {
    m_passthrough = true;
    switch (m_format.m_dataFormat)
    {
      case AE_FMT_AC3_RAW:
        m_encoding              = CJNIAudioFormat::ENCODING_AC3;
        m_format.m_channelLayout = AE_CH_LAYOUT_2_0;
        m_format.m_frames       = AC3_FRAME_SIZE * m_format.m_sampleRate / m_format.m_encodedRate;
        m_sink_sampleRate       = m_format.m_encodedRate;
        break;

      case AE_FMT_EAC3_RAW:
        m_encoding              = CJNIAudioFormat::ENCODING_E_AC3;
        m_format.m_channelLayout = AE_CH_LAYOUT_2_0;
        m_format.m_frames       = AC3_FRAME_SIZE * m_format.m_sampleRate / m_format.m_encodedRate;
        m_sink_sampleRate       = m_format.m_encodedRate;
        break;

      case AE_FMT_DTS_RAW:
        m_encoding              = CJNIAudioFormat::ENCODING_DTS;
        m_format.m_channelLayout = AE_CH_LAYOUT_2_0;
        m_format.m_frames       = DTS1_FRAME_SIZE * m_format.m_sampleRate / m_format.m_encodedRate;
        m_sink_sampleRate       = m_format.m_encodedRate;
        break;

      case AE_FMT_DTSHD_RAW:
        m_encoding              = CJNIAudioFormat::ENCODING_DTS_HD;
        m_format.m_channelLayout = AE_CH_LAYOUT_7_1;
        m_format.m_frames       = DTS1_FRAME_SIZE * m_format.m_sampleRate / m_format.m_encodedRate;
        m_sink_sampleRate       = m_format.m_encodedRate;
        break;

      case AE_FMT_TRUEHD_RAW:
        m_encoding              = CJNIAudioFormat::ENCODING_DOLBY_TRUEHD;
        m_format.m_channelLayout = AE_CH_LAYOUT_7_1;
        m_format.m_frames       = TRUEHD_UNIT * m_format.m_sampleRate / m_format.m_encodedRate;
        m_sink_sampleRate       = m_format.m_encodedRate;
        break;

      default:
        m_encoding = CJNIAudioFormat::ENCODING_PCM_16BIT;
        m_format.m_dataFormat   = AE_FMT_S16LE;
        m_sink_sampleRate       = m_format.m_encodedRate;
        break;
    }
  }
  else
  {
    m_passthrough = false;
    if (CJNIAudioManager::GetSDKVersion() >= 21)
     {
      m_encoding = CJNIAudioFormat::ENCODING_PCM_FLOAT;
      m_format.m_dataFormat     = AE_FMT_FLOAT;
    }
    else
    {
      m_encoding = CJNIAudioFormat::ENCODING_PCM_16BIT;
      m_format.m_dataFormat     = AE_FMT_S16LE;
    }
    m_format.m_sampleRate     = m_sink_sampleRate;
  }

  int atChannelMask = AEChannelMapToAUDIOTRACKChannelMask(m_format.m_channelLayout);
  m_format.m_channelLayout  = AUDIOTRACKChannelMaskToAEChannelMap(atChannelMask);

#if defined(HAS_LIBAMCODEC)
  if (aml_present() && CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_USEAMCODEC))
    aml_set_audio_passthrough(m_passthrough);
#endif

  while (!m_at_jni)
  {
    unsigned int min_buffer_size       = CJNIAudioTrack::getMinBufferSize( m_sink_sampleRate,
                                                                  atChannelMask,
                                                                  m_encoding);
    if (m_passthrough && !WantsIEC61937())
    {
      m_format.m_frameSize      = 1;
      m_sink_frameSize          = m_format.m_frameSize;
      min_buffer_size         = (min_buffer_size * 4 / (m_format.m_frameSize*m_format.m_frames)+1) * (m_format.m_frameSize*m_format.m_frames);
    }
    else
    {
      m_format.m_frameSize      = m_format.m_channelLayout.Count() *
                                    (CAEUtil::DataFormatToBits(m_format.m_dataFormat) / 8);
      m_sink_frameSize          = m_format.m_frameSize;
      m_format.m_frames       = (int)(min_buffer_size / m_sink_frameSize) / 2;
    }

    m_format.m_frameSamples   = m_format.m_frames * m_format.m_channelLayout.Count();
    m_audiotrackbuffer_sec    = (double)(min_buffer_size / m_sink_frameSize) / (double)m_sink_sampleRate;

    m_at_jni                  = CreateAudioTrack(stream, m_sink_sampleRate,
                                                 atChannelMask, m_encoding,
                                                 min_buffer_size);

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
      CLog::Log(LOGERROR, "AESinkAUDIOTRACK - Unable to create AudioTrack");
      Deinitialize();
      return false;
    }
    CLog::Log(LOGDEBUG, "CAESinkAUDIOTRACK::Initialize returned: m_sampleRate %u; format:%s; min_buffer_size %u; m_frames %u; m_frameSize %u; channels: %d", m_format.m_sampleRate, CAEUtil::DataFormatToStr(m_format.m_dataFormat), min_buffer_size, m_format.m_frames, m_format.m_frameSize, m_format.m_channelLayout.Count());
  }

  format                    = m_format;

  // Force volume to 100% for passthrough
  if (m_passthrough)
  {
    m_volume = CXBMCApp::GetSystemVolume();
    CXBMCApp::SetSystemVolume(1.0);
  }

  return true;
}

void CAESinkAUDIOTRACK::Deinitialize()
{
#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "CAESinkAUDIOTRACK::Deinitialize");
#endif
  // Restore volume
  if (m_volume != -1)
    CXBMCApp::SetSystemVolume(m_volume);

  if (!m_at_jni)
    return;

  if (IsInitialized())
  {
    m_at_jni->stop();
    m_at_jni->flush();
  }
  m_at_jni->release();

  m_frames_written = 0;
  m_lastHeadPosition = 0;
  m_ptOffset = 0;

  delete m_at_jni;
  m_at_jni = NULL;
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

  // In their infinite wisdom, Google decided to make getPlaybackHeadPosition
  // return a 32bit "int" that you should "interpret as unsigned."  As such,
  // for wrap saftey, we need to do all ops on it in 32bit integer math.
  uint32_t head_pos = (uint32_t)m_at_jni->getPlaybackHeadPosition();

  double delay;
  if (m_passthrough && !WantsIEC61937())
  {
    if (!head_pos && m_at_jni->getPlayState() == CJNIAudioTrack::PLAYSTATE_PAUSED)
      m_ptOffset = m_lastHeadPosition;

    head_pos += m_ptOffset;
    m_lastHeadPosition = head_pos;

    delay = ((double)(m_frames_written - m_silenceframes) / m_format.m_sampleRate) - ((double)head_pos / m_sink_sampleRate);
#ifdef DEBUG_VERBOSE
    CLog::Log(LOGDEBUG, "CAESinkAUDIOTRACK::GetDelay m_frames_written/head_pos %u(%u)/%u %f", m_frames_written - m_silenceframes, m_frames_written, head_pos, delay);
#endif
  }
  else
    delay = (double)(m_frames_written - head_pos) / m_sink_sampleRate;

  status.SetDelay(delay);
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

  uint8_t *buffer = data[0]+offset*m_format.m_frameSize;
  uint8_t *out_buf = buffer;
  int size = frames * m_format.m_frameSize;

  if (m_passthrough && !WantsIEC61937())
  {
    if (m_format.m_dataFormat == AE_FMT_DTSHD_RAW || m_format.m_dataFormat == AE_FMT_TRUEHD_RAW)  // Decapsulate
    {
      size = ((int*)(buffer))[0];
      out_buf = buffer + sizeof(int);
      if (!size)
      {
        size = 1;  // keepalive
        m_silenceframes += frames;
      }
    }
    // Test and ignore silence packets
    else if (out_buf[0] == 0 && !memcmp(out_buf, out_buf+1, size-1))
    {
      size = 1;  // keepalive
      m_silenceframes += frames;
    }
  }

  // write as many frames of audio as we can fit into our internal buffer.
  int written = 0;
  if (frames)
  {
    // android will auto pause the playstate when it senses idle,
    // check it and set playing if it does this. Do this before
    // writing into its buffer.
    if (m_at_jni->getPlayState() != CJNIAudioTrack::PLAYSTATE_PLAYING)
      m_at_jni->play();
    written = m_at_jni->write((char*)out_buf, 0, size);
    if (written == size || size == 1)
      written = frames * m_sink_frameSize;     // Be sure to report to AE everything has been written
    else if (written < 0)
    {
      CLog::Log(LOGERROR, "CAESinkAUDIOTRACK::AddPackets write returned error:  %d", written);
      return INT_MAX;
    }
    else
    {
      CLog::Log(LOGWARNING, "CAESinkAUDIOTRACK::AddPackets incomplete write:  %d vs. %d", written, size);
      if (m_passthrough && !WantsIEC61937())
        return 0;  // Resend full packet
    }
    m_frames_written += written / m_sink_frameSize;
  }

#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "CAESinkAUDIOTRACK::AddPackets written %d", written);
#endif

  return (unsigned int)(written/m_format.m_frameSize);
}

void CAESinkAUDIOTRACK::Drain()
{
  if (!m_at_jni)
    return;

  // TODO: does this block until last samples played out?
  // we should not return from drain as long the device is in playing state
  m_at_jni->stop();
  m_frames_written = 0;
}

bool CAESinkAUDIOTRACK::WantsIEC61937()
{
  return !(m_format.m_dataFormat >= AE_FMT_AC3_RAW && m_format.m_dataFormat <= AE_FMT_DTSHD_RAW);
}

void CAESinkAUDIOTRACK::EnumerateDevicesEx(AEDeviceInfoList &list, bool force)
{
  m_info.m_channels.Reset();
  m_info.m_dataFormats.clear();
  m_info.m_sampleRates.clear();

  m_info.m_deviceType = AE_DEVTYPE_PCM;
  m_info.m_deviceName = "AudioTrack";
  m_info.m_displayName = "android";
  m_info.m_displayNameExtra = "audiotrack";
#ifdef LIMIT_TO_STEREO_AND_5POINT1_AND_7POINT1
  if (Has71Support())
    m_info.m_channels = AE_CH_LAYOUT_7_1;
  else
    m_info.m_channels = AE_CH_LAYOUT_5_1;
#else
  m_info.m_channels = KnownChannels;
#endif
  m_info.m_dataFormats.push_back(AE_FMT_S16LE);
  if (CJNIAudioManager::GetSDKVersion() >= 21)
    m_info.m_dataFormats.push_back(AE_FMT_FLOAT);
  
  m_sink_sampleRates.clear();
  m_sink_sampleRates.insert(CJNIAudioTrack::getNativeOutputSampleRate(CJNIAudioManager::STREAM_MUSIC));

  if (!CXBMCApp::IsHeadsetPlugged())
  {
    m_info.m_deviceType = AE_DEVTYPE_HDMI;
    int test_sample[] = { 32000, 44100, 48000, 96000, 192000 };
    int test_sample_sz = sizeof(test_sample) / sizeof(int);
    int encoding = CJNIAudioFormat::ENCODING_PCM_16BIT;
    if (CJNIAudioManager::GetSDKVersion() >= 21)
      encoding = CJNIAudioFormat::ENCODING_PCM_FLOAT;
    for (int i=0; i<test_sample_sz; ++i)
    {
      if (IsSupported(test_sample[i], CJNIAudioFormat::CHANNEL_OUT_STEREO, encoding))
      {
        m_sink_sampleRates.insert(test_sample[i]);
        CLog::Log(LOGDEBUG, "AESinkAUDIOTRACK - %d supported", test_sample[i]);
      }
    }
    std::copy(m_sink_sampleRates.begin(), m_sink_sampleRates.end(), std::back_inserter(m_info.m_sampleRates));
    m_info.m_dataFormats.push_back(AE_FMT_AC3);
    m_info.m_dataFormats.push_back(AE_FMT_DTS);
    if (CJNIAudioManager::GetSDKVersion() >= 21
#if defined(HAS_LIBAMCODEC)
        && !aml_present()
#endif
        )
    {
      m_info.m_dataFormats.push_back(AE_FMT_AC3_RAW);
      m_info.m_dataFormats.push_back(AE_FMT_EAC3_RAW);
      if (CJNIAudioManager::GetSDKVersion() >= 23)
      {
        m_info.m_dataFormats.push_back(AE_FMT_DTS_RAW);
        m_info.m_dataFormats.push_back(AE_FMT_DTSHD_RAW);
      }
      if (StringUtils::StartsWithNoCase(CJNIBuild::DEVICE, "foster")) // SATV is ahead of API
      {
        m_info.m_dataFormats.push_back(AE_FMT_DTS_RAW);
        m_info.m_dataFormats.push_back(AE_FMT_DTSHD_RAW);
        m_info.m_dataFormats.push_back(AE_FMT_TRUEHD_RAW);
      }
    }
  }
#if 0 //defined(__ARM_NEON__)
  if (g_cpuInfo.GetCPUFeatures() & CPU_FEATURE_NEON)
    m_info.m_dataFormats.push_back(AE_FMT_FLOAT);
#endif

  list.push_back(m_info);
}

