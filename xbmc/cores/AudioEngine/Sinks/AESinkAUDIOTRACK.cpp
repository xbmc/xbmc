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
#include "platform/android/activity/XBMCApp.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include "platform/android/jni/AudioFormat.h"
#include "platform/android/jni/AudioManager.h"
#include "platform/android/jni/AudioTrack.h"
#include "platform/android/jni/Build.h"
#include "utils/TimeUtils.h"

#if defined(HAS_LIBAMCODEC)
#include "utils/AMLUtils.h"
#endif

//#define DEBUG_VERBOSE 1

// This is an alternative to the linear weighted delay smoothing
// advantages: only one history value needs to be stored
// in tests the linear weighted average smoother yield better results
//#define AT_USE_EXPONENTIAL_AVERAGING 1

using namespace jni;

// those are empirical values while the HD buffer
// is the max TrueHD package
const unsigned int MAX_RAW_AUDIO_BUFFER_HD = 61440;
const unsigned int MAX_RAW_AUDIO_BUFFER = 16384;
const unsigned int MOVING_AVERAGE_MAX_MEMBERS = 5;

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

// AMLogic helper for HD Audio
bool CAESinkAUDIOTRACK::HasAmlHD()
{
  return ((CJNIAudioFormat::ENCODING_DOLBY_TRUEHD != -1) && (CJNIAudioFormat::ENCODING_DTS_HD != -1));
}

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
  m_encoding = CJNIAudioFormat::ENCODING_PCM_16BIT;
  m_audiotrackbuffer_sec = 0.0;
  m_at_jni = NULL;
  m_duration_written = 0;
  m_offset = -1;
  m_volume = -1;
  m_sink_sampleRate = 0;
  m_passthrough = false;
  m_min_buffer_size = 0;
  m_lastPlaybackHeadPosition = 0;
  m_extTimer.SetExpired();
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
  m_offset = -1;
  m_lastPlaybackHeadPosition = 0;
  m_linearmovingaverage.clear();
  m_extTimer.SetExpired();
  CLog::Log(LOGDEBUG, "CAESinkAUDIOTRACK::Initialize requested: sampleRate %u; format: %s; channels: %d", format.m_sampleRate, CAEUtil::DataFormatToStr(format.m_dataFormat), format.m_channelLayout.Count());

  int stream = CJNIAudioManager::STREAM_MUSIC;
  m_encoding = CJNIAudioFormat::ENCODING_PCM_16BIT;

  // Get equal or lower supported sample rate
  std::set<unsigned int>::iterator s = m_sink_sampleRates.upper_bound(m_format.m_sampleRate);
  if (--s != m_sink_sampleRates.begin())
    m_sink_sampleRate = *s;
  else
    m_sink_sampleRate = CJNIAudioTrack::getNativeOutputSampleRate(CJNIAudioManager::STREAM_MUSIC);

  if (m_format.m_dataFormat == AE_FMT_RAW && !CXBMCApp::IsHeadsetPlugged())
  {
    m_passthrough = true;
    m_encoding = AEStreamFormatToATFormat(m_format.m_streamInfo.m_type);
    m_format.m_channelLayout = AE_CH_LAYOUT_2_0;

    if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTSHD ||
        m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD)
    {
      m_format.m_channelLayout = AE_CH_LAYOUT_7_1;
      // Shield v5 workaround
      if (!m_info.m_wantsIECPassthrough && CJNIAudioManager::GetSDKVersion() == 22 && m_sink_sampleRate > 48000)
        m_sink_sampleRate = 48000;
    }
    if (m_info.m_wantsIECPassthrough)
    {
      m_format.m_dataFormat     = AE_FMT_S16LE;
      if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTSHD ||
          m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD)
        m_sink_sampleRate = 192000;

      if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_EAC3)
        m_sink_sampleRate = m_format.m_streamInfo.m_sampleRate;

      // we are running on an old android version
      // that does neither know AC3, DTS or whatever
      // we will fallback to 16BIT passthrough
      if (m_encoding == -1)
      {
        m_format.m_channelLayout = AE_CH_LAYOUT_2_0;
        m_format.m_sampleRate     = m_sink_sampleRate;
        m_encoding = CJNIAudioFormat::ENCODING_PCM_16BIT;
      }
    }
  }
  else
  {
    m_passthrough = false;
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
  if (aml_present() && m_passthrough)
    atChannelMask = CJNIAudioFormat::CHANNEL_OUT_STEREO;
#endif

  while (!m_at_jni)
  {
    int min_buffer = CJNIAudioTrack::getMinBufferSize(m_sink_sampleRate,
                                                         atChannelMask,
                                                         m_encoding);

    if (min_buffer < 0)
    {
      CLog::Log(LOGERROR, "Minimum Buffer Size was: %d - disable passthrough (?) your hw does not support it", min_buffer);
      return false;
    }

    m_min_buffer_size = (unsigned int) min_buffer;
    CLog::Log(LOGDEBUG, "Minimum size we need for stream: %u", m_min_buffer_size);
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
        case CAEStreamInfo::STREAM_TYPE_DTSHD:
          // normal frame is max  2012 bytes + 2764 sub frame
          m_min_buffer_size = 66432; //according to the buffer model of ISO/IEC13818-1
          m_format.m_frames = m_min_buffer_size;
          rawlength_in_seconds = 8 * m_format.m_streamInfo.GetDuration() / 1000; // average value
          break;
        case CAEStreamInfo::STREAM_TYPE_DTS_512:
        case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
          // max 2012 bytes
          // depending on sample rate between 106 ms and 212 ms
          m_min_buffer_size = 8 * 2012;
          m_format.m_frames = m_min_buffer_size;
          rawlength_in_seconds = 8 * m_format.m_streamInfo.GetDuration() / 1000;
          break;
        case CAEStreamInfo::STREAM_TYPE_DTS_1024:
        case CAEStreamInfo::STREAM_TYPE_DTS_2048:
          m_min_buffer_size = 4 * 5462;
          m_format.m_frames = m_min_buffer_size;
          rawlength_in_seconds = 4 * m_format.m_streamInfo.GetDuration() / 1000;
          break;
        case CAEStreamInfo::STREAM_TYPE_AC3:
           ac3FrameSize = m_format.m_streamInfo.m_ac3FrameSize;
           if (ac3FrameSize == 0)
             ac3FrameSize = 1536; // fallback if not set, e.g. Transcoding
           m_min_buffer_size = std::max(m_min_buffer_size * 3, ac3FrameSize * 8);
           m_format.m_frames = m_min_buffer_size;
           multiplier = m_min_buffer_size / ac3FrameSize; // int division is wanted
           rawlength_in_seconds = multiplier * m_format.m_streamInfo.GetDuration() / 1000;
          break;
          // EAC3 is currently not supported
        case CAEStreamInfo::STREAM_TYPE_EAC3:
           m_min_buffer_size = 10752; // least common multiple of 1792 and 1536
           m_format.m_frames = m_min_buffer_size; // needs testing
           rawlength_in_seconds = 4 * m_format.m_streamInfo.GetDuration() / 1000;
           break;
        default:
          m_min_buffer_size = MAX_RAW_AUDIO_BUFFER;
          m_format.m_frames = m_min_buffer_size;
          rawlength_in_seconds = 0.4;
          break;
      }

      CLog::Log(LOGDEBUG, "Opening Passthrough RAW Format: %s Sink SampleRate: %u", CAEUtil::StreamTypeToStr(m_format.m_streamInfo.m_type), m_sink_sampleRate);
      m_format.m_frameSize = 1;
      m_sink_frameSize = m_format.m_frameSize;
    }
    else
    {
      if (m_passthrough)
      {
        m_min_buffer_size *= 2;
        if (m_sink_sampleRate > 48000)
          m_min_buffer_size *= (m_sink_sampleRate / 48000); // same amount of buffer in seconds as for 48 khz
        else if (m_sink_sampleRate < m_format.m_sampleRate) // eac3
          m_min_buffer_size *= (m_format.m_sampleRate / m_sink_sampleRate);
      }
      else
        m_min_buffer_size *= 2;

      m_format.m_frameSize = m_format.m_channelLayout.Count() * (CAEUtil::DataFormatToBits(m_format.m_dataFormat) / 8);
      if (m_passthrough)
        m_sink_frameSize = 2 * CAEUtil::DataFormatToBits(AE_FMT_S16LE) / 8; // sending via 2 channels 2 * 16 / 8 = 4
      else
        m_sink_frameSize = m_format.m_frameSize;
      m_format.m_frames = (int)(m_min_buffer_size / m_format.m_frameSize) / 2;
    }

    if (m_passthrough && !m_info.m_wantsIECPassthrough)
      m_audiotrackbuffer_sec = rawlength_in_seconds;
    else
     m_audiotrackbuffer_sec = (double)(m_min_buffer_size / m_sink_frameSize) / (double)m_sink_sampleRate;


    CLog::Log(LOGDEBUG, "Created Audiotrackbuffer with playing time of %lf ms min buffer size: %u bytes",
                         m_audiotrackbuffer_sec * 1000, m_min_buffer_size);

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
      CLog::Log(LOGERROR, "AESinkAUDIOTRACK - Unable to create AudioTrack");
      Deinitialize();
      return false;
    }
    CLog::Log(LOGDEBUG, "CAESinkAUDIOTRACK::Initialize returned: m_sampleRate %u; format:%s; min_buffer_size %u; m_frames %u; m_frameSize %u; channels: %d", m_sink_sampleRate, CAEUtil::DataFormatToStr(m_format.m_dataFormat), m_min_buffer_size, m_format.m_frames, m_format.m_frameSize, m_format.m_channelLayout.Count());
  }
  format = m_format;

  // Force volume to 100% for IEC passthrough
  if (m_passthrough && m_info.m_wantsIECPassthrough)
  {
    CXBMCApp::AcquireAudioFocus();
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
  {
    CXBMCApp::SetSystemVolume(m_volume);
    CXBMCApp::ReleaseAudioFocus();
  }

  if (!m_at_jni)
    return;

  if (IsInitialized())
  {
    m_at_jni->stop();
    m_at_jni->flush();
  }
  m_at_jni->release();

  m_duration_written = 0;
  m_offset = -1;

  m_lastPlaybackHeadPosition = 0;
  m_extTimer.SetExpired();
  m_linearmovingaverage.clear();

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

  // head_pos does not necessarily start at the beginning
  if (m_offset == -1 && m_at_jni->getPlayState() == CJNIAudioTrack::PLAYSTATE_PLAYING)
  {
    m_offset = head_pos;
  }

  if (m_offset > head_pos)
  {
    CLog::Log(LOGDEBUG, "You did it wrong man - fully wrong! offset %lld head pos %u", m_offset, head_pos);
    m_offset = 0;
  }
  uint32_t normHead_pos = head_pos - m_offset;

#if defined(HAS_LIBAMCODEC)
  if (aml_present() &&
      (m_encoding == CJNIAudioFormat::ENCODING_DTS_HD ||
       m_encoding == CJNIAudioFormat::ENCODING_E_AC3 ||
       m_encoding == CJNIAudioFormat::ENCODING_DOLBY_TRUEHD))
    normHead_pos /= m_sink_frameSize;  // AML wants sink in 48k but returns pos in 192k
#endif

  if (m_passthrough && !m_info.m_wantsIECPassthrough)
  {
    if (m_extTimer.MillisLeft() > 0)
    {
      const double d = GetMovingAverageDelay(GetCacheTotal());
      status.SetDelay(d);
      return;
    }
  }
  if (normHead_pos > m_lastPlaybackHeadPosition)
  {
    unsigned int differencehead = normHead_pos - m_lastPlaybackHeadPosition;
    m_lastPlaybackHeadPosition = normHead_pos;
  }

  double gone = (double) normHead_pos / (double) m_sink_sampleRate;

  // if sink is run dry without buffer time written anymore
  if (gone > m_duration_written)
    gone = m_duration_written;

  double delay = m_duration_written - gone;
  if (delay < 0)
    delay = 0;

  const double d = GetMovingAverageDelay(delay);

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
    if (m_extTimer.MillisLeft() > 0)
    {
      double sleeptime = std::min((double) m_extTimer.MillisLeft(), m_format.m_streamInfo.GetDuration());
      usleep(sleeptime * 1000);
    }
    if (m_at_jni->getPlayState() != CJNIAudioTrack::PLAYSTATE_PLAYING)
      m_at_jni->play();

    bool retried = false;
    int size_left = size;
    while (written < size)
    {
      loop_written = m_at_jni->write((char*)out_buf, 0, size_left);
      written += loop_written;
      size_left -= loop_written;

      if (loop_written < 0)
      {
        CLog::Log(LOGERROR, "CAESinkAUDIOTRACK::AddPackets write returned error:  %d", loop_written);
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
            sleep_time = (double) m_format.m_frames / m_sink_frameSize / 2.0 / (double) m_format.m_sampleRate * 1000;
            usleep(sleep_time * 1000);
          }
          bool playing = m_at_jni->getPlayState() == CJNIAudioTrack::PLAYSTATE_PLAYING;
          CLog::Log(LOGDEBUG, "Retried to write onto the sink - slept: %lf playing: %s", sleep_time, playing ? "yes" : "no");
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
          CLog::Log(LOGDEBUG, "Error writing full package to sink, left: %d", size_left);
          // Let AE wait some ms to come back
          unsigned int written_frames = (unsigned int) (written/m_format.m_frameSize);
          return written_frames;
        }
      }
      else
        m_duration_written += ((double) loop_written / m_format.m_frameSize) / m_format.m_sampleRate;

      // just try again to care for fragmentation
      if (written < size)
        out_buf = out_buf + loop_written;

      loop_written = 0;
    }
  }
  unsigned int written_frames = (unsigned int) (written/m_format.m_frameSize);
  double time_to_add_ms = 1000.0 * (CurrentHostCounter() - startTime) / CurrentHostFrequency();
  if (m_passthrough && !m_info.m_wantsIECPassthrough)
  {
    // AT does not consume in a blocking way - it runs ahead and blocks
    // exactly once with the last package for some 100 ms
    // help it sleeping a bit
    if (time_to_add_ms < m_format.m_streamInfo.GetDuration() / 2.0)
    {
      // leave enough head room for eventualities
      double extra_sleep = m_format.m_streamInfo.GetDuration() / 4.0;
      usleep(extra_sleep * 1000);
      time_to_add_ms += extra_sleep;
    }
  }
  else
  {
    double time_should_ms = written_frames / (double) m_format.m_sampleRate * 1000.0;
    double time_off = time_should_ms - time_to_add_ms;
    if (time_off > 0 && time_off > time_should_ms / 2.0)
    {
      usleep(time_should_ms / 4.0 * 1000);
      time_to_add_ms += time_should_ms / 4.0;
    }
  }

  return written_frames;
}

void CAESinkAUDIOTRACK::AddPause(unsigned int millis)
{
  if (!m_at_jni)
    return;

  // on startup the buffer is empty, it "should" take the silence if we would really send some
  // without any delay. In between we need to sleep out the frames though
  if (m_extTimer.MillisLeft() + millis <= m_audiotrackbuffer_sec * 1000 && m_offset == -1)
    m_extTimer.Set(m_extTimer.MillisLeft() + millis);
  else
  {
    usleep(millis * 1000);
    m_extTimer.Set(m_extTimer.MillisLeft() + millis);
  }
}

void CAESinkAUDIOTRACK::Drain()
{
  if (!m_at_jni)
    return;

  CLog::Log(LOGDEBUG, "Draining Audio");
  m_at_jni->stop();
  m_duration_written = 0;
  m_offset = -1;
  m_extTimer.SetExpired();
  m_lastPlaybackHeadPosition = 0;
  m_linearmovingaverage.clear();
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

  m_sink_sampleRates.clear();
  m_sink_sampleRates.insert(CJNIAudioTrack::getNativeOutputSampleRate(CJNIAudioManager::STREAM_MUSIC));

  m_info.m_wantsIECPassthrough = true;
  if (!CXBMCApp::IsHeadsetPlugged())
  {
    m_info.m_deviceType = AE_DEVTYPE_HDMI;
    m_info.m_dataFormats.push_back(AE_FMT_RAW);
    m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_AC3);
    m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD_CORE);
    m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_1024);
    m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_2048);
    m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_512);

#if defined(HAS_LIBAMCODEC)
    if (aml_present())
    {
      // passthrough
      m_info.m_wantsIECPassthrough = true;
      m_sink_sampleRates.insert(44100);
      m_sink_sampleRates.insert(48000);
      if (HasAmlHD())
      {
        m_sink_sampleRates.insert(96000);
        m_sink_sampleRates.insert(192000);
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_EAC3);
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD);
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_TRUEHD);
      }
    }
    else
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
      if (CJNIAudioManager::GetSDKVersion() >= 23)
      {
        m_info.m_wantsIECPassthrough = false;
        // here only 5.1 would work but we cannot correctly distinguish
        // m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_EAC3);
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD);
      }
      if (StringUtils::StartsWithNoCase(CJNIBuild::DEVICE, "foster")) // SATV is ahead of API
      {
        m_info.m_wantsIECPassthrough = false;
        if (CJNIAudioManager::GetSDKVersion() == 22)
          m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD);
        m_info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_TRUEHD);
      }
    }
    std::copy(m_sink_sampleRates.begin(), m_sink_sampleRates.end(), std::back_inserter(m_info.m_sampleRates));
  }

  list.push_back(m_info);
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
  // m_{LWMA}^{(n)}(t) = \frac{2}{n (n+1)} \sum_{i=1}^n i \; x(t-n+i)
  const double denom = 2.0 / (size * (size + 1));
  double sum = 0.0;
  for (size_t i = 0; i < m_linearmovingaverage.size(); i++)
    sum += (i + 1) * m_linearmovingaverage.at(i);

  return sum * denom;
}

