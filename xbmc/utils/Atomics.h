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

// TODO: Inline these methods
long cas(volatile long *pAddr, long expectedVal, long swapVal);
#if !defined(__ppc__) && !defined(__powerpc__) && !defined(__arm__)
long long cas2(volatile long long* pAddr, long long expectedVal, long long swapVal);
#endif
long AtomicIncrement(volatile long* pAddr);
long AtomicDecrement(volatile long* pAddr);
long AtomicAdd(volatile long* pAddr, long amount);
long AtomicSubtract(volatile long* pAddr, long amount);

class CAtomicSpinLock
{
public:
  CAtomicSpinLock(long& lock);
  ~CAtomicSpinLock();
private:
  long& m_Lock;
};


#endif // __ATOMICS_H__

