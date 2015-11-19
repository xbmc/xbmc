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

#ifdef HAS_DX

#include <map>
#include <d3dx11effect.h>
#include <DirectXMath.h>
#include "Geometry.h"
#include "GUIColorManager.h"

using namespace DirectX;

#define KODI_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT 4

typedef enum SHADER_METHOD {
  SHADER_METHOD_RENDER_DEFAULT,
  SHADER_METHOD_RENDER_TEXTURE_NOBLEND,
  SHADER_METHOD_RENDER_FONT,
  SHADER_METHOD_RENDER_TEXTURE_BLEND,
  SHADER_METHOD_RENDER_MULTI_TEXTURE_BLEND,
  SHADER_METHOD_RENDER_VIDEO,
  SHADER_METHOD_RENDER_VIDEO_CONTROL,
  SHADER_METHOD_RENDER_STEREO_INTERLACED_LEFT,
  SHADER_METHOD_RENDER_STEREO_INTERLACED_RIGHT,
  SHADER_METHOD_RENDER_STEREO_CHECKERBOARD_LEFT,
  SHADER_METHOD_RENDER_STEREO_CHECKERBOARD_RIGHT,
  SHADER_METHOD_RENDER_COUNT
} _SHADER_METHOD;

typedef enum SHADER_SAMPLER {
  SHADER_SAMPLER_LINEAR = 1,
  SHADER_SAMPLER_POINT = 2
} _SHADER_SAMPLER;


class ID3DResource
{
public:
  virtual ~ID3DResource() {};

  virtual void OnDestroyDevice()=0;
  virtual void OnCreateDevice()=0;
  virtual void OnLostDevice() {};
  virtual void OnResetDevice() {};
};

class CD3DHelper
{
public:
  static inline void XMStoreColor(float* floats, DWORD dword)
  {
    floats[0] = float((dword >> 16) & 0xFF) * (1.0f / 255.0f); // r
    floats[1] = float((dword >>  8) & 0xFF) * (1.0f / 255.0f); // g
    floats[2] = float((dword >>  0) & 0xFF) * (1.0f / 255.0f); // b
    floats[3] = float((dword >> 24) & 0xFF) * (1.0f / 255.0f); // a
  }

  static inline void XMStoreColor(XMFLOAT4* floats, DWORD dword)
  {
    XMStoreColor(reinterpret_cast<float*>(floats), dword);
  }

  static inline void XMStoreColor(float* floats, unsigned char a, unsigned char r, unsigned char g, unsigned char b)
  {
    floats[0] = r * (1.0f / 255.0f);
    floats[1] = g * (1.0f / 255.0f);
    floats[2] = b * (1.0f / 255.0f);
    floats[3] = a * (1.0f / 255.0f);
  }

  static inline void XMStoreColor(XMFLOAT4* floats, unsigned char a, unsigned char r, unsigned char g, unsigned char b)
  {
    XMStoreColor(reinterpret_cast<float*>(floats), a, r, g, b);
  }

  // helper function to properly "clear" shader resources
  static inline void PSClearShaderResources(ID3D11DeviceContext* pContext)
  {
    ID3D11ShaderResourceView* shader_resource_views[KODI_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
    pContext->PSSetShaderResources(0, ARRAYSIZE(shader_resource_views), shader_resource_views);
  }

  static size_t BitsPerPixel(DXGI_FORMAT fmt);
};

class CD3DTexture : public ID3DResource
{
public:
  CD3DTexture();
  ~CD3DTexture();

  bool Create(UINT width, UINT height, UINT mipLevels, D3D11_USAGE usage, DXGI_FORMAT format, const void* pInitData = nullptr, unsigned int srcPitch = 0);
  void Release();
  bool GetDesc(D3D11_TEXTURE2D_DESC *desc);
  bool LockRect(UINT subresource, D3D11_MAPPED_SUBRESOURCE *res, D3D11_MAP mapType);
  bool UnlockRect(UINT subresource);

  // Accessors
  ID3D11Texture2D* Get() const { return m_texture; };
  ID3D11ShaderResourceView* GetShaderResource();
  ID3D11RenderTargetView* GetRenderTarget();
  UINT GetWidth()  const { return m_width; }
  UINT GetHeight() const { return m_height; }
  DXGI_FORMAT GetFormat() const { return m_format; }
  void GenerateMipmaps();

  // static methods
  static void DrawQuad(const CPoint points[4], color_t color, CD3DTexture *texture, const CRect *texCoords,
    SHADER_METHOD options = SHADER_METHOD_RENDER_TEXTURE_BLEND);

  static void DrawQuad(const CPoint points[4], color_t color, unsigned numViews, ID3D11ShaderResourceView **view, const CRect *texCoords,
    SHADER_METHOD options = SHADER_METHOD_RENDER_TEXTURE_BLEND);

  static void DrawQuad(const CRect &coords, color_t color, CD3DTexture *texture, const CRect *texCoords,
    SHADER_METHOD options = SHADER_METHOD_RENDER_TEXTURE_BLEND);

  static void DrawQuad(const CRect &coords, color_t color, unsigned numViews, ID3D11ShaderResourceView **view, const CRect *texCoords,
    SHADER_METHOD options = SHADER_METHOD_RENDER_TEXTURE_BLEND);

  virtual void OnDestroyDevice();
  virtual void OnCreateDevice();
  virtual void OnLostDevice();
  virtual void OnResetDevice();

private:
  unsigned int GetMemoryUsage(unsigned int pitch) const;
  bool CreateInternal(const void* pInitData = nullptr, unsigned int srcPitch = 0);

  void SaveTexture();
  void RestoreTexture();

  // creation parameters
  UINT        m_width;
  UINT        m_height;
  UINT        m_mipLevels;
  D3D11_USAGE m_usage;
  DXGI_FORMAT m_format;
  UINT        m_pitch;
  UINT        m_bindFlags;
  UINT        m_cpuFlags;

  // created texture
  ID3D11Texture2D* m_texture;
  ID3D11ShaderResourceView* m_textureView;
  ID3D11RenderTargetView* m_renderTarget;

  // saved data
  BYTE*     m_data;
};

typedef std::map<std::string, std::string> DefinesMap;

class CD3DEffect : public ID3DResource
{
public:
  CD3DEffect();
  virtual ~CD3DEffect();
  bool Create(const std::string &effectString, DefinesMap* defines);
  void Release();
  bool SetFloatArray(LPCSTR handle, const float* val, unsigned int count);
  bool SetMatrix(LPCSTR handle, const XMFLOAT4X4* mat);
  bool SetTechnique(LPCSTR handle);
  bool SetTexture(LPCSTR handle, CD3DTexture &texture);
  bool SetResources(LPCSTR handle, ID3D11ShaderResourceView** ppSRViews, size_t count);
  bool SetConstantBuffer(LPCSTR handle, ID3D11Buffer *buffer);
  bool SetScalar(LPCSTR handle, float value);
  bool Begin(UINT *passes, DWORD flags);
  bool BeginPass(UINT pass);
  bool EndPass();
  bool End();

  ID3DX11Effect *Get() const { return m_effect; };

  virtual void OnDestroyDevice();
  virtual void OnCreateDevice();

private:
  bool         CreateEffect();

  std::string             m_effectString;
  ID3DX11Effect*          m_effect;
  ID3DX11EffectTechnique* m_techniquie;
  ID3DX11EffectPass*      m_currentPass;
  DefinesMap              m_defines;
};

class CD3DBuffer : public ID3DResource
{
public:
  CD3DBuffer();
  virtual ~CD3DBuffer();
  bool Create(D3D11_BIND_FLAG type, UINT count, UINT stride, DXGI_FORMAT format, D3D11_USAGE usage, const void* initData = nullptr);
  bool Map(void** resource);
  bool Unmap();
  void Release();
  bool Update(UINT offset, UINT size, void *data);
  unsigned int GetStride() { return m_stride; }
  DXGI_FORMAT GetFormat() { return m_format; }
  ID3D11Buffer* Get() const { return m_buffer; };

  virtual void OnDestroyDevice();
  virtual void OnCreateDevice();
private:
  bool CreateBuffer(const void *pData);
  D3D11_BIND_FLAG  m_type;
  UINT             m_length;
  UINT             m_stride;
  DXGI_FORMAT      m_format;
  D3D11_USAGE      m_usage;
  ID3D11Buffer*    m_buffer;
  UINT             m_cpuFlags;
  // saved data
  BYTE*            m_data;
};

class CD3DVertexShader : public ID3DResource
{
public:
  CD3DVertexShader();
  ~CD3DVertexShader();

  bool Create(const std::wstring& vertexFile, D3D11_INPUT_ELEMENT_DESC* vertexLayout, unsigned int vertexLayoutSize);
  bool Create(const void* code, size_t codeLength, D3D11_INPUT_ELEMENT_DESC* vertexLayout, unsigned int vertexLayoutSize);
  void ReleaseShader();
  void BindShader();
  void UnbindShader();
  void Release();
  bool IsInited() { return m_inited; }

  virtual void OnDestroyDevice();
  virtual void OnCreateDevice();

private:
  bool CreateInternal();

  ID3DBlob*                 m_VSBuffer;
  D3D11_INPUT_ELEMENT_DESC* m_vertexLayout;
  ID3D11VertexShader*       m_VS;
  ID3D11InputLayout*        m_inputLayout;
  unsigned int              m_vertexLayoutSize;
  bool                      m_inited;
};

class CD3DPixelShader : public ID3DResource
{
public:
  CD3DPixelShader();
  ~CD3DPixelShader();

  bool Create(const std::wstring& wstrFile);
  bool Create(const void* code, size_t codeLength);
  void ReleaseShader();
  void BindShader();
  void UnbindShader();
  void Release();
  bool IsInited() { return m_inited; }

  virtual void OnDestroyDevice();
  virtual void OnCreateDevice();

private:
  bool CreateInternal();

  ID3DBlob*                 m_PSBuffer;
  ID3D11PixelShader*        m_PS;
  bool                      m_inited;
};

#endif
