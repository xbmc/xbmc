/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Interface/DemuxPacket.h"

#include <span>
#include <string>
#include <vector>

struct AVChapter;
struct AVPacket;

struct ChapterFFmpeg
{
  bool operator==(const ChapterFFmpeg&) const = default;

  double m_startPts; // movie time, in seconds
  double m_endPts; // movie time, in seconds
  std::string m_name;
};

class CDVDDemuxUtils
{
public:
  static void FreeDemuxPacket(DemuxPacket* pPacket);
  static DemuxPacket* AllocateDemuxPacket(int iDataSize = 0);
  static DemuxPacket* AllocateDemuxPacket(unsigned int iDataSize,
                                          unsigned int encryptedSubsampleCount);
  static void StoreSideData(DemuxPacket* pkt, AVPacket* src);
  static std::vector<ChapterFFmpeg> LoadChapters(std::span<AVChapter*> chapters);
};
