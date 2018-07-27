/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
