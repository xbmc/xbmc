/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Atomics.h"

///////////////////////////////////////////////////////////////////////////
// 32-bit atomic compare-and-swap
// Returns previous value of *pAddr
///////////////////////////////////////////////////////////////////////////
#if defined(__ppc__) || defined(__powerpc__) // PowerPC

long cas(volatile long *pAddr, long expectedVal, long swapVal)
{
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
}

#elif defined(WIN32)

long cas(volatile long* pAddr, long expectedVal, long swapVal)
{
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
}

#elif defined(__arm__)

long cas(volatile long* pAddr, long expectedVal, long swapVal)
{
  // __sync_val_compare_and_swap -> GCC >= 401
  return __sync_val_compare_and_swap(pAddr, expectedVal, swapVal); 
}

#else // Linux / OSX86 (GCC)

long cas(volatile long* pAddr,long expectedVal, long swapVal)
{
  long prev;

  __asm__ __volatile__ (
                        "lock/cmpxchg %1, %2"
                        : "=a" (prev)
                        : "r" (swapVal), "m" (*pAddr), "0" (expectedVal)
                        : "memory" );
  return prev;

}

#endif

///////////////////////////////////////////////////////////////////////////
// 64-bit atomic compare-and-swap
// Returns previous value of *pAddr
///////////////////////////////////////////////////////////////////////////
#if defined(__ppc__) || defined(__powerpc__) || defined(__arm__)  // PowerPC & ARM

// Not available

#elif defined(WIN32)

long long cas2(volatile long long* pAddr, long long expectedVal, long long swapVal)
{
  long long prev;

  __asm
  {
    mov esi, pAddr ;
    mov eax, dword ptr [expectedVal] ;
    mov edx, dword ptr expectedVal[4] ;
    mov ebx, dword ptr [swapVal] ;
    mov ecx, dword ptr swapVal[4] ;
    lock cmpxchg8b qword ptr [esi] ;
    mov dword ptr [prev], eax ;
    mov dword ptr prev[4], edx ;
  }

  return prev;
}

#else // Linux / OSX86 (GCC)
#if !defined (__x86_64)
long long cas2(volatile long long* pAddr, long long expectedVal, long long swapVal)
{
  long long prev;

  __asm__ volatile (
                        " push %%ebx        \n"  // We have to manually handle ebx, because PIC uses it and the compiler refuses to build anything that touches it
                        " mov %4, %%ebx     \n"
                        " lock/cmpxchg8b (%%esi) \n"
                        " pop %%ebx"
                        : "=A" (prev)
                        : "c" ((unsigned long)(swapVal >> 32)), "0" (expectedVal), "S" (pAddr), "m" (swapVal)
                        : "memory");
  return prev;
}
#else
// Hack to allow compilation on x86_64
long long cas2(volatile long long* pAddr, long long expectedVal, long long swapVal)
{
  throw "cas2 is not implemented on x86_64!";
}
#endif // !defined (__x86_64)
#endif

///////////////////////////////////////////////////////////////////////////
// 32-bit atomic increment
// Returns new value of *pAddr
///////////////////////////////////////////////////////////////////////////
#if defined(__ppc__) || defined(__powerpc__) // PowerPC

long AtomicIncrement(volatile long* pAddr)
{
  long val;

  __asm__ __volatile__ (
                        "sync             \n"
                     "1: lwarx  %0, 0, %1 \n"
                        "addic  %0, %0, 1 \n"
                        "stwcx. %0, 0, %1 \n"
                        "bne-   1b        \n"
                        "isync"
                        : "=&r" (val)
                        : "r" (pAddr)
                        : "cc", "xer", "memory");
  return val;
}

#elif defined(WIN32)

long AtomicIncrement(volatile long* pAddr)
{
  long val;
  __asm
  {
    mov eax, pAddr ;
    lock inc dword ptr [eax] ;
    mov eax, [eax] ;
    mov val, eax ;
  }
  return val;
}

#elif defined(__arm__)

long AtomicIncrement(volatile long* pAddr)
{
  // __sync_fetch_and_add -> GCC >= 401
  return __sync_fetch_and_add(pAddr, 1);
}

#else // Linux / OSX86 (GCC)

long AtomicIncrement(volatile long* pAddr)
{
  register long reg __asm__ ("eax") = 1;
  __asm__ __volatile__ (
                        "lock/xadd %0, %1 \n"
                        "inc %%eax"
                        : "+r" (reg)
                        : "m" (*pAddr)
                        : "memory" );
  return reg;
}


#endif

///////////////////////////////////////////////////////////////////////////
// 32-bit atomic add
// Returns new value of *pAddr
///////////////////////////////////////////////////////////////////////////

#if defined(__ppc__) || defined(__powerpc__) // PowerPC

long AtomicAdd(volatile long* pAddr, long amount)
{
  long val;

  __asm__ __volatile__ (
                        "sync             \n"
                        "1: lwarx  %0, 0, %1 \n"
                        "add  %0, %2, %0 \n"
                        "stwcx. %0, 0, %1 \n"
                        "bne-   1b        \n"
                        "isync"
                        : "=&r" (val)
                        : "r" (pAddr), "r" (amount)
                        : "cc", "memory");
  return val;
}

#elif defined(WIN32)

long AtomicAdd(volatile long* pAddr, long amount)
{
  __asm
  {
    mov eax, amount;
    mov ebx, pAddr;
    lock xadd dword ptr [ebx], eax;
    mov ebx, [ebx];
    mov amount, ebx;
  }
  return amount;
}

#elif defined(__arm__)

long AtomicAdd(volatile long* pAddr, long amount)
{
  // TODO: ARM Assembler
  return 0;
}

#else // Linux / OSX86 (GCC)

long AtomicAdd(volatile long* pAddr, long amount)
{
  register long reg __asm__ ("eax") = amount;
  __asm__ __volatile__ (
                        "lock/xadd %0, %1 \n"
                        "dec %%eax"
                        : "+r" (reg)
                        : "m" (*pAddr)
                        : "memory" );
  return reg;
}

#endif

///////////////////////////////////////////////////////////////////////////
// 32-bit atomic decrement
// Returns new value of *pAddr
///////////////////////////////////////////////////////////////////////////
#if defined(__ppc__) || defined(__powerpc__) // PowerPC

long AtomicDecrement(volatile long* pAddr)
{
  long val;

  __asm__ __volatile__ (
                        "sync                \n"
                     "1: lwarx  %0, 0, %1    \n"
                        "addic  %0, %0, -1   \n"
                        "stwcx. %0, 0, %1    \n"
                        "bne-   1b           \n"
                        "isync"
                        : "=&r" (val)
                        : "r" (pAddr)
                        : "cc", "xer", "memory");
  return val;
}


#elif defined(WIN32)

long AtomicDecrement(volatile long* pAddr)
{
  long val;
  __asm
  {
    mov eax, pAddr ;
    lock dec dword ptr [eax] ;
    mov eax, [eax] ;
    mov val, eax ;
  }
  return val;
}

#elif defined(__arm__)

long AtomicDecrement(volatile long* pAddr)
{
  // __sync_fetch_and_add -> GCC >= 401
  return __sync_fetch_and_add(pAddr, -1);
}

#else // Linux / OSX86 (GCC)

long AtomicDecrement(volatile long* pAddr)
{
  register long reg __asm__ ("eax") = -1;
  __asm__ __volatile__ (
                        "lock/xadd %0, %1 \n"
                        "dec %%eax"
                        : "+r" (reg)
                        : "m" (*pAddr)
                        : "memory" );
  return reg;
}


#endif

///////////////////////////////////////////////////////////////////////////
// 32-bit atomic subtract
// Returns new value of *pAddr
///////////////////////////////////////////////////////////////////////////
#if defined(__ppc__) || defined(__powerpc__) // PowerPC

long AtomicSubtract(volatile long* pAddr, long amount)
{
  long val;
  amount *= -1;

  __asm__ __volatile__ (
                        "sync             \n"
                        "1: lwarx  %0, 0, %1 \n"
                        "add  %0, %2, %0 \n"
                        "stwcx. %0, 0, %1 \n"
                        "bne-   1b        \n"
                        "isync"
                        : "=&r" (val)
                        : "r" (pAddr), "r" (amount)
                        : "cc", "memory");
  return val;
}

#elif defined(WIN32)

long AtomicSubtract(volatile long* pAddr, long amount)
{
  amount *= -1;
  __asm
  {
    mov eax, amount;
    mov ebx, pAddr;
    lock xadd dword ptr [ebx], eax;
    mov ebx, [ebx];
    mov amount, ebx;
  }
  return amount;
}

#elif defined(__arm__)

long AtomicSubtract(volatile long* pAddr, long amount)
{
  // TODO: ARM Assembler
  return 0;
}

#else // Linux / OSX86 (GCC)

long AtomicSubtract(volatile long* pAddr, long amount)
{
  register long reg __asm__ ("eax") = -1 * amount;
  __asm__ __volatile__ (
                        "lock/xadd %0, %1 \n"
                        "dec %%eax"
                        : "+r" (reg)
                        : "m" (*pAddr)
                        : "memory" );
  return reg;
}

#endif

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
