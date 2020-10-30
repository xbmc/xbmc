/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDDemux.h"
#include "DVDInputStreams/InputStreamMultiSource.h"

#include <map>
#include <queue>
#include <string>
#include <utility>
#include <vector>

typedef std::shared_ptr<CDVDDemux> DemuxPtr;

struct comparator{
  bool operator()(const std::pair<double, DemuxPtr>& x, const std::pair<double, DemuxPtr>& y) const
  {
    return x.first > y.first;
  }
};

typedef std::priority_queue<std::pair<double, DemuxPtr>, std::vector<std::pair<double, DemuxPtr>>, comparator> DemuxQueue;

class CDemuxMultiSource : public CDVDDemux
{

public:
  CDemuxMultiSource();
  ~CDemuxMultiSource() override;

  bool Open(const std::shared_ptr<CDVDInputStream>& pInput);

  // implementation of CDVDDemux
  void Abort() override;
  void EnableStream(int64_t demuxerId, int id, bool enable) override;
  void Flush() override;
  int GetNrOfStreams() const override;
  CDemuxStream* GetStream(int64_t demuxerId, int iStreamId) const override;
  std::vector<CDemuxStream*> GetStreams() const override;
  std::string GetStreamCodecName(int64_t demuxerId, int iStreamId) override;
  int GetStreamLength() override;
  DemuxPacket* Read() override;
  bool Reset() override;
  bool SeekTime(double time, bool backwards = false, double* startpts = NULL) override;

protected:
  CDemuxStream* GetStream(int iStreamId) const override { return nullptr; }

private:
  void Dispose();
  void SetMissingStreamDetails(const DemuxPtr& demuxer);

  std::shared_ptr<InputStreamMultiStreams> m_pInput = NULL;
  std::map<DemuxPtr, InputStreamPtr> m_DemuxerToInputStreamMap;
  DemuxQueue m_demuxerQueue;
  std::map<int64_t, DemuxPtr> m_demuxerMap;
};
