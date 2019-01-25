/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDCodecs/Video/DXVA.h"
#include <wrl/client.h>

class CVideoBuffer;
struct VideoPicture;

enum EBufferFormat
{
  BUFFER_FMT_NONE = 0,
  BUFFER_FMT_YUV420P,
  BUFFER_FMT_YUV420P10,
  BUFFER_FMT_YUV420P16,
  BUFFER_FMT_NV12,
  BUFFER_FMT_UYVY422,
  BUFFER_FMT_YUYV422,
  BUFFER_FMT_D3D11_BYPASS,
  BUFFER_FMT_D3D11_NV12,
  BUFFER_FMT_D3D11_P010,
  BUFFER_FMT_D3D11_P016,
};

class CRenderBuffer
{
public:
  CRenderBuffer();
  ~CRenderBuffer();
  void Release();     // Release any allocated resource
  void Lock();        // Prepare the buffer to receive data from VideoPlayer
  void Unlock();      // VideoPlayer finished filling the buffer with data
  void Clear() const; // clear the buffer with solid black

  bool CreateBuffer(EBufferFormat fmt, unsigned width, unsigned height, bool software);
  bool UploadBuffer();
  void AppendPicture(const VideoPicture &picture);
  void ReleasePicture();

  unsigned int GetActivePlanes() const { return m_activePlanes; }
  HRESULT GetResource(ID3D11Resource** ppResource, unsigned* index) const;
  ID3D11View* GetView(unsigned idx = 0);

  void GetDataPtr(unsigned idx, void **pData, int *pStride) const;
  bool MapPlane(unsigned idx, void **pData, int *pStride) const;
  void UnmapPlane(unsigned idx) const;

  unsigned GetWidth() const { return m_widthTex; }
  unsigned GetHeight() const { return m_heightTex; }
  bool HasPic() const;
  bool IsValid() const { return m_activePlanes > 0; }
  void QueueCopyBuffer();

  bool loaded = false;
  unsigned int frameIdx = 0;
  unsigned int pictureFlags = 0;
  EBufferFormat format = BUFFER_FMT_NONE;
  CVideoBuffer* videoBuffer = nullptr;
  AVColorPrimaries primaries = AVCOL_PRI_UNSPECIFIED;
  AVColorSpace color_space = AVCOL_SPC_BT709;
  AVColorTransferCharacteristic color_transfer = AVCOL_TRC_BT709;
  bool full_range = false;
  int bits = 8;
  uint8_t texBits = 8;

  bool hasDisplayMetadata = false;
  bool hasLightMetadata = false;
  AVMasteringDisplayMetadata displayMetadata = {};
  AVContentLightMetadata lightMetadata = {};

private:
  bool CopyToPlanarTexture();
  bool QueueCopyingFromGpu();
  void UploadFromStaging() const;
  bool UploadFromVideoBuffer();
  HRESULT GetDXVAResource(ID3D11Resource** ppResource, unsigned* arrayIdx) const;

  bool m_locked = false;
  bool m_bPending = false;
  bool m_soft = false;
  // video buffer size
  unsigned int m_width = 0;
  unsigned int m_height = 0;
  // real render buffer size
  unsigned int m_widthTex = 0;
  unsigned int m_heightTex = 0;
  unsigned int m_activePlanes = 0;
  D3D11_MAP m_mapType = D3D11_MAP_WRITE_DISCARD;
  CD3D11_TEXTURE2D_DESC m_sDesc = {};
  Microsoft::WRL::ComPtr<ID3D11Texture2D> m_staging;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_planes[2];
  D3D11_MAPPED_SUBRESOURCE m_rects[YuvImage::MAX_PLANES] = {{}};
  CD3DTexture m_textures[YuvImage::MAX_PLANES];
};
