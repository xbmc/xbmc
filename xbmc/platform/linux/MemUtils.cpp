/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/MemUtils.h"

#include <cstdlib>
#include <fstream>

namespace KODI
{
namespace MEMORY
{

void* AlignedMalloc(size_t s, size_t alignTo)
{
  void* p = nullptr;
  int res = posix_memalign(&p, alignTo, s);
  if (res == EINVAL)
  {
    throw std::runtime_error("Failed to align memory, alignment is not a multiple of 2");
  }
  else if (res == ENOMEM)
  {
    throw std::runtime_error("Failed to align memory, insufficient memory available");
  }
  return p;
}

void AlignedFree(void* p)
{
  if (!p)
    return;

  free(p);
}

void GetMemoryStatus(MemoryStatus* buffer)
{
  if (!buffer)
    return;

  std::ifstream file("/proc/meminfo");

  if (!file.is_open())
    return;

  uint64_t buffers;
  uint64_t cached;
  uint64_t free;
  uint64_t total;
  uint64_t reclaimable;
  uint64_t available = 0;

  std::string token;

  while (file >> token)
  {
    if (token == "Buffers:")
      file >> buffers;
    if (token == "Cached:")
      file >> cached;
    if (token == "MemFree:")
      file >> free;
    if (token == "MemTotal:")
      file >> total;
    if (token == "SReclaimable:")
      file >> reclaimable;
    if (token == "MemAvailable:")
      file >> available;
  }

  buffer->totalPhys = total * 1024;
  buffer->availPhys = available ? available * 1024 : (free + cached + reclaimable + buffers) * 1024;
}

}
}
