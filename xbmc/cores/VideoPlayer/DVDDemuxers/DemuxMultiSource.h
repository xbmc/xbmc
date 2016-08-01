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
  virtual ~CDemuxMultiSource();
  
  void Abort();
  virtual void EnableStream(int64_t demuxerId, int id, bool enable) override;
  void Flush();
  virtual std::string GetFileName() { return ""; };
  int GetNrOfStreams() const override;
  virtual CDemuxStream* GetStream(int iStreamId)const override { return nullptr; } ;
  virtual CDemuxStream* GetStream(int64_t demuxerId, int iStreamId) const override;
  virtual std::vector<CDemuxStream*> GetStreams() const override;
  std::string GetStreamCodecName(int64_t demuxerId, int iStreamId) override;
  int GetStreamLength();
  bool Open(CDVDInputStream* pInput);
  DemuxPacket* Read();
  void Reset();
  bool SeekTime(int time, bool backwords = false, double* startpts = NULL);
  virtual void SetSpeed(int iSpeed) {};

private:
  void Dispose();
  void SetMissingStreamDetails(DemuxPtr demuxer);

  InputStreamMultiStreams* m_pInput = NULL;
  std::map<DemuxPtr, InputStreamPtr> m_DemuxerToInputStreamMap;
  DemuxQueue m_demuxerQueue;
  std::map<int64_t, DemuxPtr> m_demuxerMap;
};
