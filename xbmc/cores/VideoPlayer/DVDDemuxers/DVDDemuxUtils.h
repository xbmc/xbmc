/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Interface/DemuxPacket.h"
extern "C" {
#include <libavcodec/avcodec.h>
}

class CDVDDemuxUtils
{
public:
  static void FreeDemuxPacket(DemuxPacket* pPacket);
  static DemuxPacket* AllocateDemuxPacket(int iDataSize = 0);
  static DemuxPacket* AllocateDemuxPacket(unsigned int iDataSize, unsigned int encryptedSubsampleCount);
  static void StoreSideData(DemuxPacket *pkt, AVPacket *src);
};

