/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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

#include "platform/Filesystem.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/URIUtils.h"

#if defined(TARGET_LINUX)
#include <sys/statvfs.h>
#elif defined(TARGET_DARWIN) || defined(TARGET_FREEBSD)
#include <sys/param.h>
#include <sys/mount.h>
#elif defined(TARGET_ANDROID)
#include <sys/statfs.h>
#endif

#include <cstdlib>
#include <limits.h>
#include <string.h>
#include <unistd.h>

namespace KODI
{
namespace PLATFORM
{
namespace FILESYSTEM
{

space_info space(const std::string& path, std::error_code& ec)
{
  ec.clear();
  space_info sp;
#if defined(TARGET_LINUX)
  struct statvfs64 fsInfo;
  auto result = statvfs64(CSpecialProtocol::TranslatePath(path).c_str(), &fsInfo);
#else
  struct statfs fsInfo;
  // is 64-bit on android and darwin (10.6SDK + any iOS)
  auto result = statfs(CSpecialProtocol::TranslatePath(path).c_str(), &fsInfo);
#endif

  if (result != 0)
  {
    ec.assign(result, std::system_category());
    sp.available = static_cast<uintmax_t>(-1);
    sp.capacity = static_cast<uintmax_t>(-1);
    sp.free = static_cast<uintmax_t>(-1);
    return sp;
  }
  sp.available = static_cast<uintmax_t>(fsInfo.f_bavail * fsInfo.f_bsize);
  sp.capacity = static_cast<uintmax_t>(fsInfo.f_blocks * fsInfo.f_bsize);
  sp.free = static_cast<uintmax_t>(fsInfo.f_bfree * fsInfo.f_bsize);

  return sp;
}

std::string temp_directory_path(std::error_code &ec)
{
  ec.clear();

  auto result = getenv("TMPDIR");
  if (result)
    return URIUtils::AppendSlash(result);

  return "/tmp/";
}

std::string create_temp_directory(std::error_code &ec)
{
  char buf[PATH_MAX];

  auto path = temp_directory_path(ec);

  strncpy(buf, (path + "xbmctempXXXXXX").c_str(), sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  auto tmp = mkdtemp(buf);
  if (!tmp)
  {
    ec.assign(errno, std::system_category());
    return std::string();
  }

  ec.clear();
  return std::string(tmp);
}

std::string temp_file_path(std::string suffix, std::error_code &ec)
{
  char tmp[PATH_MAX];

  auto tempPath = create_temp_directory(ec);
  if (ec)
    return std::string();

  tempPath = URIUtils::AddFileToFolder(tempPath, "xbmctempfileXXXXXX" + suffix);
  if (tempPath.length() >= PATH_MAX)
  {
    ec.assign(EOVERFLOW, std::system_category());
    return std::string();
  }

  strncpy(tmp, tempPath.c_str(), sizeof(tmp) - 1);
  tmp[sizeof(tmp) - 1] = '\0';

  auto fd = mkstemps(tmp, suffix.length());
  if (fd < 0)
  {
    ec.assign(errno, std::system_category());
    return std::string();
  }

  close(fd);

  ec.clear();
  return std::string(tmp);
}

}
}
}
