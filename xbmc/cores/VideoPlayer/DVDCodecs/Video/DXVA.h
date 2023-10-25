/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Buffers/VideoBuffer.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "guilib/D3DResource.h"
#include "threads/Event.h"

#include <mutex>
#include <vector>

#include <d3d11_4.h>
#include <wrl/client.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavcodec/d3d11va.h>
}

namespace DXVA
{
class CDecoder;

class CVideoBuffer : public ::CVideoBuffer
{
  template<typename TBuffer>
  friend class CVideoBufferPoolTyped;

public:
  virtual ~CVideoBuffer();

  void SetRef(AVFrame* frame);
  void Unref();

  virtual void Initialize(CDecoder* decoder);
  virtual HRESULT GetResource(ID3D11Resource** ppResource);
  virtual unsigned GetIdx();

  ID3D11View* view = nullptr;
  DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
  unsigned width = 0;
  unsigned height = 0;

protected:
  explicit CVideoBuffer(int id);

private:
  AVFrame* m_pFrame{nullptr};
};

class CVideoBufferShared : public CVideoBuffer
{
  template<typename TBuffer>
  friend class CVideoBufferPoolTyped;

public:
  HRESULT GetResource(ID3D11Resource** ppResource) override;
  void Initialize(CDecoder* decoder) override;
  virtual ~CVideoBufferShared();

protected:
  explicit CVideoBufferShared(int id)
      : CVideoBuffer(id) {}
  void InitializeFence(CDecoder* decoder);
  void SetFence();

  HANDLE handle = INVALID_HANDLE_VALUE;
  Microsoft::WRL::ComPtr<ID3D11Resource> m_sharedRes;

  /*! \brief decoder-side fence object */
  Microsoft::WRL::ComPtr<ID3D11Fence> m_fence;
  /*! \brief decoder-side context */
  Microsoft::WRL::ComPtr<ID3D11DeviceContext4> m_deviceContext4;
  /*! \brief fence shared handle that allows opening the fence on a different device */
  HANDLE m_handleFence{INVALID_HANDLE_VALUE};
  UINT64 m_fenceValue{0};
  /*! \brief app-side fence object */
  Microsoft::WRL::ComPtr<ID3D11Fence> m_appFence;
  /*! \brief app-side context */
  Microsoft::WRL::ComPtr<ID3D11DeviceContext4> m_appContext4;
};

class CVideoBufferCopy : public CVideoBufferShared
{
  template<typename TBuffer>
  friend class CVideoBufferPoolTyped;

public:
  void Initialize(CDecoder* decoder) override;
  unsigned GetIdx() override { return 0; }

protected:
  explicit CVideoBufferCopy(int id)
      : CVideoBufferShared(id) {}

  Microsoft::WRL::ComPtr<ID3D11Resource> m_copyRes;
  Microsoft::WRL::ComPtr<ID3D11Resource> m_pResource;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pDeviceContext;
};

class CContext
{
public:
  typedef std::shared_ptr<CContext> shared_ptr;
  typedef std::weak_ptr<CContext> weak_ptr;

  ~CContext();

  static shared_ptr EnsureContext(CDecoder* decoder);
  bool GetFormatAndConfig(AVCodecContext* avctx, D3D11_VIDEO_DECODER_DESC& format, D3D11_VIDEO_DECODER_CONFIG& config) const;
  bool CreateSurfaces(const D3D11_VIDEO_DECODER_DESC& format, uint32_t count, uint32_t alignment,
                      ID3D11VideoDecoderOutputView** surfaces, HANDLE* pHandle, bool trueShared) const;
  bool CreateDecoder(const D3D11_VIDEO_DECODER_DESC& format, const D3D11_VIDEO_DECODER_CONFIG& config,
                     ID3D11VideoDecoder** decoder, ID3D11VideoContext** context);
  void Release(CDecoder* decoder);

  bool Check() const;
  bool Reset();
  bool IsContextShared() const
  {
    return m_sharingAllowed;
  }
  bool HasAMDWorkaround() const
  {
    return m_atiWorkaround;
  }

private:
  explicit CContext() = default;

  void Close();
  bool CreateContext();
  void DestroyContext();
  void QueryCaps();
  bool IsValidDecoder(CDecoder* decoder);
  bool GetConfig(const D3D11_VIDEO_DECODER_DESC& format, D3D11_VIDEO_DECODER_CONFIG& config) const;

  static weak_ptr m_context;
  static CCriticalSection m_section;

  UINT m_input_count = 0;
  GUID* m_input_list = nullptr;
  bool m_atiWorkaround = false;
  bool m_sharingAllowed = false;
  Microsoft::WRL::ComPtr<ID3D11VideoContext> m_pD3D11Context;
  Microsoft::WRL::ComPtr<ID3D11VideoDevice> m_pD3D11Device;
#ifdef _DEBUG
  Microsoft::WRL::ComPtr<ID3D11Debug> m_d3d11Debug;
#endif

  std::vector<CDecoder*> m_decoders;
};

class CVideoBufferPool : public IVideoBufferPool
{
public:
  typedef std::shared_ptr<CVideoBufferPool> shared_ptr;

  CVideoBufferPool();
  virtual ~CVideoBufferPool();

  // IVideoBufferPool overrides
  ::CVideoBuffer* Get() override;
  void Return(int id) override;

  // views pool
  void AddView(ID3D11View* view);
  bool ReturnView(ID3D11View* view);
  ID3D11View* GetView();
  bool IsValid(ID3D11View* view);
  size_t Size();
  bool HasFree();

protected:
  void Reset();
  virtual CVideoBuffer* CreateBuffer(int idx) = 0;

  CCriticalSection m_section;

  std::vector<ID3D11View*> m_views;
  std::deque<size_t> m_freeViews;
  std::vector<CVideoBuffer*> m_out;
  std::deque<size_t> m_freeOut;
};

template<typename TBuffer>
class CVideoBufferPoolTyped : public CVideoBufferPool
{
protected:
  CVideoBuffer* CreateBuffer(int idx) override
  {
    return new TBuffer(idx);
  }
};

class CDecoder : public IHardwareDecoder, public ID3DResource
{
public:
  ~CDecoder() override;

  static IHardwareDecoder* Create(CDVDStreamInfo& hint, CProcessInfo& processInfo, AVPixelFormat fmt);
  static bool Register();

  // IHardwareDecoder overrides
  bool Open(AVCodecContext* avctx, AVCodecContext* mainctx, const enum AVPixelFormat) override;
  CDVDVideoCodec::VCReturn Decode(AVCodecContext* avctx, AVFrame* frame) override;
  bool GetPicture(AVCodecContext* avctx, VideoPicture* picture) override;
  CDVDVideoCodec::VCReturn Check(AVCodecContext* avctx) override;
  const std::string Name() override { return "d3d11va"; }
  unsigned GetAllowedReferences() override;
  void Reset() override;

  bool OpenDecoder();
  int GetBuffer(AVCodecContext* avctx, AVFrame* pic);
  void ReleaseBuffer(uint8_t* data);
  void Close();
  void CloseDXVADecoder();

  //static members
  static bool Supports(enum AVPixelFormat fmt);
  static int FFGetBuffer(AVCodecContext* avctx, AVFrame* pic, int flags);
  static void FFReleaseBuffer(void* opaque, uint8_t* data);

protected:
  friend CVideoBuffer;
  friend CVideoBufferShared;
  friend CVideoBufferCopy;

  explicit CDecoder(CProcessInfo& processInfo);

  enum EDeviceState
  {
    DXVA_OPEN,
    DXVA_RESET,
    DXVA_LOST
  } m_state = DXVA_OPEN;


  // ID3DResource overrides
  void OnCreateDevice() override
  {
    std::unique_lock<CCriticalSection> lock(m_section);
    m_state = DXVA_RESET;
    m_event.Set();
  }
  void OnDestroyDevice(bool fatal) override
  {
    std::unique_lock<CCriticalSection> lock(m_section);
    m_state = DXVA_LOST;
    m_event.Reset();
  }

  CEvent m_event;
  CCriticalSection m_section;
  CProcessInfo& m_processInfo;
  Microsoft::WRL::ComPtr<ID3D11VideoDecoder> m_pD3D11Decoder;
  Microsoft::WRL::ComPtr<ID3D11VideoContext> m_pD3D11Context;
  CVideoBufferPool::shared_ptr m_bufferPool;
  CContext::shared_ptr m_dxvaContext;
  CVideoBuffer* m_videoBuffer = nullptr;
  struct AVD3D11VAContext* m_avD3D11Context = nullptr;
  int m_refs = 0;
  unsigned int m_shared = 0;
  unsigned int m_surface_alignment = 0;
  HANDLE m_sharedHandle = INVALID_HANDLE_VALUE;
  D3D11_VIDEO_DECODER_DESC m_format = {};
  bool m_DVDWorkaround = false;
};

} // namespace DXVA
