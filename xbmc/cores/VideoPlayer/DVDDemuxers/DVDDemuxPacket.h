#pragma once

/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include <cstdint>

#define DMX_SPECIALID_STREAMINFO    -10
#define DMX_SPECIALID_STREAMCHANGE  -11

typedef struct DemuxCryptoInfo
{
  uint16_t numSubSamples; //number of subsamples
  uint16_t flags; //flags for later use

  uint16_t *clearBytes; // numSubSamples uint16_t's wich define the size of clear size of a subsample
  uint32_t *cipherBytes; // numSubSamples uint32_t's wich define the size of cipher size of a subsample

  uint8_t iv[16]; // initialization vector
  uint8_t kid[16]; // key id
}DemuxCryptoInfo;

typedef struct DemuxPacket
{
  unsigned char* pData;   // data
  int iSize;     // data size
  int iStreamId; // integer representing the stream index
  int64_t demuxerId; // id of the demuxer that created the packet
  int iGroupId;  // the group this data belongs to, used to group data from different streams together
  int dispTime;

  double pts; // pts in DVD_TIME_BASE
  double dts; // dts in DVD_TIME_BASE
  double duration; // duration in DVD_TIME_BASE if available

  DemuxCryptoInfo *cryptoInfo; //necessary information to decrypt a packet; nullptr if not encrypted
} DemuxPacket;
