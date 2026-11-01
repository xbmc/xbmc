/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Buffers/VideoBuffer.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "threads/CriticalSection.h"

#include <deque>
#include <memory>
#include <vector>

// Must come before libavutil/hwcontext_cuda.h, which otherwise includes the CUDA toolkit's cuda.h.
#include <ffnvcodec/dynlink_loader.h>

extern "C"
{
#include <libavfilter/avfilter.h>
#include <libavutil/frame.h>
#include <libavutil/hwcontext.h>
}

class CProcessInfo;

namespace NVDEC
{

/*!
 * \brief CUDA driver functions, loaded at runtime from the NVIDIA driver
 * (libcuda.so) the same way FFmpeg does. Returns nullptr if no driver is present.
 */
CudaFunctions* GetCudaFunctions();

/*!
 * \brief Called by the renderer when CUDA-GL interop fails. The decoder then
 * gives way to software decoding for the rest of the session.
 */
void SetInteropFailed();

/*!
 * \brief A decoded frame that stays in GPU memory (AV_PIX_FMT_CUDA).
 * The renderer copies its planes into GL textures via CUDA-GL interop.
 */
class CVideoBufferNVDEC : public CVideoBuffer
{
public:
  explicit CVideoBufferNVDEC(int id);
  ~CVideoBufferNVDEC() override;

  //! False if the frame couldn't be referenced or isn't a CUDA frame
  bool SetRef(AVFrame* frame);
  void Unref();

  const AVFrame* GetFrame() const { return m_frame; }
  CUcontext GetCudaContext() const;
  //! The CUDA device, referenced by users that outlive the decoder
  AVBufferRef* GetDeviceRef() const;
  //! NV12 or P010/P016, the layout of the planes in GPU memory
  AVPixelFormat GetSwFormat() const { return m_swFormat; }

private:
  AVFrame* m_frame = nullptr;
  AVPixelFormat m_swFormat = AV_PIX_FMT_NONE;
};

class CVideoBufferPoolNVDEC : public IVideoBufferPool
{
public:
  ~CVideoBufferPoolNVDEC() override;
  void Return(int id) override;
  CVideoBuffer* Get() override;

private:
  CCriticalSection m_critSection;
  std::vector<CVideoBufferNVDEC*> m_all;
  std::deque<int> m_used;
  std::deque<int> m_free;
};

class CDecoder : public IHardwareDecoder
{
public:
  explicit CDecoder(CProcessInfo& processInfo);
  ~CDecoder() override;

  static IHardwareDecoder* Create(CDVDStreamInfo& hint,
                                  CProcessInfo& processInfo,
                                  AVPixelFormat fmt);
  static void Register();

  bool Open(AVCodecContext* avctx, AVCodecContext* mainctx, const enum AVPixelFormat) override;
  CDVDVideoCodec::VCReturn Decode(AVCodecContext* avctx, AVFrame* frame) override;
  bool GetPicture(AVCodecContext* avctx, VideoPicture* picture) override;
  CDVDVideoCodec::VCReturn Check(AVCodecContext* avctx) override;
  const std::string Name() override { return "nvdec"; }
  unsigned GetAllowedReferences() override;
  void Reset() override;
  void SetCodecControl(int flags) override { m_codecControl = flags; }

private:
  bool InitDeinterlacer(const AVFrame* frame);
  void CloseDeinterlacer();
  bool PullDeinterlacedFrame();
  bool SetRenderBuffer(AVFrame* frame);

  CProcessInfo& m_processInfo;
  AVBufferRef* m_deviceRef = nullptr;
  AVFilterGraph* m_filterGraph = nullptr;
  AVFilterContext* m_bufferSrc = nullptr;
  AVFilterContext* m_bufferSink = nullptr;
  AVFrame* m_filterFrame = nullptr;
  int m_codecControl = 0;
  bool m_deintFailed = false;
  bool m_filterEof = false;
  bool m_filtered = false; //!< current picture came out of the deinterlacer
  double m_deintPts = 0.0;
  CVideoBufferNVDEC* m_renderBuffer = nullptr;
  std::shared_ptr<CVideoBufferPoolNVDEC> m_videoBufferPool;
};

} // namespace NVDEC
