/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaDrmCryptoSession.h"

#include "filesystem/File.h"
#include "utils/Base64.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <stdexcept>

#include <androidjni/MediaDrm.h>
#include <androidjni/UUID.h>

using namespace DRM;
using namespace XbmcCommons;


class CharVecBuffer : public Buffer
{
public:
  inline CharVecBuffer(const Buffer& buf) : Buffer(buf) {};

  inline CharVecBuffer(const std::vector<char>& vec)
    : Buffer(vec.size())
  {
    memcpy(data(), vec.data(), vec.size());
  }

  inline operator std::vector<char>() const
  {
    return std::vector<char>(data(), data() + capacity());
  }
};


void CMediaDrmCryptoSession::Register()
{
  CCryptoSession::RegisterInterface(CMediaDrmCryptoSession::Create);
}

CMediaDrmCryptoSession::CMediaDrmCryptoSession(const std::string& UUID,
                                               const std::string& cipherAlgo,
                                               const std::string& macAlgo)
  : m_cipherAlgo(cipherAlgo), m_macAlgo(macAlgo)
{
  if (UUID.length() != 36 || UUID[8] != '-' || UUID[13] != '-' || UUID[18] != '-' ||
      UUID[23] != '-')
    throw std::runtime_error("MediaDrmCryptoSession: Invalid UUID format, expected "
                             "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX");

  int64_t mostSigBits(0), leastSigBits(0);
  unsigned int uuidPtr[16];
  if (sscanf(UUID.c_str(), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             &uuidPtr[0], &uuidPtr[1], &uuidPtr[2], &uuidPtr[3], &uuidPtr[4], &uuidPtr[5],
             &uuidPtr[6], &uuidPtr[7], &uuidPtr[8], &uuidPtr[9], &uuidPtr[10], &uuidPtr[11],
             &uuidPtr[12], &uuidPtr[13], &uuidPtr[14], &uuidPtr[15]) != 16)
  {
    throw std::runtime_error("MediaDrmCryptoSession: Cannot parse UUID: " + UUID);
  }

  for (unsigned int i(0); i < 8; ++i)
    mostSigBits = (mostSigBits << 8) | uuidPtr[i];
  for (unsigned int i(8); i < 16; ++i)
   leastSigBits = (leastSigBits << 8) | uuidPtr[i];

  CJNIUUID uuid(mostSigBits, leastSigBits);

  m_mediaDrm = new CJNIMediaDrm(uuid);

  if (xbmc_jnienv()->ExceptionCheck())
  {
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
    CLog::Log(LOGERROR, "MediaDrm: Failure creating instance");
    throw std::runtime_error("Failure creating MediaDrm");
  }

  if (!OpenSession())
  {
    CLog::Log(LOGERROR, "MediaDrm: Unable to create a session");
    throw std::runtime_error("Unable to create a session");
  }
}

CMediaDrmCryptoSession::~CMediaDrmCryptoSession()
{
  if (!m_mediaDrm)
    return;

  CloseSession();

  CJNIBase::GetSDKVersion() >= 28 ? m_mediaDrm->close() : m_mediaDrm->release();
  delete m_mediaDrm, m_mediaDrm = nullptr;
}

CCryptoSession* CMediaDrmCryptoSession::Create(const std::string& UUID, const std::string& cipherAlgo, const std::string& macAlgo)
{
  CMediaDrmCryptoSession *res = nullptr;;
  try
  {
    res = new CMediaDrmCryptoSession(UUID, cipherAlgo, macAlgo);
  }
  catch (std::runtime_error& e)
  {
    delete res, res = nullptr;
  }
  return res;
}

/*****************/

// Interface methods
Buffer CMediaDrmCryptoSession::GetKeyRequest(const Buffer& init,
  const std::string& mimeType,
  bool offlineKey,
  const std::map<std::string, std::string>& parameters)
{
  CLog::Log(LOGDEBUG, "MediaDrm: GetKeyRequest");
  if (m_mediaDrm && m_sessionId)
  {
    CJNIMediaDrmKeyRequest req = m_mediaDrm->getKeyRequest(*m_sessionId, CharVecBuffer(init), mimeType,
      offlineKey ? CJNIMediaDrm::KEY_TYPE_OFFLINE : CJNIMediaDrm::KEY_TYPE_STREAMING, parameters);

    if (xbmc_jnienv()->ExceptionCheck())
    {
      CLog::Log(LOGERROR, "MediaDrm: getKeyRequest exception");
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
      return Buffer();
    }

    Buffer data = CharVecBuffer(req.getData());

    if (xbmc_jnienv()->ExceptionCheck())
    {
      CLog::Log(LOGERROR, "MediaDrm: getKeyRequest.getData exception");
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
      return Buffer();
    }

    return data;
  }

  return Buffer();
}


std::string CMediaDrmCryptoSession::GetPropertyString(const std::string& name)
{
  if (m_mediaDrm)
    return m_mediaDrm->getPropertyString(name);

  return "";
}


std::string CMediaDrmCryptoSession::ProvideKeyResponse(const Buffer& response)
{
  CLog::Log(LOGDEBUG, "MediaDrm: ProvideKeyResponse");
  if (m_mediaDrm)
  {
    m_hasKeys = true;
    std::vector<char> res = m_mediaDrm->provideKeyResponse(*m_sessionId, CharVecBuffer(response));
    if (xbmc_jnienv()->ExceptionCheck())
    {
      CLog::Log(LOGERROR, "MediaDrm: provideKeyResponse exception");
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
      return "";
    }
    return std::string(res.data(), res.size());
  }

  return "";
}

void CMediaDrmCryptoSession::RemoveKeys()
{
  CLog::Log(LOGDEBUG, "MediaDrm: RemoveKeys");
  if (m_mediaDrm && m_sessionId && m_hasKeys)
  {
    CloseSession();
    OpenSession();
  }
}

void CMediaDrmCryptoSession::RestoreKeys(const std::string& keySetId)
{
  CLog::Log(LOGDEBUG, "MediaDrm: RestoreKeys");
  if (m_mediaDrm && keySetId != m_keySetId)
  {
    m_mediaDrm->restoreKeys(*m_sessionId, std::vector<char>(keySetId.begin(), keySetId.end()));
    if (xbmc_jnienv()->ExceptionCheck())
    {
      CLog::Log(LOGERROR, "MediaDrm: restoreKeys exception");
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
    }
    else
    {
      m_hasKeys = true;
      m_keySetId = keySetId;
    }
  }
}

void CMediaDrmCryptoSession::SetPropertyString(const std::string& name, const std::string& value)
{
  if (m_mediaDrm)
    m_mediaDrm->setPropertyString(name, value);
}

// Crypto methods
Buffer CMediaDrmCryptoSession::Decrypt(const Buffer& cipherKeyId, const Buffer& input, const Buffer& iv)
{
  CLog::Log(LOGDEBUG, "MediaDrm: Decrypt");
  if (m_cryptoSession)
  {
    Buffer decrypt = CharVecBuffer(m_cryptoSession->decrypt(
        CharVecBuffer(cipherKeyId), CharVecBuffer(input), CharVecBuffer(iv)));
    if (xbmc_jnienv()->ExceptionCheck())
    {
      CLog::Log(LOGERROR, "MediaDrm: decrypt exception");
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
      return Buffer();
    }
    return decrypt;
  }

  return Buffer();
}

Buffer CMediaDrmCryptoSession::Encrypt(const Buffer& cipherKeyId, const Buffer& input, const Buffer& iv)
{
  CLog::Log(LOGDEBUG, "MediaDrm: Encrypt");
  if (m_cryptoSession)
  {
    Buffer encrypt = CharVecBuffer(m_cryptoSession->encrypt(
        CharVecBuffer(cipherKeyId), CharVecBuffer(input), CharVecBuffer(iv)));
    if (xbmc_jnienv()->ExceptionCheck())
    {
      CLog::Log(LOGERROR, "MediaDrm: encrypt exception");
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
      return Buffer();
    }
    return encrypt;
  }

  return Buffer();
}

Buffer CMediaDrmCryptoSession::Sign(const Buffer& macKeyId, const Buffer& message)
{
  CLog::Log(LOGDEBUG, "MediaDrm: Sign");
  if (m_cryptoSession)
  {
    Buffer sign =
        CharVecBuffer(m_cryptoSession->sign(CharVecBuffer(macKeyId), CharVecBuffer(message)));
    if (xbmc_jnienv()->ExceptionCheck())
    {
      CLog::Log(LOGERROR, "MediaDrm: sign exception");
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
      return Buffer();
    }
    return sign;
  }

  return Buffer();
}

bool CMediaDrmCryptoSession::Verify(const Buffer& macKeyId, const Buffer& message, const Buffer& signature)
{
  CLog::Log(LOGDEBUG, "MediaDrm: Verify");
  if (m_cryptoSession)
  {
    bool verify = m_cryptoSession->verify(CharVecBuffer(macKeyId), CharVecBuffer(message),
                                          CharVecBuffer(signature));
    if (xbmc_jnienv()->ExceptionCheck())
    {
      CLog::Log(LOGERROR, "MediaDrm: verify exception");
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
      return false;
    }
    return verify;
  }

  return false;
}

//Private stuff
bool CMediaDrmCryptoSession::OpenSession()
{
  CLog::Log(LOGDEBUG, "MediaDrm: OpenSession");
  bool provisioned = false;
TRYAGAIN:
  m_sessionId = new CharVecBuffer(m_mediaDrm->openSession());
  if (xbmc_jnienv()->ExceptionCheck())
  {
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
    if (provisioned || !ProvisionRequest())
    {
      delete m_sessionId, m_sessionId = nullptr;
      return false;
    }
    else
    {
      provisioned = true;
      goto TRYAGAIN;
    }
  }

  m_cryptoSession = new CJNIMediaDrmCryptoSession(m_mediaDrm->getCryptoSession(*m_sessionId, m_cipherAlgo, m_macAlgo));

  if (xbmc_jnienv()->ExceptionCheck())
  {
    CLog::Log(LOGERROR, "MediaDrm: getCryptoSession failed");
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
    return false;
  }
  return true;
}

void CMediaDrmCryptoSession::CloseSession()
{
  if (m_sessionId)
  {
    m_mediaDrm->removeKeys(*m_sessionId);
    m_mediaDrm->closeSession(*m_sessionId);

    if (m_cryptoSession)
      delete(m_cryptoSession), m_cryptoSession = nullptr;

    delete m_sessionId, m_sessionId = nullptr;
    m_hasKeys = false;
    m_keySetId.clear();
  }
}

bool CMediaDrmCryptoSession::ProvisionRequest()
{
  CLog::Log(LOGINFO, "MediaDrm: starting provisioning");

  CJNIMediaDrmProvisionRequest request = m_mediaDrm->getProvisionRequest();
  if (xbmc_jnienv()->ExceptionCheck())
  {
    CLog::Log(LOGERROR, "MediaDrm: getProvisionRequest failed");
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
    return false;
  }

  std::vector<char> provData = request.getData();
  if (xbmc_jnienv()->ExceptionCheck())
  {
    CLog::Log(LOGERROR, "MediaDrm: getProvisionRequest.getData exception");
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
    return false;
  }

  std::string url = request.getDefaultUrl();
  if (xbmc_jnienv()->ExceptionCheck())
  {
    CLog::Log(LOGERROR, "MediaDrm: getProvisionRequest.getDefaultUrl exception");
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
    return false;
  }

  CLog::Log(LOGDEBUG, "MediaDrm: Provisioning: size: {}, url: {}", provData.size(), url);

  std::string tmp_str("{\"signedRequest\":\"");
  tmp_str += std::string(provData.data(), provData.size());
  tmp_str += "\"}";

  std::string encoded;
  Base64::Encode(tmp_str.data(), tmp_str.size(), encoded);

  XFILE::CFile file;
  if (!file.CURLCreate(url))
  {
    CLog::Log(LOGERROR, "MediaDrm: CURLCreate failed!");
    return false;
  }

  file.CURLAddOption(XFILE::CURL_OPTION_PROTOCOL, "Content-Type", "application/json");
  file.CURLAddOption(XFILE::CURL_OPTION_PROTOCOL, "seekable", "0");
  file.CURLAddOption(XFILE::CURL_OPTION_PROTOCOL, "postdata", encoded.c_str());

  if (!file.CURLOpen(0))
  {
    CLog::Log(LOGERROR, "MediaDrm: Provisioning server returned failure");
    return false;
  }
  provData.clear();
  char buf[8192];
  size_t nbRead;

  // read the file
  while ((nbRead = file.Read(buf, 8192)) > 0)
    provData.insert(provData.end(), buf, buf + nbRead);

  m_mediaDrm->provideProvisionResponse(provData);
  if (xbmc_jnienv()->ExceptionCheck())
  {
    CLog::Log(LOGERROR, "MediaDrm: provideProvisionResponse failed");
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
    return false;
  }
  return true;
}
