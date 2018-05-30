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

#pragma once

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
