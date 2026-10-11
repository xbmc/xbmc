/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDVideoCodec.h"
#include "DVDVideoCodecWebCodecsBridge.h"
#include "cores/VideoPlayer/Buffers/VideoBuffer.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecs.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "threads/CriticalSection.h"

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <vector>

class CVideoBufferPoolSysMem;
struct SwsContext;

// A picture whose pixels never enter the wasm heap: it names a VideoFrame the
// bridge holds on the browser main thread by decoder handle and sequence
// number. CRendererWebCodecs uploads it into a GL texture by that number, and
// returning the buffer to its pool closes the frame unless it is already closed.
class CVideoBufferWebCodecs : public CVideoBuffer
{
public:
  explicit CVideoBufferWebCodecs(int id);

  void SetFrame(int decoderHandle, int32_t sequence, AVPixelFormat format);
  int GetDecoderHandle() const { return m_decoderHandle; }
  int32_t GetSequence() const { return m_sequence; }

private:
  int m_decoderHandle{0};
  int32_t m_sequence{-1};
};

class CVideoBufferPoolWebCodecs : public IVideoBufferPool
{
public:
  CVideoBuffer* Get() override;
  void Return(int id) override;

private:
  CCriticalSection m_critSection;
  std::vector<std::unique_ptr<CVideoBufferWebCodecs>> m_all;
  std::deque<int> m_free;
};

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
  const char* GetName() override { return m_fallback ? m_fallback->GetName() : m_name.c_str(); }
  unsigned GetConvergeCount() override;
  bool GetCodecStats(double& pts, int& droppedFrames, int& skippedPics) override;
  void SetCodecControl(int flags) override;
  void Reopen() override;

private:
  bool CreateDecoder();
  void Dispose();
  bool FallBackToFFmpeg();
  bool SupportsCodec(const CDVDStreamInfo& hints) const;
  bool BuildCodecConfiguration(const CDVDStreamInfo& hints);
  void PollDecoderStats();
  static int32_t SharedLoad(const int32_t& field);
  int32_t QueuedFrames() const;
  void PublishFramesTaken(int32_t sequence);
  void WaitForDecoderSignal(uint32_t seenSignal, double maxWaitMs);
  bool DecoderBusy() const;
  bool WaitForConfigured();
  bool WaitForDrain();
  int32_t WaitForCopy(int copyId);
  void ReleaseCopyBuffer();
  void ReportPixelFormat(const char* name);
  VCReturn TakeFrame(int32_t sequence, const WebCodecsFrameInfo& info, VideoPicture* pVideoPicture);
  VCReturn CopyFrame(int32_t sequence, const WebCodecsFrameInfo& info, VideoPicture* pVideoPicture);
  VCReturn DroppedPicture(const WebCodecsFrameInfo& info,
                          AVPixelFormat pixelFormat,
                          VideoPicture* pVideoPicture);
  CVideoBuffer* AcquirePictureBuffer(std::shared_ptr<CVideoBufferPoolSysMem>& pool,
                                     AVPixelFormat pixelFormat,
                                     int bufferSize);
  CVideoBuffer* ConvertToYuv420p(CVideoBuffer* rgbBuffer, WebCodecsFrameInfo& info);
  void FillPictureMetadata(VideoPicture* pVideoPicture,
                           CVideoBuffer* videoBuffer,
                           AVPixelFormat pixelFormat,
                           const WebCodecsFrameInfo& info,
                           bool colourFromFrame) const;

  std::string m_name{"webcodecs"};
  CDVDStreamInfo m_hints;
  // Frames stay on the browser side and are imported into the renderer's
  // textures; false when the page's WebGL rejects a VideoFrame source.
  bool m_textureUpload{false};
  std::shared_ptr<CVideoBufferPoolWebCodecs> m_framePool;
  std::shared_ptr<CVideoBufferPoolSysMem> m_videoBufferPool;
  // Landing zone for packed RGB frames before ConvertToYuv420p().
  std::shared_ptr<CVideoBufferPoolSysMem> m_rgbBufferPool;
  SwsContext* m_swsContext{nullptr};
  int m_swsWidth{0};
  int m_swsHeight{0};
  AVPixelFormat m_swsFormat{AV_PIX_FMT_NONE};
  bool m_loggedRgbConversion{false};
  std::string m_reportedPixelFormat;

  int m_decoderHandle{0};
  bool m_opened{false};
  bool m_drained{false};
  bool m_waitingForKeyFrame{true};
  bool m_annexB{false};
  bool m_prependExtradata{false};
  // The browser accepted 'prefer-hardware'; false when it was left to choose
  // the decoder, which WebCodecs does not identify.
  bool m_hardware{false};
  bool m_hasDescription{false};
  int m_nalLengthSize{0};
  int m_codecControlFlags{0};
  int m_lastLoggedDroppedFrames{0};
  int m_highWaterMark{0};
  int m_recoveries{0};
  int m_framesSinceRecovery{0};
  CDVDCodecOptions m_options;
  // Decodes the rest of the stream once the browser's decoder keeps failing.
  std::unique_ptr<CDVDVideoCodec> m_fallback;
  bool m_fallbackTried{false};

  // Written by the JS bridge for the lifetime of m_decoderHandle, except
  // framesTaken, which this side publishes.
  WebCodecsSharedState m_shared{};
  // Handed to the JS bridge as copy destination; owned here until the copy settles.
  CVideoBuffer* m_copyBuffer{nullptr};
  int m_copyId{0};
  // Pushes issued; the bridge's pushesProcessed trails it by the calls the main
  // thread has not run yet.
  int m_pushCount{0};

  std::string m_codecString;
};
