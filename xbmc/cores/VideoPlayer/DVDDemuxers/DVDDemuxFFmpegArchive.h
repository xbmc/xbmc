/*
 *  Copyright (C) 2018 Arthur Liberman
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDDemuxFFmpeg.h"

class CDVDDemuxFFmpegArchive : public CDVDDemuxFFmpeg
{
public:
  CDVDDemuxFFmpegArchive();
  bool Open(std::shared_ptr<CDVDInputStream> pInput, bool streaminfo = true, bool fileinfo = false) override;
  bool SeekTime(double time, bool backwards = false, double* startpts = NULL) override;
  DemuxPacket* Read() override;
protected:
  void UpdateCurrentPTS() override;

  bool m_bIsOpening;
  double m_seekOffset;
};