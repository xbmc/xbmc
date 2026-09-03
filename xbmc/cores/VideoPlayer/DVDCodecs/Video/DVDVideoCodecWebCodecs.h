/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "DVDVideoCodec.h"
#include "DVDVideoCodecWebCodecsBridge.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"

#include <cstdint>
#include <memory>
#include <string>

class CVideoBufferPoolSysMem;

class CDVDVideoCodecWebCodecs : public CDVDVideoCodec
{
public:
  explicit CDVDVideoCodecWebCodecs(CProcessInfo& processInfo);
  ~CDVDVideoCodecWebCodecs() override;

  static std::unique_ptr<CDVDVideoCodec> Create(CProcessInfo& processInfo);
  static bool Register();

  bool Open(CDVDStreamInfo& hints, CDVDCodecOptions& options) override;
  bool AddData(const DemuxPacket& packet) override;
  void Reset() override;
  VCReturn GetPicture(VideoPicture* pVideoPicture) override;
  const char* GetName() override { return m_name.c_str(); }
  void SetCodecControl(int flags) override;

private:
  bool CreateDecoder();
  void Dispose();
  bool SupportsCodec(const CDVDStreamInfo& hints) const;
  bool BuildCodecConfiguration(const CDVDStreamInfo& hints);
  void PollDecoderStats();
  static int32_t SharedLoad(const int32_t& field);
  void WaitForDecoderSignal(uint32_t seenSignal, double maxWaitMs);
  void WaitForDrain();
  bool WaitForCopy();
  void ReleaseCopyBuffer();
  CVideoBuffer* AcquirePictureBuffer(AVPixelFormat pixelFormat, int bufferSize);
  void FillPictureMetadata(VideoPicture* pVideoPicture,
                           CVideoBuffer* videoBuffer,
                           AVPixelFormat pixelFormat,
                           int width,
                           int height,
                           bool keyFrame,
                           double ptsSeconds,
                           double durationSeconds) const;

  std::string m_name{"webcodecs"};
  CDVDStreamInfo m_hints;
  std::shared_ptr<CVideoBufferPoolSysMem> m_videoBufferPool;

  int m_decoderHandle{0};
  bool m_opened{false};
  bool m_drainSubmitted{false};
  bool m_waitingForKeyFrame{true};
  bool m_annexB{false};
  int m_nalLengthSize{0};
  int m_codecControlFlags{0};
  int m_lastLoggedDroppedFrames{0};
  int m_highWaterMark{0};

  // Written by the JS bridge for the lifetime of m_decoderHandle.
  WebCodecsSharedState m_shared{};
  // Handed to the JS bridge as copy destination; owned here until the copy settles.
  CVideoBuffer* m_copyBuffer{nullptr};

  std::string m_codecString;
};
