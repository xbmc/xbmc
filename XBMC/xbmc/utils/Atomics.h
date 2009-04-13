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

#ifndef __ATOMICS_H__
#define __ATOMICS_H__

#include <queue>

///////////////////////////////////////////////////////////////////////////
// 32-bit atomic compare-and-swap
// Returns previous value of *pAddr
///////////////////////////////////////////////////////////////////////////
#ifdef __ppc__ // PowerPC
static inline long cas(volatile long *pAddr, long expectedVal, long swapVal)
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

static inline long cas(volatile long* pAddr,long expectedVal, long swapVal)
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

static inline long cas(volatile long* pAddr,long expectedVal, long swapVal)
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

#elif defined(WIN32)

#else // Linux / OSX86 (GCC)

static inline long long cas2(volatile long long* pAddr, long long expectedVal, long long swapVal)
{
  long long prev;
  
  __asm__ __volatile__ (
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

#elif defined(WIN32)

#else // Linux / OSX86 (GCC)

static inline long AtomicIncrement(volatile long* pAddr)
{
  register long reg __asm__ ("eax") = 1;
  __asm__ __volatile__ (
                        "lock xaddl %0, %1 \n"
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

#elif defined(WIN32)

#else // Linux / OSX86 (GCC)

static inline long AtomicDecrement(volatile long* pAddr)
{
  register long reg __asm__ ("eax") = -1;
  __asm__ __volatile__ (
                        "lock xaddl %0, %1 \n"
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
#if defined (DEBUG)
      unsigned int spinCount = 0;
      while (cas(&m_Lock, 0, 1) != 0)
        spinCount++;
      
      if (spinCount > 10000)
        CLog::Log(LOGDEBUG, "CAtomicSpinLock: Spinning: spinCount = %u", spinCount);
#else
      while (cas(&m_Lock, 0, 1) != 0); // Lock
#endif
    }
    ~CAtomicSpinLock()
    {
      m_Lock = 0; // Unlock
    }
  private:
    long& m_Lock;
  };

///////////////////////////////////////////////////////////////////////////////////
// Trivial threadsafe queue implemented using a single lock for synchronization
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



#endif // __ATOMICS_H__

