/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CryptThreading.h"
#if (OPENSSL_VERSION_NUMBER < 0x10100000L)

#include "threads/Thread.h"
#include "utils/log.h"

#include <atomic>

namespace
{

CCriticalSection* getlock(int index)
{
  return g_cryptThreadingInitializer.GetLock(index);
}

void lock_callback(int mode, int type, const char* file, int line)
{
  if (mode & CRYPTO_LOCK)
    getlock(type)->lock();
  else
    getlock(type)->unlock();
}

unsigned long GetCryptThreadId()
{
  static std::atomic<unsigned long> tidSequence{0};
  static thread_local unsigned long tidTl{0};

  if (tidTl == 0)
    tidTl = ++tidSequence;
  return tidTl;
}

void thread_id(CRYPTO_THREADID* tid)
{
  // C-style cast required due to vastly differing native ID return types
  CRYPTO_THREADID_set_numeric(tid, GetCryptThreadId());
}

}

CryptThreadingInitializer::CryptThreadingInitializer()
{
  // OpenSSL < 1.1 needs integration code to support multi-threading
  // This is absolutely required for libcurl if it uses the OpenSSL backend
  m_locks.resize(CRYPTO_num_locks());
  CRYPTO_THREADID_set_callback(thread_id);
  CRYPTO_set_locking_callback(lock_callback);
}

CryptThreadingInitializer::~CryptThreadingInitializer()
{
  CSingleLock l(m_locksLock);
  CRYPTO_set_locking_callback(nullptr);
  m_locks.clear();
}

CCriticalSection* CryptThreadingInitializer::GetLock(int index)
{
  CSingleLock l(m_locksLock);
  auto& curlock = m_locks[index];
  if (!curlock)
  {
    curlock.reset(new CCriticalSection());
  }

  return curlock.get();
}

unsigned long CryptThreadingInitializer::GetCurrentCryptThreadId()
{
  return GetCryptThreadId();
}

#endif
