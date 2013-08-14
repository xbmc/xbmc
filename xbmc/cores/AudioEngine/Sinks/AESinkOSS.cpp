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

#include "AESinkOSS.h"
#include <stdint.h>
#include <limits.h>

#include "Utils/AEUtil.h"
#include "utils/StdString.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include <sstream>

#include <sys/ioctl.h>
#include <sys/fcntl.h>

#if defined(OSS4) || defined(TARGET_FREEBSD)
  #include <sys/soundcard.h>
#else
  #include <linux/soundcard.h>
#endif

#define OSS_FRAMES 256

static enum AEChannel OSSChannelMap[9] =
  {AE_CH_FL, AE_CH_FR, AE_CH_BL, AE_CH_BR, AE_CH_FC, AE_CH_LFE, AE_CH_SL, AE_CH_SR, AE_CH_NULL};

#if defined(SNDCTL_SYSINFO) && defined(SNDCTL_CARDINFO)
static int OSSSampleRateList[] =
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
#endif

CAESinkOSS::CAESinkOSS()
{
  m_fd = 0;
  m_blockingNeedsUpdate = true;
}

CAESinkOSS::~CAESinkOSS()
{
  Deinitialize();
}

std::string CAESinkOSS::GetDeviceUse(const AEAudioFormat format, const std::string &device)
{
#ifdef OSS4
  if (AE_IS_RAW(format.m_dataFormat))
  {
    if (device.find_first_of('/') != 0)
      return "/dev/dsp_ac3";
    return device;
  }

  if (device.find_first_of('/') != 0)
    return "/dev/dsp_multich";
#else
  if (device.find_first_of('/') != 0)
    return "/dev/dsp";
#endif

  return device;
}

bool CAESinkOSS::Initialize(AEAudioFormat &format, std::string &device)
{
  m_initFormat = format;
  format.m_channelLayout = GetChannelLayout(format);
  device = GetDeviceUse(format, device);

#ifdef __linux__
  /* try to open in exclusive mode first (no software mixing) */
  m_fd = open(device.c_str(), O_WRONLY | O_EXCL, 0);
  if (m_fd == -1)
#endif
    m_fd = open(device.c_str(), O_WRONLY, 0);
  if (m_fd == -1)
  {
    CLog::Log(LOGERROR, "CAESinkOSS::Initialize - Failed to open the audio device: %s", device.c_str());
    return false;
  }

  int format_mask;
  if (ioctl(m_fd, SNDCTL_DSP_GETFMTS, &format_mask) == -1)
  {
    close(m_fd);
    CLog::Log(LOGERROR, "CAESinkOSS::Initialize - Failed to get supported formats, assuming AFMT_S16_NE");
    return false;
  }

#ifdef OSS4
  bool useCooked = true;
#endif

  int oss_fmt = 0;
#ifdef AFMT_FLOAT
  if ((format.m_dataFormat == AE_FMT_FLOAT) && (format_mask & AFMT_FLOAT ))
    oss_fmt = AFMT_FLOAT;
  else
#endif
#ifdef AFMT_S32_NE
  if ((format.m_dataFormat == AE_FMT_S32NE) && (format_mask & AFMT_S32_NE))
    oss_fmt = AFMT_S32_NE;
  else if ((format.m_dataFormat == AE_FMT_S32BE) && (format_mask & AFMT_S32_BE))
    oss_fmt = AFMT_S32_BE;
  else if ((format.m_dataFormat == AE_FMT_S32LE) && (format_mask & AFMT_S32_LE))
    oss_fmt = AFMT_S32_LE;
  else
#endif
  if ((format.m_dataFormat == AE_FMT_S16NE) && (format_mask & AFMT_S16_NE))
    oss_fmt = AFMT_S16_NE;
  else if ((format.m_dataFormat == AE_FMT_S16BE) && (format_mask & AFMT_S16_BE))
    oss_fmt = AFMT_S16_BE;
  else if ((format.m_dataFormat == AE_FMT_S16LE) && (format_mask & AFMT_S16_LE))
    oss_fmt = AFMT_S16_LE;
  else if ((format.m_dataFormat == AE_FMT_S8   ) && (format_mask & AFMT_S8    ))
    oss_fmt = AFMT_S8;
  else if ((format.m_dataFormat == AE_FMT_U8   ) && (format_mask & AFMT_U8    ))
    oss_fmt = AFMT_U8;
  else if ((AE_IS_RAW(format.m_dataFormat)     ) && (format_mask & AFMT_AC3   ))
    oss_fmt = AFMT_AC3;
  else if (AE_IS_RAW(format.m_dataFormat))
  {
    close(m_fd);
    CLog::Log(LOGERROR, "CAESinkOSS::Initialize - Failed to find a suitable RAW output format");
    return false;
  }
  else
  {
    CLog::Log(LOGINFO, "CAESinkOSS::Initialize - Your hardware does not support %s, trying other formats", CAEUtil::DataFormatToStr(format.m_dataFormat));

    /* fallback to the best supported format */
#ifdef AFMT_FLOAT
    if (format_mask & AFMT_FLOAT )
    {
      oss_fmt = AFMT_FLOAT;
      format.m_dataFormat = AE_FMT_FLOAT;
    }
    else
#endif
#ifdef AFMT_S32_NE
    if (format_mask & AFMT_S32_NE)
    {
      oss_fmt = AFMT_S32_NE;
      format.m_dataFormat = AE_FMT_S32NE;
    }
    else if (format_mask & AFMT_S32_BE)
    {
      oss_fmt = AFMT_S32_BE;
      format.m_dataFormat = AE_FMT_S32BE;
    }
    else if (format_mask & AFMT_S32_LE)
    {
      oss_fmt = AFMT_S32_LE;
      format.m_dataFormat = AE_FMT_S32LE;
    }
    else
#endif
    if (format_mask & AFMT_S16_NE)
    {
      oss_fmt = AFMT_S16_NE;
      format.m_dataFormat = AE_FMT_S16NE;
    }
    else if (format_mask & AFMT_S16_BE)
    {
      oss_fmt = AFMT_S16_BE;
      format.m_dataFormat = AE_FMT_S16BE;
    }
    else if (format_mask & AFMT_S16_LE)
    {
      oss_fmt = AFMT_S16_LE;
      format.m_dataFormat = AE_FMT_S16LE;
    }
    else if (format_mask & AFMT_S8    )
    {
      oss_fmt = AFMT_S8;
      format.m_dataFormat = AE_FMT_S8;
    }
    else if (format_mask & AFMT_U8    )
    {
      oss_fmt = AFMT_U8;
      format.m_dataFormat = AE_FMT_U8;
    }
    else
    {
      CLog::Log(LOGERROR, "CAESinkOSS::Initialize - Failed to find a suitable native output format, will try to use AE_FMT_S16NE anyway");
      oss_fmt             = AFMT_S16_NE;
      format.m_dataFormat = AE_FMT_S16NE;
#ifdef OSS4
      /* dont use cooked if we did not find a native format, OSS might be able to convert */
      useCooked           = false;
#endif
    }
  }

#ifdef OSS4
  if (useCooked)
  {
    int oss_cooked = 1;
    if (ioctl(m_fd, SNDCTL_DSP_COOKEDMODE, &oss_cooked) == -1)
      CLog::Log(LOGWARNING, "CAESinkOSS::Initialize - Failed to set cooked mode");
  }
#endif

  if (ioctl(m_fd, SNDCTL_DSP_SETFMT, &oss_fmt) == -1)
  {
    close(m_fd);
    CLog::Log(LOGERROR, "CAESinkOSS::Initialize - Failed to set the data format (%s)", CAEUtil::DataFormatToStr(format.m_dataFormat));
    return false;
  }

  /* find the number we need to open to access the channels we need */
  bool found = false;
  int oss_ch = 0;
  for (int ch = format.m_channelLayout.Count(); ch < 9; ++ch)
  {
    oss_ch = ch;
    if (ioctl(m_fd, SNDCTL_DSP_CHANNELS, &oss_ch) != -1 && oss_ch >= (int)format.m_channelLayout.Count())
    {
      found = true;
      break;
    }
  }

  if (!found)
    CLog::Log(LOGWARNING, "CAESinkOSS::Initialize - Failed to access the number of channels required, falling back");

#if defined(TARGET_FREEBSD)
  /* fix hdmi 8 channels order */
  if (!AE_IS_RAW(format.m_dataFormat) && 8 == oss_ch)
  {
    unsigned long long order = 0x0000000087346521ULL;

    if (ioctl(m_fd, SNDCTL_DSP_SET_CHNORDER, &order) == -1)
      CLog::Log(LOGWARNING, "CAESinkOSS::Initialize - Failed to set the channel order");
  }
#elif defined(OSS4)
  unsigned long long order = 0;

  for (unsigned int i = 0; i < format.m_channelLayout.Count(); ++i)
    switch (format.m_channelLayout[i])
    {
      case AE_CH_FL : order = (order << 4) | CHID_L  ; break;
      case AE_CH_FR : order = (order << 4) | CHID_R  ; break;
      case AE_CH_FC : order = (order << 4) | CHID_C  ; break;
      case AE_CH_LFE: order = (order << 4) | CHID_LFE; break;
      case AE_CH_SL : order = (order << 4) | CHID_LS ; break;
      case AE_CH_SR : order = (order << 4) | CHID_RS ; break;
      case AE_CH_BL : order = (order << 4) | CHID_LR ; break;
      case AE_CH_BR : order = (order << 4) | CHID_RR ; break;

      default:
        continue;
    }

  if (ioctl(m_fd, SNDCTL_DSP_SET_CHNORDER, &order) == -1)
  {
    if (ioctl(m_fd, SNDCTL_DSP_GET_CHNORDER, &order) == -1)
    {
      CLog::Log(LOGWARNING, "CAESinkOSS::Initialize - Failed to get the channel order, assuming CHNORDER_NORMAL");
    }
  }
#endif

  int tmp = (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3) * format.m_channelLayout.Count() * OSS_FRAMES;
  int pos = 0;
  while ((tmp & 0x1) == 0x0)
  {
    tmp = tmp >> 1;
    ++pos;
  }

  int oss_frag = (4 << 16) | pos;
  if (ioctl(m_fd, SNDCTL_DSP_SETFRAGMENT, &oss_frag) == -1)
    CLog::Log(LOGWARNING, "CAESinkOSS::Initialize - Failed to set the fragment size");

  int oss_sr = format.m_sampleRate;
  if (ioctl(m_fd, SNDCTL_DSP_SPEED, &oss_sr) == -1)
  {
    close(m_fd);
    CLog::Log(LOGERROR, "CAESinkOSS::Initialize - Failed to set the sample rate");
    return false;
  }

  audio_buf_info bi;
  if (ioctl(m_fd, SNDCTL_DSP_GETOSPACE, &bi) == -1)
  {
    close(m_fd);
    CLog::Log(LOGERROR, "CAESinkOSS::Initialize - Failed to get the output buffer size");
    return false;
  }

  format.m_sampleRate    = oss_sr;
  format.m_frameSize     = (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3) * format.m_channelLayout.Count();
  format.m_frames        = bi.fragsize / format.m_frameSize;
  format.m_frameSamples  = format.m_frames * format.m_channelLayout.Count();

  m_device = device;
  m_format = format;
  return true;
}

void CAESinkOSS::Deinitialize()
{
  Stop();

  if (m_fd != -1)
    close(m_fd);
  
  m_blockingNeedsUpdate = true;
}

inline CAEChannelInfo CAESinkOSS::GetChannelLayout(AEAudioFormat format)
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
        if (format.m_channelLayout[i] == OSSChannelMap[c])
        {
          count = c + 1;
          break;
        }
  }

  CAEChannelInfo info;
  for (unsigned int i = 0; i < count; ++i)
    info += OSSChannelMap[i];

  return info;
}

bool CAESinkOSS::IsCompatible(const AEAudioFormat &format, const std::string &device)
{
  AEAudioFormat tmp  = format;
  tmp.m_channelLayout = GetChannelLayout(format);

  return (
    tmp.m_sampleRate    == m_initFormat.m_sampleRate    &&
    tmp.m_dataFormat    == m_initFormat.m_dataFormat    &&
    tmp.m_channelLayout == m_initFormat.m_channelLayout &&
    GetDeviceUse(tmp, device) == m_device
  );
}

void CAESinkOSS::Stop()
{
#ifdef SNDCTL_DSP_RESET
  if (m_fd != -1)
    ioctl(m_fd, SNDCTL_DSP_RESET, NULL);
#endif
}

double CAESinkOSS::GetDelay()
{
  if (m_fd == -1)
    return 0.0;
  
  int delay;
  if (ioctl(m_fd, SNDCTL_DSP_GETODELAY, &delay) == -1)
    return 0.0;

  return (double)delay / (m_format.m_frameSize * m_format.m_sampleRate);
}

unsigned int CAESinkOSS::AddPackets(uint8_t *data, unsigned int frames, bool hasAudio, bool blocking)
{
  int size = frames * m_format.m_frameSize;
  if (m_fd == -1)
  {
    CLog::Log(LOGERROR, "CAESinkOSS::AddPackets - Failed to write");
    return INT_MAX;
  }

  if(m_blockingNeedsUpdate)
  {
    if(!blocking)
    {
      if (fcntl(m_fd, F_SETFL,  fcntl(m_fd, F_GETFL, 0) | O_NONBLOCK) == -1)
      {
        CLog::Log(LOGERROR, "CAESinkOSS::Initialize - Failed to set non blocking writes");
      }
    }
    m_blockingNeedsUpdate = false;
  }

  int wrote = write(m_fd, data, size);
  if (wrote < 0)
  {
    if(!blocking && (errno == EAGAIN || errno == EWOULDBLOCK))
      return 0;

    CLog::Log(LOGERROR, "CAESinkOSS::AddPackets - Failed to write");
    return INT_MAX;
  }

  return wrote / m_format.m_frameSize;
}

void CAESinkOSS::Drain()
{
  if (m_fd == -1)
    return;

  if(ioctl(m_fd, SNDCTL_DSP_SYNC, NULL) == -1)
  {
    CLog::Log(LOGERROR, "CAESinkOSS::Drain - Draining the Sink failed");
  }
}

void CAESinkOSS::EnumerateDevicesEx(AEDeviceInfoList &list, bool force)
{
  int mixerfd;
  const char * mixerdev = "/dev/mixer";

  if ((mixerfd = open(mixerdev, O_RDWR, 0)) == -1)
  {
    CLog::Log(LOGNOTICE,
	  "CAESinkOSS::EnumerateDevicesEx - No OSS mixer device present: %s", mixerdev);
    return;
  }	

#if defined(SNDCTL_SYSINFO) && defined(SNDCTL_CARDINFO)
  oss_sysinfo sysinfo;
  if (ioctl(mixerfd, SNDCTL_SYSINFO, &sysinfo) == -1)
  {
    // hardware not supported
	// OSSv4 required ?
    close(mixerfd);
    return;
  }

  for (int i = 0; i < sysinfo.numcards; ++i)
  {
    std::stringstream devicepath;
    std::stringstream devicename;
    CAEDeviceInfo info;
    oss_card_info cardinfo;

    devicepath << "/dev/dsp" << i;
    info.m_deviceName = devicepath.str();

    cardinfo.card = i;
    if (ioctl(mixerfd, SNDCTL_CARDINFO, &cardinfo) == -1)
      break;

    devicename << cardinfo.shortname << " " << cardinfo.longname;
    info.m_displayName = devicename.str();

    if (info.m_displayName.find("HDMI") != std::string::npos)
      info.m_deviceType = AE_DEVTYPE_HDMI;
    else if (info.m_displayName.find("Digital") != std::string::npos)
      info.m_deviceType = AE_DEVTYPE_IEC958;
    else
      info.m_deviceType = AE_DEVTYPE_PCM;
 
    oss_audioinfo ainfo;
    memset(&ainfo, 0, sizeof(ainfo));
    ainfo.dev = i;
    if (ioctl(mixerfd, SNDCTL_AUDIOINFO, &ainfo) != -1) {
#if 0
      if (ainfo.oformats & AFMT_S32_LE)
        info.m_dataFormats.push_back(AE_FMT_S32LE);
      if (ainfo.oformats & AFMT_S16_LE)
        info.m_dataFormats.push_back(AE_FMT_S16LE);
#endif
      for (int j = 0;
        j < ainfo.max_channels && AE_CH_NULL != OSSChannelMap[j];
        ++j)
          info.m_channels += OSSChannelMap[j];

      for (int *rate = OSSSampleRateList; *rate != 0; ++rate)
        if (*rate >= ainfo.min_rate && *rate <= ainfo.max_rate)
          info.m_sampleRates.push_back(*rate);
    }
    list.push_back(info);
  }
#endif
  close(mixerfd);
}

