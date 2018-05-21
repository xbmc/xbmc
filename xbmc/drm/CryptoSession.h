#pragma once
/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>
#include <map>
#include <stdint.h>

#include "commons/Buffer.h"

namespace DRM
{
  class CCryptoSession;

  typedef CCryptoSession* (*GET_CRYPTO_SESSION_INTERFACE_FN)(const std::string &UUID, const std::string &cipherAlgo, const std::string &hmacAlgo);

  class CCryptoSession
  {
  public:
    // Interface registration
    static CCryptoSession* GetCryptoSession(const std::string &UUID, const std::string &cipherAlgo, const std::string &macAlgo);
    virtual ~CCryptoSession() {};

    // Interface methods
    virtual XbmcCommons::Buffer GetKeyRequest(const XbmcCommons::Buffer &init, const std::string &mimeType, bool offlineKey, const std::map<std::string, std::string> &parameters) = 0;
    virtual std::string GetPropertyString(const std::string &name) = 0;
    virtual std::string ProvideKeyResponse(const XbmcCommons::Buffer &response) = 0;
    virtual void RemoveKeys() = 0;
    virtual void RestoreKeys(const std::string &keySetId) = 0;
    virtual void SetPropertyString(const std::string &name, const std::string &value) = 0;

    // Crypto methods
    virtual XbmcCommons::Buffer Decrypt(const XbmcCommons::Buffer &cipherKeyId, const XbmcCommons::Buffer &input, const XbmcCommons::Buffer &iv) = 0;
    virtual XbmcCommons::Buffer Encrypt(const XbmcCommons::Buffer &cipherKeyId, const XbmcCommons::Buffer &input, const XbmcCommons::Buffer &iv) = 0;
    virtual XbmcCommons::Buffer Sign(const XbmcCommons::Buffer &macKeyId, const XbmcCommons::Buffer &message) = 0;
    virtual bool Verify(const XbmcCommons::Buffer &macKeyId, const XbmcCommons::Buffer &message, const XbmcCommons::Buffer &signature ) = 0;

  protected:
    static void RegisterInterface(GET_CRYPTO_SESSION_INTERFACE_FN fn);

  private:
    static std::vector<GET_CRYPTO_SESSION_INTERFACE_FN> s_registeredInterfaces;
  };

} //namespace
