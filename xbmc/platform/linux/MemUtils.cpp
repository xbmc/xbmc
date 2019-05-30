/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/MemUtils.h"

#include <cstdlib>

#include <sys/sysinfo.h>

namespace KODI
{
namespace MEMORY
{

void* AlignedMalloc(size_t s, size_t alignTo)
{
  return aligned_alloc(alignTo, s);
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

  struct sysinfo info;
  sysinfo(&info);

  buffer->availPageFile = (info.freeswap * info.mem_unit);
  buffer->availPhys = ((info.freeram + info.bufferram) * info.mem_unit);
  buffer->availVirtual = ((info.freeram + info.bufferram) * info.mem_unit);
  buffer->totalPhys = (info.totalram * info.mem_unit);
  buffer->totalVirtual = (info.totalram * info.mem_unit);
}

}
}
