/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileHandle.h"
#include "Mmap.h"

#include <memory>

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
