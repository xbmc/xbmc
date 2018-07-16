/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/Process/VideoBuffer.h"
#include "guilib/D3DResource.h"
#include "threads/Event.h"

#include <libavcodec/avcodec.h>
#include <libavcodec/d3d11va.h>
#include <vector>
#include <wrl/client.h>

class CProcessInfo;

namespace DXVA {

class CDXVABufferPool;

class CDXVAOutputBuffer : public CVideoBuffer
{
  friend CDXVABufferPool;
public:
  virtual ~CDXVAOutputBuffer();

  void SetRef(AVFrame *frame);
  void Unref();

  HANDLE GetHandle();
  unsigned GetIdx();

  ID3D11View* view{ nullptr };
  DXGI_FORMAT format{ DXGI_FORMAT_UNKNOWN };
  unsigned width{ 0 };
  unsigned height{ 0 };
  bool shared{ false };

private:
  CDXVAOutputBuffer(int id);

  AVFrame* m_pFrame{ nullptr };
};


class CDXVABufferPool : public IVideoBufferPool
{
public:
  CDXVABufferPool();
  virtual ~CDXVABufferPool();

  // IVideoBufferPool overrides
  CVideoBuffer* Get() override;
  void Return(int id) override;

  // views pool
  void AddView(ID3D11View* view);
  void ReturnView(ID3D11View* view);
  ID3D11View* GetView();
  bool IsValid(ID3D11View* view);
  size_t Size();
  bool HasFree();
  bool HasRefs();

protected:
  void Reset();
  CCriticalSection m_section;

  std::vector<ID3D11View*> m_views;
  std::deque<size_t> m_freeViews;
  std::vector<CDXVAOutputBuffer*> m_out;
  std::deque<size_t> m_freeOut;
};

class CDecoder;

class CDXVAContext
{
public:
  static bool EnsureContext(CDXVAContext **ctx, CDecoder *decoder);
  bool GetFormatAndConfig(AVCodecContext* avctx, D3D11_VIDEO_DECODER_DESC &format, D3D11_VIDEO_DECODER_CONFIG &config) const;
  bool CreateSurfaces(const D3D11_VIDEO_DECODER_DESC &format, const uint32_t count, const uint32_t alignment
                    , ID3D11VideoDecoderOutputView **surfaces) const;
  bool CreateDecoder(const D3D11_VIDEO_DECODER_DESC &format, const D3D11_VIDEO_DECODER_CONFIG &config
                   , ID3D11VideoDecoder **decoder, ID3D11VideoContext **context);
  void Release(CDecoder *decoder);
  ID3D11VideoContext* GetVideoContext() const { return m_vcontext.Get(); }
  bool IsContextShared() const { return m_sharingAllowed; }

private:
  CDXVAContext();
  void Close();
  bool CreateContext();
  void DestroyContext();
  void QueryCaps();
  bool IsValidDecoder(CDecoder *decoder);
  bool GetConfig(const D3D11_VIDEO_DECODER_DESC &format, D3D11_VIDEO_DECODER_CONFIG &config) const;

  static CDXVAContext *m_context;
  static CCriticalSection m_section;

  Microsoft::WRL::ComPtr<ID3D11VideoContext> m_vcontext;
  Microsoft::WRL::ComPtr<ID3D11VideoDevice> m_service;
  int m_refCount;
  UINT m_input_count;
  GUID *m_input_list;
  std::vector<CDecoder*> m_decoders;
  bool m_atiWorkaround;
  bool m_sharingAllowed;
};

class CDecoder
  : public IHardwareDecoder
  , public ID3DResource
{
public:
  virtual ~CDecoder();

  static IHardwareDecoder* Create(CDVDStreamInfo &hint, CProcessInfo &processInfo, AVPixelFormat fmt);
  static bool Register();

  // IHardwareDecoder overrides
  bool Open(AVCodecContext* avctx, AVCodecContext* mainctx, const enum AVPixelFormat) override;
  CDVDVideoCodec::VCReturn Decode(AVCodecContext* avctx, AVFrame* frame) override;
  bool GetPicture(AVCodecContext* avctx, VideoPicture* picture) override;
  CDVDVideoCodec::VCReturn Check(AVCodecContext* avctx) override;
  const std::string Name() override { return "d3d11va"; }
  unsigned GetAllowedReferences() override;
  void Reset() override;

  // IDVDResourceCounted overrides
  long Release() override;

  bool OpenDecoder();
  int GetBuffer(AVCodecContext *avctx, AVFrame *pic);
  void ReleaseBuffer(uint8_t *data);
  void Close();
  void CloseDXVADecoder();

  //static members
  static bool Supports(enum AVPixelFormat fmt);
  static int FFGetBuffer(AVCodecContext *avctx, AVFrame *pic, int flags);
  static void FFReleaseBuffer(void *opaque, uint8_t *data);

protected:
  enum EDeviceState
  { DXVA_OPEN
  , DXVA_RESET
  , DXVA_LOST
  } m_state;

  explicit CDecoder(CProcessInfo& processInfo);

  // ID3DResource overrides
  void OnCreateDevice() override  { CSingleLock lock(m_section); m_state = DXVA_RESET; m_event.Set(); }
  void OnDestroyDevice(bool fatal) override { CSingleLock lock(m_section); m_state = DXVA_LOST;  m_event.Reset(); }

  int m_refs;
  HANDLE m_device;
  Microsoft::WRL::ComPtr<ID3D11VideoDecoder> m_decoder;
  Microsoft::WRL::ComPtr<ID3D11VideoContext> m_vcontext;
  D3D11_VIDEO_DECODER_DESC m_format;
  CDXVAOutputBuffer *m_videoBuffer;
  struct AVD3D11VAContext *m_context;
  std::shared_ptr<CDXVABufferPool> m_bufferPool;
  CDXVAContext *m_dxva_context;
  AVCodecContext *m_avctx;
  unsigned int m_shared;
  unsigned int m_surface_alignment;
  CCriticalSection m_section;
  CEvent m_event;
  CProcessInfo& m_processInfo;
};

}
