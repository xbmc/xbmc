/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
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

#include <map>
#include <memory>
#include <vector>
#include "DVDInputStream.h"

class CFileItem;
class IDemux;
class IVideoPlayer;
struct PVR_STREAM_PROPERTIES;

namespace PVR
{
  class CPVRClient;
}

class CInputStreamPVRBase
  : public CDVDInputStream
  , public CDVDInputStream::ITimes
  , public CDVDInputStream::IDemux
{
public:
  CInputStreamPVRBase(IVideoPlayer* pPlayer, const CFileItem& fileitem);
  ~CInputStreamPVRBase() override;
  bool Open() override;
  void Close() override;
  int Read(uint8_t* buf, int buf_size) override;
  int64_t Seek(int64_t offset, int whence) override;
  bool Pause(double dTime) override { return false; }
  bool IsEOF() override;
  int64_t GetLength() override;
  int GetBlockSize() override;

  ENextStream NextStream() override;
  bool IsRealtime() override;

  CDVDInputStream::ITimes* GetITimes() override { return this; }
  bool GetTimes(Times &times) override;

  bool CanSeek() override;
  bool CanPause() override;
  void Pause(bool bPaused);

  // Demux interface
  CDVDInputStream::IDemux* GetIDemux() override { return nullptr; };
  bool OpenDemux() override;
  DemuxPacket* ReadDemux() override;
  CDemuxStream* GetStream(int iStreamId) const override;
  std::vector<CDemuxStream*> GetStreams() const override;
  int GetNrOfStreams() const override;
  void SetSpeed(int iSpeed) override;
  bool SeekTime(double time, bool backward = false, double* startpts = NULL) override;
  void AbortDemux() override;
  void FlushDemux() override;

protected:
  void UpdateStreamMap();
  std::shared_ptr<CDemuxStream> GetStreamInternal(int iStreamId);

  virtual bool OpenPVRStream() = 0;
  virtual void ClosePVRStream() = 0;
  virtual int ReadPVRStream(uint8_t* buf, int buf_size) = 0;
  virtual int64_t SeekPVRStream(int64_t offset, int whence) = 0;
  virtual int64_t GetPVRStreamLength() = 0;
  virtual ENextStream NextPVRStream() = 0;
  virtual bool CanPausePVRStream() = 0;
  virtual bool CanSeekPVRStream() = 0;

  bool m_eof;
  std::shared_ptr<PVR_STREAM_PROPERTIES> m_StreamProps;
  std::map<int, std::shared_ptr<CDemuxStream>> m_streamMap;
  std::shared_ptr<PVR::CPVRClient> m_client;
};
