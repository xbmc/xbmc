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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "system.h"
#ifdef HAS_ALSA

#include <stdint.h>
#include <limits.h>
#include <sstream>

#include "AESinkALSA.h"
#include "Utils/AEUtil.h"
#include "Utils/AEELDParser.h"
#include "utils/StdString.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "threads/SingleLock.h"
#include "settings/GUISettings.h"

#define ALSA_OPTIONS (SND_PCM_NONBLOCK | SND_PCM_NO_AUTO_FORMAT | SND_PCM_NO_AUTO_RESAMPLE)
#define ALSA_PERIODS 16

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
  m_pcm(NULL)
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

void CAESinkALSA::GetPassthroughDevice(AEAudioFormat format, std::string& device)
{
  device += ",AES0=0x06,AES1=0x82,AES2=0x00";
       if (format.m_sampleRate == 192000) device += ",AES3=0x0e";
  else if (format.m_sampleRate == 176400) device += ",AES3=0x0c";
  else if (format.m_sampleRate ==  96000) device += ",AES3=0x0a";
  else if (format.m_sampleRate ==  88200) device += ",AES3=0x08";
  else if (format.m_sampleRate ==  48000) device += ",AES3=0x02";
  else if (format.m_sampleRate ==  44100) device += ",AES3=0x00";
  else if (format.m_sampleRate ==  32000) device += ",AES3=0x03";
  else device += ",AES3=0x01";
}

bool CAESinkALSA::Initialize(AEAudioFormat &format, std::string &device)
{
  m_initDevice = device;
  m_initFormat = format;

  /* if we are raw, correct the data format */
  if (AE_IS_RAW(format.m_dataFormat))
  {
    m_channelLayout     = GetChannelLayout(format);
    format.m_dataFormat = AE_FMT_S16NE;
    m_passthrough       = true;
  }
  else
  {
    m_channelLayout = GetChannelLayout(format);
    m_passthrough   = false;
  }

  if (m_channelLayout.Count() == 0)
  {
    CLog::Log(LOGERROR, "CAESinkALSA::Initialize - Unable to open the requested channel layout");
    return false;
  }

  format.m_channelLayout = m_channelLayout;

  /* if passthrough we need the additional AES flags */
  if (m_passthrough)
    GetPassthroughDevice(format, device);

  m_device = device;
  CLog::Log(LOGINFO, "CAESinkALSA::Initialize - Attempting to open device %s", device.c_str());

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

  if (!InitializeHW(format) || !InitializeSW(format))
    return false;

  snd_pcm_nonblock(m_pcm, 1);
  snd_pcm_prepare (m_pcm);

  m_format              = format;
  m_formatSampleRateMul = 1.0 / (double)m_format.m_sampleRate;

  return true;
}

bool CAESinkALSA::IsCompatible(const AEAudioFormat format, const std::string device)
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
    return SND_PCM_FORMAT_S16_LE;

  switch (format)
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
    CLog::Log(LOGERROR, "CAESinkALSA::InitializeHW - Unable to open the required number of channels");
    return false;
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

  unsigned int periods;

  snd_pcm_uframes_t periodSize, bufferSize;
  snd_pcm_hw_params_get_buffer_size_max(hw_params, &bufferSize);

  bufferSize  = std::min(bufferSize, (snd_pcm_uframes_t)8192);
  periodSize  = bufferSize / ALSA_PERIODS;
  periods     = ALSA_PERIODS;

  CLog::Log(LOGDEBUG, "CAESinkALSA::InitializeHW - Request: periodSize %lu, periods %u, bufferSize %lu", periodSize, periods, bufferSize);

  /* work on a copy of the hw params */
  snd_pcm_hw_params_t *hw_params_copy;
  snd_pcm_hw_params_alloca(&hw_params_copy);

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
          return false;
	}
      }
    }
  }

  snd_pcm_hw_params_get_period_size(hw_params_copy, &periodSize, NULL);
  snd_pcm_hw_params_get_buffer_size(hw_params_copy, &bufferSize);
  

  CLog::Log(LOGDEBUG, "CAESinkALSA::InitializeHW - Got: periodSize %lu, periods %u, bufferSize %lu", periodSize, periods, bufferSize);

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
  Stop();

  if (m_pcm)
  {
    snd_pcm_drop (m_pcm);
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

unsigned int CAESinkALSA::AddPackets(uint8_t *data, unsigned int frames)
{
  if (!m_pcm)
    return 0;

  if (snd_pcm_state(m_pcm) == SND_PCM_STATE_PREPARED)
    snd_pcm_start(m_pcm);

  int ret;

  ret = snd_pcm_avail(m_pcm);
  if (ret < 0) 
  {
    HandleError("snd_pcm_avail", ret);
    ret = 0;
  }

  if ((unsigned int)ret < frames);
  {
    ret = snd_pcm_wait(m_pcm, m_timeout);
    if (ret < 0)
      HandleError("snd_pcm_wait", ret);
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
  snd_pcm_nonblock(m_pcm, 1);
}

void CAESinkALSA::EnumerateDevicesEx(AEDeviceInfoList &list)
{
  /* ensure that ALSA has been initialized */
  if(!snd_config)
    snd_config_update();

  snd_ctl_t *ctlhandle;
  snd_pcm_t *pcmhandle;

  snd_ctl_card_info_t *ctlinfo;
  snd_ctl_card_info_alloca(&ctlinfo);
  memset(ctlinfo, 0, snd_ctl_card_info_sizeof());

  snd_pcm_hw_params_t *hwparams;
  snd_pcm_hw_params_alloca(&hwparams);
  memset(hwparams, 0, snd_pcm_hw_params_sizeof());

  snd_pcm_info_t *pcminfo;
  snd_pcm_info_alloca(&pcminfo);
  memset(pcminfo, 0, snd_pcm_info_sizeof());

  /* get the sound config */
  snd_config_t *config;
  snd_config_copy(&config, snd_config);

  std::string strHwName;
  int n_cards = -1;
  while (snd_card_next(&n_cards) == 0 && n_cards != -1)
  {
    std::stringstream sstr;
    sstr << "hw:" << n_cards;
    std::string strHwName = sstr.str();

    if (snd_ctl_open_lconf(&ctlhandle, strHwName.c_str(), 0, config) != 0)
    {
      CLog::Log(LOGDEBUG, "CAESinkALSA::EnumerateDevicesEx - Unable to open control for device %s", strHwName.c_str());
      continue;
    }

    if (snd_ctl_card_info(ctlhandle, ctlinfo) != 0)
    {
      CLog::Log(LOGDEBUG, "CAESinkALSA::EnumerateDevicesEx - Unable to get card control info for device %s", strHwName.c_str());
      snd_ctl_close(ctlhandle);
      continue;
    }

    snd_hctl_t *hctl;
    if (snd_hctl_open_ctl(&hctl, ctlhandle) != 0)
      hctl = NULL;
    snd_hctl_load(hctl);

    int pcm_index    = 0;
    int iec958_index = 0;
    int hdmi_index   = 0;

    int dev = -1;
    while (snd_ctl_pcm_next_device(ctlhandle, &dev) == 0 && dev != -1)
    {
      snd_pcm_info_set_device   (pcminfo, dev);
      snd_pcm_info_set_subdevice(pcminfo, 0  );
      snd_pcm_info_set_stream   (pcminfo, SND_PCM_STREAM_PLAYBACK);

      if (snd_ctl_pcm_info(ctlhandle, pcminfo) < 0)
      {
        CLog::Log(LOGDEBUG, "CAESinkALSA::EnumerateDevicesEx - Skipping device %s,%d as it does not have PCM playback ability", strHwName.c_str(), dev);
        continue;
      }

      int dev_index;
      sstr.str(std::string());
      CAEDeviceInfo info;
      std::string devname = snd_pcm_info_get_name(pcminfo);

      bool maybeHDMI = false;

      /* detect HDMI */
      if (devname.find("HDMI") != std::string::npos)
      { 
        info.m_deviceType = AE_DEVTYPE_HDMI;
        dev_index = hdmi_index++;
        sstr << "hdmi";
      }
      else
      {
        /* detect IEC958 */

        /* some HDMI devices (intel) report Digital for HDMI also */
        if (devname.find("Digital") != std::string::npos)
          maybeHDMI = true;

        if (maybeHDMI || devname.find("IEC958" ) != std::string::npos)
        {
          info.m_deviceType = AE_DEVTYPE_IEC958;
          dev_index = iec958_index; /* dont increment, it might be HDMI */
          sstr << "iec958";
        }
        else
        {
          info.m_deviceType = AE_DEVTYPE_PCM;
          dev_index = pcm_index++;
          sstr << "hw";
        }
      }

      /* build the driver string to pass to ALSA */
      sstr << ":CARD=" << snd_ctl_card_info_get_id(ctlinfo) << ",DEV=" << dev_index;
      info.m_deviceName = sstr.str();

      /* get the friendly display name*/
      info.m_displayName      = snd_ctl_card_info_get_name(ctlinfo);
      info.m_displayNameExtra = devname;

      /* open the device for testing */
      int err = snd_pcm_open_lconf(&pcmhandle, info.m_deviceName.c_str(), SND_PCM_STREAM_PLAYBACK, 0, config);

      /* if open of possible IEC958 failed and it could be HDMI, try as HDMI */
      if (err < 0 && maybeHDMI)
      {
        /* check for HDMI if it failed */
        sstr.str(std::string());
        dev_index = hdmi_index;

        sstr << "hdmi";
        sstr << ":CARD=" << snd_ctl_card_info_get_id(ctlinfo) << ",DEV=" << dev_index;
        info.m_deviceName = sstr.str();
        err = snd_pcm_open_lconf(&pcmhandle, info.m_deviceName.c_str(), SND_PCM_STREAM_PLAYBACK, 0, config);

        /* if it was valid, increment the index and set the type */
        if (err >= 0)
        {
          ++hdmi_index;
          info.m_deviceType = AE_DEVTYPE_HDMI;
        }
      }

      /* if it's still IEC958, increment the index */
      if (info.m_deviceType == AE_DEVTYPE_IEC958)
        ++iec958_index;

      /* final error check */
      if (err < 0)
      {
        CLog::Log(LOGINFO, "CAESinkALSA::EnumerateDevicesEx - Unable to open %s for capability detection", strHwName.c_str());
        continue;
      }

      /* see if we can get ELD for this device */
      if (info.m_deviceType == AE_DEVTYPE_HDMI)
      {
        bool badHDMI = false;
        if (hctl && !GetELD(hctl, dev, info, badHDMI))
          CLog::Log(LOGDEBUG, "CAESinkALSA::EnumerateDevicesEx - Unable to obtain ELD information for device %s, make sure you have ALSA >= 1.0.25", info.m_deviceName.c_str());

        if (badHDMI)
        {
          CLog::Log(LOGDEBUG, "CAESinkALSA::EnumerateDevicesEx - Skipping HDMI device %s as it has no ELD data", info.m_deviceName.c_str());
          snd_pcm_close(pcmhandle);
          continue;
        }
      }

      /* ensure we can get a playback configuration for the device */
      if (snd_pcm_hw_params_any(pcmhandle, hwparams) < 0)
      {
        CLog::Log(LOGINFO, "CAESinkALSA::EnumerateDevicesEx - No playback configurations available for device %s", info.m_deviceName.c_str());
        snd_pcm_close(pcmhandle);
        continue;
      }

      /* detect the available sample rates */
      for (unsigned int *rate = ALSASampleRateList; *rate != 0; ++rate)
        if (snd_pcm_hw_params_test_rate(pcmhandle, hwparams, *rate, 0) >= 0)
          info.m_sampleRates.push_back(*rate);

      /* detect the channels available */
      int channels = 0;
      for (int i = 1; i <= ALSA_MAX_CHANNELS; ++i)
        if (snd_pcm_hw_params_test_channels(pcmhandle, hwparams, i) >= 0)
          channels = i;

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

    /* snd_hctl_close also closes ctlhandle */
    if (hctl)
      snd_hctl_close(hctl);
    else
      snd_ctl_close(ctlhandle);
  }
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
  /* if there is no ELD data, then its a bad HDMI device, either nothing attached OR an invalid nVidia HDMI device */
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
#endif
