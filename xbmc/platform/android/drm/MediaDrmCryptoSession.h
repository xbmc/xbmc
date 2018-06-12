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

#include "drm/CryptoSession.h"

class CJNIMediaDrm;
class CJNIMediaDrmCryptoSession;

namespace DRM
{
  class CharVecBuffer : public XbmcCommons::Buffer
  {
  public:
    inline CharVecBuffer(const XbmcCommons::Buffer &buf) : XbmcCommons::Buffer(buf) {};

    inline CharVecBuffer(const std::vector<char> &vec)
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
    CMediaDrmCryptoSession(const std::string &UUID, const std::string &cipherAlgo, const std::string &macAlgo);
    virtual ~CMediaDrmCryptoSession();

    // Interface methods
    XbmcCommons::Buffer GetKeyRequest(const XbmcCommons::Buffer &init, const std::string &mimeType, bool offlineKey, const std::map<std::string, std::string> &parameters) override;
    std::string GetPropertyString(const std::string &name) override;
    std::string ProvideKeyResponse(const XbmcCommons::Buffer &response) override;
    void RemoveKeys() override;
    void RestoreKeys(const std::string &keySetId) override;
    void SetPropertyString(const std::string &name, const std::string &value) override;

    // Crypto methods
    XbmcCommons::Buffer Decrypt(const XbmcCommons::Buffer &cipherKeyId, const XbmcCommons::Buffer &input, const XbmcCommons::Buffer &iv) override;
    XbmcCommons::Buffer Encrypt(const XbmcCommons::Buffer &cipherKeyId, const XbmcCommons::Buffer &input, const XbmcCommons::Buffer &iv) override;
    XbmcCommons::Buffer Sign(const XbmcCommons::Buffer &macKeyId, const XbmcCommons::Buffer &message) override;
    bool Verify(const XbmcCommons::Buffer &macKeyId, const XbmcCommons::Buffer &message, const XbmcCommons::Buffer &signature ) override;

  private:
    static CCryptoSession* Create(const std::string &UUID, const std::string &cipherAlgo, const std::string &hmacAlgo);
    bool OpenSession();
    void CloseSession();

    CJNIMediaDrm *m_mediaDrm;
    CJNIMediaDrmCryptoSession *m_cryptoSession;

    std::string m_cipherAlgo;
    std::string  m_macAlgo;
    std::string m_keySetId;

    bool m_hasKeys;

    CharVecBuffer *m_sessionId;
  };

} //namespace
