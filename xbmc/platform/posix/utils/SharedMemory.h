/*
 *      Copyright (C) 2017-present Team Kodi
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
#pragma once

#include <memory>

#include "FileHandle.h"
#include "Mmap.h"

namespace KODI
{
namespace UTILS
{
namespace POSIX
{

/**
 * Get a chunk of shared memory of specified size
 *
 * The shared memory is automatically allocated, truncated to the correct size
 * and memory-mapped.
 */
class CSharedMemory
{
public:
  explicit CSharedMemory(std::size_t size);

  std::size_t Size() const
  {
    return m_size;
  }
  void* Data() const
  {
    return m_mmap.Data();
  }
  int Fd() const
  {
    return m_fd;
  }

private:
  CSharedMemory(CSharedMemory const& other) = delete;
  CSharedMemory& operator=(CSharedMemory const& other) = delete;

  CFileHandle Open();
  CFileHandle OpenMemfd();
  CFileHandle OpenShm();

  std::size_t m_size;
  CFileHandle m_fd;
  CMmap m_mmap;
};

}
}
}