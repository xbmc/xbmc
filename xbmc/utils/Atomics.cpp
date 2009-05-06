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

#ifdef WIN32
#include <stdafx.h>
#endif
#include "Atomics.h"

///////////////////////////////////////////////////////////////////////////
// 32-bit atomic compare-and-swap
// Returns previous value of *pAddr
///////////////////////////////////////////////////////////////////////////
#ifdef __ppc__ // PowerPC

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
#ifdef __ppc__ // PowerPC

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

long long cas2(volatile long long* pAddr, long long expectedVal, long long swapVal)
{
  long long prev;
  
  __asm__ volatile (
                        " push %%ebx        \n"  // We have to manually handle ebx, because PIC uses it and the compiler refuses to build anything that touches it
                        " mov %4, %%ebx     \n" // TODO: Consider #if __PIC__ to prevent other ebx issues (since we touch it but don't tell the compiler)
                        " lock/cmpxchg8b %3 \n"
                        " pop %%ebx"
                        : "=A" (prev)
                        : "c" ((unsigned long)(swapVal >> 32)), "0" (expectedVal), "m" (*pAddr), "m" (swapVal)
                        : "memory");
  return prev;
}

#endif

///////////////////////////////////////////////////////////////////////////
// 32-bit atomic increment
// Returns new value of *pAddr
///////////////////////////////////////////////////////////////////////////
#ifdef __ppc__ // PowerPC

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

#else // Linux / OSX86 (GCC)

long AtomicIncrement(volatile long* pAddr)
{
  register long reg __asm__ ("eax") = 1;
  __asm__ __volatile__ (
                        "lock/xaddl %0, %1 \n"
                        "incl %%eax"
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
#ifdef __ppc__ // PowerPC

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

#else // Linux / OSX86 (GCC)

long AtomicDecrement(volatile long* pAddr)
{
  register long reg __asm__ ("eax") = -1;
  __asm__ __volatile__ (
                        "lock/xaddl %0, %1 \n"
                        "decl %%eax"
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
  while (cas(&m_Lock, 0, 1) != 0); // Lock
}
CAtomicSpinLock::~CAtomicSpinLock()
{
  m_Lock = 0; // Unlock
}
