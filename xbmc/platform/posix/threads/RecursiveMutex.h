/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <mutex>

#include <pthread.h>
namespace XbmcThreads
{

/*!
 * \brief This class exists purely for the ability to
 *        set mutex attribute PTHREAD_PRIO_INHERIT.
 *        Currently there is no way to set this using
 *        std::recursive_mutex.
 *
 */
class CRecursiveMutex
{
private:
  pthread_mutex_t m_mutex;

  static pthread_mutexattr_t& getRecursiveAttr();

public:
  CRecursiveMutex(const CRecursiveMutex&) = delete;
  CRecursiveMutex& operator=(const CRecursiveMutex&) = delete;

  inline CRecursiveMutex() { pthread_mutex_init(&m_mutex, &getRecursiveAttr()); }

  inline ~CRecursiveMutex() { pthread_mutex_destroy(&m_mutex); }

  inline void lock() { pthread_mutex_lock(&m_mutex); }

  inline void unlock() { pthread_mutex_unlock(&m_mutex); }

  inline bool try_lock() { return (pthread_mutex_trylock(&m_mutex) == 0); }

  inline std::recursive_mutex::native_handle_type native_handle() { return &m_mutex; }
};

} // namespace XbmcThreads
