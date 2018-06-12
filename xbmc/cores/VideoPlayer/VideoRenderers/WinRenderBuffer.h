/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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
  void Release();            // Release any allocated resource
  void Lock();        // Prepare the buffer to receive data from VideoPlayer
  void Unlock();        // VideoPlayer finished filling the buffer with data
  void Clear() const;        // clear the buffer with solid black

  bool CreateBuffer(EBufferFormat format, unsigned width, unsigned height, bool software);
  bool UploadBuffer();
  void AppendPicture(const VideoPicture &picture);
  void ReleasePicture();

  unsigned int GetActivePlanes() const { return m_activePlanes; }
  HRESULT GetResource(ID3D11Resource** ppResource, unsigned* arrayIdx);
  ID3D11View* GetView(unsigned idx = 0);

  void GetDataPtr(unsigned idx, void **pData, int *pStride) const;
  bool MapPlane(unsigned idx, void **pData, int *pStride) const;
  bool UnmapPlane(unsigned idx) const;

  unsigned GetWidth() const { return m_widthTex; }
  unsigned GetHeight() const { return m_heightTex; }
  bool HasPic() const;
  bool IsValid() const { return m_activePlanes > 0; }
  void QueueCopyBuffer();

  bool loaded;
  unsigned int frameIdx;
  EBufferFormat format;
  CVideoBuffer* videoBuffer;
  AVColorPrimaries primaries;
  AVColorSpace color_space;
  AVColorTransferCharacteristic color_transfer;
  bool full_range;
  int bits;
  uint8_t texBits;

  bool hasDisplayMetadata = false;
  bool hasLightMetadata = false;
  AVMasteringDisplayMetadata displayMetadata;
  AVContentLightMetadata lightMetadata;

private:
  bool CopyToD3D11();
  bool CopyToStaging();
  void CopyFromStaging() const;
  bool CopyBuffer();
  HRESULT GetDXVAResource(ID3D11Resource** ppResource, unsigned* arrayIdx);

  bool m_locked;
  bool m_bPending;
  bool m_soft;
  // video buffer size
  unsigned int m_width;
  unsigned int m_height;
  // real render bufer size
  unsigned int m_widthTex;
  unsigned int m_heightTex;
  unsigned int m_activePlanes;
  D3D11_MAP m_mapType;
  CD3D11_TEXTURE2D_DESC m_sDesc;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> m_staging;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_planes[2];

  D3D11_MAPPED_SUBRESOURCE m_rects[YuvImage::MAX_PLANES];
  CD3DTexture m_textures[YuvImage::MAX_PLANES];
};
