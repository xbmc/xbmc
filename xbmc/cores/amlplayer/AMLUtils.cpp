/*
 *      Copyright (C) 2011-2012 Team XBMC
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

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>

int aml_set_sysfs_str(const char *path, const char *val)
{
  int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (fd >= 0)
  {
    write(fd, val, strlen(val));
    close(fd);
    return 0;
  }
  return -1;
}

int aml_get_sysfs_str(const char *path, char *valstr, const int size)
{
  int fd = open(path, O_RDONLY);
  if (fd >= 0)
  {
    read(fd, valstr, size - 1);
    valstr[strlen(valstr)] = '\0';
    close(fd);
    return 0;
  }

  sprintf(valstr, "%s", "fail");
  return -1;
}

int aml_set_sysfs_int(const char *path, const int val)
{
  int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (fd >= 0)
  {
    char bcmd[16];
    sprintf(bcmd, "%d", val);
    write(fd, bcmd, strlen(bcmd));
    close(fd);
    return 0;
  }
  return -1;
}

int aml_get_sysfs_int(const char *path)
{
  int val = 0;
  int fd = open(path, O_RDONLY);
  if (fd >= 0)
  {
    char bcmd[16];
    read(fd, bcmd, sizeof(bcmd));
    val = strtol(bcmd, NULL, 16);
    close(fd);
  }
  return val;
}

bool aml_present()
{
  static int has_aml = -1;
  if (has_aml == -1)
  {
    if (aml_get_sysfs_int("/sys/class/amhdmitx/amhdmitx0/disp_cap") != -1)
      has_aml = 1;
    else
      has_aml = 0;
  }
  return has_aml;
}

void aml_cpufreq_limit(bool limit)
{
  static int audiotrack_cputype = -1;
  if (audiotrack_cputype == -1)
  {
    // defualt to m1 SoC
    audiotrack_cputype = 1;

    FILE *cpuinfo_fd = fopen("/proc/cpuinfo", "r");
    if (cpuinfo_fd)
    {
      char buffer[512];
      while (fgets(buffer, sizeof(buffer), cpuinfo_fd))
      {
        std::string stdbuffer(buffer);
        if (stdbuffer.find("MESON-M3") != std::string::npos)
        {
          audiotrack_cputype = 3;
          break;
        }
      }
      fclose(cpuinfo_fd);
    }
  }
  // On M1 SoCs, when playing hw decoded audio, we cannot drop below 600MHz
  // or risk hw audio dropouts. AML code does a 2X scaling based off
  // /sys/class/audiodsp/codec_mips but tests show that this is
  // seems risky so we just clamp to 600Mhz to be safe.
  if (audiotrack_cputype == 3)
    return;

  int cpufreq = 300000;
  if (limit)
    cpufreq = 600000;

  aml_set_sysfs_int("/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq", cpufreq);
}

void aml_set_audio_passthrough(bool passthrough)
{
  if (aml_present())
    aml_set_sysfs_int("/sys/class/audiodsp/digital_raw", passthrough ? 1:0);
}
