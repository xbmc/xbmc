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

#include <stdafx.h>
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
                        "  loop:   lwarx   %0,0,%2  \n" /* Load the current value of *pAddr,  (%2) into prev (%0) and lock pAddr,  */
                        "          cmpw    0,%0,%3  \n" /* Verify that the current value (%2) == old value (%3) */
                        "          bne     exit     \n" /* Bail if the two values are not equal [not as expected] */
                        "          stwcx.  %4,0,%2  \n" /* Attempt to store swapVal (%4) value into *pAddr (%2) [p must still be reserved] */
                        "          bne-    loop     \n" /* Loop if p was no longer reserved */
                        "          sync             \n" /* Reconcile multiple processors [if present] */
                        "  exit:                    \n"
                        : "=&r" (prev), "=m" (*pAddr)   /* Outputs [prev, *pAddr] */
                        : "r" (pAddr), "r" (expectedVal), "r" (swapVal), "m" (*pAddr) /* Inputs [pAddr, expectedVal, swapVal, *pAddr] */
                        : "cc", "memory");             /* Clobbers */
  
  return prev;
}

#elif defined(WIN32)

long cas(volatile long* pAddr,long expectedVal, long swapVal)
{
  long prev;
  
  __asm
  {
    // Load parameters
    mov eax, expectedVal ;
    mov ebx, pAddr ;
    mov ecx, swapVal ;
    
    // Do Swap
    lock cmpxchg [ebx], ecx ;
    
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

// OSAtomicCompareAndSwap64
// OSAtomicCompareAndSwap64Barrier
/*

compare_and_swap64b:        // bool OSAtomicCompareAndSwapBarrier64( int64_t old, int64_t new, int64_t *value);
lwsync                      // write barrier, NOP'd on a UP
1:
    ldarx   r7,0,r5
    cmpld   r7,r3
    bne--   2f
    stdcx.  r4,0,r5
    bne--   1b
    isync                       // read barrier, NOP'd on a UP
    li              r3,1
    blr
2:
    li              r8,-8                           // on 970, must release reservation
    li              r3,0                            // return failure
    stdcx.  r4,r8,r1                        // store into red zone to release
    blr

 */
#elif defined(WIN32)

#else // Linux / OSX86 (GCC)

long long cas2(volatile long long* pAddr, long long expectedVal, long long swapVal)
{
  long long prev;
  
  __asm__ volatile (
                        " push %%ebx        \n"  // We have to manually handle ebx, because PIC uses it and the compiler refuses to build anything that touches it
                        " mov %4, %%ebx     \n"
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
// OSAtomicIncrement32Barrier
#elif defined(WIN32)

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
// OSAtomicDecrement32Barrier
#elif defined(WIN32)

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
class CAtomicSpinLock
{
public:
  CAtomicSpinLock(long& lock) : m_Lock(lock)
  {
    while (cas(&m_Lock, 0, 1) != 0); // Lock
  }
  ~CAtomicSpinLock()
  {
    m_Lock = 0; // Unlock
  }
private:
  long& m_Lock;
};

///////////////////////////////////////////////////////////////////////////////////
// Trivial threadsafe queue implemented using a single spinlock for synchronization
//////////////////////////////////////////////////////////////////////////////////
template <class T>
class CSafeQueue
  {
  public:
    CSafeQueue(size_t maxItems = 0) : m_Lock(0), m_MaxItems(maxItems) {}
    bool Push(const T& elem)
    {
      CAtomicSpinLock(m_Lock);
      if (m_Queue.size() >= m_MaxItems)
        return false;
      
      m_Queue.Push(elem);
      return true;
    }
    
    void Pop()
    {
      CAtomicSpinLock(m_Lock); 
      m_Queue.pop();
    }
    
    void Clear() 
    {
      CAtomicSpinLock(m_Lock); 
      while (!m_Queue.empty())
        m_Queue.pop();
    }
    
    bool SetMaxItems(size_t maxItems)
    {
      CAtomicSpinLock(m_Lock);
      if (maxItems < m_MaxItems)
        if (maxItems < m_Queue.size())
          return false;
      m_MaxItems = maxItems;
      return true;
    }
    size_t GetMaxItems() {CAtomicSpinLock(m_Lock); return m_MaxItems;}
    T& Head() {CAtomicSpinLock(m_Lock); return m_Queue.front();}
    T& Tail() {CAtomicSpinLock(m_Lock); return m_Queue.back();}
    bool Empty() {CAtomicSpinLock(m_Lock); return m_Queue.empty();}
    size_t Count() {CAtomicSpinLock(m_Lock); return m_Queue.size();}
  protected:
    long m_Lock;
    std::queue<T> m_Queue;
    size_t m_MaxItems;
  };

