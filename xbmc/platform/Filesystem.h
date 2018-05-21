#pragma once
/*
*      Copyright (C) 2005-present Team Kodi
*      http://kodi.tv
*
*  Kodi is free software: you can redistribute it and/or modify it
*  under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 2 of the License, or
*  (at your option) any later version.
*
*  Kodi is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
*
*/

#include <string>
#include <system_error>
namespace KODI
{
namespace PLATFORM
{
namespace FILESYSTEM
{
struct space_info {
  std::uintmax_t capacity;
  std::uintmax_t free;
  std::uintmax_t available;
};

space_info space(const std::string &path, std::error_code &ec);

std::string temp_directory_path(std::error_code &ec);
std::string create_temp_directory(std::error_code &ec);
std::string temp_file_path(std::string suffix, std::error_code &ec);
}
}
}
