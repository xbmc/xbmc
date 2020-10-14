/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <inttypes.h>
#include <string.h>

#define STREAMCRYPTO_VERSION_LEVEL 1

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  enum CRYPTO_KEY_SYSTEM
  {
    CRYPTO_KEY_SYSTEM_NONE = 0,
    CRYPTO_KEY_SYSTEM_WIDEVINE,
    CRYPTO_KEY_SYSTEM_PLAYREADY,
    CRYPTO_KEY_SYSTEM_WISEPLAY,
    CRYPTO_KEY_SYSTEM_COUNT
  };

  enum CRYPTO_FLAGS
  {
    CRYPTO_FLAG_NONE = 0,

    /// @brief is set in flags if decoding has to be done in TEE environment
    CRYPTO_FLAG_SECURE_DECODER = (1 << 0)
  };

  struct CRYPTO_INFO
  {
    CRYPTO_KEY_SYSTEM
        m_CryptoKeySystem; /*!< @brief keysystem for encrypted media, KEY_SYSTEM_NONE for unencrypted media */
    uint8_t flags;
    uint16_t m_CryptoSessionIdSize; /*!< @brief The size of the crypto session key id */
    const char* m_CryptoSessionId; /*!< @brief The crypto session key id */
  };

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
