/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "media/drm/CryptoSession.h"

class CJNIMediaDrm;
class CJNIMediaDrmCryptoSession;

namespace DRM
{
  class CharVecBuffer : public XbmcCommons::Buffer
  {
  public:
    inline CharVecBuffer(const XbmcCommons::Buffer& buf) : XbmcCommons::Buffer(buf) {}

    inline CharVecBuffer(const std::vector<char>& vec)
      : XbmcCommons::Buffer(vec.size())
    {
      memcpy(data(), vec.data(), vec.size());
    }

    inline operator std::vector<char>() const
    {
      return std::vector<char>(data(), data() + capacity());
    }
  };

  class CharVecBuffer;

  class CMediaDrmCryptoSession : public CCryptoSession
  {
  public:
    static void Register();
    CMediaDrmCryptoSession(const std::string& UUID, const std::string& cipherAlgo, const std::string& macAlgo);
    ~CMediaDrmCryptoSession() override;

    // Interface methods
    XbmcCommons::Buffer GetKeyRequest(const XbmcCommons::Buffer& init, const std::string& mimeType, bool offlineKey, const std::map<std::string, std::string>& parameters) override;
    std::string GetPropertyString(const std::string& name) override;
    std::string ProvideKeyResponse(const XbmcCommons::Buffer& response) override;
    void RemoveKeys() override;
    void RestoreKeys(const std::string& keySetId) override;
    void SetPropertyString(const std::string& name, const std::string& value) override;

    // Crypto methods
    XbmcCommons::Buffer Decrypt(const XbmcCommons::Buffer& cipherKeyId, const XbmcCommons::Buffer& input, const XbmcCommons::Buffer& iv) override;
    XbmcCommons::Buffer Encrypt(const XbmcCommons::Buffer& cipherKeyId, const XbmcCommons::Buffer& input, const XbmcCommons::Buffer& iv) override;
    XbmcCommons::Buffer Sign(const XbmcCommons::Buffer& macKeyId, const XbmcCommons::Buffer& message) override;
    bool Verify(const XbmcCommons::Buffer& macKeyId, const XbmcCommons::Buffer& message, const XbmcCommons::Buffer& signature ) override;

  private:
    static CCryptoSession* Create(const std::string& UUID, const std::string& cipherAlgo, const std::string& hmacAlgo);
    bool OpenSession();
    void CloseSession();
    bool ProvisionRequest();

    CJNIMediaDrm* m_mediaDrm = nullptr;
    CJNIMediaDrmCryptoSession* m_cryptoSession = nullptr;

    std::string m_cipherAlgo;
    std::string  m_macAlgo;
    std::string m_keySetId;

    bool m_hasKeys = false;

    CharVecBuffer* m_sessionId = nullptr;
  };

} //namespace
