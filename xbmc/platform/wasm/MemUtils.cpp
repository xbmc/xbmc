/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/MemUtils.h"

#include "platform/wasm/TizenWebApis.h"

#include <chrono>
#include <cstdlib>

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

  // Avoid querying the TV on every GUI frame without locking across a main-thread proxy call.
  thread_local MemoryStatus status{};
  thread_local std::chrono::steady_clock::time_point nextUpdate{};
  const auto now = std::chrono::steady_clock::now();
  if (now >= nextUpdate)
  {
    status = KODI::PLATFORM::WASM::CTizenWebApis::GetMemoryStatus();
    nextUpdate = now + std::chrono::seconds(1);
  }
  *buffer = status;
}

} // namespace MEMORY
} // namespace KODI
