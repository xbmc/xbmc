/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/inputstream/stream_crypto.h"

#include <string>

//CryptoSession is usually obtained once per stream, but could change if an key expires

enum CryptoSessionSystem : uint8_t
{
  CRYPTO_SESSION_SYSTEM_NONE,
  CRYPTO_SESSION_SYSTEM_WIDEVINE,
  CRYPTO_SESSION_SYSTEM_PLAYREADY,
  CRYPTO_SESSION_SYSTEM_WISEPLAY,
  CRYPTO_SESSION_SYSTEM_CLEARKEY,
};

struct DemuxCryptoSession
{
  DemuxCryptoSession(const CryptoSessionSystem sys, const char* sData, const uint8_t flags)
    : sessionId(sData), keySystem(sys), flags(flags)
  {
  }

  bool operator == (const DemuxCryptoSession &other) const
  {
    return keySystem == other.keySystem && sessionId == other.sessionId;
  };

  // encryped stream infos
  std::string sessionId;
  CryptoSessionSystem keySystem;

  static const uint8_t FLAG_SECURE_DECODER = 1;
  uint8_t flags;
private:
  DemuxCryptoSession(const DemuxCryptoSession&) = delete;
  DemuxCryptoSession& operator=(const DemuxCryptoSession&) = delete;
};

//CryptoInfo stores the information to decrypt a sample

struct DemuxCryptoInfo : DEMUX_CRYPTO_INFO
{
  explicit DemuxCryptoInfo(const unsigned int numSubs)
  {
    numSubSamples = numSubs;
    flags = 0;
    clearBytes = new uint16_t[numSubs];
    cipherBytes = new uint32_t[numSubs];
  };

  ~DemuxCryptoInfo()
  {
    delete[] clearBytes;
    delete[] cipherBytes;
  }

private:
  DemuxCryptoInfo(const DemuxCryptoInfo&) = delete;
  DemuxCryptoInfo& operator=(const DemuxCryptoInfo&) = delete;
};
