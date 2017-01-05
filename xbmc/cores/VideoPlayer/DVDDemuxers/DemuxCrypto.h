/*
*      Copyright (C) 2005-2016 Team XBMC
*      http://xbmc.org
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
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#pragma once

#include <inttypes.h>
#include <string.h>

//CryptoSession is usually obtained once per stream, but could change if an key expires

enum CryptoSessionSystem :uint16_t
{
  CRYPTO_SESSION_SYSTEM_NONE,
  CRYPTO_SESSION_SYSTEM_WIDEVINE,
  CRYPTO_SESSION_SYSTEM_PLAYREADY
};

struct DemuxCryptoSession
{
  DemuxCryptoSession(const CryptoSessionSystem sys, const uint16_t sSize, const char *sData)
    : sessionId(new char[sSize])
    , sessionIdSize(sSize)
    , keySystem(sys)
  {
    memcpy(sessionId, sData, sSize);
  };

  ~DemuxCryptoSession()
  {
    delete[] sessionId;
  }

  // encryped stream infos
  char * sessionId;
  uint16_t sessionIdSize;
  CryptoSessionSystem keySystem;
};

//CryptoInfo stores the information to decrypt a sample

struct DemuxCryptoInfo
{
  DemuxCryptoInfo(const unsigned int numSubs)
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
};
