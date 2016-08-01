#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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
#include <map>
#include <vector>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

class CDVDDemuxClient : public CDVDDemux
{
public:

  CDVDDemuxClient();
  ~CDVDDemuxClient();

  bool Open(CDVDInputStream* pInput);
  void Dispose();
  void Reset() override;
  void Abort() override;
  void Flush() override;
  DemuxPacket* Read() override;
  bool SeekTime(int time, bool backwords = false, double* startpts = NULL) override;
  void SetSpeed(int iSpeed) override;
  int GetStreamLength() override { return 0; }
  CDemuxStream* GetStream(int iStreamId) const override;
  std::vector<CDemuxStream*> GetStreams() const override;
  int GetNrOfStreams() const override;
  std::string GetFileName() override;
  virtual std::string GetStreamCodecName(int iStreamId) override;
  virtual void EnableStream(int id, bool enable) override;
  virtual void SetVideoResolution(int width, int height) override;

protected:
  void RequestStreams();
  void ParsePacket(DemuxPacket* pPacket);
  void DisposeStream(int iStreamId);
  void DisposeStreams();
  std::shared_ptr<CDemuxStream> GetStreamInternal(int iStreamId);
  
  CDVDInputStream* m_pInput;
  CDVDInputStream::IDemux *m_IDemux;
  std::map<int, std::shared_ptr<CDemuxStream>> m_streams;
  int m_displayTime;
  double m_dtsAtDisplayTime;
};

