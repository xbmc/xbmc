/*
 *      Copyright (C) 2005-2017 Team Kodi
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

#include "platform/Filesystem.h"
#include "system.h"
#include "filesystem/SpecialProtocol.h"

#ifdef TARGET_POSIX
#include <sys/stat.h>
#endif
#if !defined(TARGET_DARWIN) && !defined(TARGET_FREEBSD) && !defined(TARGET_ANDROID)
#include <sys/vfs.h>
#else
#include <sys/param.h>
#include <sys/mount.h>
#endif

#if defined(TARGET_ANDROID)
#include <sys/statfs.h>
#endif

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
#if defined(TARGET_ANDROID) || defined(TARGET_DARWIN)
  struct statfs fsInfo;
  // is 64-bit on android and darwin (10.6SDK + any iOS)
  auto result = statfs(CSpecialProtocol::TranslatePath(path).c_str(), &fsInfo);
#else
  struct statfs64 fsInfo;
  auto result = statfs64(CSpecialProtocol::TranslatePath(path).c_str(), &fsInfo);
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
}
}
}