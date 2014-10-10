#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "XMemUtils.h"
#include "Util.h"

#if defined(TARGET_DARWIN)
#include <mach/mach.h>
#endif

#undef ALIGN
#define ALIGN(value, alignment) (((value)+(alignment-1))&~(alignment-1))

// aligned memory allocation.
// in order to do so - we alloc extra space and store the original allocation in it (so that we can free later on).
// the returned address will be the nearest alligned address within the space allocated.
void *_aligned_malloc(size_t s, size_t alignTo) {

  char *pFull = (char*)malloc(s + alignTo + sizeof(char *));
  char *pAlligned = (char *)ALIGN(((unsigned long)pFull + sizeof(char *)), alignTo);

  *(char **)(pAlligned - sizeof(char*)) = pFull;

  return(pAlligned);
}

void _aligned_free(void *p) {
  if (!p)
    return;

  char *pFull = *(char **)(((char *)p) - sizeof(char *));
  free(pFull);
}

#ifndef TARGET_WINDOWS

#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN) && !defined(TARGET_FREEBSD)
static FILE* procMeminfoFP = NULL;
#endif

void GlobalMemoryStatusEx(LPMEMORYSTATUSEX lpBuffer)
{
  if (!lpBuffer)
    return;

  memset(lpBuffer, 0, sizeof(MEMORYSTATUSEX));
  lpBuffer->dwLength = sizeof(MEMORYSTATUSEX);

#if defined(TARGET_DARWIN)
  uint64_t physmem;
  size_t len = sizeof physmem;
  int mib[2] = { CTL_HW, HW_MEMSIZE };
  size_t miblen = ARRAY_SIZE(mib);

  // Total physical memory.
  if (sysctl(mib, miblen, &physmem, &len, NULL, 0) == 0 && len == sizeof (physmem))
      lpBuffer->ullTotalPhys = physmem;

  // Virtual memory.
  mib[0] = CTL_VM; mib[1] = VM_SWAPUSAGE;
  struct xsw_usage swap;
  len = sizeof(struct xsw_usage);
  if (sysctl(mib, miblen, &swap, &len, NULL, 0) == 0)
  {
      lpBuffer->ullAvailPageFile = swap.xsu_avail;
      lpBuffer->ullTotalVirtual = lpBuffer->ullTotalPhys + swap.xsu_total;
  }

  // In use.
  mach_port_t stat_port = mach_host_self();
  vm_statistics_data_t vm_stat;
  mach_msg_type_number_t count = sizeof(vm_stat) / sizeof(natural_t);
  if (host_statistics(stat_port, HOST_VM_INFO, (host_info_t)&vm_stat, &count) == 0)
  {
      // Find page size.
      int pageSize;
      mib[0] = CTL_HW; mib[1] = HW_PAGESIZE;
      len = sizeof(int);
      if (sysctl(mib, miblen, &pageSize, &len, NULL, 0) == 0)
      {
          uint64_t used = (vm_stat.active_count + vm_stat.inactive_count + vm_stat.wire_count) * pageSize;

          lpBuffer->ullAvailPhys = lpBuffer->ullTotalPhys - used;
          lpBuffer->ullAvailVirtual  = lpBuffer->ullAvailPhys; // FIXME.
      }
  }
#elif defined(TARGET_FREEBSD)
  /* sysctl hw.physmem */
  size_t physmem = 0, mem_free = 0, pagesize = 0, swap_free = 0;
  size_t mem_avail = 0, mem_inactive = 0, mem_cache = 0, len = 0;

  /* physmem */
  len = sizeof(physmem);
  if (sysctlbyname("hw.physmem", &physmem, &len, NULL, 0) == 0) {
    lpBuffer->ullTotalPhys = physmem;
    lpBuffer->ullTotalVirtual = physmem;
  }
  /* pagesize */
  len = sizeof(pagesize);
  if (sysctlbyname("hw.pagesize", &pagesize, &len, NULL, 0) != 0)
    pagesize = 4096;
  /* mem_inactive */
  len = sizeof(mem_inactive);
  if (sysctlbyname("vm.stats.vm.v_inactive_count", &mem_inactive, &len, NULL, 0) == 0)
    mem_inactive *= pagesize;
  /* mem_cache */
  len = sizeof(mem_cache);
  if (sysctlbyname("vm.stats.vm.v_cache_count", &mem_cache, &len, NULL, 0) == 0)
    mem_cache *= pagesize;
  /* mem_free */
  len = sizeof(mem_free);
  if (sysctlbyname("vm.stats.vm.v_free_count", &mem_free, &len, NULL, 0) == 0)
    mem_free *= pagesize;

  /* mem_avail = mem_inactive + mem_cache + mem_free */
  lpBuffer->ullAvailPhys = mem_inactive + mem_cache + mem_free;
  lpBuffer->ullAvailVirtual = mem_inactive + mem_cache + mem_free;

  if (sysctlbyname("vm.stats.vm.v_swappgsout", &swap_free, &len, NULL, 0) == 0)
    lpBuffer->ullAvailPageFile = swap_free * pagesize;
#else
  struct sysinfo info;
  char name[32];
  unsigned val;
  if (!procMeminfoFP && (procMeminfoFP = fopen("/proc/meminfo", "r")) == NULL)
    sysinfo(&info);
  else
  {
    memset(&info, 0, sizeof(struct sysinfo));
    info.mem_unit = 4096;
    while (fscanf(procMeminfoFP, "%31s %u%*[^\n]\n", name, &val) != EOF)
    {
      if (strncmp("MemTotal:", name, 9) == 0)
        info.totalram = val/4;
      else if (strncmp("MemFree:", name, 8) == 0)
        info.freeram = val/4;
      else if (strncmp("Buffers:", name, 8) == 0)
        info.bufferram += val/4;
      else if (strncmp("Cached:", name, 7) == 0)
        info.bufferram += val/4;
      else if (strncmp("SwapTotal:", name, 10) == 0)
        info.totalswap = val/4;
      else if (strncmp("SwapFree:", name, 9) == 0)
        info.freeswap = val/4;
      else if (strncmp("HighTotal:", name, 10) == 0)
        info.totalhigh = val/4;
      else if (strncmp("HighFree:", name, 9) == 0)
        info.freehigh = val/4;
    }
    rewind(procMeminfoFP);
    fflush(procMeminfoFP);
  }
  lpBuffer->dwLength        = sizeof(MEMORYSTATUSEX);
  lpBuffer->ullAvailPageFile = (info.freeswap * info.mem_unit);
  lpBuffer->ullAvailPhys     = ((info.freeram + info.bufferram) * info.mem_unit);
  lpBuffer->ullAvailVirtual  = ((info.freeram + info.bufferram) * info.mem_unit);
  lpBuffer->ullTotalPhys     = (info.totalram * info.mem_unit);
  lpBuffer->ullTotalVirtual  = (info.totalram * info.mem_unit);
#endif
}

#endif

