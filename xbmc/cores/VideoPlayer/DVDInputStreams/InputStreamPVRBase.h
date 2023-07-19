/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDInputStream.h"

#include <map>
#include <memory>
#include <vector>

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
  bool IsEOF() override;
  int64_t GetLength() override;
  int GetBlockSize() override;

  ENextStream NextStream() override;
  bool IsRealtime() override;

  CDVDInputStream::ITimes* GetITimes() override { return this; }
  bool GetTimes(Times &times) override;

  bool CanSeek() override; //! @todo drop this
  bool CanPause() override;
  void Pause(bool bPaused);

  // Demux interface
  CDVDInputStream::IDemux* GetIDemux() override { return nullptr; }
  bool OpenDemux() override;
  DemuxPacket* ReadDemux() override;
  CDemuxStream* GetStream(int iStreamId) const override;
  std::vector<CDemuxStream*> GetStreams() const override;
  int GetNrOfStreams() const override;
  void SetSpeed(int iSpeed) override;
  void FillBuffer(bool mode) override;
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

  bool m_eof = true;
  std::shared_ptr<PVR_STREAM_PROPERTIES> m_StreamProps;
  std::map<int, std::shared_ptr<CDemuxStream>> m_streamMap;
  std::shared_ptr<PVR::CPVRClient> m_client;
};
