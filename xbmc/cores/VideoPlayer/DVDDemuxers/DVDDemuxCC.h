/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDDemux.h"
#include "cea.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"

#include <atomic>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <vector>

class CDVDDemuxCC : public CDVDDemux
{
public:
  explicit CDVDDemuxCC(AVCodecID codec, const uint8_t* extradata, int extrasize);
  ~CDVDDemuxCC() override;

  bool Reset() override { return true; }
  void Flush() override; // drains the queue and reinitializes libcea
  DemuxPacket* Read() override { return nullptr; }
  bool SeekTime(double time, bool backwards = false, double* startpts = nullptr) override
  {
    return true;
  }
  CDemuxStream* GetStream(int iStreamId) const override;
  std::vector<CDemuxStream*> GetStreams() const override;
  int GetNrOfStreams() const override;

  std::vector<DemuxPacket*> Read(DemuxPacket* packet);

protected:
  void RunVideoPacketFeedThread(); // m_videoPacketFeedThread entry point
  void StartVideoPacketFeedThread();
  void StopVideoPacketFeedThread();

  static void CaptionCallback(const cea_caption* cap, void* userdata);
  static void LogCallback(cea_log_level level, const char* msg, void* userdata);

  bool InitLibcea();
  void EnsureStream(int field, int channel);
  bool HasStream(int uniqueId) const;
  CDemuxStreamSubtitle CreateStream(int field, int channel) const;
  static std::string BuildStreamName(int field, int channel);

  std::thread m_videoPacketFeedThread;
  std::atomic<bool> m_stopVideoPacketFeedThread{false};

  CEvent m_videoPacketFeedEvent;
  CCriticalSection m_videoPacketFeedSection;
  std::queue<DemuxPacket*> m_videoPacketFeedQueue;

  // Guards m_streams/m_captionQueue.
  mutable CCriticalSection m_captionSection;
  std::vector<CDemuxStreamSubtitle> m_streams;
  std::queue<DemuxPacket*> m_captionQueue;

  // Stored to allow reinitialization on Flush().
  cea_codec_type m_ceaCodec;
  cea_packaging_type m_ceaPkg;
  std::vector<uint8_t> m_extradata;

  struct CeaCtxDeleter
  {
    void operator()(cea_ctx* ctx) const { cea_free(ctx); }
  };
  std::unique_ptr<cea_ctx, CeaCtxDeleter> m_ceaCtx;
};
