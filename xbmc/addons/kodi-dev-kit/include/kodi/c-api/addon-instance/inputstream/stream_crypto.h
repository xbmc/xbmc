/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_INPUTSTREAM_STREAMCRYPTO_H
#define C_API_ADDONINSTANCE_INPUTSTREAM_STREAMCRYPTO_H

#include <stdint.h>
#include <string.h>

#define STREAMCRYPTO_VERSION_LEVEL 2

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// @defgroup cpp_kodi_addon_inputstream_Defs_StreamEncryption_STREAM_CRYPTO_KEY_SYSTEM enum STREAM_CRYPTO_KEY_SYSTEM
  /// @ingroup cpp_kodi_addon_inputstream_Defs_StreamEncryption
  /// @brief **Available ways to process stream cryptography**\n
  /// For @ref cpp_kodi_addon_inputstream_Defs_StreamEncryption_StreamCryptoSession,
  /// this defines the used and required auxiliary modules which are required to
  /// process the encryption stream.
  ///
  /// Used to set wanted [digital rights management](https://en.wikipedia.org/wiki/Digital_rights_management)
  /// (DRM) technology provider for stream.
  ///@{
  enum STREAM_CRYPTO_KEY_SYSTEM
  {
    /// @brief **0** - If no path is to be used, this is the default
    STREAM_CRYPTO_KEY_SYSTEM_NONE = 0,

    /// @brief **1** - To use [Widevine](https://en.wikipedia.org/wiki/Widevine) for processing
    STREAM_CRYPTO_KEY_SYSTEM_WIDEVINE,

    /// @brief **2** - To use [Playready](https://en.wikipedia.org/wiki/PlayReady) for processing
    STREAM_CRYPTO_KEY_SYSTEM_PLAYREADY,

    /// @brief **3** - To use Wiseplay for processing
    STREAM_CRYPTO_KEY_SYSTEM_WISEPLAY,

    /// @brief **4** - To use ClearKey for processing
    STREAM_CRYPTO_KEY_SYSTEM_CLEARKEY,

    /// @brief **5** - The maximum value to use in a list.
    STREAM_CRYPTO_KEY_SYSTEM_COUNT
  };
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_inputstream_Defs_StreamEncryption_STREAM_CRYPTO_FLAGS enum STREAM_CRYPTO_FLAGS
  /// @ingroup cpp_kodi_addon_inputstream_Defs_StreamEncryption
  /// @brief **Cryptography flags to use special conditions**\n
  /// To identify special extra conditions.
  ///
  /// @note These variables are bit flags which are created using "|" can be used
  /// together.
  ///
  ///@{
  enum STREAM_CRYPTO_FLAGS
  {
    /// @brief **0000 0000** - Empty to set if nothing is used.
    STREAM_CRYPTO_FLAG_NONE = 0,

    /// @brief **0000 0001** - Is set in flags if decoding has to be done in
    /// [trusted execution environment (TEE)](https://en.wikipedia.org/wiki/Trusted_execution_environment).
    STREAM_CRYPTO_FLAG_SECURE_DECODER = (1 << 0)
  };
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_inputstream_Defs_StreamEncryption_DEMUX_CRYPTO_INFO struct DEMUX_CRYPTO_INFO
  /// @ingroup cpp_kodi_addon_inputstream_Defs_StreamEncryption
  /// @brief **C data structure for processing encrypted streams.**\n
  /// If required, this structure is used for every DEMUX_PACKET to be processed.
  ///
  ///@{
  struct DEMUX_CRYPTO_INFO
  {
    /// @brief Number of subsamples.
    uint16_t numSubSamples;

    /// @brief Flags for later use.
    uint16_t flags;

    /// @brief @ref numSubSamples uint16_t's which define the size of clear size
    /// of a subsample.
    uint16_t* clearBytes;

    /// @brief @ref numSubSamples uint32_t's which define the size of cipher size
    /// of a subsample.
    uint32_t* cipherBytes;

    /// @brief Initialization vector
    uint8_t iv[16];

    /// @brief Key id
    uint8_t kid[16];

    /// @brief Encryption mode
    uint16_t mode;

    /// @brief Crypt blocks - number of blocks to encrypt in sample encryption pattern
    uint8_t cryptBlocks;

    /// @brief Skip blocks - number of blocks to skip in sample encryption pattern
    uint8_t skipBlocks;
  };
  ///@}
  //----------------------------------------------------------------------------

  // Data to manage stream cryptography
  struct STREAM_CRYPTO_SESSION
  {
    // keysystem for encrypted media, STREAM_CRYPTO_KEY_SYSTEM_NONE for unencrypted
    // media.
    //
    // See STREAM_CRYPTO_KEY_SYSTEM for available options.
    enum STREAM_CRYPTO_KEY_SYSTEM keySystem;

    // Flags to use special conditions, see STREAM_CRYPTO_FLAGS for available flags.
    uint8_t flags;

    // The crypto session key id.
    char sessionId[256];
  };
  ///@}
  //----------------------------------------------------------------------------

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_INPUTSTREAM_STREAMCRYPTO_H */
