/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDDemux.h"

#ifdef TARGET_WINDOWS
#define __attribute__(dummy_val)
#pragma pack(push)
#pragma pack(1)
#endif

typedef struct
{
  char fourcc[4];
  uint32_t type;
  uint32_t channels;
  uint32_t sampleRate;
  uint32_t bitsPerSample;
  uint64_t durationMs;
} __attribute__((__packed__)) Demux_BXA_FmtHeader;

#ifdef TARGET_WINDOWS
#pragma pack(pop)
#endif

#include <vector>

#define BXA_PACKET_TYPE_FMT_DEMUX 1

class CDemuxStreamAudioBXA;

class CDVDDemuxBXA : public CDVDDemux
{
public:

  CDVDDemuxBXA();
  ~CDVDDemuxBXA() override;

  bool Open(const std::shared_ptr<CDVDInputStream>& pInput);
  void Dispose();
  bool Reset() override;
  void Abort() override;
  void Flush() override;
  DemuxPacket* Read() override;
  bool SeekTime(double time, bool backwards = false, double* startpts = NULL) override { return false; }
  int GetStreamLength() override { return (int)m_header.durationMs; }
  CDemuxStream* GetStream(int iStreamId) const override;
  std::vector<CDemuxStream*> GetStreams() const override;
  int GetNrOfStreams() const override;
  std::string GetFileName() override;
  std::string GetStreamCodecName(int iStreamId) override;

protected:
  friend class CDemuxStreamAudioBXA;
  std::shared_ptr<CDVDInputStream> m_pInput;
  int64_t m_bytes;

  CDemuxStreamAudioBXA *m_stream;

  Demux_BXA_FmtHeader m_header;
};

