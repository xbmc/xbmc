/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/MemUtils.h"

#include <stdlib.h>

#include <androidjni/ActivityManager.h>
#include <androidjni/Context.h>
#include <androidjni/jutils-details.hpp>

namespace KODI
{
namespace MEMORY
{

void* AlignedMalloc(size_t s, size_t alignTo)
{
  void* p;
  posix_memalign(&p, alignTo, s);

  return p;
}

void AlignedFree(void* p)
{
  free(p);
}

void GetMemoryStatus(MemoryStatus* buffer)
{
  if (!buffer)
    return;

  auto activityManager = std::make_unique<CJNIActivityManager>(
      CJNIContext::getSystemService(CJNIContext::ACTIVITY_SERVICE));

  if (activityManager)
  {
    CJNIActivityManager::MemoryInfo info;
    activityManager->getMemoryInfo(info);
    if (xbmc_jnienv()->ExceptionCheck())
    {
      xbmc_jnienv()->ExceptionClear();
      return;
    }

    *buffer = {};
    // The libandroidjni accessors return long, which truncates Java long values on 32-bit Android.
    buffer->totalPhys = static_cast<uint64_t>(jni::get_field<jlong>(info.get_raw(), "totalMem"));
    buffer->availPhys = static_cast<uint64_t>(jni::get_field<jlong>(info.get_raw(), "availMem"));
  }
}

} // namespace MEMORY
} // namespace KODI
