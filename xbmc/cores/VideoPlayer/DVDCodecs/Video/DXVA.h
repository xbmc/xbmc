/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/Process/VideoBuffer.h"
#include "guilib/D3DResource.h"
#include "threads/Event.h"

#include <libavcodec/avcodec.h>
#include <libavcodec/d3d11va.h>
#include <vector>

class CProcessInfo;

namespace DXVA {

class CDXVABufferPool;

class CDXVAOutputBuffer : public CVideoBuffer
{
  friend CDXVABufferPool;
public:
  virtual ~CDXVAOutputBuffer();

  ID3D11View* GetSRV(unsigned idx);
  void SetRef(AVFrame *frame);
  void Unref();

  ID3D11View* view{ nullptr };
  DXGI_FORMAT format{ DXGI_FORMAT_UNKNOWN };
  unsigned width{ 0 };
  unsigned height{ 0 };

private:
  CDXVAOutputBuffer(int id);

  ID3D11View* planes[2]{ nullptr, nullptr };
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
  int Size();
  bool HasFree();
  bool HasRefs();

protected:
  void Reset();
  CCriticalSection m_section;

  std::vector<ID3D11View*> m_views;
  std::deque<int> m_freeViews;
  std::vector<CDXVAOutputBuffer*> m_out;
  std::deque<int> m_freeOut;
};

class CDecoder;

class CDXVAContext
{
public:
  static bool EnsureContext(CDXVAContext **ctx, CDecoder *decoder);
  bool GetInputAndTarget(int codec, bool bHighBitdepth, GUID &inGuid, DXGI_FORMAT &outFormat) const;
  bool GetConfig(const D3D11_VIDEO_DECODER_DESC *format, D3D11_VIDEO_DECODER_CONFIG &config) const;
  bool CreateSurfaces(D3D11_VIDEO_DECODER_DESC format, unsigned int count, unsigned int alignment, ID3D11VideoDecoderOutputView **surfaces) const;
  bool CreateDecoder(D3D11_VIDEO_DECODER_DESC *format, const D3D11_VIDEO_DECODER_CONFIG *config, ID3D11VideoDecoder **decoder, ID3D11VideoContext **context);
  void Release(CDecoder *decoder);
  ID3D11VideoContext* GetVideoContext() const { return m_vcontext; }

private:
  CDXVAContext();
  void Close();
  bool CreateContext();
  void DestroyContext();
  void QueryCaps();
  bool IsValidDecoder(CDecoder *decoder);

  static CDXVAContext *m_context;
  static CCriticalSection m_section;

  ID3D11VideoContext* m_vcontext;
  ID3D11VideoDevice* m_service;
  int m_refCount;
  UINT m_input_count;
  GUID *m_input_list;
  std::vector<CDecoder*> m_decoders;
  bool m_atiWorkaround;
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
  void OnCreateDevice() override  {}
  void OnDestroyDevice(bool fatal) override { CSingleLock lock(m_section); m_state = DXVA_LOST;  m_event.Reset(); }
  void OnLostDevice() override    { CSingleLock lock(m_section); m_state = DXVA_LOST;  m_event.Reset(); }
  void OnResetDevice() override   { CSingleLock lock(m_section); m_state = DXVA_RESET; m_event.Set();   }

  int m_refs;
  HANDLE m_device;
  ID3D11VideoDecoder *m_decoder;
  ID3D11VideoContext *m_vcontext;
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