/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <inttypes.h>
#include <string.h>

//CryptoSession is usually obtained once per stream, but could change if an key expires

enum CryptoSessionSystem : uint8_t
{
  CRYPTO_SESSION_SYSTEM_NONE,
  CRYPTO_SESSION_SYSTEM_WIDEVINE,
  CRYPTO_SESSION_SYSTEM_PLAYREADY,
  CRYPTO_SESSION_SYSTEM_WISEPLAY,
};

struct DemuxCryptoSession
{
  DemuxCryptoSession(const CryptoSessionSystem sys, const uint16_t sSize, const char *sData, const uint8_t flags)
    : sessionId(new char[sSize])
    , sessionIdSize(sSize)
    , keySystem(sys)
    , flags(flags)
  {
    memcpy(sessionId, sData, sSize);
  };

  ~DemuxCryptoSession()
  {
    delete[] sessionId;
  }

  bool operator == (const DemuxCryptoSession &other) const
  {
    return sessionIdSize == other.sessionIdSize &&
      keySystem == other.keySystem &&
      memcmp(sessionId, other.sessionId, sessionIdSize) == 0;
  };

  // encryped stream infos
  char * sessionId;
  uint16_t sessionIdSize;
  CryptoSessionSystem keySystem;

  static const uint8_t FLAG_SECURE_DECODER = 1;
  uint8_t flags;
private:
  DemuxCryptoSession(const DemuxCryptoSession&) = delete;
  DemuxCryptoSession& operator=(const DemuxCryptoSession&) = delete;
};

//CryptoInfo stores the information to decrypt a sample

struct DemuxCryptoInfo
{
  explicit DemuxCryptoInfo(const unsigned int numSubs)
    : numSubSamples(numSubs)
    , flags(0)
    , clearBytes(new uint16_t[numSubs])
    , cipherBytes(new uint32_t[numSubs])
  {};

  ~DemuxCryptoInfo()
  {
    delete[] clearBytes;
    delete[] cipherBytes;
  }

  uint16_t numSubSamples; //number of subsamples
  uint16_t flags; //flags for later use

  uint16_t *clearBytes; // numSubSamples uint16_t's wich define the size of clear size of a subsample
  uint32_t *cipherBytes; // numSubSamples uint32_t's wich define the size of cipher size of a subsample

  uint8_t iv[16]; // initialization vector
  uint8_t kid[16]; // key id
private:
  DemuxCryptoInfo(const DemuxCryptoInfo&) = delete;
  DemuxCryptoInfo& operator=(const DemuxCryptoInfo&) = delete;
};
