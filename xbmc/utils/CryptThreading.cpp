/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifdef TARGET_WINDOWS
#error "The threading options for the cryptography libraries don't need to be and shouldn't be set on Windows. Do not include CryptThreading in your windows project."
#endif

#include "CryptThreading.h"
#include "threads/Thread.h"
#include "utils/log.h"

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#else
#define HAVE_OPENSSL
#endif

#ifdef HAVE_OPENSSL
#include <openssl/crypto.h>
#endif

#ifdef HAVE_GCRYPT
#include <gcrypt.h>
#include <errno.h>

GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

/* ========================================================================= */
/* openssl locking implementation for curl */
static CCriticalSection* getlock(int index)
{
  return g_cryptThreadingInitializer.get_lock(index);
}

static void lock_callback(int mode, int type, const char* file, int line)
{
  if (mode & 0x01 /* CRYPTO_LOCK from openssl/crypto.h */ )
    getlock(type)->lock();
  else
    getlock(type)->unlock();
}

static unsigned long thread_id()
{
  return (unsigned long)CThread::GetCurrentThreadId();
}
/* ========================================================================= */

CryptThreadingInitializer::CryptThreadingInitializer()
{
  bool attemptedToSetSSLMTHook = false;
#ifdef HAVE_OPENSSL
  // set up OpenSSL
  numlocks = CRYPTO_num_locks();
  CRYPTO_set_id_callback(thread_id);
  CRYPTO_set_locking_callback(lock_callback);
  attemptedToSetSSLMTHook = true;
#else
  numlocks = 1;
#endif

  locks = new CCriticalSection*[numlocks];
  for (int i = 0; i < numlocks; i++)
    locks[i] = NULL;

#ifdef HAVE_GCRYPT
  // set up gcrypt
  gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
  attemptedToSetSSLMTHook = true;
#endif

  if (!attemptedToSetSSLMTHook)
    CLog::Log(LOGWARNING, "Could not determine the libcurl security library to set the locking scheme. This may cause problem with multithreaded use of ssl or libraries that depend on it (libcurl).");
  
}

CryptThreadingInitializer::~CryptThreadingInitializer()
{
  CSingleLock l(locksLock);
#ifdef HAVE_OPENSSL
  CRYPTO_set_locking_callback(NULL);
#endif

  for (int i = 0; i < numlocks; i++)
    delete locks[i]; // I always forget ... delete is NULL safe.

  delete [] locks;
}

CCriticalSection* CryptThreadingInitializer::get_lock(int index)
{
  CSingleLock l(locksLock);
  CCriticalSection* curlock = locks[index];
  if (curlock == NULL)
  {
    curlock = new CCriticalSection();
    locks[index] = curlock;
  }

  return curlock;
}




