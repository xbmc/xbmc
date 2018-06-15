/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaDrmCryptoSession.h"

#include <androidjni/MediaDrm.h>
#include <androidjni/UUID.h>
#include <stdexcept>

#include "utils/StringUtils.h"

using namespace DRM;
using namespace XbmcCommons;


class CharVecBuffer : public Buffer
{
public:
  inline CharVecBuffer(const Buffer &buf) : Buffer(buf) {};

  inline CharVecBuffer(const std::vector<char> &vec)
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

CMediaDrmCryptoSession::CMediaDrmCryptoSession(const std::string &UUID, const std::string &cipherAlgo, const std::string &macAlgo)
  : m_mediaDrm(nullptr)
  , m_cryptoSession(nullptr)
  , m_cipherAlgo(cipherAlgo)
  , m_macAlgo(macAlgo)
  , m_hasKeys(false)
  , m_sessionId(nullptr)
{
  if (!StringUtils::EqualsNoCase(UUID, "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed"))
    throw std::runtime_error("mediaDrm: Invalid UUID size");

  int64_t mostSigBits(0), leastSigBits(0);
  const uint8_t uuidPtr[16] = {0xed,0xef,0x8b,0xa9,0x79,0xd6,0x4a,0xce,0xa3,0xc8,0x27,0xdc,0xd5,0x1d,0x21,0xed};
  for (unsigned int i(0); i < 8; ++i)
    mostSigBits = (mostSigBits << 8) | uuidPtr[i];
  for (unsigned int i(8); i < 16; ++i)
   leastSigBits = (leastSigBits << 8) | uuidPtr[i];

  CJNIUUID uuid(mostSigBits, leastSigBits);

  m_mediaDrm = new CJNIMediaDrm(uuid);

  if (xbmc_jnienv()->ExceptionCheck())
  {
    xbmc_jnienv()->ExceptionClear();
    throw std::runtime_error("Failure creating MediaDrm");
  }

  if (!OpenSession())
    throw std::runtime_error("Unable to create a session");
}

CMediaDrmCryptoSession::~CMediaDrmCryptoSession()
{
  if (!m_mediaDrm)
    return;

  CloseSession();

  m_mediaDrm->release();
  delete m_mediaDrm, m_mediaDrm = nullptr;
}

CCryptoSession* CMediaDrmCryptoSession::Create(const std::string &UUID, const std::string &cipherAlgo, const std::string &macAlgo)
{
  CMediaDrmCryptoSession *res = nullptr;;
  try
  {
    res = new CMediaDrmCryptoSession(UUID, cipherAlgo, macAlgo);
  }
  catch (std::runtime_error &e)
  {
    delete res, res = nullptr;
  }
  return res;
}

/*****************/

// Interface methods
Buffer CMediaDrmCryptoSession::GetKeyRequest(const Buffer &init,
  const std::string &mimeType,
  bool offlineKey,
  const std::map<std::string, std::string> &parameters)
{
  if (m_mediaDrm && m_sessionId)
  {
    CJNIMediaDrmKeyRequest req = m_mediaDrm->getKeyRequest(*m_sessionId, CharVecBuffer(init), mimeType,
      offlineKey ? CJNIMediaDrm::KEY_TYPE_OFFLINE : CJNIMediaDrm::KEY_TYPE_STREAMING, parameters);
    return CharVecBuffer(req.getData());
  }

  return Buffer();
}


std::string CMediaDrmCryptoSession::GetPropertyString(const std::string &name)
{
  if (m_mediaDrm)
    return m_mediaDrm->getPropertyString(name);

  return "";
}


std::string CMediaDrmCryptoSession::ProvideKeyResponse(const Buffer &response)
{
  if (m_mediaDrm)
  {
    m_hasKeys = true;
    std::vector<char> res = m_mediaDrm->provideKeyResponse(*m_sessionId, CharVecBuffer(response));
    return std::string(res.data(), res.size());
  }

  return "";
}

void CMediaDrmCryptoSession::RemoveKeys()
{
  if (m_mediaDrm && m_sessionId && m_hasKeys)
  {
    CloseSession();
    OpenSession();
  }
}

void CMediaDrmCryptoSession::RestoreKeys(const std::string &keySetId)
{
  if (m_mediaDrm && keySetId != m_keySetId)
  {
    m_mediaDrm->restoreKeys(*m_sessionId, std::vector<char>(keySetId.begin(), keySetId.end()));
    m_hasKeys = true;
    m_keySetId = keySetId;
  }
}

void CMediaDrmCryptoSession::SetPropertyString(const std::string &name, const std::string &value)
{
  if (m_mediaDrm)
    m_mediaDrm->setPropertyString(name, value);
}

// Crypto methods
Buffer CMediaDrmCryptoSession::Decrypt(const Buffer &cipherKeyId, const Buffer &input, const Buffer &iv)
{
  if (m_cryptoSession)
    return CharVecBuffer(m_cryptoSession->decrypt(CharVecBuffer(cipherKeyId), CharVecBuffer(input), CharVecBuffer(iv)));

  return Buffer();
}

Buffer CMediaDrmCryptoSession::Encrypt(const Buffer &cipherKeyId, const Buffer &input, const Buffer &iv)
{
  if (m_cryptoSession)
    return CharVecBuffer(m_cryptoSession->encrypt(CharVecBuffer(cipherKeyId), CharVecBuffer(input), CharVecBuffer(iv)));

  return Buffer();
}

Buffer CMediaDrmCryptoSession::Sign(const Buffer &macKeyId, const Buffer &message)
{
  if (m_cryptoSession)
    return CharVecBuffer(m_cryptoSession->sign(CharVecBuffer(macKeyId), CharVecBuffer(message)));

  return Buffer();
}

bool CMediaDrmCryptoSession::Verify(const Buffer &macKeyId, const Buffer &message, const Buffer &signature)
{
  if (m_cryptoSession)
    return m_cryptoSession->verify(CharVecBuffer(macKeyId), CharVecBuffer(message), CharVecBuffer(signature));

  return false;
}

//Private stuff
bool CMediaDrmCryptoSession::OpenSession()
{
  m_sessionId = new CharVecBuffer(m_mediaDrm->openSession());
  if (xbmc_jnienv()->ExceptionCheck())
  {
    delete m_sessionId, m_sessionId = nullptr;
    xbmc_jnienv()->ExceptionClear();
    return false;;
  }

  m_cryptoSession = new CJNIMediaDrmCryptoSession(m_mediaDrm->getCryptoSession(*m_sessionId, m_cipherAlgo, m_macAlgo));

  if (xbmc_jnienv()->ExceptionCheck())
  {
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
