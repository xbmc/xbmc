/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDDemux.h"

#include <memory>
#include <vector>

class CCaptionBlock;
class CDecoderCC708;

class CDVDDemuxCC : public CDVDDemux
{
public:
  explicit CDVDDemuxCC(AVCodecID codec);
  ~CDVDDemuxCC() override;

  bool Reset() override { return true; }
  void Flush() override {};
  DemuxPacket* Read() override { return NULL; }
  bool SeekTime(double time, bool backwards = false, double* startpts = NULL) override
  {
    return true;
  }
  CDemuxStream* GetStream(int iStreamId) const override;
  std::vector<CDemuxStream*> GetStreams() const override;
  int GetNrOfStreams() const override;

  DemuxPacket* Read(DemuxPacket *packet);
  static void Handler(int service, void *userdata);

protected:
  bool OpenDecoder();
  void Dispose();
  DemuxPacket* Decode();

  struct streamdata
  {
    int streamIdx;
    int service;
    bool hasData ;
    double pts;
  };
  std::vector<streamdata> m_streamdata;
  std::vector<CDemuxStreamSubtitle> m_streams;
  bool m_hasData;
  double m_curPts;
  std::vector<CCaptionBlock*> m_ccReorderBuffer;
  std::vector<CCaptionBlock*> m_ccTempBuffer;
  std::unique_ptr<CDecoderCC708> m_ccDecoder;
  AVCodecID m_codec;
};
