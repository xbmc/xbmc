#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "DVDDemux.h"

#include <memory>
#include <vector>

class CDVDOverlayCodecFFmpeg;
class CDVDInputStream;
class CDVDDemuxFFmpeg;

class CDVDDemuxVobsub : public CDVDDemux
{
public:
  CDVDDemuxVobsub();
  ~CDVDDemuxVobsub() override;

  void Reset() override;
  void Abort() override {};
  void Flush() override;
  DemuxPacket* Read() override;
  bool SeekTime(double time, bool backwards, double* startpts = NULL) override;
  void SetSpeed(int speed) override {}
  CDemuxStream* GetStream(int index) const override { return m_Streams[index]; }
  std::vector<CDemuxStream*> GetStreams() const override;
  int GetNrOfStreams() const override { return m_Streams.size(); }
  int  GetStreamLength() override { return 0; }
  std::string GetFileName() override { return m_Filename; }

  bool Open(const std::string& filename, int source, const std::string& subfilename);
  void EnableStream(int id, bool enable) override;

private:
  class CStream
    : public CDemuxStreamSubtitle
  {
  public:
    CStream(CDVDDemuxVobsub* parent)
      : m_discard(false), m_parent(parent)
    {}

    bool m_discard;
    CDVDDemuxVobsub* m_parent;
  };

  typedef struct STimestamp
  {
    int64_t pos;
    double  pts;
    int     id;
  } STimestamp;

  std::string m_Filename;
  std::unique_ptr<CDVDInputStream> m_Input;
  std::unique_ptr<CDVDDemuxFFmpeg> m_Demuxer;
  std::vector<STimestamp> m_Timestamps;
  std::vector<STimestamp>::iterator m_Timestamp;
  std::vector<CStream*> m_Streams;
  int m_source = -1;

  typedef struct SState
  {
    int id;
    double delay;
    std::string extra;
  } SState;

  struct sorter
  {
    bool operator()(const STimestamp &p1, const STimestamp &p2)
    {
      return p1.pts < p2.pts || (p1.pts == p2.pts && p1.id < p2.id);
    }
  };

  bool ParseLangIdx(SState& state, char* line);
  bool ParseDelay(SState& state, char* line);
  bool ParseId(SState& state, char* line);
  bool ParseExtra(SState& state, char* line);
  bool ParseTimestamp(SState& state, char* line);
};
