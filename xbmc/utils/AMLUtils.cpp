/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>

#include "AMLUtils.h"
#include "utils/CPUInfo.h"
#include "utils/log.h"
#include "utils/SysfsUtils.h"
#include "utils/StringUtils.h"
#include "guilib/gui3d.h"
#include "utils/RegExp.h"

bool aml_present()
{
  static int has_aml = -1;
  if (has_aml == -1)
  {
    if (SysfsUtils::Has("/sys/class/audiodsp/digital_raw"))
      has_aml = 1;
    else
      has_aml = 0;
    if (has_aml)
      CLog::Log(LOGNOTICE, "AML device detected");
  }
  return has_aml == 1;
}

bool aml_hw3d_present()
{
  static int has_hw3d = -1;
  if (has_hw3d == -1)
  {
    if (SysfsUtils::Has("/sys/class/ppmgr/ppmgr_3d_mode"))
      has_hw3d = 1;
    else
      has_hw3d = 0;
    if (has_hw3d)
      CLog::Log(LOGNOTICE, "AML 3D support detected");
  }
  return has_hw3d == 1;
}

bool aml_wired_present()
{
  static int has_wired = -1;
  if (has_wired == -1)
  {
    std::string test;
    if (SysfsUtils::GetString("/sys/class/net/eth0/operstate", test) != -1)
      has_wired = 1;
    else
      has_wired = 0;
  }
  return has_wired == 1;
}

bool aml_permissions()
{  
  if (!aml_present())
    return false;

  static int permissions_ok = -1;
  if (permissions_ok == -1)
  {
    permissions_ok = 1;

    if (!SysfsUtils::HasRW("/dev/amvideo"))
    {
      CLog::Log(LOGERROR, "AML: no rw on /dev/amvideo");
      permissions_ok = 0;
    }
    if (!SysfsUtils::HasRW("/dev/amstream_mpts"))
    {
      CLog::Log(LOGERROR, "AML: no rw on /dev/amstream*");
      permissions_ok = 0;
    }
    if (!SysfsUtils::HasRW("/sys/class/video/axis"))
    {
      CLog::Log(LOGERROR, "AML: no rw on /sys/class/video/axis");
      permissions_ok = 0;
    }
    if (!SysfsUtils::HasRW("/sys/class/video/screen_mode"))
    {
      CLog::Log(LOGERROR, "AML: no rw on /sys/class/video/screen_mode");
      permissions_ok = 0;
    }
    if (!SysfsUtils::HasRW("/sys/class/video/disable_video"))
    {
      CLog::Log(LOGERROR, "AML: no rw on /sys/class/video/disable_video");
      permissions_ok = 0;
    }
    if (!SysfsUtils::HasRW("/sys/class/tsync/pts_pcrscr"))
    {
      CLog::Log(LOGERROR, "AML: no rw on /sys/class/tsync/pts_pcrscr");
      permissions_ok = 0;
    }
    if (!SysfsUtils::HasRW("/dev/video10"))
    {
      CLog::Log(LOGERROR, "AML: no rw on /dev/video10");
      permissions_ok = 0;
    }
    if (!SysfsUtils::HasRW("/sys/module/amvideo/parameters/omx_pts"))
    {
      CLog::Log(LOGERROR, "AML: no rw on /sys/module/amvideo/parameters/omx_pts");
      permissions_ok = 0;
    }
    if (!SysfsUtils::HasRW("/sys/module/amlvideodri/parameters/freerun_mode"))
    {
      CLog::Log(LOGERROR, "AML: no rw on /sys/module/amlvideodri/parameters/freerun_mode");
      permissions_ok = 0;
    }
    if (!SysfsUtils::HasRW("/sys/class/audiodsp/digital_raw"))
    {
      CLog::Log(LOGERROR, "AML: no rw on /sys/class/audiodsp/digital_raw");
    }
    if (!SysfsUtils::HasRW("/sys/class/ppmgr/ppmgr_3d_mode"))
    {
      CLog::Log(LOGERROR, "AML: no rw on /sys/class/ppmgr/ppmgr_3d_mode");
    }
#ifndef TARGET_ANDROID
    if (!SysfsUtils::HasRW("/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq"))
    {
      CLog::Log(LOGERROR, "AML: no rw on /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
    }
    if (!SysfsUtils::HasRW("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"))
    {
      CLog::Log(LOGERROR, "AML: no rw on /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
    }
    if (!SysfsUtils::HasRW("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"))
    {
      CLog::Log(LOGERROR, "AML: no rw on /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    }
#endif
  }

  return permissions_ok == 1;
}

bool aml_support_hevc()
{
  static int has_hevc = -1;

  if (has_hevc == -1)
  {
    std::string valstr;
    if(SysfsUtils::GetString("/sys/class/amstream/vcodec_profile", valstr) != 0)
      has_hevc = 0;
    else
      has_hevc = (valstr.find("hevc:") != std::string::npos) ? 1: 0;
  }
  return (has_hevc == 1);
}

bool aml_support_hevc_4k2k()
{
  static int has_hevc_4k2k = -1;

  if (has_hevc_4k2k == -1)
  {
    CRegExp regexp;
    regexp.RegComp("hevc:.*4k");
    std::string valstr;
    if (SysfsUtils::GetString("/sys/class/amstream/vcodec_profile", valstr) != 0)
      has_hevc_4k2k = 0;
    else
      has_hevc_4k2k = (regexp.RegFind(valstr) >= 0) ? 1 : 0;
  }
  return (has_hevc_4k2k == 1);
}

bool aml_support_h264_4k2k()
{
  static int has_h264_4k2k = -1;

  if (has_h264_4k2k == -1)
  {
    std::string valstr;
    if (SysfsUtils::GetString("/sys/class/amstream/vcodec_profile", valstr) != 0)
    {
      return false;
    }
    return (valstr.find("h264_4k2k:") != std::string::npos);
  }
  return (has_h264_4k2k == 1);
}

void aml_set_audio_passthrough(bool passthrough)
{
  SysfsUtils::SetInt("/sys/class/audiodsp/digital_raw", passthrough ? 2:0);
}

void aml_probe_hdmi_audio()
{
  // Audio {format, channel, freq, cce}
  // {1, 7, 7f, 7}
  // {7, 5, 1e, 0}
  // {2, 5, 7, 0}
  // {11, 7, 7e, 1}
  // {10, 7, 6, 0}
  // {12, 7, 7e, 0}

  int fd = open("/sys/class/amhdmitx/amhdmitx0/edid", O_RDONLY);
  if (fd >= 0)
  {
    char valstr[1024] = {0};

    read(fd, valstr, sizeof(valstr) - 1);
    valstr[strlen(valstr)] = '\0';
    close(fd);

    std::vector<std::string> probe_str = StringUtils::Split(valstr, "\n");

    for (std::vector<std::string>::const_iterator i = probe_str.begin(); i != probe_str.end(); ++i)
    {
      if (i->find("Audio") == std::string::npos)
      {
        for (std::vector<std::string>::const_iterator j = i + 1; j != probe_str.end(); ++j)
        {
          if      (j->find("{1,")  != std::string::npos)
            printf(" PCM found {1,\n");
          else if (j->find("{2,")  != std::string::npos)
            printf(" AC3 found {2,\n");
          else if (j->find("{3,")  != std::string::npos)
            printf(" MPEG1 found {3,\n");
          else if (j->find("{4,")  != std::string::npos)
            printf(" MP3 found {4,\n");
          else if (j->find("{5,")  != std::string::npos)
            printf(" MPEG2 found {5,\n");
          else if (j->find("{6,")  != std::string::npos)
            printf(" AAC found {6,\n");
          else if (j->find("{7,")  != std::string::npos)
            printf(" DTS found {7,\n");
          else if (j->find("{8,")  != std::string::npos)
            printf(" ATRAC found {8,\n");
          else if (j->find("{9,")  != std::string::npos)
            printf(" One_Bit_Audio found {9,\n");
          else if (j->find("{10,") != std::string::npos)
            printf(" Dolby found {10,\n");
          else if (j->find("{11,") != std::string::npos)
            printf(" DTS_HD found {11,\n");
          else if (j->find("{12,") != std::string::npos)
            printf(" MAT found {12,\n");
          else if (j->find("{13,") != std::string::npos)
            printf(" ATRAC found {13,\n");
          else if (j->find("{14,") != std::string::npos)
            printf(" WMA found {14,\n");
          else
            break;
        }
        break;
      }
    }
  }
}

int aml_axis_value(AML_DISPLAY_AXIS_PARAM param)
{
  std::string axis;
  int value[8];

  SysfsUtils::GetString("/sys/class/display/axis", axis);
  sscanf(axis.c_str(), "%d %d %d %d %d %d %d %d", &value[0], &value[1], &value[2], &value[3], &value[4], &value[5], &value[6], &value[7]);

  return value[param];
}

bool aml_IsHdmiConnected()
{
  int hpd_state;
  SysfsUtils::GetInt("/sys/class/amhdmitx/amhdmitx0/hpd_state", hpd_state);
  if (hpd_state == 2)
  {
    return 1;
  }

  return 0;
}

bool aml_mode_to_resolution(const char *mode, RESOLUTION_INFO *res)
{
  if (!res)
    return false;

  res->iWidth = 0;
  res->iHeight= 0;

  if(!mode)
    return false;

  std::string fromMode = mode;
  StringUtils::Trim(fromMode);
  // strips, for example, 720p* to 720p
  // the * indicate the 'native' mode of the display
  if (StringUtils::EndsWith(fromMode, "*"))
    fromMode.erase(fromMode.size() - 1);

  if (StringUtils::EqualsNoCase(fromMode, "panel"))
  {
    res->iWidth = aml_axis_value(AML_DISPLAY_AXIS_PARAM_WIDTH);
    res->iHeight= aml_axis_value(AML_DISPLAY_AXIS_PARAM_HEIGHT);
    res->iScreenWidth = aml_axis_value(AML_DISPLAY_AXIS_PARAM_WIDTH);
    res->iScreenHeight= aml_axis_value(AML_DISPLAY_AXIS_PARAM_HEIGHT);
    res->fRefreshRate = 60;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if ((StringUtils::EqualsNoCase(fromMode, "480cvbs")) || (StringUtils::EqualsNoCase(fromMode, "480i")))
  {
    res->iWidth = 720;
    res->iHeight= 480;
    res->iScreenWidth = 720;
    res->iScreenHeight= 480;
    res->fRefreshRate = 60;
    res->dwFlags = D3DPRESENTFLAG_INTERLACED;
  }
  else if ((StringUtils::EqualsNoCase(fromMode, "576cvbs")) || (StringUtils::EqualsNoCase(fromMode, "576i")))
  {
    res->iWidth = 720;
    res->iHeight= 576;
    res->iScreenWidth = 720;
    res->iScreenHeight= 576;
    res->fRefreshRate = 50;
    res->dwFlags = D3DPRESENTFLAG_INTERLACED;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "480p"))
  {
    res->iWidth = 720;
    res->iHeight= 480;
    res->iScreenWidth = 720;
    res->iScreenHeight= 480;
    res->fRefreshRate = 60;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "576p"))
  {
    res->iWidth = 720;
    res->iHeight= 576;
    res->iScreenWidth = 720;
    res->iScreenHeight= 576;
    res->fRefreshRate = 50;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "720p") || StringUtils::EqualsNoCase(fromMode, "720p60hz"))
  {
    res->iWidth = 1280;
    res->iHeight= 720;
    res->iScreenWidth = 1280;
    res->iScreenHeight= 720;
    res->fRefreshRate = 60;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "720p50hz"))
  {
    res->iWidth = 1280;
    res->iHeight= 720;
    res->iScreenWidth = 1280;
    res->iScreenHeight= 720;
    res->fRefreshRate = 50;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "1080p") || StringUtils::EqualsNoCase(fromMode, "1080p60hz"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 1920;
    res->iScreenHeight= 1080;
    res->fRefreshRate = 60;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "1080p23hz"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 1920;
    res->iScreenHeight= 1080;
    res->fRefreshRate = 23.976;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "1080p24hz"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 1920;
    res->iScreenHeight= 1080;
    res->fRefreshRate = 24;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "1080p30hz"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 1920;
    res->iScreenHeight= 1080;
    res->fRefreshRate = 30;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "1080p50hz"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 1920;
    res->iScreenHeight= 1080;
    res->fRefreshRate = 50;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "1080p59hz"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 1920;
    res->iScreenHeight= 1080;
    res->fRefreshRate = 59.940;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "1080i") || StringUtils::EqualsNoCase(fromMode, "1080i60hz"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 1920;
    res->iScreenHeight= 1080;
    res->fRefreshRate = 60;
    res->dwFlags = D3DPRESENTFLAG_INTERLACED;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "1080i50hz"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 1920;
    res->iScreenHeight= 1080;
    res->fRefreshRate = 50;
    res->dwFlags = D3DPRESENTFLAG_INTERLACED;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "1080i59hz"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 1920;
    res->iScreenHeight= 1080;
    res->fRefreshRate = 59.940;
    res->dwFlags = D3DPRESENTFLAG_INTERLACED;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "4k2ksmpte") || StringUtils::EqualsNoCase(fromMode, "smpte24hz"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 4096;
    res->iScreenHeight= 2160;
    res->fRefreshRate = 24;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "4k2k23hz") || StringUtils::EqualsNoCase(fromMode, "2160p23hz"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 3840;
    res->iScreenHeight= 2160;
    res->fRefreshRate = 23.976;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "4k2k24hz") || StringUtils::EqualsNoCase(fromMode, "2160p24hz"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 3840;
    res->iScreenHeight= 2160;
    res->fRefreshRate = 24;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "4k2k25hz") || StringUtils::EqualsNoCase(fromMode, "2160p25hz"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 3840;
    res->iScreenHeight= 2160;
    res->fRefreshRate = 25;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "4k2k29hz") || StringUtils::EqualsNoCase(fromMode, "2160p29hz"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 3840;
    res->iScreenHeight= 2160;
    res->fRefreshRate = 29.970;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "4k2k30hz") || StringUtils::EqualsNoCase(fromMode, "2160p30hz"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 3840;
    res->iScreenHeight= 2160;
    res->fRefreshRate = 30;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "2160p50hz420"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 3840;
    res->iScreenHeight= 2160;
    res->fRefreshRate = 50;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (StringUtils::EqualsNoCase(fromMode, "2160p60hz420"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 3840;
    res->iScreenHeight= 2160;
    res->fRefreshRate = 60;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else
  {
    return false;
  }


  res->iScreen       = 0;
  res->bFullScreen   = true;
  res->iSubtitles    = (int)(0.965 * res->iHeight);
  res->fPixelRatio   = 1.0f;
  res->strId         = fromMode;
  res->strMode       = StringUtils::Format("%dx%d @ %.2f%s - Full Screen", res->iScreenWidth, res->iScreenHeight, res->fRefreshRate,
    res->dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");

  return res->iWidth > 0 && res->iHeight> 0;
}

