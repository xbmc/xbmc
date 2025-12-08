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

#if defined(HAVE_LIBBLURAY)
#include "DVDInputStreams/DVDInputStreamBluray.h"
#endif
#include "DVDInputStreams/DVDInputStream.h"
#include <queue>

extern "C" {
#include "libavformat/avformat.h"
}

class CDemuxStreamSSIF
{
public:
  CDemuxStreamSSIF() {};
  ~CDemuxStreamSSIF() { Flush(); }

  DemuxPacket* AddPacket(DemuxPacket* &scrPkt);
  void Flush();
  void SetH264StreamId(int id) { m_h264StreamId = id; };
  void SetMVCStreamId(int id) { m_mvcStreamId = id; };
  int GetH264StreamId() const { return m_h264StreamId; };
  int GetMVCStreamId() const { return m_mvcStreamId; };
  void AddMVCExtPacket(DemuxPacket* &scrPkt);
  void SetBluRay(const std::shared_ptr<CDVDInputStream::IExtentionStream> &bluRay) { m_bluRay = bluRay; };
  bool IsBluRay() const { return m_bluRay != nullptr; };

private:
  DemuxPacket* GetMVCPacket();
  DemuxPacket* MergePacket(DemuxPacket* &srcPkt, DemuxPacket* &appendPkt);
  bool FillMVCQueue(double dtsBase);

  std::shared_ptr<CDVDInputStream::IExtentionStream> m_bluRay = nullptr;
  std::queue<DemuxPacket*> m_H264queue;
  std::queue<DemuxPacket*> m_MVCqueue;
  int m_h264StreamId = -1;
  int m_mvcStreamId = -1;
};
