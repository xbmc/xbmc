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

#include "Filesystem.h"

#include <string>
#include <system_error>

namespace KODI
{
namespace PLATFORM
{
namespace FILESYSTEM
{

filesystem_error::filesystem_error(const std::string &what_arg, std::error_code ec)
    : system_error(ec)
    , m_what(what_arg)
{
}

filesystem_error::filesystem_error(const std::string &what_arg,
                                   const std::string &p1,
                                   std::error_code ec)
    : filesystem_error(what_arg, p1, std::string(), ec)
{
}

filesystem_error::filesystem_error(const std::string &what_arg,
                                   const std::string &p1,
                                   const std::string &p2,
                                   std::error_code ec)
    : system_error(ec)
    , m_what(what_arg)
    , m_path1(p1)
    , m_path2(p2)
{
}

const std::string &filesystem_error::path1() const noexcept
{
  return m_path1;
}

const std::string &filesystem_error::path2() const noexcept
{
  return m_path2;
}

const char *filesystem_error::what() const noexcept
{
  return m_what.c_str();
}

file_status::file_status() noexcept
    : file_status(file_type::none, perms::unknown)
{
}

file_status::file_status(file_type ft) noexcept
    : file_status(ft, perms::unknown)
{
}

file_status::file_status(file_type ft, perms prms) noexcept
    : m_type(ft)
    , m_perms(prms)
{
}

file_status::file_status(const file_status &) noexcept = default;
file_status::file_status(file_status &&) noexcept = default;
file_status::~file_status() = default;

file_status &file_status::operator=(const file_status &) noexcept = default;
file_status &file_status::operator=(file_status &&) noexcept = default;

void file_status::type(file_type ft) noexcept
{
  m_type = ft;
}

void file_status::permissions(perms prms) noexcept
{
  m_perms = prms;
}

file_type file_status::type() const noexcept
{
  return m_type;
}
perms file_status::permissions() const noexcept
{
  return m_perms;
}
}
}
}