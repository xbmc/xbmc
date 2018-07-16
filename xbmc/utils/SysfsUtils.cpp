/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SysfsUtils.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#ifdef TARGET_WINDOWS_STORE
#include <io.h>
#endif

int SysfsUtils::SetString(const std::string& path, const std::string& valstr)
{
  int fd = open(path.c_str(), O_RDWR, 0644);
  int ret = 0;
  if (fd >= 0)
  {
    if (write(fd, valstr.c_str(), valstr.size()) < 0)
      ret = -1;
    close(fd);
  }
  if (ret)
    CLog::Log(LOGERROR, "%s: error writing %s",__FUNCTION__, path.c_str());

  return ret;
}

int SysfsUtils::GetString(const std::string& path, std::string& valstr)
{
  int len;
  char buf[256] = {0};

  int fd = open(path.c_str(), O_RDONLY);
  if (fd >= 0)
  {
    valstr.clear();
    while ((len = read(fd, buf, 256)) > 0)
      valstr.append(buf, len);
    close(fd);

    StringUtils::Trim(valstr);

    return 0;
  }

  CLog::Log(LOGERROR, "%s: error reading %s",__FUNCTION__, path.c_str());
  valstr = "fail";
  return -1;
}

int SysfsUtils::SetInt(const std::string& path, const int val)
{
  int fd = open(path.c_str(), O_RDWR, 0644);
  int ret = 0;
  if (fd >= 0)
  {
    char bcmd[16];
    sprintf(bcmd, "%d", val);
    if (write(fd, bcmd, strlen(bcmd)) < 0)
      ret = -1;
    close(fd);
  }
  if (ret)
    CLog::Log(LOGERROR, "%s: error writing %s",__FUNCTION__, path.c_str());

  return ret;
}

int SysfsUtils::GetInt(const std::string& path, int& val)
{
  int fd = open(path.c_str(), O_RDONLY);
  int ret = 0;
  if (fd >= 0)
  {
    char bcmd[16];
    if (read(fd, bcmd, sizeof(bcmd)) < 0)
      ret = -1;
    else
      val = strtol(bcmd, NULL, 16);

    close(fd);
  }
  if (ret)
    CLog::Log(LOGERROR, "%s: error reading %s",__FUNCTION__, path.c_str());

  return ret;
}

bool SysfsUtils::Has(const std::string &path)
{
  int fd = open(path.c_str(), O_RDONLY);
  if (fd >= 0)
  {
    close(fd);
    return true;
  }
  return false;
}

bool SysfsUtils::HasRW(const std::string &path)
{
  int fd = open(path.c_str(), O_RDWR);
  if (fd >= 0)
  {
    close(fd);
    return true;
  }
  return false;
}
