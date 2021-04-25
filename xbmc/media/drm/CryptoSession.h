/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "commons/Buffer.h"

#include <map>
#include <string>
#include <vector>

namespace DRM
{
  class CCryptoSession;

  typedef CCryptoSession* (*GET_CRYPTO_SESSION_INTERFACE_FN)(const std::string& UUID, const std::string& cipherAlgo, const std::string& hmacAlgo);

  class CCryptoSession
  {
  public:
    // Interface registration
    static CCryptoSession* GetCryptoSession(const std::string& UUID, const std::string& cipherAlgo, const std::string& macAlgo);
    virtual ~CCryptoSession() = default;

    // Interface methods
    virtual XbmcCommons::Buffer GetKeyRequest(const XbmcCommons::Buffer& init, const std::string& mimeType, bool offlineKey, const std::map<std::string, std::string>& parameters) = 0;
    virtual std::string GetPropertyString(const std::string& name) = 0;
    virtual std::string ProvideKeyResponse(const XbmcCommons::Buffer& response) = 0;
    virtual void RemoveKeys() = 0;
    virtual void RestoreKeys(const std::string& keySetId) = 0;
    virtual void SetPropertyString(const std::string& name, const std::string& value) = 0;

    // Crypto methods
    virtual XbmcCommons::Buffer Decrypt(const XbmcCommons::Buffer& cipherKeyId, const XbmcCommons::Buffer& input, const XbmcCommons::Buffer& iv) = 0;
    virtual XbmcCommons::Buffer Encrypt(const XbmcCommons::Buffer& cipherKeyId, const XbmcCommons::Buffer& input, const XbmcCommons::Buffer& iv) = 0;
    virtual XbmcCommons::Buffer Sign(const XbmcCommons::Buffer& macKeyId, const XbmcCommons::Buffer& message) = 0;
    virtual bool Verify(const XbmcCommons::Buffer& macKeyId, const XbmcCommons::Buffer& message, const XbmcCommons::Buffer& signature ) = 0;

  protected:
    static void RegisterInterface(GET_CRYPTO_SESSION_INTERFACE_FN fn);

  private:
    static std::vector<GET_CRYPTO_SESSION_INTERFACE_FN> s_registeredInterfaces;
  };

} //namespace
