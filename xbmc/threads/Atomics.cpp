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
// Fast spinlock implementation. No backoff when busy
///////////////////////////////////////////////////////////////////////////
CAtomicSpinLock::CAtomicSpinLock(std::atomic_flag& lock) : m_Lock(lock)
{
  while (atomic_flag_test_and_set(&m_Lock)) {} // Lock
}

CAtomicSpinLock::~CAtomicSpinLock()
{
  std::atomic_flag_clear(&m_Lock); // Unlock
}
