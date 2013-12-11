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
#include "Utils/AEUtil.h"
#include "Utils/AERingBuffer.h"
#include "android/activity/XBMCApp.h"
#include "settings/Settings.h"
#if defined(HAS_LIBAMCODEC)
#include "utils/AMLUtils.h"
#endif
#include "utils/log.h"

#include <jni.h>

#if defined(__ARM_NEON__)
#include <arm_neon.h>
#include "utils/CPUInfo.h"
#include "android/jni/JNIThreading.h"

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

static jint GetStaticIntField(JNIEnv *jenv, std::string class_name, std::string field_name)
{
  class_name.insert(0, "android/media/");
  jclass cls = jenv->FindClass(class_name.c_str());
  jfieldID field = jenv->GetStaticFieldID(cls, field_name.c_str(), "I");
  jint int_field = jenv->GetStaticIntField(cls, field);
  jenv->DeleteLocalRef(cls);
  return int_field;
}

class CAudiotrackJNI
{
public:
  JNIEnv *jenv;
  jclass jcAudioTrack;
  jobject joAudioTrack;
  jmethodID jmInit;
  jmethodID jmPlay;
  jmethodID jmStop;
  jmethodID jmFlush;
  jmethodID jmRelease;
  jmethodID jmWrite;
  jmethodID jmPlayState;
  jmethodID jmPlayHeadPosition;
  jmethodID jmGetMinBufferSize;
  jint playing;
  jarray jbuffer;
};

CAEDeviceInfo CAESinkAUDIOTRACK::m_info;
////////////////////////////////////////////////////////////////////////////////////////////
CAESinkAUDIOTRACK::CAESinkAUDIOTRACK()
{
  m_alignedS16 = NULL;
  m_min_frames = 0;
  m_sink_frameSize = 0;
  m_audiotrackbuffer_sec = 0.0;
  m_volume = 1.0;
  m_at_jni = NULL;
  m_frames_written = 0;
}

CAESinkAUDIOTRACK::~CAESinkAUDIOTRACK()
{
  Deinitialize();
}

bool CAESinkAUDIOTRACK::Initialize(AEAudioFormat &format, std::string &device)
{
  m_format = format;

  if (AE_IS_RAW(m_format.m_dataFormat))
    m_passthrough = true;
  else
    m_passthrough = false;

#if defined(HAS_LIBAMCODEC)
  if (CSettings::Get().GetBool("videoplayer.useamcodec"))
    aml_set_audio_passthrough(m_passthrough);
#endif

  // default to 44100, all android devices support it.
  // then check if we can support the requested rate.
  unsigned int sampleRate = 44100;
  for (size_t i = 0; i < m_info.m_sampleRates.size(); i++)
  {
    if (m_format.m_sampleRate == m_info.m_sampleRates[i])
    {
      sampleRate = m_format.m_sampleRate;
      break;
    }
  }
  m_format.m_sampleRate = sampleRate;

  // default to AE_FMT_S16LE,
  // then check if we can support the requested format.
  AEDataFormat dataFormat = AE_FMT_S16LE;
  for (size_t i = 0; i < m_info.m_dataFormats.size(); i++)
  {
    if (m_format.m_dataFormat == m_info.m_dataFormats[i])
    {
      dataFormat = m_format.m_dataFormat;
      break;
    }
  }
  m_format.m_dataFormat = dataFormat;

  m_format.m_channelLayout = m_info.m_channels;
  m_format.m_frameSize = m_format.m_channelLayout.Count() * (CAEUtil::DataFormatToBits(m_format.m_dataFormat) >> 3);

  InitializeAT();
  m_format.m_frames = m_min_frames / 2;

  m_format.m_frameSamples = m_format.m_frames * m_format.m_channelLayout.Count();
  format = m_format;

  return true;
}

void CAESinkAUDIOTRACK::Deinitialize()
{
  if (!m_at_jni)
    return;

  JNIEnv *jenv = xbmc_jnienv();

  jenv->CallVoidMethod(m_at_jni->joAudioTrack, m_at_jni->jmStop);
  jenv->CallVoidMethod(m_at_jni->joAudioTrack, m_at_jni->jmFlush);
  jenv->CallVoidMethod(m_at_jni->joAudioTrack, m_at_jni->jmRelease);

  // might toss an exception on jmRelease so catch it.
  jthrowable exception = jenv->ExceptionOccurred();
  if (exception)
  {
    jenv->ExceptionDescribe();
    jenv->ExceptionClear();
  }

  jenv->DeleteLocalRef(m_at_jni->jbuffer);
  jenv->DeleteLocalRef(m_at_jni->joAudioTrack);
  jenv->DeleteLocalRef(m_at_jni->jcAudioTrack);

  delete m_at_jni;
}

bool CAESinkAUDIOTRACK::IsCompatible(const AEAudioFormat &format, const std::string &device)
{
  return ((m_format.m_sampleRate    == format.m_sampleRate) &&
          (m_format.m_dataFormat    == format.m_dataFormat) &&
          (m_format.m_channelLayout == format.m_channelLayout));
}

double CAESinkAUDIOTRACK::GetDelay()
{
  if (!m_at_jni)
    return 0.0;

  JNIEnv *jenv = xbmc_jnienv();
  // In their infinite wisdom, Google decided to make getPlaybackHeadPosition
  // return a 32bit "int" that you should "interpret as unsigned."  As such,
  // for wrap saftey, we need to do all ops on it in 32bit integer math.
  uint32_t head_pos = (uint32_t)jenv->CallIntMethod(m_at_jni->joAudioTrack, m_at_jni->jmPlayHeadPosition);

  double delay = (double)(m_frames_written - head_pos) / m_format.m_sampleRate;

  return delay;
}

double CAESinkAUDIOTRACK::GetCacheTime()
{
  // depreciated, not used by ActiveAE
  return 0;
}

double CAESinkAUDIOTRACK::GetCacheTotal()
{
  // total amount that the audio sink can buffer in units of seconds
  return m_audiotrackbuffer_sec;
}

// this method is supposed to block until all frames are written to the device buffer
// when it returns ActiveAESink will take the next buffer out of a queue
unsigned int CAESinkAUDIOTRACK::AddPackets(uint8_t *data, unsigned int frames, bool hasAudio, bool blocking)
{
  if (!m_at_jni)
    return INT_MAX;

  // write as many frames of audio as we can fit into our internal buffer.
  JNIEnv *jenv = m_at_jni->jenv;
  int written = 0;
  if (frames)
  {
    // android will auto pause the playstate when it senses idle,
    // check it and set playing if it does this. Do this before
    // writing into its buffer.
    if (jenv->CallIntMethod(m_at_jni->joAudioTrack, m_at_jni->jmPlayState) != m_at_jni->playing)
      jenv->CallVoidMethod( m_at_jni->joAudioTrack, m_at_jni->jmPlay);

    // Write a buffer of audio data to Java AudioTrack.
    // Warning, no other JNI function can be called after
    // GetPrimitiveArrayCritical until ReleasePrimitiveArrayCritical.
    void *pBuffer = jenv->GetPrimitiveArrayCritical(m_at_jni->jbuffer, NULL);
    if (pBuffer)
    {
      memcpy(pBuffer, data, frames*m_sink_frameSize);
      jenv->ReleasePrimitiveArrayCritical(m_at_jni->jbuffer, pBuffer, 0);
      // jmWrite is blocking and returns when the data has been transferred
      // from the Java layer and queued for playback.
      written = jenv->CallIntMethod(m_at_jni->joAudioTrack, m_at_jni->jmWrite, m_at_jni->jbuffer, 0, frames*m_sink_frameSize);
      m_frames_written += written / m_sink_frameSize;
    }
  }

  return (unsigned int)(written/m_sink_frameSize);
}

void CAESinkAUDIOTRACK::Drain()
{
  if (!m_at_jni)
    return;

  JNIEnv *jenv = m_at_jni->jenv;

  // TODO: does this block until last samples played out?
  // we should not return from drain as long the device is in playing state
  jenv->CallVoidMethod(m_at_jni->joAudioTrack, m_at_jni->jmStop);
  m_frames_written = 0;

  // might toss an exception so catch it.
  jthrowable exception = jenv->ExceptionOccurred();
  if (exception)
  {
    jenv->ExceptionDescribe();
    jenv->ExceptionClear();
  }
}

bool CAESinkAUDIOTRACK::HasVolume()
{
  return true;
}

void  CAESinkAUDIOTRACK::SetVolume(float scale)
{
  if (!m_at_jni)
    return;

  // Android uses fixed steps, reverse scale back to percent
  float gain = CAEUtil::ScaleToGain(scale);
  m_volume = CAEUtil::GainToPercent(gain);
  if (!m_passthrough)
  {
    CXBMCApp::SetSystemVolume(m_at_jni->jenv, m_volume);
  }
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
  m_info.m_channels += AE_CH_FL;
  m_info.m_channels += AE_CH_FR;
  m_info.m_sampleRates.push_back(44100);
  m_info.m_sampleRates.push_back(48000);
  m_info.m_dataFormats.push_back(AE_FMT_S16LE);
#if 0 //defined(__ARM_NEON__)
  if (g_cpuInfo.GetCPUFeatures() & CPU_FEATURE_NEON)
    m_info.m_dataFormats.push_back(AE_FMT_FLOAT);
#endif

  list.push_back(m_info);
}

void CAESinkAUDIOTRACK::InitializeAT()
{
  m_at_jni = new CAudiotrackJNI();

  JNIEnv *jenv = xbmc_jnienv();
  m_at_jni->jenv = jenv;

  m_at_jni->jcAudioTrack        = jenv->FindClass("android/media/AudioTrack");
  m_at_jni->jmInit              = jenv->GetMethodID(m_at_jni->jcAudioTrack, "<init>", "(IIIIII)V");
  m_at_jni->jmPlay              = jenv->GetMethodID(m_at_jni->jcAudioTrack, "play", "()V");
  m_at_jni->jmStop              = jenv->GetMethodID(m_at_jni->jcAudioTrack, "stop", "()V");
  m_at_jni->jmFlush             = jenv->GetMethodID(m_at_jni->jcAudioTrack, "flush", "()V");
  m_at_jni->jmRelease           = jenv->GetMethodID(m_at_jni->jcAudioTrack, "release", "()V");
  m_at_jni->jmWrite             = jenv->GetMethodID(m_at_jni->jcAudioTrack, "write", "([BII)I");
  m_at_jni->jmPlayState         = jenv->GetMethodID(m_at_jni->jcAudioTrack, "getPlayState", "()I");
  m_at_jni->jmPlayHeadPosition  = jenv->GetMethodID(m_at_jni->jcAudioTrack, "getPlaybackHeadPosition", "()I");
  m_at_jni->jmGetMinBufferSize  = jenv->GetStaticMethodID(m_at_jni->jcAudioTrack, "getMinBufferSize", "(III)I");

  jint audioFormat    = GetStaticIntField(jenv, "AudioFormat", "ENCODING_PCM_16BIT");
  jint channelConfig  = GetStaticIntField(jenv, "AudioFormat", "CHANNEL_OUT_STEREO");

  jint min_buffer_size = jenv->CallStaticIntMethod(m_at_jni->jcAudioTrack, m_at_jni->jmGetMinBufferSize,
    m_format.m_sampleRate, channelConfig, audioFormat);

  m_sink_frameSize = m_format.m_channelLayout.Count() * CAEUtil::DataFormatToBits(AE_FMT_S16LE) >> 3;
  m_min_frames = min_buffer_size / m_sink_frameSize;

  m_audiotrackbuffer_sec = (double)m_min_frames / (double)m_format.m_sampleRate;

  m_at_jni->joAudioTrack = jenv->NewObject(m_at_jni->jcAudioTrack, m_at_jni->jmInit,
    GetStaticIntField(jenv, "AudioManager", "STREAM_MUSIC"),
    m_format.m_sampleRate,
    channelConfig,
    audioFormat,
    min_buffer_size,
    GetStaticIntField(jenv, "AudioTrack", "MODE_STREAM"));

  // Set the initial volume
  float volume = 1.0;
  if (!m_passthrough)
    volume = m_volume;
  CXBMCApp::SetSystemVolume(jenv, volume);

  // cache the playing int value.
  m_at_jni->playing = GetStaticIntField(jenv, "AudioTrack", "PLAYSTATE_PLAYING");

  // create a java byte buffer for writing pcm data to AudioTrack.
  m_at_jni->jbuffer = jenv->NewByteArray(min_buffer_size);

  m_frames_written = 0;
}
