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

#include <algorithm>

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

#define CONSTANT_BUFFER_SIZE_SD 16384
#define CONSTANT_BUFFER_SIZE_HD 61440

#define TRUEHD_UNIT 960
#define SMOOTHED_DELAY_MAX 10

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
  m_sink_frameSize = 0;
  m_audiotrackbuffer_sec = 0.0;
  m_at_jni = NULL;
  m_duration_written = 0;
  m_last_duration_written = 0;
  m_last_head_pos = 0;
  m_sink_delay = 0;
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
  m_smoothedDelayCount = 0;
  m_smoothedDelayVec.clear();

  CLog::Log(LOGDEBUG, "CAESinkAUDIOTRACK::Initialize requested: %p, sampleRate %u; format: %s(%d); channels: %d", this, format.m_sampleRate, CAEUtil::DataFormatToStr(format.m_dataFormat), format.m_dataFormat, format.m_channelLayout.Count());

  int stream = CJNIAudioManager::STREAM_MUSIC;

  if (AE_IS_RAW(m_format.m_dataFormat) && !CXBMCApp::IsHeadsetPlugged())
  {
    m_passthrough = true;

    // Get equal or lower supported sample rate
    std::set<unsigned int>::iterator s = m_sink_sampleRates.upper_bound(m_format.m_encodedRate);
    if (--s != m_sink_sampleRates.begin())
      m_sink_sampleRate = *s;
    else
      m_sink_sampleRate = CJNIAudioTrack::getNativeOutputSampleRate(CJNIAudioManager::STREAM_MUSIC);

    m_encoding = CJNIAudioFormat::ENCODING_PCM_16BIT;
//    m_sink_sampleRate       = CJNIAudioTrack::getNativeOutputSampleRate(CJNIAudioManager::STREAM_MUSIC);

    switch (m_format.m_dataFormat)
    {
      case AE_FMT_AC3 + PT_FORMAT_RAW_CLASS:
        if (CJNIAudioFormat::ENCODING_AC3 != -1)
        {
          m_encoding              = CJNIAudioFormat::ENCODING_AC3;
          m_format.m_channelLayout = AE_CH_LAYOUT_2_0;
          m_format.m_frames       = m_format.m_sampleRate * 0.032;
        }
        else
          m_format.m_dataFormat   = AE_FMT_S16LE;
        break;

      case AE_FMT_EAC3 + PT_FORMAT_RAW_CLASS:
        if (CJNIAudioFormat::ENCODING_E_AC3 != -1)
        {
          m_encoding              = CJNIAudioFormat::ENCODING_E_AC3;
          m_format.m_channelLayout = AE_CH_LAYOUT_2_0;
          m_format.m_frames       = m_format.m_sampleRate * (1536.0 / m_format.m_encodedRate);
        }
        else
          m_format.m_dataFormat   = AE_FMT_S16LE;
        break;

      case AE_FMT_DTS + PT_FORMAT_RAW_CLASS:
        if (CJNIAudioFormat::ENCODING_DTS != -1)
        {
          m_encoding              = CJNIAudioFormat::ENCODING_DTS;
          m_format.m_channelLayout = AE_CH_LAYOUT_2_0;
          m_format.m_frames       = m_format.m_sampleRate * (512.0 / m_format.m_encodedRate);
        }
        else
          m_format.m_dataFormat   = AE_FMT_S16LE;
        break;

      case AE_FMT_DTSHD + PT_FORMAT_RAW_CLASS:
        if (CJNIAudioFormat::ENCODING_DTS_HD != -1)
        {
          m_encoding              = CJNIAudioFormat::ENCODING_DTS_HD;
          m_format.m_channelLayout = AE_CH_LAYOUT_7_1;
          m_format.m_frames       = CONSTANT_BUFFER_SIZE_HD;
        }
        else
          m_format.m_dataFormat   = AE_FMT_S16LE;
        break;

      case AE_FMT_TRUEHD + PT_FORMAT_RAW_CLASS:
        if (CJNIAudioFormat::ENCODING_DOLBY_TRUEHD != -1)
        {
          m_encoding              = CJNIAudioFormat::ENCODING_DOLBY_TRUEHD;
          m_format.m_channelLayout = AE_CH_LAYOUT_7_1;
          m_format.m_frames       = CONSTANT_BUFFER_SIZE_HD;
        }
        else
          m_format.m_dataFormat   = AE_FMT_S16LE;
        break;

      default:
        m_encoding = CJNIAudioFormat::ENCODING_PCM_16BIT;
        m_format.m_dataFormat   = AE_FMT_S16LE;
        m_sink_sampleRate       = CJNIAudioTrack::getNativeOutputSampleRate(CJNIAudioManager::STREAM_MUSIC);
        break;
    }
  }
  else
  {
    m_passthrough = false;

    // Get equal or lower supported sample rate
    std::set<unsigned int>::iterator s = m_sink_sampleRates.upper_bound(m_format.m_sampleRate);
    if (--s != m_sink_sampleRates.begin())
      m_sink_sampleRate = *s;
    else
      m_sink_sampleRate = CJNIAudioTrack::getNativeOutputSampleRate(CJNIAudioManager::STREAM_MUSIC);

    m_format.m_sampleRate     = m_sink_sampleRate;
    if (CJNIAudioManager::GetSDKVersion() >= 21 && m_format.m_channelLayout.Count() == 2)
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

  int atChannelMask = AEChannelMapToAUDIOTRACKChannelMask(m_format.m_channelLayout);
  m_format.m_channelLayout  = AUDIOTRACKChannelMaskToAEChannelMap(atChannelMask);

#if defined(HAS_LIBAMCODEC)
  if (aml_present() && CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_USEAMCODEC))
    aml_set_audio_passthrough(m_passthrough);
#endif

  while (!m_at_jni)
  {
    m_buffer_size       = CJNIAudioTrack::getMinBufferSize( m_sink_sampleRate,
                                                                  atChannelMask,
                                                                  m_encoding);
    if (m_passthrough && !WantsIEC61937())
    {
      m_format.m_frameSize      = 1;
      m_sink_frameSize          = m_format.m_frameSize;
      m_buffer_size             = std::max((unsigned int) m_format.m_frames, m_buffer_size);
    }
    else
    {
      m_format.m_frameSize      = m_format.m_channelLayout.Count() *
                                    (CAEUtil::DataFormatToBits(m_format.m_dataFormat) / 8);
      m_sink_frameSize          = m_format.m_frameSize;
      m_format.m_frames       = (int)(m_buffer_size / m_format.m_frameSize) / 2;
    }

    m_format.m_frameSamples   = m_format.m_frames * m_format.m_channelLayout.Count();
    m_audiotrackbuffer_sec    = (double)(m_buffer_size / m_sink_frameSize) / (double)m_sink_sampleRate;

    m_at_jni                  = CreateAudioTrack(stream, m_sink_sampleRate,
                                                 atChannelMask, m_encoding,
                                                 m_buffer_size);

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
    CLog::Log(LOGDEBUG, "CAESinkAUDIOTRACK::Initialize returned: m_sampleRate %u; format:%s(%d); min_buffer_size %u; m_frames %u; m_frameSize %u; channels: %d; m_audiotrackbuffer_sec(%f), m_sink_saplerate(%d)", m_format.m_sampleRate, CAEUtil::DataFormatToStr(m_format.m_dataFormat), m_format.m_dataFormat, m_buffer_size, m_format.m_frames, m_format.m_frameSize, m_format.m_channelLayout.Count(), m_audiotrackbuffer_sec, m_sink_sampleRate);
  }

  format                    = m_format;

  // Force volume to 100% for passthrough
  if (m_passthrough && !WantsIEC61937())
  {
    m_volume = CXBMCApp::GetSystemVolume();
    CXBMCApp::SetSystemVolume(1.0);
  }

  return true;
}

void CAESinkAUDIOTRACK::Deinitialize()
{
//#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "CAESinkAUDIOTRACK::Deinitialize %p", this);
//#endif
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

  m_duration_written = 0;
  m_last_duration_written = 0;
  m_last_head_pos = 0;
  m_sink_delay = 0;

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

  if (m_passthrough && !WantsIEC61937() && m_sink_delay)
  {
    status.SetDelay(m_sink_delay);
    return;
  }

  // In their infinite wisdom, Google decided to make getPlaybackHeadPosition
  // return a 32bit "int" that you should "interpret as unsigned."  As such,
  // for wrap saftey, we need to do all ops on it in 32bit integer math.
  uint32_t head_pos = (uint32_t)m_at_jni->getPlaybackHeadPosition();
  if (!head_pos)
  {
    status.SetDelay(0);
    return;
  }

  double delay = m_duration_written - ((double)head_pos / m_sink_sampleRate);
  if (m_duration_written != m_last_duration_written && head_pos != m_last_head_pos)
  {

    m_smoothedDelayVec.push_back(delay);
    if (m_smoothedDelayCount <= SMOOTHED_DELAY_MAX)
      m_smoothedDelayCount++;
    else
      m_smoothedDelayVec.erase(m_smoothedDelayVec.begin());

    m_last_duration_written = m_duration_written;
    m_last_head_pos = head_pos;
  }

  double smootheDelay = 0;
  for (double d : m_smoothedDelayVec)
    smootheDelay += d;
  smootheDelay /= m_smoothedDelayCount;

  if (m_passthrough && !WantsIEC61937() && m_smoothedDelayCount == SMOOTHED_DELAY_MAX && !m_sink_delay)
    m_sink_delay = smootheDelay;

#ifdef DEBUG_VERBOSE
  if (m_passthrough)
    CLog::Log(LOGDEBUG, "CAESinkAUDIOTRACK::GetDelay m_duration_written/head_pos %f/%u %f(%f)", m_duration_written, head_pos, smootheDelay, delay);
#endif

    status.SetDelay(smootheDelay);
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

  unsigned int time = XbmcThreads::SystemClockMillis();

  uint8_t *buffer = data[0]+offset*m_format.m_frameSize;
  uint8_t *out_buf = buffer;
  int size = frames * m_format.m_frameSize;

  if (m_passthrough && !WantsIEC61937())
  {
    // Decapsulate
    size = ((int*)(buffer))[0];
    out_buf = buffer + sizeof(int);
    if (!size)
    {
      if (m_at_jni->getPlayState() == CJNIAudioTrack::PLAYSTATE_PLAYING)
        m_at_jni->pause();
      return frames;
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
    unsigned int toWrite = size;
    while (toWrite > 0)
    {
      int bsize = std::min(toWrite, m_buffer_size);
      int len = m_at_jni->write((char*)(&out_buf[written]), 0, bsize);
      if (len < 0)
      {
        CLog::Log(LOGERROR, "CAESinkAUDIOTRACK::AddPackets write returned error:  %d(%d)", len, written);
        return INT_MAX;
      }
      written += len;
      toWrite -= len;
    }
    written = frames * m_format.m_frameSize;     // Be sure to report to AE everything has been written

    double duration = (double)(written / m_format.m_frameSize) / m_format.m_sampleRate;
    m_duration_written += duration;
    unsigned int sleep_ms = (duration * 1000.0) - (XbmcThreads::SystemClockMillis() - time) - 2 /* overhead */;
    if (sleep_ms > 0)
      usleep(sleep_ms * 1000.0);
  }


#ifdef DEBUG_VERBOSE
  if (m_passthrough)
    CLog::Log(LOGDEBUG, "CAESinkAUDIOTRACK::AddPackets written %d(%d), tm:%d", written, size, XbmcThreads::SystemClockMillis() - time);
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
  m_duration_written = 0;
  m_last_duration_written = 0;
  m_last_head_pos = 0;
  m_sink_delay = 0;
}

bool CAESinkAUDIOTRACK::WantsIEC61937()
{
  return !(AE_IS_RAW_RAW(m_format.m_dataFormat));
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
    // passthrough
    m_sink_sampleRates.insert(44100);
    m_sink_sampleRates.insert(48000);
    m_info.m_dataFormats.push_back(AE_FMT_AC3);
    m_info.m_dataFormats.push_back(AE_FMT_DTS);
#if defined(HAS_LIBAMCODEC)
    if (!aml_present())
#endif
    {
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
      if (CJNIAudioFormat::ENCODING_AC3 != -1)
        m_info.m_dataFormats.push_back((AEDataFormat)(AE_FMT_AC3 + PT_FORMAT_RAW_CLASS));
      if (CJNIAudioFormat::ENCODING_E_AC3 != -1)
        m_info.m_dataFormats.push_back((AEDataFormat)(AE_FMT_EAC3 + PT_FORMAT_RAW_CLASS));
      if (CJNIAudioFormat::ENCODING_DTS != -1)
          m_info.m_dataFormats.push_back((AEDataFormat)(AE_FMT_DTS + PT_FORMAT_RAW_CLASS));
      if (CJNIAudioFormat::ENCODING_DTS_HD != -1)
          m_info.m_dataFormats.push_back((AEDataFormat)(AE_FMT_DTSHD + PT_FORMAT_RAW_CLASS));
      if (CJNIAudioFormat::ENCODING_DOLBY_TRUEHD != -1)
          m_info.m_dataFormats.push_back((AEDataFormat)(AE_FMT_TRUEHD + PT_FORMAT_RAW_CLASS));
    }
    std::copy(m_sink_sampleRates.begin(), m_sink_sampleRates.end(), std::back_inserter(m_info.m_sampleRates));
  }

  list.push_back(m_info);
}

