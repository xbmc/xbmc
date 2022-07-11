/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDDemux.h"
#include "DVDInputStreams/DVDInputStream.h"

#include <map>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

class CDVDDemuxClient : public CDVDDemux
{
public:

  CDVDDemuxClient();
  ~CDVDDemuxClient() override;

  bool Open(std::shared_ptr<CDVDInputStream> pInput);
  void Dispose();
  bool Reset() override;
  void Abort() override;
  void Flush() override;
  DemuxPacket* Read() override;
  bool SeekTime(double time, bool backwards = false, double* startpts = NULL) override;
  void SetSpeed(int iSpeed) override;
  void FillBuffer(bool mode) override;
  CDemuxStream* GetStream(int iStreamId) const override;
  std::vector<CDemuxStream*> GetStreams() const override;
  int GetNrOfStreams() const override;
  std::string GetFileName() override;
  std::string GetStreamCodecName(int iStreamId) override;
  void EnableStream(int id, bool enable) override;
  void OpenStream(int id) override;
  void SetVideoResolution(unsigned int width, unsigned int height) override;

protected:
  void RequestStreams();
  void SetStreamProps(CDemuxStream *stream, std::map<int, std::shared_ptr<CDemuxStream>> &map, bool forceInit);
  bool ParsePacket(DemuxPacket* pPacket);
  void DisposeStreams();
  std::shared_ptr<CDemuxStream> GetStreamInternal(int iStreamId);
  bool IsVideoReady();

  std::shared_ptr<CDVDInputStream> m_pInput;
  std::shared_ptr<CDVDInputStream::IDemux> m_IDemux;
  std::map<int, std::shared_ptr<CDemuxStream>> m_streams;
  int m_displayTime;
  double m_dtsAtDisplayTime;
  std::unique_ptr<DemuxPacket> m_packet;
  int m_videoStreamPlaying = -1;

private:
  static inline bool CodecHasExtraData(AVCodecID id);
};

