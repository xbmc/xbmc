/*
 *      Copyright (C) 2005-2018 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DrmCryptoSession.h"
#include "drm/CryptoSession.h"

using namespace XbmcCommons;

namespace XBMCAddon
{
  namespace xbmcdrm
  {
    CryptoSession::CryptoSession(String UUID, String cipherAlgorithm, String macAlgorithm)
      : m_cryptoSession(DRM::CCryptoSession::GetCryptoSession(UUID, cipherAlgorithm, macAlgorithm))
    {
    }

    CryptoSession::~CryptoSession()
    {
      delete m_cryptoSession;
    }

    Buffer CryptoSession::GetKeyRequest(const Buffer &init, const String &mimeType, bool offlineKey, const std::map<String, String> &parameters)
    {
      if (m_cryptoSession)
        return m_cryptoSession->GetKeyRequest(init, mimeType, offlineKey, parameters);

      return Buffer();
    }

    String CryptoSession::GetPropertyString(const String &name)
    {
      if (m_cryptoSession)
        return m_cryptoSession->GetPropertyString(name);

      return "";
    }

    String CryptoSession::ProvideKeyResponse(const Buffer &response)
    {
      if (m_cryptoSession)
        return m_cryptoSession->ProvideKeyResponse(response);

      return "";
    }

    void CryptoSession::RemoveKeys()
    {
      if (m_cryptoSession)
        m_cryptoSession->RemoveKeys();
    }

    void CryptoSession::RestoreKeys(String keySetId)
    {
      if (m_cryptoSession)
        m_cryptoSession->RestoreKeys(keySetId);
    }

    void CryptoSession::SetPropertyString(const String &name, const String &value)
    {
      if (m_cryptoSession)
        return m_cryptoSession->SetPropertyString(name, value);
    }

    /*******************Crypto section *****************/

    Buffer CryptoSession::Decrypt(const Buffer &cipherKeyId, const Buffer &input, const Buffer &iv)
    {
      if (m_cryptoSession)
        return m_cryptoSession->Decrypt(cipherKeyId, input, iv);

      return Buffer();
    }

    Buffer CryptoSession::Encrypt(const Buffer &cipherKeyId, const Buffer &input, const Buffer &iv)
    {
      if (m_cryptoSession)
        return m_cryptoSession->Encrypt(cipherKeyId, input, iv);

      return Buffer();
    }

    Buffer CryptoSession::Sign(const Buffer &macKeyId, const Buffer &message)
    {
      if (m_cryptoSession)
        return m_cryptoSession->Sign(macKeyId, message);

      return Buffer();
    }

    bool CryptoSession::Verify(const Buffer &macKeyId, const Buffer &message, const Buffer &signature)
    {
      if (m_cryptoSession)
        return m_cryptoSession->Verify(macKeyId, message, signature);

      return false;
    }

  } //namespace xbmcdrm
} //namespace XBMCAddon
