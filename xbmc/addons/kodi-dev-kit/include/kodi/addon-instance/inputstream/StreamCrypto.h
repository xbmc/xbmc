/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/addon-instance/inputstream/stream_crypto.h"

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

class CInstanceInputStream;
class InputstreamInfo;
class VideoCodecInitdata;

//==============================================================================
/// @defgroup cpp_kodi_addon_inputstream_Defs_StreamEncryption_StreamCryptoSession class StreamCryptoSession
/// @ingroup cpp_kodi_addon_inputstream_Defs_StreamEncryption
/// @brief **Data to manage stream cryptography**\n
/// This class structure manages any encryption values required in order to have
/// them available in their stream processing.
///
/// Used on inputstream by @ref kodi::addon::InputstreamInfo::SetCryptoSession /
/// @ref kodi::addon::InputstreamInfo::GetCryptoSession and are given to the
/// used video codec to decrypt related data.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_inputstream_Defs_Info_StreamCryptoSession_Help
///
///@{
class ATTR_DLL_LOCAL StreamCryptoSession
  : public CStructHdl<StreamCryptoSession, STREAM_CRYPTO_SESSION>
{
  /*! \cond PRIVATE */
  friend class CInstanceInputStream;
  friend class InputstreamInfo;
  friend class VideoCodecInitdata;
  /*! \endcond */

public:
  /*! \cond PRIVATE */
  StreamCryptoSession() { memset(m_cStructure, 0, sizeof(STREAM_CRYPTO_SESSION)); }
  StreamCryptoSession(const StreamCryptoSession& session) : CStructHdl(session) {}
  StreamCryptoSession& operator=(const StreamCryptoSession&) = default;
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_inputstream_Defs_Info_StreamCryptoSession_Help Value Help
  /// @ingroup cpp_kodi_addon_inputstream_Defs_Info_StreamCryptoSession
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_inputstream_Defs_Info_StreamCryptoSession :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Keysystem for encrypted media** | @ref STREAM_CRYPTO_KEY_SYSTEM | @ref StreamCryptoSession::SetKeySystem "SetKeySystem" | @ref StreamCryptoSession::GetKeySystem "GetKeySystem"
  /// | **Flags for special conditions** | `uint8_t` | @ref StreamCryptoSession::SetFlags "SetFlags" | @ref StreamCryptoSession::GetFlags "GetFlags"
  /// | **Crypto session key id** | `std::string` | @ref StreamCryptoSession::SetSessionId "SetSessionId" | @ref StreamCryptoSession::GetSessionId "GetSessionId"

  /// @brief To set keysystem for encrypted media, @ref STREAM_CRYPTO_KEY_SYSTEM_NONE for
  /// unencrypted media.
  ///
  /// See @ref STREAM_CRYPTO_KEY_SYSTEM for available options.
  void SetKeySystem(STREAM_CRYPTO_KEY_SYSTEM keySystem) { m_cStructure->keySystem = keySystem; }

  /// @brief Get keysystem for encrypted media.
  STREAM_CRYPTO_KEY_SYSTEM GetKeySystem() const { return m_cStructure->keySystem; }

  /// @brief Set bit flags to use special conditions, see @ref STREAM_CRYPTO_FLAGS
  /// for available flags.
  void SetFlags(uint8_t flags) { m_cStructure->flags = flags; }

  /// @brief Get flags for special conditions.
  uint8_t GetFlags() const { return m_cStructure->flags; }

  /// @brief To set the crypto session key identifier.
  void SetSessionId(const std::string& sessionId)
  {
    strncpy(m_cStructure->sessionId, sessionId.c_str(), sizeof(m_cStructure->sessionId) - 1);
  }

  /// @brief To get the crypto session key identifier.
  std::string GetSessionId() const { return m_cStructure->sessionId; }

private:
  StreamCryptoSession(const STREAM_CRYPTO_SESSION* session) : CStructHdl(session) {}
  StreamCryptoSession(STREAM_CRYPTO_SESSION* session) : CStructHdl(session) {}
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
