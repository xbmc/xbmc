/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
  int64_t demuxerId = -1; // id of the demuxer that created the packet
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
