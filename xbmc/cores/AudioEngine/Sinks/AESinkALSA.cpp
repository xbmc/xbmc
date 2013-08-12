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
#include "system.h"
#ifdef HAS_ALSA

#include <stdint.h>
#include <limits.h>
#include <set>
#include <sstream>

#include "AESinkALSA.h"
#include "Utils/AEUtil.h"
#include "Utils/AEELDParser.h"
#include "utils/StdString.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "threads/SingleLock.h"
#if defined(HAS_AMLPLAYER)
#include "cores/amlplayer/AMLUtils.h"
#endif

#define ALSA_OPTIONS (SND_PCM_NONBLOCK | SND_PCM_NO_AUTO_FORMAT | SND_PCM_NO_AUTO_CHANNELS | SND_PCM_NO_AUTO_RESAMPLE)

#define ALSA_MAX_CHANNELS 16
static enum AEChannel ALSAChannelMap[ALSA_MAX_CHANNELS + 1] = {
  AE_CH_FL      , AE_CH_FR      , AE_CH_BL      , AE_CH_BR      , AE_CH_FC      , AE_CH_LFE     , AE_CH_SL      , AE_CH_SR      ,
  AE_CH_UNKNOWN1, AE_CH_UNKNOWN2, AE_CH_UNKNOWN3, AE_CH_UNKNOWN4, AE_CH_UNKNOWN5, AE_CH_UNKNOWN6, AE_CH_UNKNOWN7, AE_CH_UNKNOWN8, /* for p16v devices */
  AE_CH_NULL
};

static unsigned int ALSASampleRateList[] =
{
  5512,
  8000,
  11025,
  16000,
  22050,
  32000,
  44100,
  48000,
  64000,
  88200,
  96000,
  176400,
  192000,
  384000,
  0
};

CAESinkALSA::CAESinkALSA() :
  m_bufferSize(0),
  m_formatSampleRateMul(0.0),
  m_passthrough(false),
  m_pcm(NULL),
  m_timeout(0)
{
  /* ensure that ALSA has been initialized */
  if (!snd_config)
    snd_config_update();
}

CAESinkALSA::~CAESinkALSA()
{
  Deinitialize();
}

inline CAEChannelInfo CAESinkALSA::GetChannelLayout(AEAudioFormat format)
{
  unsigned int count = 0;

       if (format.m_dataFormat == AE_FMT_AC3 ||
           format.m_dataFormat == AE_FMT_DTS ||
           format.m_dataFormat == AE_FMT_EAC3)
           count = 2;
  else if (format.m_dataFormat == AE_FMT_TRUEHD ||
           format.m_dataFormat == AE_FMT_DTSHD)
           count = 8;
  else
  {
    for (unsigned int c = 0; c < 8; ++c)
      for (unsigned int i = 0; i < format.m_channelLayout.Count(); ++i)
        if (format.m_channelLayout[i] == ALSAChannelMap[c])
        {
          count = c + 1;
          break;
        }
  }

  CAEChannelInfo info;
  for (unsigned int i = 0; i < count; ++i)
    info += ALSAChannelMap[i];

  return info;
}

void CAESinkALSA::GetAESParams(AEAudioFormat format, std::string& params)
{
  if (m_passthrough)
    params = "AES0=0x06";
  else
    params = "AES0=0x04";

  params += ",AES1=0x82,AES2=0x00";

       if (format.m_sampleRate == 192000) params += ",AES3=0x0e";
  else if (format.m_sampleRate == 176400) params += ",AES3=0x0c";
  else if (format.m_sampleRate ==  96000) params += ",AES3=0x0a";
  else if (format.m_sampleRate ==  88200) params += ",AES3=0x08";
  else if (format.m_sampleRate ==  48000) params += ",AES3=0x02";
  else if (format.m_sampleRate ==  44100) params += ",AES3=0x00";
  else if (format.m_sampleRate ==  32000) params += ",AES3=0x03";
  else params += ",AES3=0x01";
}

bool CAESinkALSA::Initialize(AEAudioFormat &format, std::string &device)
{
  CAEChannelInfo channelLayout;
  m_initDevice = device;
  m_initFormat = format;

  /* if we are raw, correct the data format */
  if (AE_IS_RAW(format.m_dataFormat))
  {
    channelLayout     = GetChannelLayout(format);
    format.m_dataFormat = AE_FMT_S16NE;
    m_passthrough       = true;
  }
  else
  {
    channelLayout = GetChannelLayout(format);
    m_passthrough   = false;
  }
#if defined(HAS_AMLPLAYER) || defined(HAS_LIBAMCODEC)
  if (aml_present())
  {
    aml_set_audio_passthrough(m_passthrough);
    device = "default";
  }
#endif

  if (channelLayout.Count() == 0)
  {
    CLog::Log(LOGERROR, "CAESinkALSA::Initialize - Unable to open the requested channel layout");
    return false;
  }

  format.m_channelLayout = channelLayout;

  AEDeviceType devType = AEDeviceTypeFromName(device);

  std::string AESParams;
  /* digital interfaces should have AESx set, though in practice most
   * receivers don't care */
  if (m_passthrough || devType == AE_DEVTYPE_HDMI || devType == AE_DEVTYPE_IEC958)
    GetAESParams(format, AESParams);

  CLog::Log(LOGINFO, "CAESinkALSA::Initialize - Attempting to open device \"%s\"", device.c_str());

  /* get the sound config */
  snd_config_t *config;
  snd_config_copy(&config, snd_config);

  if (!OpenPCMDevice(device, AESParams, channelLayout.Count(), &m_pcm, config))
  {
    CLog::Log(LOGERROR, "CAESinkALSA::Initialize - failed to initialize device \"%s\"", device.c_str());
    snd_config_delete(config);
    return false;
  }

  /* get the actual device name that was used */
  device = snd_pcm_name(m_pcm);
  m_device = device;

  CLog::Log(LOGINFO, "CAESinkALSA::Initialize - Opened device \"%s\"", device.c_str());

  /* free the sound config */
  snd_config_delete(config);

  if (!InitializeHW(format) || !InitializeSW(format))
    return false;

  snd_pcm_nonblock(m_pcm, 1);
  snd_pcm_prepare (m_pcm);

  m_format              = format;
  m_formatSampleRateMul = 1.0 / (double)m_format.m_sampleRate;

  return true;
}

bool CAESinkALSA::IsCompatible(const AEAudioFormat &format, const std::string &device)
{
  return (
      /* compare against the requested format and the real format */
      (m_initFormat.m_sampleRate    == format.m_sampleRate    || m_format.m_sampleRate    == format.m_sampleRate   ) &&
      (m_initFormat.m_dataFormat    == format.m_dataFormat    || m_format.m_dataFormat    == format.m_dataFormat   ) &&
      (m_initFormat.m_channelLayout == format.m_channelLayout || m_format.m_channelLayout == format.m_channelLayout) &&
      (m_initDevice == device)
  );
}

snd_pcm_format_t CAESinkALSA::AEFormatToALSAFormat(const enum AEDataFormat format)
{
  if (AE_IS_RAW(format))
    return SND_PCM_FORMAT_S16;

  switch (format)
  {
    case AE_FMT_S8    : return SND_PCM_FORMAT_S8;
    case AE_FMT_U8    : return SND_PCM_FORMAT_U8;
    case AE_FMT_S16NE : return SND_PCM_FORMAT_S16;
    case AE_FMT_S16LE : return SND_PCM_FORMAT_S16_LE;
    case AE_FMT_S16BE : return SND_PCM_FORMAT_S16_BE;
    case AE_FMT_S24NE4: return SND_PCM_FORMAT_S24;
#ifdef __BIG_ENDIAN__
    case AE_FMT_S24NE3: return SND_PCM_FORMAT_S24_3BE;
#else
    case AE_FMT_S24NE3: return SND_PCM_FORMAT_S24_3LE;
#endif
    case AE_FMT_S32NE : return SND_PCM_FORMAT_S32;
    case AE_FMT_FLOAT : return SND_PCM_FORMAT_FLOAT;

    default:
      return SND_PCM_FORMAT_UNKNOWN;
  }
}

bool CAESinkALSA::InitializeHW(AEAudioFormat &format)
{
  snd_pcm_hw_params_t *hw_params;

  snd_pcm_hw_params_alloca(&hw_params);
  memset(hw_params, 0, snd_pcm_hw_params_sizeof());

  snd_pcm_hw_params_any(m_pcm, hw_params);
  snd_pcm_hw_params_set_access(m_pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);

  unsigned int sampleRate   = format.m_sampleRate;
  unsigned int channelCount = format.m_channelLayout.Count();
  snd_pcm_hw_params_set_rate_near    (m_pcm, hw_params, &sampleRate, NULL);
  snd_pcm_hw_params_set_channels_near(m_pcm, hw_params, &channelCount);

  /* ensure we opened X channels or more */
  if (format.m_channelLayout.Count() > channelCount)
  {
    CLog::Log(LOGINFO, "CAESinkALSA::InitializeHW - Unable to open the required number of channels");
  }

  /* update the channelLayout to what we managed to open */
  format.m_channelLayout.Reset();
  for (unsigned int i = 0; i < channelCount; ++i)
    format.m_channelLayout += ALSAChannelMap[i];

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
    for (enum AEDataFormat i = AE_FMT_MAX; i > AE_FMT_INVALID; i = (enum AEDataFormat)((int)i - 1))
    {
      if (AE_IS_RAW(i) || i == AE_FMT_MAX)
        continue;

      if (m_passthrough && i != AE_FMT_S16BE && i != AE_FMT_S16LE)
	continue;

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
      return false;
    }
  }

  snd_pcm_uframes_t periodSize, bufferSize;
  snd_pcm_hw_params_get_buffer_size_max(hw_params, &bufferSize);
  snd_pcm_hw_params_get_period_size_max(hw_params, &periodSize, NULL);

  /* 
   We want to make sure, that we have max 200 ms Buffer with 
   a periodSize of approx 50 ms. Choosing a higher bufferSize
   will cause problems with menu sounds. Buffer will be increased
   after those are fixed.
  */
  periodSize  = std::min(periodSize, (snd_pcm_uframes_t) sampleRate / 20);
  bufferSize  = std::min(bufferSize, (snd_pcm_uframes_t) sampleRate / 5);
  
  /* 
   According to upstream we should set buffer size first - so make sure it is always at least
   4x period size to not get underruns (some systems seem to have issues with only 2 periods)
  */
  periodSize = std::min(periodSize, bufferSize / 4);

  CLog::Log(LOGDEBUG, "CAESinkALSA::InitializeHW - Request: periodSize %lu, bufferSize %lu", periodSize, bufferSize);

  snd_pcm_hw_params_t *hw_params_copy;
  snd_pcm_hw_params_alloca(&hw_params_copy);
  snd_pcm_hw_params_copy(hw_params_copy, hw_params); // copy what we have and is already working
  
  // first trying bufferSize, PeriodSize
  // for more info see here:
  // http://mailman.alsa-project.org/pipermail/alsa-devel/2009-September/021069.html
  // the last three tries are done as within pulseaudio

  // backup periodSize and bufferSize first. Restore them after every failed try
  snd_pcm_uframes_t periodSizeTemp, bufferSizeTemp;
  periodSizeTemp = periodSize;
  bufferSizeTemp = bufferSize;
  if (snd_pcm_hw_params_set_buffer_size_near(m_pcm, hw_params_copy, &bufferSize) != 0
    || snd_pcm_hw_params_set_period_size_near(m_pcm, hw_params_copy, &periodSize, NULL) != 0
    || snd_pcm_hw_params(m_pcm, hw_params_copy) != 0)
  {
    bufferSize = bufferSizeTemp;
    periodSize = periodSizeTemp;
    // retry with PeriodSize, bufferSize
    snd_pcm_hw_params_copy(hw_params_copy, hw_params); // restore working copy
    if (snd_pcm_hw_params_set_period_size_near(m_pcm, hw_params_copy, &periodSize, NULL) != 0
      || snd_pcm_hw_params_set_buffer_size_near(m_pcm, hw_params_copy, &bufferSize) != 0
      || snd_pcm_hw_params(m_pcm, hw_params_copy) != 0)
    {
      // try only periodSize
      periodSize = periodSizeTemp;
      snd_pcm_hw_params_copy(hw_params_copy, hw_params); // restore working copy
      if(snd_pcm_hw_params_set_period_size_near(m_pcm, hw_params_copy, &periodSize, NULL) != 0 
        || snd_pcm_hw_params(m_pcm, hw_params_copy) != 0)
      {
        // try only BufferSize
        bufferSize = bufferSizeTemp;
        snd_pcm_hw_params_copy(hw_params_copy, hw_params); // restory working copy
        if (snd_pcm_hw_params_set_buffer_size_near(m_pcm, hw_params_copy, &bufferSize) != 0
          || snd_pcm_hw_params(m_pcm, hw_params_copy) != 0)
        {
          // set default that Alsa would choose
          CLog::Log(LOGWARNING, "CAESinkAlsa::IntializeHW - Using default alsa values - set failed");
          if (snd_pcm_hw_params(m_pcm, hw_params) != 0)
          {
            CLog::Log(LOGDEBUG, "CAESinkALSA::InitializeHW - Could not init a valid sink");
            return false;
          }
        }
      }
      // reread values when alsa default was kept
      snd_pcm_get_params(m_pcm, &bufferSize, &periodSize);
    }
  }
  
  CLog::Log(LOGDEBUG, "CAESinkALSA::InitializeHW - Got: periodSize %lu, bufferSize %lu", periodSize, bufferSize);

  /* set the format parameters */
  format.m_sampleRate   = sampleRate;
  format.m_frames       = periodSize;
  format.m_frameSamples = periodSize * format.m_channelLayout.Count();
  format.m_frameSize    = snd_pcm_frames_to_bytes(m_pcm, 1);

  m_bufferSize = (unsigned int)bufferSize;
  m_timeout    = std::ceil((double)(bufferSize * 1000) / (double)sampleRate);

  CLog::Log(LOGDEBUG, "CAESinkALSA::InitializeHW - Setting timeout to %d ms", m_timeout);

  return true;
}

bool CAESinkALSA::InitializeSW(AEAudioFormat &format)
{
  snd_pcm_sw_params_t *sw_params;
  snd_pcm_uframes_t boundary;

  snd_pcm_sw_params_alloca(&sw_params);
  memset(sw_params, 0, snd_pcm_sw_params_sizeof());

  snd_pcm_sw_params_current              (m_pcm, sw_params);
  snd_pcm_sw_params_set_start_threshold  (m_pcm, sw_params, INT_MAX);
  snd_pcm_sw_params_set_silence_threshold(m_pcm, sw_params, 0);
  snd_pcm_sw_params_get_boundary         (sw_params, &boundary);
  snd_pcm_sw_params_set_silence_size     (m_pcm, sw_params, boundary);
  snd_pcm_sw_params_set_avail_min        (m_pcm, sw_params, format.m_frames);

  if (snd_pcm_sw_params(m_pcm, sw_params) < 0)
  {
    CLog::Log(LOGERROR, "CAESinkALSA::InitializeSW - Failed to set the parameters");
    return false;
  }

  return true;
}

void CAESinkALSA::Deinitialize()
{
  if (m_pcm)
  {
    snd_pcm_nonblock(m_pcm, 0);
    Stop();
    snd_pcm_close(m_pcm);
    m_pcm = NULL;
  }
}

void CAESinkALSA::Stop()
{
  if (!m_pcm)
    return;
  snd_pcm_drop(m_pcm);
}

double CAESinkALSA::GetDelay()
{
  if (!m_pcm)
    return 0;
  snd_pcm_sframes_t frames = 0;
  snd_pcm_delay(m_pcm, &frames);

  if (frames < 0)
  {
#if SND_LIB_VERSION >= 0x000901 /* snd_pcm_forward() exists since 0.9.0rc8 */
    snd_pcm_forward(m_pcm, -frames);
#endif
    frames = 0;
  }

  return (double)frames * m_formatSampleRateMul;
}

double CAESinkALSA::GetCacheTime()
{
  if (!m_pcm)
    return 0.0;

  int space = snd_pcm_avail_update(m_pcm);
  if (space == 0)
  {
    snd_pcm_state_t state = snd_pcm_state(m_pcm);
    if (state < 0)
    {
      HandleError("snd_pcm_state", state);
      space = m_bufferSize;
    }

    if (state != SND_PCM_STATE_RUNNING && state != SND_PCM_STATE_PREPARED)
      space = m_bufferSize;
  }

  return (double)(m_bufferSize - space) * m_formatSampleRateMul;
}

double CAESinkALSA::GetCacheTotal()
{
  return (double)m_bufferSize * m_formatSampleRateMul;
}

unsigned int CAESinkALSA::AddPackets(uint8_t *data, unsigned int frames, bool hasAudio, bool blocking)
{
  if (!m_pcm)
  {
    SoftResume();
    if(!m_pcm)
      return 0;

    CLog::Log(LOGDEBUG, "CAESinkALSA - the grAEken is hunger, feed it (I am the downmost fallback - fix your code)");
  }

  int ret;

  ret = snd_pcm_avail(m_pcm);
  if (ret < 0) 
  {
    HandleError("snd_pcm_avail", ret);
    ret = 0;
  }

  if ((unsigned int)ret < frames)
  {
    if(blocking)
    {
      ret = snd_pcm_wait(m_pcm, m_timeout);
      if (ret < 0)
        HandleError("snd_pcm_wait", ret);
    }
    else
      return 0;
  }

  ret = snd_pcm_writei(m_pcm, (void*)data, frames);
  if (ret < 0)
  {
    HandleError("snd_pcm_writei(1)", ret);
    ret = snd_pcm_writei(m_pcm, (void*)data, frames);
    if (ret < 0)
    {
      HandleError("snd_pcm_writei(2)", ret);
      ret = 0;
    }
  }

  if ( ret > 0 && snd_pcm_state(m_pcm) == SND_PCM_STATE_PREPARED)
    snd_pcm_start(m_pcm);

  return ret;
}

void CAESinkALSA::HandleError(const char* name, int err)
{
  switch(err)
  {
    case -EPIPE:
      CLog::Log(LOGERROR, "CAESinkALSA::HandleError(%s) - underrun", name);
      if ((err = snd_pcm_prepare(m_pcm)) < 0)
        CLog::Log(LOGERROR, "CAESinkALSA::HandleError(%s) - snd_pcm_prepare returned %d (%s)", name, err, snd_strerror(err));
      break;

    case -ESTRPIPE:
      CLog::Log(LOGINFO, "CAESinkALSA::HandleError(%s) - Resuming after suspend", name);

      /* try to resume the stream */
      while((err = snd_pcm_resume(m_pcm)) == -EAGAIN)
        Sleep(1);

      /* if the hardware doesnt support resume, prepare the stream */
      if (err == -ENOSYS)
        if ((err = snd_pcm_prepare(m_pcm)) < 0)
          CLog::Log(LOGERROR, "CAESinkALSA::HandleError(%s) - snd_pcm_prepare returned %d (%s)", name, err, snd_strerror(err));
      break;

    default:
      CLog::Log(LOGERROR, "CAESinkALSA::HandleError(%s) - snd_pcm_writei returned %d (%s)", name, err, snd_strerror(err));
      break;
  }
}

void CAESinkALSA::Drain()
{
  if (!m_pcm)
    return;

  snd_pcm_nonblock(m_pcm, 0);
  snd_pcm_drain(m_pcm);
  snd_pcm_prepare(m_pcm);
  snd_pcm_nonblock(m_pcm, 1);
}

void CAESinkALSA::AppendParams(std::string &device, const std::string &params)
{
  /* Note: escaping, e.g. "plug:'something:X=y'" isn't handled,
   * but it is not normally encountered at this point. */

  device += (device.find(':') == std::string::npos) ? ':' : ',';
  device += params;
}

bool CAESinkALSA::TryDevice(const std::string &name, snd_pcm_t **pcmp, snd_config_t *lconf)
{
  /* Check if this device was already open (e.g. when checking for supported
   * channel count in EnumerateDevice()) */
  if (*pcmp)
  {
    if (name == snd_pcm_name(*pcmp))
      return true;

    snd_pcm_close(*pcmp);
    *pcmp = NULL;
  }

  int err = snd_pcm_open_lconf(pcmp, name.c_str(), SND_PCM_STREAM_PLAYBACK, ALSA_OPTIONS, lconf);
  if (err < 0)
  {
    CLog::Log(LOGINFO, "CAESinkALSA - Unable to open device \"%s\" for playback", name.c_str());
  }

  return err == 0;
}

bool CAESinkALSA::TryDeviceWithParams(const std::string &name, const std::string &params, snd_pcm_t **pcmp, snd_config_t *lconf)
{
  if (!params.empty())
  {
    std::string nameWithParams = name;
    AppendParams(nameWithParams, params);
    if (TryDevice(nameWithParams, pcmp, lconf))
      return true;
  }

  /* Try the variant without extra parameters.
   * Custom devices often do not take the AESx parameters, for example.
   */
  return TryDevice(name, pcmp, lconf);
}

bool CAESinkALSA::OpenPCMDevice(const std::string &name, const std::string &params, int channels, snd_pcm_t **pcmp, snd_config_t *lconf)
{
 /* Special name denoting surroundXX mangling. This is needed for some
   * devices for multichannel to work. */
  if (name == "@" || name.substr(0, 2) == "@:")
  {
    std::string openName = name.substr(1);

    /* These device names allow alsa-lib to perform special routing if needed
     * for multichannel to work with the audio hardware.
     * Fall through in switch() so that devices with more channels are
     * added as fallback. */
    switch (channels)
    {
      case 3:
      case 4:
        if (TryDeviceWithParams("surround40" + openName, params, pcmp, lconf))
          return true;
      case 5:
      case 6:
        if (TryDeviceWithParams("surround51" + openName, params, pcmp, lconf))
          return true;
      case 7:
      case 8:
        if (TryDeviceWithParams("surround71" + openName, params, pcmp, lconf))
          return true;
    }

    /* Try "sysdefault" and "default" (they provide dmix if needed, and route
     * audio to all extra channels on subdeviced cards),
     * unless the selected devices is not DEV=0 of the card, in which case
     * "sysdefault" and "default" would point to another device.
     * "sysdefault" is a newish device name that won't be overwritten in case
     * system configuration redefines "default". "default" is still tried
     * because "sysdefault" is rather new. */
    size_t devPos = openName.find(",DEV=");
    if (devPos == std::string::npos || (devPos + 5 < openName.size() && openName[devPos+5] == '0'))
    {
      /* "sysdefault" and "default" do not have "DEV=0", drop it */
      std::string nameWithoutDev = openName;
      if (devPos != std::string::npos)
        nameWithoutDev.erase(nameWithoutDev.begin() + devPos, nameWithoutDev.begin() + devPos + 6);

      if (TryDeviceWithParams("sysdefault" + nameWithoutDev, params, pcmp, lconf)
          || TryDeviceWithParams("default" + nameWithoutDev, params, pcmp, lconf))
        return true;
    }

    /* Try "front" (no dmix, no audio in other channels on subdeviced cards) */
    if (TryDeviceWithParams("front" + openName, params, pcmp, lconf))
      return true;

  }
  else
  {
    /* Non-surroundXX device, just add it */
    if (TryDeviceWithParams(name, params, pcmp, lconf))
      return true;
  }

  return false;
}

void CAESinkALSA::EnumerateDevicesEx(AEDeviceInfoList &list, bool force)
{
  /* ensure that ALSA has been initialized */
  snd_lib_error_set_handler(sndLibErrorHandler);
  if(!snd_config || force)
  {
    if(force)
      snd_config_update_free_global();

    snd_config_update();
  }

  snd_config_t *config;
  snd_config_copy(&config, snd_config);

  /* Always enumerate the default device.
   * Note: If "default" is a stereo device, EnumerateDevice()
   * will automatically add "@" instead to enable surroundXX mangling.
   * We don't want to do that if "default" can handle multichannel
   * itself (e.g. in case of a pulseaudio server). */
  EnumerateDevice(list, "default", "", config);

  void **hints;

  if (snd_device_name_hint(-1, "pcm", &hints) < 0)
  {
    CLog::Log(LOGINFO, "CAESinkALSA - Unable to get a list of devices");
    return;
  }

  std::string defaultDescription;

  for (void** hint = hints; *hint != NULL; ++hint)
  {
    char *io = snd_device_name_get_hint(*hint, "IOID");
    char *name = snd_device_name_get_hint(*hint, "NAME");
    char *desc = snd_device_name_get_hint(*hint, "DESC");
    if ((!io || strcmp(io, "Output") == 0) && name
        && strcmp(name, "null") != 0)
    {
      std::string baseName = std::string(name);
      baseName = baseName.substr(0, baseName.find(':'));

      if (strcmp(name, "default") == 0)
      {
        /* added already, but lets get the description if we have one */
        if (desc)
          defaultDescription = desc;
      }
      else if (baseName == "front")
      {
        /* Enumerate using the surroundXX mangling */
        /* do not enumerate basic "front", it is already handled
         * by the default "@" entry added in the very beginning */
        if (strcmp(name, "front") != 0)
          EnumerateDevice(list, std::string("@") + (name+5), desc ? desc : name, config);
      }

      /* Do not enumerate "default", it is already enumerated above. */

      /* Do not enumerate the sysdefault or surroundXX devices, those are
       * always accompanied with a "front" device and it is handled above
       * as "@". The below devices will be automatically used if available
       * for a "@" device. */

      /* Ubuntu has patched their alsa-lib so that "defaults.namehint.extended"
       * defaults to "on" instead of upstream "off", causing lots of unwanted
       * extra devices (many of which are not actually routed properly) to be
       * found by the enumeration process. Skip them as well ("hw", "dmix",
       * "plughw", "dsnoop"). */

      else if (baseName != "default"
            && baseName != "sysdefault"
            && baseName != "surround40"
            && baseName != "surround41"
            && baseName != "surround50"
            && baseName != "surround51"
            && baseName != "surround71"
            && baseName != "hw"
            && baseName != "dmix"
            && baseName != "plughw"
            && baseName != "dsnoop")
      {
        EnumerateDevice(list, name, desc ? desc : name, config);
      }
    }
    free(io);
    free(name);
    free(desc);
  }
  snd_device_name_free_hint(hints);

  /* set the displayname for default device */
  if (!list.empty() && list[0].m_deviceName == "default")
  {
    /* If we have one from a hint (DESC), use it */
    if (!defaultDescription.empty())
      list[0].m_displayName = defaultDescription;
    /* Otherwise use the discovered name or (unlikely) "Default" */
    else if (list[0].m_displayName.empty())
      list[0].m_displayName = "Default";
  }

  /* lets check uniqueness, we may need to append DEV or CARD to DisplayName */
  /* If even a single device of card/dev X clashes with Y, add suffixes to
   * all devices of both them, for clarity. */

  /* clashing card names, e.g. "NVidia", "NVidia_2" */
  std::set<std::string> cardsToAppend;

  /* clashing basename + cardname combinations, e.g. ("hdmi","Nvidia") */
  std::set<std::pair<std::string, std::string> > devsToAppend;

  for (AEDeviceInfoList::iterator it1 = list.begin(); it1 != list.end(); ++it1)
  {
    for (AEDeviceInfoList::iterator it2 = it1+1; it2 != list.end(); ++it2)
    {
      if (it1->m_displayName == it2->m_displayName
       && it1->m_displayNameExtra == it2->m_displayNameExtra)
      {
        /* something needs to be done */
        std::string cardString1 = GetParamFromName(it1->m_deviceName, "CARD");
        std::string cardString2 = GetParamFromName(it2->m_deviceName, "CARD");

        if (cardString1 != cardString2)
        {
          /* card name differs, add identifiers to all devices */
          cardsToAppend.insert(cardString1);
          cardsToAppend.insert(cardString2);
          continue;
        }

        std::string devString1 = GetParamFromName(it1->m_deviceName, "DEV");
        std::string devString2 = GetParamFromName(it2->m_deviceName, "DEV");

        if (devString1 != devString2)
        {
          /* device number differs, add identifiers to all such devices */
          devsToAppend.insert(std::make_pair(it1->m_deviceName.substr(0, it1->m_deviceName.find(':')), cardString1));
          devsToAppend.insert(std::make_pair(it2->m_deviceName.substr(0, it2->m_deviceName.find(':')), cardString2));
          continue;
        }

        /* if we got here, the configuration is really weird, just append the whole device string */
        it1->m_displayName += " (" + it1->m_deviceName + ")";
        it2->m_displayName += " (" + it2->m_deviceName + ")";
      }
    }
  }

  for (std::set<std::string>::iterator it = cardsToAppend.begin();
       it != cardsToAppend.end(); ++it)
  {
    for (AEDeviceInfoList::iterator itl = list.begin(); itl != list.end(); ++itl)
    {
      std::string cardString = GetParamFromName(itl->m_deviceName, "CARD");
      if (cardString == *it)
        /* "HDA NVidia (NVidia)", "HDA NVidia (NVidia_2)", ... */
        itl->m_displayName += " (" + cardString + ")";
    }
  }

  for (std::set<std::pair<std::string, std::string> >::iterator it = devsToAppend.begin();
       it != devsToAppend.end(); ++it)
  {
    for (AEDeviceInfoList::iterator itl = list.begin(); itl != list.end(); ++itl)
    {
      std::string baseName = itl->m_deviceName.substr(0, itl->m_deviceName.find(':'));
      std::string cardString = GetParamFromName(itl->m_deviceName, "CARD");
      if (baseName == it->first && cardString == it->second)
      {
        std::string devString = GetParamFromName(itl->m_deviceName, "DEV");
        /* "HDMI #0", "HDMI #1" ... */
        itl->m_displayNameExtra += " #" + devString;
      }
    }
  }
}

AEDeviceType CAESinkALSA::AEDeviceTypeFromName(const std::string &name)
{
  if (name.substr(0, 4) == "hdmi")
    return AE_DEVTYPE_HDMI;
  else if (name.substr(0, 6) == "iec958" || name.substr(0, 5) == "spdif")
    return AE_DEVTYPE_IEC958;

  return AE_DEVTYPE_PCM;
}

std::string CAESinkALSA::GetParamFromName(const std::string &name, const std::string &param)
{
  /* name = "hdmi:CARD=x,DEV=y" param = "CARD" => return "x" */
  size_t parPos = name.find(param + '=');
  if (parPos != std::string::npos)
  {
    parPos += param.size() + 1;
    return name.substr(parPos, name.find_first_of(",'\"", parPos)-parPos);
  }

  return "";
}

void CAESinkALSA::EnumerateDevice(AEDeviceInfoList &list, const std::string &device, const std::string &description, snd_config_t *config)
{
  snd_pcm_t *pcmhandle = NULL;
  if (!OpenPCMDevice(device, "", ALSA_MAX_CHANNELS, &pcmhandle, config))
    return;

  snd_pcm_info_t *pcminfo;
  snd_pcm_info_alloca(&pcminfo);
  memset(pcminfo, 0, snd_pcm_info_sizeof());

  int err = snd_pcm_info(pcmhandle, pcminfo);
  if (err < 0)
  {
    CLog::Log(LOGINFO, "CAESinkALSA - Unable to get pcm_info for \"%s\"", device.c_str());
    snd_pcm_close(pcmhandle);
  }

  int cardNr = snd_pcm_info_get_card(pcminfo);

  CAEDeviceInfo info;
  info.m_deviceName = device;
  info.m_deviceType = AEDeviceTypeFromName(device);

  if (cardNr >= 0)
  {
    /* "HDA NVidia", "HDA Intel", "HDA ATI HDMI", "SB Live! 24-bit External", ... */
    char *cardName;
    if (snd_card_get_name(cardNr, &cardName) == 0)
      info.m_displayName = cardName;

    if (info.m_deviceType == AE_DEVTYPE_HDMI && info.m_displayName.size() > 5 &&
        info.m_displayName.substr(info.m_displayName.size()-5) == " HDMI")
    {
      /* We already know this is HDMI, strip it */
      info.m_displayName.erase(info.m_displayName.size()-5);
    }

    /* "CONEXANT Analog", "USB Audio", "HDMI 0", "ALC889 Digital" ... */
    std::string pcminfoName = snd_pcm_info_get_name(pcminfo);

    /*
     * Filter "USB Audio", in those cases snd_card_get_name() is more
     * meaningful already
     */
    if (pcminfoName != "USB Audio")
      info.m_displayNameExtra = pcminfoName;

    if (info.m_deviceType == AE_DEVTYPE_HDMI)
    {
      /* replace, this was likely "HDMI 0" */
      info.m_displayNameExtra = "HDMI";

      int dev = snd_pcm_info_get_device(pcminfo);

      if (dev >= 0)
      {
        /* lets see if we can get ELD info */

        snd_ctl_t *ctlhandle;
        std::stringstream sstr;
        sstr << "hw:" << cardNr;
        std::string strHwName = sstr.str();

        if (snd_ctl_open_lconf(&ctlhandle, strHwName.c_str(), 0, config) == 0)
        {
          snd_hctl_t *hctl;
          if (snd_hctl_open_ctl(&hctl, ctlhandle) == 0)
          {
            snd_hctl_load(hctl);
            bool badHDMI = false;
            if (!GetELD(hctl, dev, info, badHDMI))
              CLog::Log(LOGDEBUG, "CAESinkALSA - Unable to obtain ELD information for device \"%s\" (not supported by device, or kernel older than 3.2)",
                        device.c_str());

            /* snd_hctl_close also closes ctlhandle */
            snd_hctl_close(hctl);

            if (badHDMI)
            {
              /* 
               * Warn about disconnected devices, but keep them enabled 
               * Detection can go wrong on Intel, Nvidia and on all 
               * AMD (fglrx) hardware, so it is not safe to close those
               * handles
               */
              CLog::Log(LOGDEBUG, "CAESinkALSA - HDMI device \"%s\" may be unconnected (no ELD data)", device.c_str());
            }
          }
          else
          {
            snd_ctl_close(ctlhandle);
          }
        }
      }
    }
    else if (info.m_deviceType == AE_DEVTYPE_IEC958)
    {
      /* append instead of replace, pcminfoName is useful for S/PDIF */
      if (!info.m_displayNameExtra.empty())
        info.m_displayNameExtra += ' ';
      info.m_displayNameExtra += "S/PDIF";
    }
    else if (info.m_displayNameExtra.empty())
    {
      /* for USB audio, it gets a bit confusing as there is
       * - "SB Live! 24-bit External"
       * - "SB Live! 24-bit External, S/PDIF"
       * so add "Analog" qualifier to the first one */
      info.m_displayNameExtra = "Analog";
    }

    /* "default" is a device that will be used for all inputs, while
     * "@" will be mangled to front/default/surroundXX as necessary */
    if (device == "@" || device == "default")
    {
      /* Make it "Default (whatever)" */
      info.m_displayName = "Default (" + info.m_displayName + (info.m_displayNameExtra.empty() ? "" : " " + info.m_displayNameExtra + ")");
      info.m_displayNameExtra = "";
    }

  }
  else
  {
    /* virtual devices: "default", "pulse", ... */
    /* description can be e.g. "PulseAudio Sound Server" - for hw devices it is
     * normally uninteresting, like "HDMI Audio Output" or "Default Audio Device",
     * so we only use it for virtual devices that have no better display name */
    info.m_displayName = description;
  }

  snd_pcm_hw_params_t *hwparams;
  snd_pcm_hw_params_alloca(&hwparams);
  memset(hwparams, 0, snd_pcm_hw_params_sizeof());

  /* ensure we can get a playback configuration for the device */
  if (snd_pcm_hw_params_any(pcmhandle, hwparams) < 0)
  {
    CLog::Log(LOGINFO, "CAESinkALSA - No playback configurations available for device \"%s\"", device.c_str());
    snd_pcm_close(pcmhandle);
    return;
  }

  /* detect the available sample rates */
  for (unsigned int *rate = ALSASampleRateList; *rate != 0; ++rate)
    if (snd_pcm_hw_params_test_rate(pcmhandle, hwparams, *rate, 0) >= 0)
      info.m_sampleRates.push_back(*rate);

  /* detect the channels available */
  int channels = 0;
  for (int i = ALSA_MAX_CHANNELS; i >= 1; --i)
  {
    /* Reopen the device if needed on the special "surroundXX" cases */
    if (info.m_deviceType == AE_DEVTYPE_PCM && (i == 8 || i == 6 || i == 4))
      OpenPCMDevice(device, "", i, &pcmhandle, config);

    if (snd_pcm_hw_params_test_channels(pcmhandle, hwparams, i) >= 0)
    {
      channels = i;
      break;
    }
  }

  if (device == "default" && channels == 2)
  {
    /* This looks like the ALSA standard default stereo dmix device, we
     * probably want to use "@" instead to get surroundXX. */
    snd_pcm_close(pcmhandle);
    EnumerateDevice(list, "@", description, config);
    return;
  }

  CAEChannelInfo alsaChannels;
  for (int i = 0; i < channels; ++i)
  {
    if (!info.m_channels.HasChannel(ALSAChannelMap[i]))
      info.m_channels += ALSAChannelMap[i];
    alsaChannels += ALSAChannelMap[i];
  }

  /* remove the channels from m_channels that we cant use */
  info.m_channels.ResolveChannels(alsaChannels);

  /* detect the PCM sample formats that are available */
  for (enum AEDataFormat i = AE_FMT_MAX; i > AE_FMT_INVALID; i = (enum AEDataFormat)((int)i - 1))
  {
    if (AE_IS_RAW(i) || i == AE_FMT_MAX)
      continue;
    snd_pcm_format_t fmt = AEFormatToALSAFormat(i);
    if (fmt == SND_PCM_FORMAT_UNKNOWN)
      continue;

    if (snd_pcm_hw_params_test_format(pcmhandle, hwparams, fmt) >= 0)
      info.m_dataFormats.push_back(i);
  }

  snd_pcm_close(pcmhandle);
  list.push_back(info);
}

bool CAESinkALSA::GetELD(snd_hctl_t *hctl, int device, CAEDeviceInfo& info, bool& badHDMI)
{
  badHDMI = false;

  snd_ctl_elem_id_t    *id;
  snd_ctl_elem_info_t  *einfo;
  snd_ctl_elem_value_t *control;
  snd_hctl_elem_t      *elem;

  snd_ctl_elem_id_alloca(&id);
  memset(id, 0, snd_ctl_elem_id_sizeof());

  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_PCM);
  snd_ctl_elem_id_set_name     (id, "ELD" );
  snd_ctl_elem_id_set_device   (id, device);
  elem = snd_hctl_find_elem(hctl, id);
  if (!elem)
    return false;

  snd_ctl_elem_info_alloca(&einfo);
  memset(einfo, 0, snd_ctl_elem_info_sizeof());

  if (snd_hctl_elem_info(elem, einfo) < 0)
    return false;

  if (!snd_ctl_elem_info_is_readable(einfo))
    return false;

  if (snd_ctl_elem_info_get_type(einfo) != SND_CTL_ELEM_TYPE_BYTES)
    return false;

  snd_ctl_elem_value_alloca(&control);
  memset(control, 0, snd_ctl_elem_value_sizeof());

  if (snd_hctl_elem_read(elem, control) < 0)
    return false;

  int dataLength = snd_ctl_elem_info_get_count(einfo);
  /* if there is no ELD data, then its a bad HDMI device, either nothing attached OR an invalid nVidia HDMI device
   * OR the driver doesn't properly support ELD (notably ATI/AMD, 2012-05) */
  if (!dataLength)
    badHDMI = true;
  else
    CAEELDParser::Parse(
      (const uint8_t*)snd_ctl_elem_value_get_bytes(control),
      dataLength,
      info
    );

  info.m_deviceType = AE_DEVTYPE_HDMI;
  return true;
}

bool CAESinkALSA::SoftSuspend()
{
  if(m_pcm) // it is still there
   Deinitialize();

  return true;
}
bool CAESinkALSA::SoftResume()
{
    // reinit all the clibber
    if(!m_pcm)
    {
      if (!snd_config)
        snd_config_update();

    // Initialize what we had before again, SoftAE might keep it
    // but ignore ret value to give the chance to do reopening
    Initialize(m_initFormat, m_initDevice);
    }
   // make sure that OpenInternalSink is done again
   return false;
}

void CAESinkALSA::sndLibErrorHandler(const char *file, int line, const char *function, int err, const char *fmt, ...)
{
  va_list arg;
  va_start(arg, fmt);
  char *errorStr;
  if (vasprintf(&errorStr, fmt, arg) >= 0)
  {
    CLog::Log(LOGINFO, "CAESinkALSA - ALSA: %s:%d:(%s) %s%s%s",
              file, line, function, errorStr, err ? ": " : "", err ? snd_strerror(err) : "");
    free(errorStr);
  }
  va_end(arg);
}

#endif
