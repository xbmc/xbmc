/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CryptThreading.h"
#include "threads/Thread.h"
#include "utils/log.h"

#include <openssl/crypto.h>

//! @todo Remove support for OpenSSL <1.0 in v19

#define KODI_OPENSSL_NEEDS_LOCK_CALLBACK (OPENSSL_VERSION_NUMBER < 0x10100000L)
#define KODI_OPENSSL_USE_THREADID (OPENSSL_VERSION_NUMBER >= 0x10000000L)

#if KODI_OPENSSL_NEEDS_LOCK_CALLBACK
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

#if KODI_OPENSSL_USE_THREADID
void thread_id(CRYPTO_THREADID* tid)
{
  // C-style cast required due to vastly differing native ID return types
  CRYPTO_THREADID_set_numeric(tid, (unsigned long)CThread::GetCurrentThreadId());
}
#else
unsigned long thread_id()
{
  // C-style cast required due to vastly differing native ID return types
  return (unsigned long)CThread::GetCurrentThreadId();
}
#endif

}
#endif

CryptThreadingInitializer::CryptThreadingInitializer()
{
#if KODI_OPENSSL_NEEDS_LOCK_CALLBACK
  // OpenSSL < 1.1 needs integration code to support multi-threading
  // This is absolutely required for libcurl if it uses the OpenSSL backend
  m_locks.resize(CRYPTO_num_locks());
#if KODI_OPENSSL_USE_THREADID
  CRYPTO_THREADID_set_callback(thread_id);
#else
  CRYPTO_set_id_callback(thread_id);
#endif
  CRYPTO_set_locking_callback(lock_callback);
#endif
}

CryptThreadingInitializer::~CryptThreadingInitializer()
{
#if KODI_OPENSSL_NEEDS_LOCK_CALLBACK
  CSingleLock l(m_locksLock);
#if !KODI_OPENSSL_USE_THREADID
  CRYPTO_set_id_callback(nullptr);
#endif
  CRYPTO_set_locking_callback(nullptr);
  m_locks.clear();
#endif
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




