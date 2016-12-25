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

#include "Atomics.h"

#if defined(__mips__)
#include "MipsAtomics.h"
pthread_mutex_t cmpxchg_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

///////////////////////////////////////////////////////////////////////////
// 32-bit atomic compare-and-swap
// Returns previous value of *pAddr
///////////////////////////////////////////////////////////////////////////
long cas(volatile long *pAddr, long expectedVal, long swapVal)
{
#if defined(HAS_BUILTIN_SYNC_VAL_COMPARE_AND_SWAP)
  return(__sync_val_compare_and_swap(pAddr, expectedVal, swapVal));
#elif defined(__ppc__) || defined(__powerpc__) // PowerPC
  unsigned int prev;
  __asm__ __volatile__ (
    "  1:      lwarx   %0,0,%2  \n" /* Load the current value of *pAddr(%2) into prev (%0) and lock pAddr,  */
    "          cmpw     0,%0,%3 \n" /* Verify that the current value (%2) == old value (%3) */
    "          bne-     2f      \n" /* Bail if the two values are not equal [not as expected] */
    "          stwcx.  %4,0,%2  \n" /* Attempt to store swapVal (%4) value into *pAddr (%2) [p must still be reserved] */
    "          bne-    1b       \n" /* Loop if p was no longer reserved */
    "          isync            \n" /* Reconcile multiple processors [if present] */
    "  2:                       \n"
    : "=&r" (prev), "+m" (*pAddr)                   /* Outputs [prev, *pAddr] */
    : "r" (pAddr), "r" (expectedVal), "r" (swapVal) /* Inputs [pAddr, expectedVal, swapVal] */
    : "cc", "memory");                              /* Clobbers */
  return prev;

#elif defined(__arm__)
  long prev;
  asm volatile (
    "dmb      ish            \n" // Memory barrier. Make sure all memory accesses appearing before this complete before any that appear after
    "1:                      \n"
    "ldrex    %0, [%1]       \n" // Load the current value of *pAddr(%1) into prev (%0) and lock pAddr,
    "cmp      %0,  %2        \n" // Verify that the current value (%0) == old value (%2)
    "bne      2f             \n" // Bail if the two values are not equal [not as expected]
    "strex    r1,  %3, [%1]  \n"
    "cmp      r1,  #0        \n"
    "bne      1b             \n"
    "dmb      ish            \n" // Memory barrier.
    "2:                      \n"
    : "=&r" (prev)
    : "r"(pAddr), "r"(expectedVal),"r"(swapVal)
    : "r1"
    );
  return prev;

#elif defined(__mips__)
  return cmpxchg32(pAddr, expectedVal, swapVal);

#elif defined(TARGET_WINDOWS)
  long prev;
  __asm
  {
    // Load parameters
    mov eax, expectedVal ;
    mov ebx, pAddr ;
    mov ecx, swapVal ;

    // Do Swap
    lock cmpxchg dword ptr [ebx], ecx ;

    // Store the return value
    mov prev, eax;
  }
  return prev;

#else // Linux / OSX86 (GCC)
  long prev;
  __asm__ __volatile__ (
    "lock/cmpxchg %1, %2"
    : "=a" (prev)
    : "r" (swapVal), "m" (*pAddr), "0" (expectedVal)
    : "memory" );
  return prev;

#endif
}

///////////////////////////////////////////////////////////////////////////
// Fast spinlock implmentation. No backoff when busy
///////////////////////////////////////////////////////////////////////////
CAtomicSpinLock::CAtomicSpinLock(long& lock) : m_Lock(lock)
{
  while (cas(&m_Lock, 0, 1) != 0) {} // Lock
}
CAtomicSpinLock::~CAtomicSpinLock()
{
  m_Lock = 0; // Unlock
}
