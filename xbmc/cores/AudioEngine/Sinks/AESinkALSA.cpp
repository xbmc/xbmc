/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "AESinkALSA.h"
#include <stdint.h>
#include <limits.h>

#include "AEUtil.h"
#include "utils/StdString.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "settings/GUISettings.h"

#define ALSA_OPTIONS (SND_PCM_NONBLOCK | SND_PCM_NO_AUTO_CHANNELS | SND_PCM_NO_AUTO_FORMAT | SND_PCM_NO_AUTO_RESAMPLE)
#define ALSA_PERIODS 32

static enum AEChannel ALSAChannelMap[9] =
  {AE_CH_FL, AE_CH_FR, AE_CH_BL, AE_CH_BR, AE_CH_FC, AE_CH_LFE, AE_CH_SL, AE_CH_SR, AE_CH_NULL};

CAESinkALSA::CAESinkALSA() :
  m_channelLayout(NULL ),
  m_pcm          (NULL )
{
  /* ensure that ALSA has been initialized */
  if(!snd_config)
    snd_config_update();  
}

CAESinkALSA::~CAESinkALSA()
{
  Deinitialize();
}

inline unsigned int CAESinkALSA::GetChannelCount(const AEAudioFormat format)
{
  if (format.m_dataFormat == AE_FMT_RAW)
    return 2;
  else
  {
    unsigned int out = 0;
    for(unsigned int c = 0; c < 8; ++c)
      for(unsigned int i = 0; format.m_channelLayout[i] != AE_CH_NULL; ++i)
        if (format.m_channelLayout[i] == ALSAChannelMap[c])
        {
          out = c + 1;
          break;
        }

    return out;
  }
}

CStdString CAESinkALSA::GetDeviceUse(const AEAudioFormat format, CStdString device, bool passthrough)
{
  if (format.m_dataFormat == AE_FMT_RAW || passthrough)
  {
    if (device == "default")
    {
      if (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_HDMI)
        device = "hdmi";
      else
        device = "iec958";
    }

//    if (device == "iec958")
    {
      device += ":AES0=0x06,AES1=0x82,AES2=0x00";
           if (format.m_sampleRate == 192000) device += ",AES3=0x0e";
      else if (format.m_sampleRate == 176400) device += ",AES3=0x0c";
      else if (format.m_sampleRate ==  96000) device += ",AES3=0x0a";
      else if (format.m_sampleRate ==  88200) device += ",AES3=0x08";
      else if (format.m_sampleRate ==  48000) device += ",AES3=0x02";
      else if (format.m_sampleRate ==  44100) device += ",AES3=0x00";
      else if (format.m_sampleRate ==  32000) device += ",AES3=0x03";
      else device += ",AES3=0x01";
    }
  }

  if (device == "default" && g_guiSettings.GetInt("audiooutput.mode") == AUDIO_HDMI)
    device = "hdmi";

  if (device == "hdmi")
    return "plug:hdmi";

  if (device == "default")
    switch(format.m_channelCount)
    {
      case 8: return "plug:surround71";
      case 6: return "plug:surround51";
      case 5: return "plug:surround50";
      case 4: return "plug:surround40";
    }

  return device;
}

bool CAESinkALSA::Initialize(AEAudioFormat &format, CStdString &device)
{
  format.m_channelCount = GetChannelCount(format);
  m_initFormat = format;

  if (format.m_channelCount == 0)
  {
    CLog::Log(LOGERROR, "CAESinkALSA::Initialize - Unable to open the requested channel layout");
    return false;
  }

  /* if we are raw, correct the data format */
  if (format.m_dataFormat == AE_FMT_RAW)
  {
    format.m_dataFormat   = AE_FMT_S16NE;
    format.m_channelCount = 2;
    m_passthrough         = true;
  }
  else
    m_passthrough = false;

  m_channelLayout = new enum AEChannel[format.m_channelCount + 1];
  memcpy(m_channelLayout, ALSAChannelMap, format.m_channelCount * sizeof(enum AEChannel));
  m_channelLayout[format.m_channelCount] = AE_CH_NULL;

  /* set the channelLayout and the output device */
  format.m_channelLayout = m_channelLayout;
  m_device = device      = GetDeviceUse(format, device, m_passthrough);

  /* get the sound config */
  snd_config_t *config;
  snd_config_copy(&config, snd_config);
  int error;

  error = snd_pcm_open_lconf(&m_pcm, device.c_str(), SND_PCM_STREAM_PLAYBACK, ALSA_OPTIONS, config);
  if (error < 0)
  {
    CLog::Log(LOGERROR, "CAESinkALSA::Initialize - snd_pcm_open_lconf(%d) - %s", error, device.c_str());
    snd_config_delete(config);
    return false;
  }

  /* free the sound config */
  snd_config_delete(config);

  if (!InitializeHW(format)) return false;
  if (!InitializeSW(format)) return false;

  snd_pcm_nonblock(m_pcm, 1);
  snd_pcm_prepare (m_pcm);

  m_format = format;
  return true;
}

bool CAESinkALSA::IsCompatible(const AEAudioFormat format, const CStdString device)
{
  AEAudioFormat tmp  = format;
  tmp.m_channelCount = GetChannelCount(format);

  return (
    tmp.m_sampleRate   == m_initFormat.m_sampleRate    &&
    tmp.m_dataFormat   == m_initFormat.m_dataFormat    &&
    tmp.m_channelCount == m_initFormat.m_channelCount  &&
    GetDeviceUse(tmp, device, m_passthrough) == m_device
  );
}

snd_pcm_format_t CAESinkALSA::AEFormatToALSAFormat(const enum AEDataFormat format)
{
  switch(format)
  {
    case AE_FMT_S8    : return SND_PCM_FORMAT_S8;
    case AE_FMT_U8    : return SND_PCM_FORMAT_U8;
    case AE_FMT_S16NE : return SND_PCM_FORMAT_S16;
    case AE_FMT_S24NE4: return SND_PCM_FORMAT_S24;
#ifdef __BIG_ENDIAN__
    case AE_FMT_S24NE3: return SND_PCM_FORMAT_S24_3BE;
#else
    case AE_FMT_S24NE3: return SND_PCM_FORMAT_S24_3LE;
#endif
    case AE_FMT_S32NE : return SND_PCM_FORMAT_S32;
    case AE_FMT_FLOAT : return SND_PCM_FORMAT_FLOAT;
    case AE_FMT_RAW   : return SND_PCM_FORMAT_S16_LE;

    default:
      return SND_PCM_FORMAT_UNKNOWN;
  }
}

bool CAESinkALSA::InitializeHW(AEAudioFormat &format)
{
  snd_pcm_hw_params_t *hw_params;

  snd_pcm_hw_params_malloc(&hw_params);
  snd_pcm_hw_params_any(m_pcm, hw_params);
  snd_pcm_hw_params_set_access(m_pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);

  snd_pcm_format_t fmt = AEFormatToALSAFormat(format.m_dataFormat);
  if (fmt == SND_PCM_FORMAT_UNKNOWN)
  {
      /* if we dont support the requested format, fallback to float */
      format.m_dataFormat = AE_FMT_FLOAT;
      fmt                 = SND_PCM_FORMAT_FLOAT;
  }

  /* try the data format */
  if (snd_pcm_hw_params_set_format(m_pcm, hw_params, fmt) < 0)
  {
    /* if the chosen format is not supported, try each one in decending order */
    CLog::Log(LOGINFO, "CAESinkALSA::InitializeHW - Your hardware does not support %s, trying other formats", CAEUtil::DataFormatToStr(format.m_dataFormat));
    for(enum AEDataFormat i = AE_FMT_MAX; i > AE_FMT_INVALID; i = (enum AEDataFormat)((int)i - 1))
    {
      if (i == AE_FMT_RAW || i == AE_FMT_MAX) continue;
      fmt = AEFormatToALSAFormat(i);

      if (fmt == SND_PCM_FORMAT_UNKNOWN || snd_pcm_hw_params_set_format(m_pcm, hw_params, fmt) < 0)
      {
        fmt = SND_PCM_FORMAT_UNKNOWN;
        continue;
      }

      int fmtBits = CAEUtil::DataFormatToBits(i);
      int bits    = snd_pcm_hw_params_get_sbits(hw_params);
      if (bits != fmtBits)
      {
        /* if we opened in 32bit and only have 24bits, pack into 24 */
        if (fmtBits == 32 && bits == 24)
          i = AE_FMT_S24NE4;
        else
          continue;
      }

      /* record that the format fell back to X */
      format.m_dataFormat = i;
      CLog::Log(LOGINFO, "CAESinkALSA::InitializeHW - Using data format %s", CAEUtil::DataFormatToStr(format.m_dataFormat));
      break;
    }

    /* if we failed to find a valid output format */
    if (fmt == SND_PCM_FORMAT_UNKNOWN)
    {
      CLog::Log(LOGERROR, "CAESinkALSA::InitializeHW - Unable to find a suitable output format");
      snd_pcm_hw_params_free(hw_params);
      return false;
    }
  }

  snd_pcm_hw_params_t *hw_params_copy;
  snd_pcm_hw_params_malloc(&hw_params_copy);

  unsigned int sampleRate = format.m_sampleRate;
  snd_pcm_hw_params_set_rate_near(m_pcm, hw_params, &sampleRate          , NULL);
  snd_pcm_hw_params_set_channels (m_pcm, hw_params, format.m_channelCount      );

  unsigned int frames  = sampleRate / 1000;
  unsigned int periods = ALSA_PERIODS;

  snd_pcm_uframes_t periodSize = frames * (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3) * format.m_channelCount;
  snd_pcm_uframes_t bufferSize = periodSize * periods;

  /* try to set the buffer size then the period size */
  snd_pcm_hw_params_copy(hw_params_copy, hw_params);
  snd_pcm_hw_params_set_buffer_size_near(m_pcm, hw_params_copy, &bufferSize);
  snd_pcm_hw_params_set_period_size_near(m_pcm, hw_params_copy, &periodSize, NULL);
  snd_pcm_hw_params_set_periods_near    (m_pcm, hw_params_copy, &periods   , NULL);
  if (snd_pcm_hw_params(m_pcm, hw_params_copy) != 0)
  {
    /* try to set the period size then the buffer size */
    snd_pcm_hw_params_copy(hw_params_copy, hw_params);
    snd_pcm_hw_params_set_period_size_near(m_pcm, hw_params_copy, &periodSize, NULL);
    snd_pcm_hw_params_set_buffer_size_near(m_pcm, hw_params_copy, &bufferSize);
    snd_pcm_hw_params_set_periods_near    (m_pcm, hw_params_copy, &periods   , NULL);
    if (snd_pcm_hw_params(m_pcm, hw_params_copy) != 0)
    {
      /* try to just set the buffer size */
      snd_pcm_hw_params_copy(hw_params_copy, hw_params);
      snd_pcm_hw_params_set_buffer_size_near(m_pcm, hw_params_copy, &bufferSize);
      snd_pcm_hw_params_set_periods_near    (m_pcm, hw_params_copy, &periods   , NULL);
      if (snd_pcm_hw_params(m_pcm, hw_params_copy) != 0)
      {
        /* try to just set the period size */
        snd_pcm_hw_params_copy(hw_params_copy, hw_params);
        snd_pcm_hw_params_set_period_size_near(m_pcm, hw_params_copy, &periodSize, NULL);
        snd_pcm_hw_params_set_periods_near    (m_pcm, hw_params_copy, &periods   , NULL);
        if (snd_pcm_hw_params(m_pcm, hw_params_copy) != 0)
        {
          CLog::Log(LOGERROR, "CAESinkALSA::InitializeHW - Failed to set the parameters");
          snd_pcm_hw_params_free(hw_params_copy);
          snd_pcm_hw_params_free(hw_params     );
          return false;
	}
      }
    }
  }

  snd_pcm_hw_params_get_period_size(hw_params_copy, &periodSize, NULL);
  snd_pcm_hw_params_get_buffer_size(hw_params_copy, &bufferSize);

  /* set the format parameters */
  format.m_sampleRate   = sampleRate;
  format.m_frames       = snd_pcm_bytes_to_frames(m_pcm, periodSize);
  format.m_frameSamples = format.m_frames * format.m_channelCount;
  format.m_frameSize    = snd_pcm_frames_to_bytes(m_pcm, 1);
  m_timeout             = -1;//((float)format.m_frames / sampleRate * 1000.0f) * 2;

  snd_pcm_hw_params_free(hw_params_copy);
  snd_pcm_hw_params_free(hw_params    );
  return true;
}

bool CAESinkALSA::InitializeSW(AEAudioFormat &format)
{
  snd_pcm_sw_params_t *sw_params;
  snd_pcm_uframes_t boundary;

  snd_pcm_sw_params_malloc(&sw_params);

  snd_pcm_sw_params_current              (m_pcm, sw_params);
  snd_pcm_sw_params_set_start_threshold  (m_pcm, sw_params, INT_MAX);
  snd_pcm_sw_params_set_silence_threshold(m_pcm, sw_params, 0);
  snd_pcm_sw_params_get_boundary         (sw_params, &boundary);
  snd_pcm_sw_params_set_silence_size     (m_pcm, sw_params, boundary);
  snd_pcm_sw_params_set_avail_min        (m_pcm, sw_params, format.m_frames);

  if (snd_pcm_sw_params(m_pcm, sw_params) < 0)
  {
    CLog::Log(LOGERROR, "CAESinkALSA::InitializeSW - Failed to set the parameters");
    snd_pcm_sw_params_free(sw_params);
    return false;
  }

  return true;
}

void CAESinkALSA::Deinitialize()
{
  Stop();

  if (m_pcm)
  {
    snd_pcm_drop (m_pcm);
    snd_pcm_close(m_pcm);
    m_pcm = NULL;
  }

  delete[] m_channelLayout;
  m_channelLayout = NULL;
}

void CAESinkALSA::Stop()
{
  if (!m_pcm) return;
  snd_pcm_drop(m_pcm);
}

float CAESinkALSA::GetDelay()
{
  if (!m_pcm) return 0;
  snd_pcm_sframes_t frames = 0;
  snd_pcm_delay(m_pcm, &frames);

  if (frames < 0)
  {
#if SND_LIB_VERSION >= 0x000901 /* snd_pcm_forward() exists since 0.9.0rc8 */
    snd_pcm_forward(m_pcm, -frames);
#endif
    frames = 0;
  }

  return (double)frames / m_format.m_sampleRate;
}

unsigned int CAESinkALSA::AddPackets(uint8_t *data, unsigned int frames)
{
  if (!m_pcm) return 0;

  if(snd_pcm_state(m_pcm) == SND_PCM_STATE_PREPARED)
    snd_pcm_start(m_pcm);

  if (snd_pcm_wait(m_pcm, m_timeout) == 0)
  {
    CLog::Log(LOGERROR, "CAESinkALSA::AddPackets - Timeout waiting for space");
    return 0;
  }

  int ret = snd_pcm_writei(m_pcm, (void*)data, frames);
  if (ret < 0)
  {
    if (ret == -EPIPE)
    {
      CLog::Log(LOGERROR, "CAESinkALSA::AddPackets - Underrun");
      if (snd_pcm_prepare(m_pcm) < 0)
        return 0;
    }

    CLog::Log(LOGERROR, "CAESinkALSA::AddPackets - snd_pcm_writei returned %d", ret);
    return 0;
  }

  return ret;
}

void CAESinkALSA::Drain()
{
  if (!m_pcm) return;

  snd_pcm_nonblock(m_pcm, 0);
  snd_pcm_drain(m_pcm);
  snd_pcm_nonblock(m_pcm, 1);
}

void CAESinkALSA::EnumerateDevices (AEDeviceList &devices, bool passthrough)
{
  if (!passthrough)
  {
    devices.push_back(AEDevice("default", "alsa:default"));
    devices.push_back(AEDevice("iec958" , "alsa:plug:iec958"));
    devices.push_back(AEDevice("hdmi"   , "alsa:plug:hdmi"));
  }
  else
  {
    devices.push_back(AEDevice("iec958" , "alsa:iec958"));
    devices.push_back(AEDevice("hdmi"   , "alsa:hdmi"));
  }

  int n_cards = -1;
  int numberCards = 0;
  while(snd_card_next( &n_cards ) == 0 && n_cards >= 0)
    numberCards++;

  if (numberCards <= 1)
    return;

  snd_ctl_t *handle;
  snd_ctl_card_info_t *info;
  snd_ctl_card_info_alloca( &info );
  CStdString strHwName;
  n_cards = -1;

  while(snd_card_next( &n_cards ) == 0 && n_cards >= 0)
  {
    strHwName.Format("hw:%d", n_cards);
    if (snd_ctl_open( &handle, strHwName.c_str(), 0 ) == 0)
    {
      if (snd_ctl_card_info( handle, info ) == 0)
      {
        CStdString strReadableCardName = snd_ctl_card_info_get_name( info );
        CStdString strCardName = snd_ctl_card_info_get_id( info );

        if (!passthrough)
          GenSoundLabel(devices, "default", strCardName, strReadableCardName);
        GenSoundLabel(devices, "iec958", strCardName, strReadableCardName);
        GenSoundLabel(devices, "hdmi", strCardName, strReadableCardName);
      }
      else
        CLog::Log(LOGERROR,"((ALSAENUM))control hardware info (%i): failed.\n", n_cards );
      snd_ctl_close( handle );
    }
    else
      CLog::Log(LOGERROR,"((ALSAENUM))control open (%i) failed.\n", n_cards );
  }
}

bool CAESinkALSA::SoundDeviceExists(const CStdString& device)
{
  void **hints, **n;
  char *name;
  bool retval = false;

  if (snd_device_name_hint(-1, "pcm", &hints) == 0)
  {
    for (n = hints; *n; n++)
    {
      if ((name = snd_device_name_get_hint(*n, "NAME")) != NULL)
      {
        CStdString strName = name;
        free(name);
        if (strName.find(device) != CStdString::npos)
        {
          retval = true;
          break;
        }
      }
    }
    snd_device_name_free_hint(hints);
  }
  return retval;
}

void CAESinkALSA::GenSoundLabel(AEDeviceList& devices, CStdString sink, CStdString card, CStdString readableCard)
{
  CStdString deviceString;
  deviceString.Format("%s:CARD=%s", sink, card.c_str());
  if (sink.Equals("default") || SoundDeviceExists(deviceString.c_str()))
  {
    CStdString finalSink;
    finalSink.Format("alsa:%s", deviceString.c_str());
    CStdString label = readableCard + " " + sink;
    devices.push_back(AEDevice(label, finalSink));
  }
}

