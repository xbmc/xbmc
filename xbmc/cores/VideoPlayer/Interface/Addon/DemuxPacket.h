/*
 *      Copyright (C) 2012-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "TimingConstants.h"
#include <cstdint>
#include <memory>

#define DMX_SPECIALID_STREAMINFO    -10
#define DMX_SPECIALID_STREAMCHANGE  -11

struct DemuxCryptoInfo;

typedef struct DemuxPacket
{
  DemuxPacket() = default;

  uint8_t *pData = nullptr;
  int iSize = 0;
  int iStreamId = -1;
  int64_t demuxerId; // id of the demuxer that created the packet
  int iGroupId = -1; // the group this data belongs to, used to group data from different streams together

  void *pSideData = nullptr;
  int iSideDataElems = 0;

  double pts = DVD_NOPTS_VALUE;
  double dts = DVD_NOPTS_VALUE;
  double duration = 0; // duration in DVD_TIME_BASE if available
  int dispTime = 0;
  bool recoveryPoint = false;

  std::shared_ptr<DemuxCryptoInfo> cryptoInfo;
} DemuxPacket;
