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
#include "android/activity/XBMCApp.h"
#include "settings/Settings.h"
#if defined(HAS_LIBAMCODEC)
#include "utils/AMLUtils.h"
#endif
#include "utils/log.h"

#include "android/jni/AudioFormat.h"
#include "android/jni/AudioManager.h"
#include "android/jni/AudioTrack.h"

using namespace jni;

#if 0 //defined(__ARM_NEON__)
#include <arm_neon.h>
#include "utils/CPUInfo.h"

// LGPLv2 from PulseAudio
// float values from AE are pre-clamped so we do not need to clamp again here
static void pa_sconv_s16le_from_f32ne_neon(unsigned n, const float32_t *a, int16_t *b)
{
  unsigned int i;

  const float32x4_t half4     = vdupq_n_f32(0.5f);
  const float32x4_t scale4    = vdupq_n_f32(32767.0f);
  const uint32x4_t  mask4     = vdupq_n_u32(0x80000000);

  for (i = 0; i < (n & ~3); i += 4)
  {
    const float32x4_t v4 = vmulq_f32(vld1q_f32(&a[i]), scale4);
    const float32x4_t w4 = vreinterpretq_f32_u32(
      vorrq_u32(vandq_u32(vreinterpretq_u32_f32(v4), mask4), vreinterpretq_u32_f32(half4)));
    vst1_s16(&b[i], vmovn_s32(vcvtq_s32_f32(vaddq_f32(v4, w4))));
  }
  // leftovers
  for ( ; i < n; i++)
    b[i] = (int16_t) lrintf(a[i] * 0x7FFF);
}
#endif

/*
 * ADT-1 on L preview as of 2014-10 downmixes all non-5.1/7.1 content
 * to stereo, so use 7.1 or 5.1 for all multichannel content for now to
 * avoid that (except passthrough).
 * If other devices surface that support other multichannel layouts,
 * this should be disabled or adapted accordingly.
 */
#define LIMIT_TO_STEREO_AND_5POINT1_AND_7POINT1 1

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
  if (info[0] == AE_CH_RAW)
    return CJNIAudioFormat::CHANNEL_OUT_STEREO;
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

static jni::CJNIAudioTrack *CreateAudioTrack(int sampleRate, int channelMask, int bufferSize)
{
  jni::CJNIAudioTrack *jniAt = NULL;

  try
  {
    jniAt = new CJNIAudioTrack(CJNIAudioManager::STREAM_MUSIC,
                               sampleRate,
                               channelMask,
                               CJNIAudioFormat::ENCODING_PCM_16BIT,
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
////////////////////////////////////////////////////////////////////////////////////////////
CAESinkAUDIOTRACK::CAESinkAUDIOTRACK()
{
  m_alignedS16 = NULL;
  m_min_frames = 0;
  m_sink_frameSize = 0;
  m_audiotrackbuffer_sec = 0.0;
  m_at_jni = NULL;
  m_frames_written = 0;
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
  m_lastFormat  = format;
  m_format      = format;
  m_volume      = -1;

  if (AE_IS_RAW(m_format.m_dataFormat))
    m_passthrough = true;
  else
    m_passthrough = false;

#if defined(HAS_LIBAMCODEC)
  if (CSettings::Get().GetBool("videoplayer.useamcodec"))
    aml_set_audio_passthrough(m_passthrough);
#endif

  int atChannelMask = AEChannelMapToAUDIOTRACKChannelMask(m_format.m_channelLayout);

  m_format.m_sampleRate     = CJNIAudioTrack::getNativeOutputSampleRate(CJNIAudioManager::STREAM_MUSIC);
  m_format.m_dataFormat     = AE_FMT_S16LE;

  while (!m_at_jni)
  {
    m_format.m_channelLayout  = AUDIOTRACKChannelMaskToAEChannelMap(atChannelMask);
    m_format.m_frameSize      = m_format.m_channelLayout.Count() *
                                (CAEUtil::DataFormatToBits(m_format.m_dataFormat) / 8);
    int min_buffer_size       = CJNIAudioTrack::getMinBufferSize( m_format.m_sampleRate,
                                                                  atChannelMask,
                                                                  CJNIAudioFormat::ENCODING_PCM_16BIT);
    m_sink_frameSize          = m_format.m_channelLayout.Count() *
                                (CAEUtil::DataFormatToBits(AE_FMT_S16LE) / 8);
    m_min_frames              = min_buffer_size / m_sink_frameSize;
    m_audiotrackbuffer_sec    = (double)m_min_frames / (double)m_format.m_sampleRate;

    m_at_jni                  = CreateAudioTrack(m_format.m_sampleRate,
                                                 atChannelMask,
                                                 min_buffer_size);

    if (!m_at_jni)
    {
      if (atChannelMask != CJNIAudioFormat::CHANNEL_OUT_STEREO &&
          atChannelMask != CJNIAudioFormat::CHANNEL_OUT_5POINT1)
      {
        atChannelMask = CJNIAudioFormat::CHANNEL_OUT_5POINT1;
        CLog::Log(LOGDEBUG, "AESinkAUDIOTRACK - Retrying multichannel playback with a 5.1 layout");
      }
      else if (atChannelMask != CJNIAudioFormat::CHANNEL_OUT_STEREO)
      {
        atChannelMask = CJNIAudioFormat::CHANNEL_OUT_STEREO;
        CLog::Log(LOGDEBUG, "AESinkAUDIOTRACK - Retrying with a stereo layout");
      }
      else
      {
        CLog::Log(LOGERROR, "AESinkAUDIOTRACK - Unable to create AudioTrack");
        return false;
      }
    }
  }

  m_format.m_frames         = m_min_frames / 2;

  m_format.m_frameSamples   = m_format.m_frames * m_format.m_channelLayout.Count();
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
  // Restore volume
  if (m_volume != -1)
    CXBMCApp::SetSystemVolume(m_volume);

  if (!m_at_jni)
    return;

  m_at_jni->stop();
  m_at_jni->flush();
  m_at_jni->release();
  
  m_frames_written = 0;

  delete m_at_jni;
  m_at_jni = NULL;
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

  double delay = (double)(m_frames_written - head_pos) / m_format.m_sampleRate;

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
  if (!m_at_jni)
    return INT_MAX;

  uint8_t *buffer = data[0]+offset*m_format.m_frameSize;

  // write as many frames of audio as we can fit into our internal buffer.
  int written = 0;
  if (frames)
  {
    // android will auto pause the playstate when it senses idle,
    // check it and set playing if it does this. Do this before
    // writing into its buffer.
    if (m_at_jni->getPlayState() != CJNIAudioTrack::PLAYSTATE_PLAYING)
      m_at_jni->play();

    written = m_at_jni->write((char*)buffer, 0, frames * m_sink_frameSize);
    m_frames_written += written / m_sink_frameSize;
  }

  return (unsigned int)(written/m_sink_frameSize);
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

bool CAESinkAUDIOTRACK::HasVolume()
{
  return true;
}

void  CAESinkAUDIOTRACK::SetVolume(float scale)
{
  // Ignore in passthrough
  if (m_passthrough)
    return;

  if (!m_at_jni)
    return;

  CXBMCApp::SetSystemVolume(scale);
}

void CAESinkAUDIOTRACK::EnumerateDevicesEx(AEDeviceInfoList &list, bool force)
{
  m_info.m_channels.Reset();
  m_info.m_dataFormats.clear();
  m_info.m_sampleRates.clear();

  m_info.m_deviceType = AE_DEVTYPE_HDMI;
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
  int test_sample[] = { 44100, 48000, 96000, 192000 };
  int test_sample_sz = sizeof(test_sample) / sizeof(int);
  for (int i=0; i<test_sample_sz; ++i)
  {
    if (IsSupported(test_sample[i], CJNIAudioFormat::CHANNEL_OUT_STEREO, CJNIAudioFormat::ENCODING_PCM_16BIT))
    {
      m_info.m_sampleRates.push_back(test_sample[i]);
      CLog::Log(LOGDEBUG, "AESinkAUDIOTRACK - %d supported", test_sample[i]);
    }
  }
  m_info.m_dataFormats.push_back(AE_FMT_S16LE);
  m_info.m_dataFormats.push_back(AE_FMT_AC3);
  m_info.m_dataFormats.push_back(AE_FMT_DTS);
#if 0 //defined(__ARM_NEON__)
  if (g_cpuInfo.GetCPUFeatures() & CPU_FEATURE_NEON)
    m_info.m_dataFormats.push_back(AE_FMT_FLOAT);
#endif

  list.push_back(m_info);
}

