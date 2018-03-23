/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "CryptThreading.h"
#include "threads/Thread.h"
#include "utils/log.h"

#include <openssl/crypto.h>

#define KODI_OPENSSL_NEEDS_LOCK_CALLBACK (OPENSSL_VERSION_NUMBER < 0x10100000L)

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

unsigned long thread_id()
{
  // C-style cast required due to vastly differing native ID return types
  return (unsigned long)CThread::GetCurrentThreadId();
}

}
#endif

CryptThreadingInitializer::CryptThreadingInitializer()
{
#if KODI_OPENSSL_NEEDS_LOCK_CALLBACK
  // OpenSSL < 1.1 needs integration code to support multi-threading
  // This is absolutely required for libcurl if it uses the OpenSSL backend
  m_locks.resize(CRYPTO_num_locks());
  CRYPTO_set_id_callback(thread_id);
  CRYPTO_set_locking_callback(lock_callback);
#endif
}

CryptThreadingInitializer::~CryptThreadingInitializer()
{
#if KODI_OPENSSL_NEEDS_LOCK_CALLBACK
  CSingleLock l(m_locksLock);
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




