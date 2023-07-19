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
#include <string>
#include <vector>

class CDVDOverlayCodecFFmpeg;
class CDVDInputStream;
class CDVDDemuxFFmpeg;

class CDVDDemuxVobsub : public CDVDDemux
{
public:
  CDVDDemuxVobsub();
  ~CDVDDemuxVobsub() override;

  bool Open(const std::string& filename, int source, const std::string& subfilename);

  // implementation of CDVDDemux
  bool Reset() override;
  void Flush() override;
  DemuxPacket* Read() override;
  bool SeekTime(double time, bool backwards, double* startpts = NULL) override;
  CDemuxStream* GetStream(int index) const override { return m_Streams[index]; }
  std::vector<CDemuxStream*> GetStreams() const override;
  int GetNrOfStreams() const override { return m_Streams.size(); }
  std::string GetFileName() override { return m_Filename; }
  void EnableStream(int id, bool enable) override;

private:
  class CStream
    : public CDemuxStreamSubtitle
  {
  public:
    explicit CStream(CDVDDemuxVobsub* parent) : m_parent(parent) {}

    bool m_discard = false;
    CDVDDemuxVobsub* m_parent;
  };

  typedef struct STimestamp
  {
    int64_t pos;
    double  pts;
    int     id;
  } STimestamp;

  std::string m_Filename;
  std::shared_ptr<CDVDInputStream> m_Input;
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

  bool ParseLangIdx(SState& state, std::string& line);
  bool ParseDelay(SState& state, std::string& line);
  bool ParseId(SState& state, std::string& line);
  bool ParseExtra(SState& state, const std::string& line);
  bool ParseTimestamp(SState& state, std::string& line);
};
