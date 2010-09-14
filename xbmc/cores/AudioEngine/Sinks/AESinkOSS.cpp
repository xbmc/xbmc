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

#include "AESinkOSS.h"
#include <stdint.h>
#include <limits.h>

#include "AEUtil.h"
#include "StdString.h"
#include "utils/log.h"
#include "utils/SingleLock.h"

#include <sys/ioctl.h>
#include <linux/soundcard.h>

#define OSS_FRAMES 128

CAESinkOSS::CAESinkOSS()
{
}

CAESinkOSS::~CAESinkOSS()
{
}

CStdString CAESinkOSS::GetDeviceUse(AEAudioFormat format, CStdString device)
{
  if (format.m_dataFormat == AE_FMT_RAW)
  {
    if (device == "default")
      return "/dev/dsp_ac3";

    return device;
  }

  if (device == "default")
  {
    if (format.m_channelCount > 2)
      return "/dev/dsp_multich";

    return "/dev/dsp";
  }

  return device;
}

bool CAESinkOSS::Initialize(AEAudioFormat &format, CStdString &device)
{
  m_initFormat = format;
  device = GetDeviceUse(format, device);

  /* try to open in exclusive mode first (no software mixing) */
  m_fd = open(device.c_str(), O_WRONLY | O_EXCL, 0);
  if (!m_fd)
    m_fd = open(device.c_str(), O_WRONLY, 0);
  if (!m_fd) return false;

#ifdef SNDCTL_ENGINEINFO
  oss_audioinfo ai;
  ai.dev = -1;
  if (ioctl(m_fd, SNDCTL_ENGINEINFO, &ai) == -1)
    CLog::Log(LOGWARNING, "CAESinkOSS::Initialize - Failed to get engine info");

  /* todo: select format based on info */
#endif

#ifdef SNDCTL_DSP_COOKEDMODE
  int oss_cooked = 1;
  if (ioctl(m_fd, SNDCTL_DSP_COOKEDMODE, &oss_cooked) == -1)
    CLog::Log(LOGWARNING, "CAESinkOSS::Initialize - Failed to set cooked mode");
#endif

  int oss_fmt;
  switch(format.m_dataFormat)
  {
    case AE_FMT_U8   : oss_fmt = AFMT_U8    ; break;
    case AE_FMT_S8   : oss_fmt = AFMT_S8    ; break;
    case AE_FMT_S16NE: oss_fmt = AFMT_S16_NE; break;
    case AE_FMT_S16BE: oss_fmt = AFMT_S16_BE; break;
    case AE_FMT_S16LE: oss_fmt = AFMT_S16_LE; break;
    case AE_FMT_RAW  : oss_fmt = AFMT_AC3   ; break;
    default:
      format.m_dataFormat = AE_FMT_S16NE;
      oss_fmt = AFMT_S16_NE;
  }

  if (ioctl(m_fd, SNDCTL_DSP_SETFMT, &oss_fmt) == -1)
  {
    close(m_fd);
    CLog::Log(LOGERROR, "CAESinkOSS::Initialize - Failed to set the data format (%s)", CAEUtil::DataFormatToStr(format.m_dataFormat));
    return false;
  }

  int tmp = (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3) * format.m_channelCount * OSS_FRAMES;
  int pos = 0;
  while((tmp & 0x1) == 0x0)
  {
    tmp = tmp >> 1;
    ++pos;
  }

  int oss_frag = (8 << 16) | pos;
  if (ioctl(m_fd, SNDCTL_DSP_SETFRAGMENT, &oss_frag) == -1)
    CLog::Log(LOGWARNING, "CAESinkOSS::Initialize - Failed to set the fragment size");

  int oss_ch = format.m_channelCount;
  if (ioctl(m_fd, SNDCTL_DSP_CHANNELS, &oss_ch) == -1)
  {
    close(m_fd);
    CLog::Log(LOGERROR, "CAESinkOSS::Initialize - Failed to set the number of channels");
    return false;
  }

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


  format.m_sampleRate   = oss_sr;
  format.m_frameSize    = (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3) * format.m_channelCount;
  format.m_frames       = bi.bytes / format.m_frameSize / OSS_FRAMES;
  format.m_frameSamples = format.m_frames * format.m_channelCount;

  m_device = device;
  m_format = format;
  return true; 
}

void CAESinkOSS::Deinitialize()
{
  close(m_fd);
}

bool CAESinkOSS::IsCompatible(const AEAudioFormat format, const CStdString device)
{
  return (
    format.m_sampleRate   == m_initFormat.m_sampleRate    &&
    format.m_dataFormat   == m_initFormat.m_dataFormat    &&
    format.m_channelCount == m_initFormat.m_channelCount  &&
    GetDeviceUse(format, device) == m_device
  );
}

void CAESinkOSS::Stop()
{
}

float CAESinkOSS::GetDelay()
{
  int delay;
  if (ioctl(m_fd, SNDCTL_DSP_GETODELAY, &delay) == -1)
    return 0.0f;

  return (float)delay / (m_format.m_frameSize * m_format.m_sampleRate);
}

unsigned int CAESinkOSS::AddPackets(uint8_t *data, unsigned int frames)
{
  int size = frames * m_format.m_frameSize;
  int wrote = write(m_fd, data, size);
  if (wrote < 0)
  {
    CLog::Log(LOGERROR, "CAESinkOSS::AddPackets - Failed to write");
    return frames;
  }

  return wrote / m_format.m_frameSize;
}

