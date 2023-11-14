/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDStreamInfo.h"
#include "DVDVideoCodec.h"
#include "cores/VideoPlayer/Buffers/VideoBuffer.h"
#include "threads/SingleLock.h"
#include "threads/Thread.h"
#include "utils/Geometry.h"

#include "platform/android/activity/JNIXBMCVideoView.h"

#include <atomic>
#include <deque>
#include <memory>
#include <utility>
#include <vector>

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <androidjni/Surface.h>

class CJNISurface;
class CJNISurfaceTexture;
class CJNIMediaCodec;
class CJNIMediaCrypto;
class CJNIMediaFormat;
class CJNIMediaCodecBufferInfo;
class CDVDMediaCodecOnFrameAvailable;
class CJNIByteBuffer;
class CBitstreamConverter;

struct DemuxCryptoInfo;
struct mpeg2_sequence;


typedef struct amc_demux
{
  uint8_t* pData;
  int iSize;
  double dts;
  double pts;
} amc_demux;

class CMediaCodecVideoBufferPool;

class CMediaCodecVideoBuffer : public CVideoBuffer
{
public:
  CMediaCodecVideoBuffer(int id) : CVideoBuffer(id) {}
  ~CMediaCodecVideoBuffer() override = default;

  void Set(int internalId,
           int textureId,
           std::shared_ptr<CJNISurfaceTexture> surfaceTexture,
           std::shared_ptr<CDVDMediaCodecOnFrameAvailable> frameAvailable,
           std::shared_ptr<jni::CJNIXBMCVideoView> videoView);

  // meat and potatoes
  bool WaitForFrame(int millis);
  // MediaCodec related
  void ReleaseOutputBuffer(bool render,
                           int64_t displayTime,
                           CMediaCodecVideoBufferPool* pool = nullptr);
  // SurfaceTexture released
  int GetBufferId() const;
  int GetTextureId() const;
  void GetTransformMatrix(float* textureMatrix);
  void UpdateTexImage();
  void RenderUpdate(const CRect& DestRect, int64_t displayTime);
  bool HasSurfaceTexture() const { return m_surfacetexture.operator bool(); }

private:
  int m_bufferId = -1;
  unsigned int m_textureId = 0;
  // shared_ptr bits, shared between
  // CDVDVideoCodecAndroidMediaCodec and LinuxRenderGLES.
  std::shared_ptr<CJNISurfaceTexture> m_surfacetexture;
  std::shared_ptr<CDVDMediaCodecOnFrameAvailable> m_frameready;
  std::shared_ptr<jni::CJNIXBMCVideoView> m_videoview;
};

class CMediaCodecVideoBufferPool : public IVideoBufferPool
{
public:
  CMediaCodecVideoBufferPool(std::shared_ptr<CJNIMediaCodec> mediaCodec)
    : m_codec(std::move(mediaCodec))
  {
  }

  ~CMediaCodecVideoBufferPool() override;

  CVideoBuffer* Get() override;
  void Return(int id) override;

  std::shared_ptr<CJNIMediaCodec> GetMediaCodec();
  void ResetMediaCodec();
  void ReleaseMediaCodecBuffers();

private:
  CCriticalSection m_criticalSection;
  std::shared_ptr<CJNIMediaCodec> m_codec;

  std::vector<CMediaCodecVideoBuffer*> m_videoBuffers;
  std::vector<int> m_freeBuffers;
};

class CDVDVideoCodecAndroidMediaCodec : public CDVDVideoCodec, public CJNISurfaceHolderCallback
{
public:
  CDVDVideoCodecAndroidMediaCodec(CProcessInfo& processInfo, bool surface_render = false);
  ~CDVDVideoCodecAndroidMediaCodec() override;

  // registration
  static std::unique_ptr<CDVDVideoCodec> Create(CProcessInfo& processInfo);
  static bool Register();

  // required overrides
  bool Open(CDVDStreamInfo& hints, CDVDCodecOptions& options) override;
  bool AddData(const DemuxPacket& packet) override;
  void Reset() override;
  bool Reconfigure(CDVDStreamInfo& hints) override;
  VCReturn GetPicture(VideoPicture* pVideoPicture) override;
  const char* GetName() override { return m_formatname.c_str(); }
  void SetCodecControl(int flags) override;
  unsigned GetAllowedReferences() override;

protected:
  void Dispose();
  void FlushInternal(void);
  void SignalEndOfStream();
  void InjectExtraData(CJNIMediaFormat& mediaformat);
  std::vector<uint8_t> GetHDRStaticMetadata();
  bool ConfigureMediaCodec(void);
  int GetOutputPicture(void);
  void ConfigureOutputFormat(CJNIMediaFormat& mediaformat);
  void UpdateFpsDuration();

  // surface handling functions
  static void CallbackInitSurfaceTexture(void*);
  void InitSurfaceTexture(void);
  void ReleaseSurfaceTexture(void);

  CDVDStreamInfo m_hints;
  std::string m_mime;
  std::string m_codecname;
  std::string m_formatname;
  bool m_opened = false;
  bool m_needSecureDecoder = false;
  int m_codecControlFlags;
  int m_state;

  std::shared_ptr<jni::CJNIXBMCVideoView> m_jnivideoview;
  CJNISurface m_jnivideosurface;
  unsigned int m_textureId = 0;
  std::shared_ptr<CJNIMediaCodec> m_codec;
  CJNIMediaCrypto* m_crypto = nullptr;
  std::shared_ptr<CJNISurfaceTexture> m_surfaceTexture;
  std::shared_ptr<CDVDMediaCodecOnFrameAvailable> m_frameAvailable;

  amc_demux m_demux_pkt;
  std::shared_ptr<CMediaCodecVideoBufferPool> m_videoBufferPool;

  uint32_t m_OutputDuration = 0, m_fpsDuration = 0;
  int64_t m_lastPTS = -1;
  int64_t m_invalidPTSValue = 0;
  double m_dtsShift = DVD_NOPTS_VALUE;

  static std::atomic<bool> m_InstanceGuard;

  std::unique_ptr<CBitstreamConverter> m_bitstream;
  VideoPicture m_videobuffer;

  int m_indexInputBuffer;
  bool m_render_surface;
  mpeg2_sequence* m_mpeg2_sequence = nullptr;
  int m_src_offset[4];
  int m_src_stride[4];
  bool m_useDTSforPTS = false;

  // CJNISurfaceHolderCallback interface
public:
  void surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height) override;
  void surfaceCreated(CJNISurfaceHolder holder) override;
  void surfaceDestroyed(CJNISurfaceHolder holder) override;
};
