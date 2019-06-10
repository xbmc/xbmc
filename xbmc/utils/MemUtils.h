/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <memory>

namespace KODI
{
namespace MEMORY
{
struct MemoryStatus
{
  unsigned int memoryLoad;

  uint64_t totalPhys;
  uint64_t availPhys;
};

void* AlignedMalloc(size_t s, size_t alignTo);
void AlignedFree(void* p);
void GetMemoryStatus(MemoryStatus* buffer);
}
}
