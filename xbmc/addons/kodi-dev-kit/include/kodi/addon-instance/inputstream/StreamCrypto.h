/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../c-api/addon-instance/inputstream/stream_crypto.h"

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

class CInstanceInputStream;
class InputstreamInfo;

class ATTRIBUTE_HIDDEN StreamCryptoSession
  : public CStructHdl<StreamCryptoSession, STREAM_CRYPTO_SESSION>
{
  friend class CInstanceInputStream;
  friend class InputstreamInfo;

public:
  /*! \cond PRIVATE */
  StreamCryptoSession() { memset(m_cStructure, 0, sizeof(STREAM_CRYPTO_SESSION)); }
  StreamCryptoSession(const StreamCryptoSession& session)
    : CStructHdl(session), m_sessionId(session.m_sessionId)
  {
  }
  /*! \endcond */

  void SetKeySystem(STREAM_CRYPTO_KEY_SYSTEM keySystem) { m_cStructure->keySystem = keySystem; }

  STREAM_CRYPTO_KEY_SYSTEM GetKeySystem() const { return m_cStructure->keySystem; }

  void SetFlags(uint8_t flags) { m_cStructure->flags = flags; }

  uint8_t GetFlags() const { return m_cStructure->flags; }

  void SetSessionId(const std::string& sessionId)
  {
    m_sessionId = sessionId;
    if (!m_sessionId.empty())
    {
      m_cStructure->sessionId = m_sessionId.c_str();
      m_cStructure->sessionIdSize = m_sessionId.size();
    }
    else
    {
      m_cStructure->sessionId = nullptr;
      m_cStructure->sessionIdSize = 0;
    }
  }

  std::string GetSessionId() const { return m_sessionId; }

private:
  StreamCryptoSession(const STREAM_CRYPTO_SESSION* session)
    : CStructHdl(session), m_sessionId(session->sessionId)
  {
  }
  StreamCryptoSession(STREAM_CRYPTO_SESSION* session)
    : CStructHdl(session), m_sessionId(session->sessionId)
  {
  }

  std::string m_sessionId;
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
