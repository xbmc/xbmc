/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "utils/MemUtils.h"

#include <algorithm>
#include <cstdlib>

#include <emscripten/heap.h>

namespace KODI
{
namespace MEMORY
{

void* AlignedMalloc(size_t s, size_t alignTo)
{
  void* p = nullptr;
  int res = posix_memalign(&p, alignTo, s);
  if (res == EINVAL)
    throw std::runtime_error("Failed to align memory, alignment is not a multiple of 2");
  else if (res == ENOMEM)
    throw std::runtime_error("Failed to align memory, insufficient memory available");
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

  uint64_t heapMax = static_cast<uint64_t>(emscripten_get_heap_max());
  uint64_t heapUsed = static_cast<uint64_t>(emscripten_get_heap_size());
  buffer->totalPhys = heapMax;
  buffer->availPhys = (heapMax > heapUsed) ? (heapMax - heapUsed) : 0;
}

} // namespace MEMORY
} // namespace KODI
