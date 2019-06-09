/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <openssl/crypto.h>

//! @todo - once we're at OpenSSL 1.1 this class and its .cpp file should be deleted.
#if (OPENSSL_VERSION_NUMBER < 0x10100000L)

#include <memory>
#include <vector>
#include "utils/GlobalsHandling.h"
#include "threads/CriticalSection.h"

class CryptThreadingInitializer
{
  std::vector<std::unique_ptr<CCriticalSection>> m_locks;
  CCriticalSection m_locksLock;

public:
  CryptThreadingInitializer();
  ~CryptThreadingInitializer();

  CCriticalSection* GetLock(int index);

  /**
   * This is so testing can reach the thread id generation.
   */
  unsigned long GetCurrentCryptThreadId();

private:
  CryptThreadingInitializer(const CryptThreadingInitializer &rhs) = delete;
  CryptThreadingInitializer& operator=(const CryptThreadingInitializer&) = delete;
};

XBMC_GLOBAL_REF(CryptThreadingInitializer,g_cryptThreadingInitializer);
#define g_cryptThreadingInitializer XBMC_GLOBAL_USE(CryptThreadingInitializer)

#endif
