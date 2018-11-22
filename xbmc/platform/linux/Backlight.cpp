/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Backlight.h"

#include "platform/linux/SysfsPath.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <vector>

#include <dirent.h>
#include <unistd.h>

void CBacklight::Register(std::shared_ptr<CBacklight> backlight)
{
  IBacklight::Register(backlight);
}

int CBacklight::GetBrightness() const
{
  return m_actualBrightnessPath.Get<int>();
}

int CBacklight::GetMaxBrightness() const
{
  return m_maxBrightnessPath.Get<int>();
}

bool CBacklight::SetBrightness(int brightness)
{
  return m_brightnessPath.Set(brightness);
}

bool CBacklight::Init(const std::string& drmDevicePath)
{
  std::string backlightPath{"/sys/class/backlight"};

  std::vector<std::string> drmDevicePathSplit = StringUtils::Split(drmDevicePath, "/");

  std::string drmDeviceSysPath{"/sys/class/drm/" + drmDevicePathSplit.back() + "/device"};

  CLog::Log(LOGDEBUG, "{}: using drm sys path {}", __FUNCTION__, drmDeviceSysPath);

  std::string buffer(256, '\0');
  auto length = readlink(drmDeviceSysPath.c_str(), &buffer[0], buffer.size());
  if (length < 0)
  {
    CLog::Log(LOGERROR, "{}: failed to readlink for {} - {}", __FUNCTION__, drmDeviceSysPath,
              strerror(errno));
    return false;
  }

  buffer.resize(length);

  std::string drmPciName = basename(buffer.c_str());

  CLog::Log(LOGDEBUG, "{}: drm pci name {}", __FUNCTION__, drmPciName);

  auto backlightDir = opendir(backlightPath.c_str());

  if (!backlightDir)
  {
    CLog::Log(LOGERROR, "{}: failed to open directory {} - {}", __FUNCTION__, backlightPath,
              strerror(errno));
    return false;
  }

  dirent* entry{nullptr};
  while ((entry = readdir(backlightDir)))
  {
    if (entry->d_name[0] == '.')
      continue;

    std::string backlightDirEntryPath{backlightPath + "/" + entry->d_name};

    CLog::Log(LOGDEBUG, "{}: backlight path {}", __FUNCTION__, backlightDirEntryPath);

    CSysfsPath backlightTypePath{backlightDirEntryPath + "/type"};

    std::string backlightType = backlightTypePath.Get<std::string>();
    if (backlightType.empty())
      continue;

    CLog::Log(LOGDEBUG, "{}: backlight type {}", __FUNCTION__, backlightType);

    std::string backlightDirEntryDevicePath{backlightDirEntryPath + "/device"};

    CLog::Log(LOGDEBUG, "{}: backlight device path {}", __FUNCTION__, backlightDirEntryDevicePath);

    buffer.resize(256);
    length = readlink(backlightDirEntryDevicePath.c_str(), &buffer[0], buffer.size());
    if (length < 0)
    {
      CLog::Log(LOGERROR, "{}: failed to readlink for {} - {}", __FUNCTION__,
                backlightDirEntryDevicePath, strerror(errno));
      continue;
    }

    buffer.resize(length);

    std::string backlightPciName = basename(buffer.c_str());

    if (backlightType.find("raw") != std::string::npos || backlightType.find("firmware") != std::string::npos)
    {
      if (!drmPciName.empty() && drmPciName != backlightPciName)
      {
        continue;
      }
    }

    m_actualBrightnessPath = backlightDirEntryPath + "actual_brightness";
    m_maxBrightnessPath = backlightDirEntryPath + "max_brightness";
    m_brightnessPath = backlightDirEntryPath + "brightness";

    CLog::Log(LOGDEBUG, "{}: backlight path {}", __FUNCTION__, backlightDirEntryPath);

    return true;
  }

  CLog::Log(LOGDEBUG, "{}: no backlight found", __FUNCTION__);

  closedir(backlightDir);
  return false;
}
