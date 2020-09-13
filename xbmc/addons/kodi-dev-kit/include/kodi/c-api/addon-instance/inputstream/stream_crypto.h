/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifndef C_API_ADDONINSTANCE_INPUTSTREAM_STREAMCRYPTO_H
#define C_API_ADDONINSTANCE_INPUTSTREAM_STREAMCRYPTO_H

#include <stdint.h>
#include <string.h>

#define STREAMCRYPTO_VERSION_LEVEL 1

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  enum STREAM_CRYPTO_KEY_SYSTEM
  {
    STREAM_CRYPTO_KEY_SYSTEM_NONE = 0,
    STREAM_CRYPTO_KEY_SYSTEM_WIDEVINE,
    STREAM_CRYPTO_KEY_SYSTEM_PLAYREADY,
    STREAM_CRYPTO_KEY_SYSTEM_WISEPLAY,
    STREAM_CRYPTO_KEY_SYSTEM_COUNT
  };

  enum STREAM_CRYPTO_FLAGS
  {
    STREAM_CRYPTO_FLAG_NONE = 0,

    /// @brief is set in flags if decoding has to be done in TEE environment
    STREAM_CRYPTO_FLAG_SECURE_DECODER = (1 << 0)
  };

  struct STREAM_CRYPTO_SESSION
  {
    enum STREAM_CRYPTO_KEY_SYSTEM keySystem; /*!< @brief keysystem for encrypted media,
                                                KEY_SYSTEM_NONE for unencrypted media */
    uint8_t flags;
    uint16_t sessionIdSize; /*!< @brief The size of the crypto session key id */
    const char* sessionId; /*!< @brief The crypto session key id */
  };

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_INPUTSTREAM_STREAMCRYPTO_H */
