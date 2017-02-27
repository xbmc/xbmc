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
#include "platform/win32/CharsetConverter.h"

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace KODI
{
namespace PLATFORM
{
namespace FILESYSTEM
{
space_info space(const std::string& path, std::error_code& ec)
{
  using WINDOWS::ToW;

  ec.clear();
  space_info sp;
  auto pathW = ToW(path);

  ULARGE_INTEGER capacity;
  ULARGE_INTEGER available;
  ULARGE_INTEGER free;
  auto result = GetDiskFreeSpaceExW(pathW.c_str(), &available, &capacity, &free);

  if (result == FALSE)
  {
    ec.assign(GetLastError(), std::system_category());
    sp.available = static_cast<uintmax_t>(-1);
    sp.capacity = static_cast<uintmax_t>(-1);
    sp.free = static_cast<uintmax_t>(-1);
    return sp;
  }

  sp.available = static_cast<uintmax_t>(available.QuadPart);
  sp.capacity = static_cast<uintmax_t>(capacity.QuadPart);
  sp.free = static_cast<uintmax_t>(free.QuadPart);

  return sp;
}
}
}
}