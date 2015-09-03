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
  virtual ~CDVDDemuxVobsub();

  virtual void          Reset();
  virtual void          Abort() {};
  virtual void          Flush();
  virtual DemuxPacket*  Read();
  virtual bool          SeekTime(int time, bool backwords, double* startpts = NULL);
  virtual void          SetSpeed(int speed) {}
  virtual CDemuxStream* GetStream(int index) { return m_Streams[index]; }
  virtual int           GetNrOfStreams()     { return m_Streams.size(); }
  virtual int           GetStreamLength()    { return 0; }
  virtual std::string   GetFileName()        { return m_Filename; }

  bool                  Open(const std::string& filename, int source, const std::string& subfilename);

private:
  class CStream
    : public CDemuxStreamSubtitle
  {
  public:
    CStream(CDVDDemuxVobsub* parent)
      : m_discard(AVDISCARD_NONE), m_parent(parent)
    {}
    virtual void      SetDiscard(AVDiscard discard) { m_discard = discard; }
    virtual AVDiscard GetDiscard()                  { return m_discard; }

    AVDiscard        m_discard;
    CDVDDemuxVobsub* m_parent;
  };

  typedef struct STimestamp
  {
    int64_t pos;
    double  pts;
    int     id;
  } STimestamp;

  std::string                        m_Filename;
  std::unique_ptr<CDVDInputStream>     m_Input;
  std::unique_ptr<CDVDDemuxFFmpeg>     m_Demuxer;
  std::vector<STimestamp>            m_Timestamps;
  std::vector<STimestamp>::iterator  m_Timestamp;
  std::vector<CStream*> m_Streams;
  int m_source;

  typedef struct SState
  {
    int         id;
    double      delay;
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
