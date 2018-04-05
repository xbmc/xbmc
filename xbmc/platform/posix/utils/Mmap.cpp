/*
 *      Copyright (C) 2017 Team XBMC
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

#include "Mmap.h"

#include <system_error>

using namespace KODI::UTILS::POSIX;

CMmap::CMmap(void* addr, std::size_t length, int prot, int flags, int fildes, off_t offset)
: m_size{length}, m_memory{mmap(addr, length, prot, flags, fildes, offset)}
{
  if (m_memory == MAP_FAILED)
  {
    throw std::system_error(errno, std::generic_category(), "mmap failed");
  }
}

CMmap::~CMmap()
{
  munmap(m_memory, m_size);
}