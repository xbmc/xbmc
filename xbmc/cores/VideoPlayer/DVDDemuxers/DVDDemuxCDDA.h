/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDDemux.h"
#include <vector>

#ifdef TARGET_WINDOWS
#define __attribute__(dummy_val)
#endif

class CDemuxStreamAudioCDDA;

class CDVDDemuxCDDA : public CDVDDemux
{
public:

  CDVDDemuxCDDA();
  ~CDVDDemuxCDDA() override;

  bool Open(std::shared_ptr<CDVDInputStream> pInput);
  void Dispose();
  bool Reset() override;
  void Abort() override;
  void Flush() override;
  DemuxPacket* Read() override;
  bool SeekTime(double time, bool backwards = false, double* startpts = NULL) override;
  int GetStreamLength() override;
  CDemuxStream* GetStream(int iStreamId) const override;
  std::vector<CDemuxStream*> GetStreams() const override;
  int GetNrOfStreams() const override;
  std::string GetFileName() override;
  std::string GetStreamCodecName(int iStreamId) override;

protected:
  friend class CDemuxStreamAudioCDDA;
  std::shared_ptr<CDVDInputStream> m_pInput;
  int64_t m_bytes;

  CDemuxStreamAudioCDDA *m_stream;
};
