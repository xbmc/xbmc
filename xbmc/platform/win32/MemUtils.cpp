/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/MemUtils.h"

#include <malloc.h>
#include <windows.h>

namespace KODI
{
namespace MEMORY
{

void* AlignedMalloc(size_t s, size_t alignTo)
{
  return _aligned_malloc(s, alignTo);
}

void AlignedFree(void* p)
{
  _aligned_free(p);
}

void GetMemoryStatus(MemoryStatus* buffer)
{
  MEMORYSTATUSEX memory;
  memory.dwLength = sizeof(memory);
  GlobalMemoryStatusEx(&memory);

  buffer->totalPhys = memory.ullTotalPhys;
  buffer->availPhys = memory.ullAvailPhys;
}

}
}
