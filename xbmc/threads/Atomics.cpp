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

///////////////////////////////////////////////////////////////////////////
// Fast spinlock implmentation. No backoff when busy
///////////////////////////////////////////////////////////////////////////
CAtomicSpinLock::CAtomicSpinLock(std::atomic<long>& lock) : m_Lock(lock)
{
  long expected = 0;
  while (!m_Lock.compare_exchange_strong(expected, 1L)) {} // Lock
}
CAtomicSpinLock::~CAtomicSpinLock()
{
  m_Lock = 0; // Unlock
}
