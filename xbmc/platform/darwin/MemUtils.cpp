/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/MemUtils.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <stdio.h>

#include <mach/mach.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>

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

  uint64_t physmem;
  size_t len = sizeof physmem;

#if defined(__apple_build_version__) && __apple_build_version__ < 10000000
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif
  std::array<int, 2> mib =
  {
    CTL_HW,
    HW_MEMSIZE,
  };
#if defined(__apple_build_version__) && __apple_build_version__ < 10000000
#pragma clang diagnostic pop
#endif

  // Total physical memory.
  if (sysctl(mib.data(), mib.size(), &physmem, &len, nullptr, 0) == 0 && len == sizeof(physmem))
    buffer->totalPhys = physmem;

  // In use.
  mach_port_t stat_port = mach_host_self();
  vm_statistics_data_t vm_stat;
  mach_msg_type_number_t count = sizeof(vm_stat) / sizeof(natural_t);
  if (host_statistics(stat_port, HOST_VM_INFO, reinterpret_cast<host_info_t>(&vm_stat), &count) == 0)
  {
    // Find page size.
#if defined(TARGET_DARWIN_IOS)
    // on ios with 64bit ARM CPU the page size is wrongly given as 16K
    // when using the sysctl approach. We can use the host_page_size
    // function instead which will give the proper 4k pagesize
    // on both 32 and 64 bit ARM CPUs
    vm_size_t pageSize;
    host_page_size(stat_port, &pageSize);
#else
    int pageSize;
    mib[0] = CTL_HW;
    mib[1] = HW_PAGESIZE;
    len = sizeof(int);
    if (sysctl(mib.data(), mib.size(), &pageSize, &len, nullptr, 0) == 0)
#endif
    {
      uint64_t used = (vm_stat.active_count + vm_stat.inactive_count + vm_stat.wire_count) * pageSize;
      buffer->availPhys = buffer->totalPhys - used;
    }
  }
}

}
}
