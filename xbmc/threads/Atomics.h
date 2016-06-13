#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

//! @todo Inline these methods
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

