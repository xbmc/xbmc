/*
 *      Copyright (C) 2005-2015 Team XBMC
 *      http://xbmc.org
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
#include "DVDDemux.h"
#include "DVDInputStreams/InputStreamMultiSource.h"

#include <map>
#include <queue>
#include <string>
#include <utility>
#include <vector>

typedef std::shared_ptr<CDVDDemux> DemuxPtr;

struct comparator{
  bool operator() (std::pair<double, DemuxPtr> x, std::pair<double, DemuxPtr> y) const
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
  
  void Abort() override;
  void EnableStream(int64_t demuxerId, int id, bool enable) override;
  void Flush() override;
  std::string GetFileName() override { return ""; };
  int GetNrOfStreams() const override;
  CDemuxStream* GetStream(int iStreamId)const override { return nullptr; } ;
  CDemuxStream* GetStream(int64_t demuxerId, int iStreamId) const override;
  std::vector<CDemuxStream*> GetStreams() const override;
  std::string GetStreamCodecName(int64_t demuxerId, int iStreamId) override;
  int GetStreamLength() override;
  bool Open(CDVDInputStream* pInput);
  DemuxPacket* Read() override;
  void Reset() override;
  bool SeekTime(double time, bool backwards = false, double* startpts = NULL) override;
  void SetSpeed(int iSpeed) override {};

private:
  void Dispose();
  void SetMissingStreamDetails(DemuxPtr demuxer);

  InputStreamMultiStreams* m_pInput = NULL;
  std::map<DemuxPtr, InputStreamPtr> m_DemuxerToInputStreamMap;
  DemuxQueue m_demuxerQueue;
  std::map<int64_t, DemuxPtr> m_demuxerMap;
};
