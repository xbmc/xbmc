#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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
#include <vector>

#ifdef TARGET_WINDOWS
#define __attribute__(dummy_val)
#endif

class CDemuxStreamAudioCDDA;

class CDVDDemuxCDDA : public CDVDDemux
{
public:

  CDVDDemuxCDDA();
  ~CDVDDemuxCDDA();

  bool Open(CDVDInputStream* pInput);
  void Dispose();

  // implementation of CDVDDemux
  virtual void Reset() override;
  virtual void Abort() override;
  virtual void Flush() override;
  virtual DemuxPacket* Read() override;
  bool SeekTime(double time, bool backwards = false, double* startpts = NULL) override;
  virtual void SetSpeed(int iSpeed) override {}
  virtual int GetStreamLength() override;
  CDemuxStream* GetStream(int iStreamId) const override;
  std::vector<CDemuxStream*> GetStreams() const override;
  int GetNrOfStreams() const override;
  virtual std::string GetFileName() override;
  virtual std::string GetStreamCodecName(int iStreamId) override;

protected:
  friend class CDemuxStreamAudioCDDA;
  CDVDInputStream* m_pInput;
  int64_t m_bytes;

  CDemuxStreamAudioCDDA *m_stream;
};
