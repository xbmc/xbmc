/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Interface/DemuxPacket.h"

#include <chrono>
#include <span>
#include <string>
#include <vector>

struct AVChapter;
struct AVPacket;

struct ChapterFFmpeg
{
  bool operator==(const ChapterFFmpeg&) const = default;

  std::chrono::milliseconds m_startPts;
  std::chrono::milliseconds m_endPts;
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

  /*!
   * \brief Snap a container-declared frame rate that is exactly 1000/N fps
   * (a whole number of milliseconds N per frame, the fingerprint of a rate
   * derived from millisecond-quantised Matroska timestamps) to the standard
   * rate whose millisecond-rounded frame duration equals N.
   * \param[in,out] fpsRate frame rate numerator, rewritten on success
   * \param[in,out] fpsScale frame rate denominator, rewritten on success
   * \param hintFps measured rate from container statistics (frame count /
   * duration) used to resolve rates that quantise to the same duration
   * (23.976 vs 24); pass 0 when unknown to prefer the fractional rate
   * \return true when the rate was rewritten
   */
  static bool SnapMsQuantisedFrameRate(int& fpsRate, int& fpsScale, double hintFps);
};
