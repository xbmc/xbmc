 /*
 *      Copyright (C) 2010-2012 Team XBMC
 *      http://www.xbmc.org
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
#include "utils/log.h"
#if defined(HAS_AMLPLAYER)
#include "cores/amlplayer/AMLUtils.h"
#endif

#include <jni.h>

#if defined(__ARM_NEON__)
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

static jint GetStaticIntField(JNIEnv *jenv, std::string class_name, std::string field_name)
{
  class_name.insert(0, "android/media/");
  jclass cls = jenv->FindClass(class_name.c_str());
  jfieldID field = jenv->GetStaticFieldID(cls, field_name.c_str(), "I");
  jint int_field = jenv->GetStaticIntField(cls, field);
  jenv->DeleteLocalRef(cls);
  return int_field;
}

CAEDeviceInfo CAESinkAUDIOTRACK::m_info;
////////////////////////////////////////////////////////////////////////////////////////////
CAESinkAUDIOTRACK::CAESinkAUDIOTRACK()
  : CThread("audiotrack")
{
  m_sinkbuffer = NULL;
  m_alignedS16LE = NULL;
#if defined(HAS_AMLPLAYER)
  aml_cpufreq_limit(true);
#endif
}

CAESinkAUDIOTRACK::~CAESinkAUDIOTRACK()
{
#if defined(HAS_AMLPLAYER)
  aml_cpufreq_limit(false);
#endif
}

bool CAESinkAUDIOTRACK::Initialize(AEAudioFormat &format, std::string &device)
{
  m_format = format;

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
  m_format.m_frameSize = format.m_channelLayout.Count() * (CAEUtil::DataFormatToBits(m_format.m_dataFormat) >> 3);

  m_draining = false;
  m_volume_changed = false;
  // launch the process thread and wait for the
  // AutoTrack jni object to get created and setup.
  m_wake.Reset();
  m_inited.Reset();
  Create();
  if(!m_inited.WaitMSec(100))
  {
    while(!m_inited.WaitMSec(1))
      Sleep(10);
  }

  // m_min_frames is volatile and has been setup by Process()
  m_format.m_frames = m_min_frames;
  m_format.m_frameSamples = m_format.m_frames * m_format.m_channelLayout.Count();
  format = m_format;

  return true;
}

void CAESinkAUDIOTRACK::Deinitialize()
{
  // force m_bStop and set m_wake, if might be sleeping.
  m_bStop = true;
  m_wake.Set();
  StopThread();
  delete m_sinkbuffer, m_sinkbuffer = NULL;
  if (m_alignedS16LE)
    _aligned_free(m_alignedS16LE), m_alignedS16LE = NULL;
}

bool CAESinkAUDIOTRACK::IsCompatible(const AEAudioFormat format, const std::string device)
{
  return ((m_format.m_sampleRate    == format.m_sampleRate) &&
          (m_format.m_dataFormat    == format.m_dataFormat) &&
          (m_format.m_channelLayout == format.m_channelLayout));
}

double CAESinkAUDIOTRACK::GetDelay()
{
  // this includes any latency due to AudioTrack buffer,
  // AudioMixer (if any) and audio hardware driver.

  double sinkbuffer_seconds_to_empty = m_sinkbuffer_sec_per_byte * (double)m_sinkbuffer->GetReadSize();
  return sinkbuffer_seconds_to_empty + m_audiotrack_empty_sec;
}

double CAESinkAUDIOTRACK::GetCacheTime()
{
  // returns the time in seconds that it will take
  // to underrun the buffer if no sample is added.

  double sinkbuffer_seconds_to_empty = m_sinkbuffer_sec_per_byte * (double)m_sinkbuffer->GetReadSize();
  return sinkbuffer_seconds_to_empty + m_audiotrack_empty_sec;
}

double CAESinkAUDIOTRACK::GetCacheTotal()
{
  // total amount that the audio sink can buffer in units of seconds

  return m_sinkbuffer_sec + m_audiotrackbuffer_sec;
}

unsigned int CAESinkAUDIOTRACK::AddPackets(uint8_t *data, unsigned int frames, bool hasAudio)
{
  // write as many frames of audio as we can fit into our internal buffer.

  // our internal sink buffer is always AE_FMT_S16LE
  unsigned int write_frames = (m_sinkbuffer->GetWriteSize() / m_sink_frameSize) % frames;
  if (hasAudio && write_frames)
  {
    switch(m_format.m_dataFormat)
    {
      case AE_FMT_S16LE:
        m_sinkbuffer->Write(data, write_frames * m_sink_frameSize);
        m_wake.Set();
        break;
#if defined(__ARM_NEON__)
      case AE_FMT_FLOAT:
        if (!m_alignedS16LE)
          m_alignedS16LE = (int16_t*)_aligned_malloc(m_format.m_frames * m_sink_frameSize, 16);
        // neon convert AE_FMT_S16LE to AE_FMT_FLOAT
        pa_sconv_s16le_from_f32ne_neon(write_frames * m_format.m_channelLayout.Count(), (const float32_t *)data, m_alignedS16LE);
        m_sinkbuffer->Write((unsigned char*)m_alignedS16LE, write_frames * m_sink_frameSize);
        m_wake.Set();
        break;
#endif
      default:
        break;
    }
  }
  // AddPackets runs under a non-idled AE thread we must block or sleep.
  // Trying to calc the optimal sleep is tricky so just a minimal sleep.
  Sleep(10);

  return hasAudio ? write_frames:frames;
}

void CAESinkAUDIOTRACK::Drain()
{
  CLog::Log(LOGDEBUG, "CAESinkAUDIOTRACK::Drain");
  m_draining = true;
  m_wake.Set();
}

bool CAESinkAUDIOTRACK::HasVolume()
{
  return true;
}

void  CAESinkAUDIOTRACK::SetVolume(float volume)
{
  m_volume = volume;
  m_volume_changed = true;
}

void CAESinkAUDIOTRACK::EnumerateDevicesEx(AEDeviceInfoList &list)
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
#if defined(__ARM_NEON__)
  if (g_cpuInfo.GetCPUFeatures() & CPU_FEATURE_NEON)
    m_info.m_dataFormats.push_back(AE_FMT_FLOAT);
#endif

  list.push_back(m_info);
}

void CAESinkAUDIOTRACK::Process()
{
  CLog::Log(LOGDEBUG, "CAESinkAUDIOTRACK::Process");

  JNIEnv *jenv = NULL;
  CXBMCApp::AttachCurrentThread(&jenv, NULL);

  jclass jcAudioTrack = jenv->FindClass("android/media/AudioTrack");

  jmethodID jmInit              = jenv->GetMethodID(jcAudioTrack, "<init>", "(IIIIII)V");
  jmethodID jmPlay              = jenv->GetMethodID(jcAudioTrack, "play", "()V");
  jmethodID jmStop              = jenv->GetMethodID(jcAudioTrack, "stop", "()V");
  jmethodID jmFlush             = jenv->GetMethodID(jcAudioTrack, "flush", "()V");
  jmethodID jmRelease           = jenv->GetMethodID(jcAudioTrack, "release", "()V");
  jmethodID jmWrite             = jenv->GetMethodID(jcAudioTrack, "write", "([BII)I");
  jmethodID jmPlayState         = jenv->GetMethodID(jcAudioTrack, "getPlayState", "()I");
  jmethodID jmSetStereoVolume   = jenv->GetMethodID(jcAudioTrack, "setStereoVolume", "(FF)I");
  jmethodID jmPlayHeadPosition  = jenv->GetMethodID(jcAudioTrack, "getPlaybackHeadPosition", "()I");
  jmethodID jmGetMinBufferSize  = jenv->GetStaticMethodID(jcAudioTrack, "getMinBufferSize", "(III)I");

  jint audioFormat    = GetStaticIntField(jenv, "AudioFormat", "ENCODING_PCM_16BIT");
  jint channelConfig  = GetStaticIntField(jenv, "AudioFormat", "CHANNEL_OUT_STEREO");

  jint min_buffer_size = jenv->CallStaticIntMethod(jcAudioTrack, jmGetMinBufferSize,
    m_format.m_sampleRate, channelConfig, audioFormat);

  m_sink_frameSize = m_format.m_channelLayout.Count() * CAEUtil::DataFormatToBits(AE_FMT_S16LE) >> 3;
  m_min_frames = min_buffer_size / m_sink_frameSize;

  m_audiotrackbuffer_sec = (double)m_min_frames / (double)m_format.m_sampleRate;
  m_audiotrack_empty_sec = 0.0;

  // setup a 1/4 second internal sink lockless ring buffer
  m_sinkbuffer = new AERingBuffer(m_sink_frameSize * m_format.m_sampleRate / 4);
  m_sinkbuffer_sec_per_byte = 1.0 / (double)(m_sink_frameSize * m_format.m_sampleRate);
  m_sinkbuffer_sec = (double)m_sinkbuffer_sec_per_byte * m_sinkbuffer->GetMaxSize();

  jobject joAudioTrack = jenv->NewObject(jcAudioTrack, jmInit,
    GetStaticIntField(jenv, "AudioManager", "STREAM_MUSIC"),
    m_format.m_sampleRate,
    channelConfig,
    audioFormat,
    min_buffer_size,
    GetStaticIntField(jenv, "AudioTrack", "MODE_STREAM"));

  // The AudioTrack object has been created and waiting to play,
  m_inited.Set();
  // yield to give other threads a chance to do some work.
  sched_yield();

  // cache the playing int value.
  jint playing = GetStaticIntField(jenv, "AudioTrack", "PLAYSTATE_PLAYING");

  // create a java byte buffer for writing pcm data to AudioTrack.
  jarray jbuffer = jenv->NewByteArray(min_buffer_size);

  int64_t frames_written = 0;
  int64_t frame_position = 0;

  while (!m_bStop)
  {
    if (m_volume_changed)
    {
      // check of volume changes and make them,
      // do it here to keep jni calls local to this thread.
      jfloat jvolume = m_volume;
      jenv->CallIntMethod(joAudioTrack, jmSetStereoVolume, jvolume, jvolume);
      m_volume_changed = false;
    }
    if (m_draining)
    {
      unsigned char byte_drain[1024];
      unsigned int  byte_drain_size = m_sinkbuffer->GetReadSize() % 1024;
      while (byte_drain_size)
      {
        m_sinkbuffer->Read(byte_drain, byte_drain_size);
        byte_drain_size = m_sinkbuffer->GetReadSize() % 1024;
      }
      jenv->CallVoidMethod(joAudioTrack, jmStop);
      jenv->CallVoidMethod(joAudioTrack, jmFlush);
    }

    unsigned int read_bytes = m_sinkbuffer->GetReadSize() % min_buffer_size;
    if (read_bytes > 0)
    {
      // android will auto pause the playstate when it senses idle,
      // check it and set playing if it does this. Do this before
      // writing into its buffer.
      if (jenv->CallIntMethod(joAudioTrack, jmPlayState) != playing)
        jenv->CallVoidMethod(joAudioTrack, jmPlay);

      // Write a buffer of audio data to Java AudioTrack.
      // Warning, no other JNI function can be called after
      // GetPrimitiveArrayCritical until ReleasePrimitiveArrayCritical.
      void *pBuffer = jenv->GetPrimitiveArrayCritical(jbuffer, NULL);
      if (pBuffer)
      {
        m_sinkbuffer->Read((unsigned char*)pBuffer, read_bytes);
        jenv->ReleasePrimitiveArrayCritical(jbuffer, pBuffer, 0);
        // jmWrite is blocking and returns when the data has been transferred
        // from the Java layer and queued for playback.
        jenv->CallIntMethod(joAudioTrack, jmWrite, jbuffer, 0, read_bytes);
      }
    }
    // calc the number of seconds until audiotrack buffer is empty.
    frame_position = jenv->CallIntMethod(joAudioTrack, jmPlayHeadPosition);
    if (frame_position == 0)
      frames_written = 0;
    frames_written += read_bytes / m_sink_frameSize;
    m_audiotrack_empty_sec = (double)(frames_written - frame_position) / m_format.m_sampleRate;
    // some times, we can get frame_position
    // ahead of frames_written, not a clue why. clamp it.
    if (m_audiotrack_empty_sec < 0.0f)
      m_audiotrack_empty_sec = 0.0f;

    if (m_sinkbuffer->GetReadSize() == 0)
    {
      // the sink buffer is empty, stop playback.
      // Audiotrack will playout any written contents.
      jenv->CallVoidMethod(joAudioTrack, jmStop);
      // sleep this audio thread, we will get woken when we have audio data.
      m_wake.WaitMSec(250);
    }
  }

  jenv->CallVoidMethod(joAudioTrack, jmStop);
  jenv->CallVoidMethod(joAudioTrack, jmFlush);
  jenv->CallVoidMethod(joAudioTrack, jmRelease);

  // might toss an exception on jmRelease so catch it.
  jthrowable exception = jenv->ExceptionOccurred();
  if (exception)
  {
    jenv->ExceptionDescribe();
    jenv->ExceptionClear();
  }

  jenv->DeleteLocalRef(jbuffer);
  jenv->DeleteLocalRef(joAudioTrack);
  jenv->DeleteLocalRef(jcAudioTrack);

  CXBMCApp::DetachCurrentThread();
}
