/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <sys/mman.h>

#include <cstddef>

namespace KODI
{
namespace UTILS
{
namespace POSIX
{

/**
 * Wrapper for mapped memory that automatically calls munmap on destruction
 */
class CMmap
{
public:
  /**
   * See mmap(3p) for parameter description
   */
  CMmap(void* addr, std::size_t length, int prot, int flags, int fildes, off_t offset);
  ~CMmap();

  void* Data() const
  {
    return m_memory;
  }
  std::size_t Size() const
  {
    return m_size;
  }

private:
  CMmap(CMmap const& other) = delete;
  CMmap& operator=(CMmap const& other) = delete;

  std::size_t m_size;
  void* m_memory;
};

}
}
}
