/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIColorManager.h"
#include "utils/Color.h"
#include "utils/Geometry.h"

#include <map>

#include <DirectXMath.h>
#include <d3dx11effect.h>
#include <wrl/client.h>

#define KODI_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT 4

typedef enum SHADER_METHOD {
  SHADER_METHOD_RENDER_DEFAULT,
  SHADER_METHOD_RENDER_TEXTURE_NOBLEND,
  SHADER_METHOD_RENDER_FONT,
  SHADER_METHOD_RENDER_TEXTURE_BLEND,
  SHADER_METHOD_RENDER_MULTI_TEXTURE_BLEND,
  SHADER_METHOD_RENDER_STEREO_INTERLACED_LEFT,
  SHADER_METHOD_RENDER_STEREO_INTERLACED_RIGHT,
  SHADER_METHOD_RENDER_STEREO_CHECKERBOARD_LEFT,
  SHADER_METHOD_RENDER_STEREO_CHECKERBOARD_RIGHT,
  SHADER_METHOD_RENDER_COUNT
} _SHADER_METHOD;

class ID3DResource
{
public:
  virtual ~ID3DResource() {}

  virtual void OnDestroyDevice(bool fatal)=0;
  virtual void OnCreateDevice()=0;

protected:
  void Register();
  void Unregister();

  bool m_bRegistered = false;
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

  static inline void XMStoreColor(DirectX::XMFLOAT4* floats, DWORD dword)
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

  static inline void XMStoreColor(DirectX::XMFLOAT4* floats, unsigned char a, unsigned char r, unsigned char g, unsigned char b)
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
  virtual ~CD3DTexture();

  bool Create(UINT width, UINT height, UINT mipLevels, D3D11_USAGE usage, DXGI_FORMAT format, const void* pInitData = nullptr, unsigned int srcPitch = 0);

  void Release();
  bool GetDesc(D3D11_TEXTURE2D_DESC *desc) const;
  bool LockRect(UINT subresource, D3D11_MAPPED_SUBRESOURCE *res, D3D11_MAP mapType) const;
  bool UnlockRect(UINT subresource) const;

  // Accessors
  ID3D11Texture2D* Get() const { return m_texture.Get(); }
  ID3D11ShaderResourceView* GetShaderResource(DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
  ID3D11ShaderResourceView** GetAddressOfSRV(DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
  ID3D11RenderTargetView* GetRenderTarget();
  ID3D11RenderTargetView** GetAddressOfRTV();
  UINT GetWidth()  const { return m_width; }
  UINT GetHeight() const { return m_height; }
  DXGI_FORMAT GetFormat() const { return m_format; }
  void GenerateMipmaps();

  // static methods
  static void DrawQuad(const CPoint points[4], UTILS::Color color, CD3DTexture *texture, const CRect *texCoords,
    SHADER_METHOD options = SHADER_METHOD_RENDER_TEXTURE_BLEND);

  static void DrawQuad(const CPoint points[4], UTILS::Color color, unsigned numViews, ID3D11ShaderResourceView **view, const CRect *texCoords,
    SHADER_METHOD options = SHADER_METHOD_RENDER_TEXTURE_BLEND);

  static void DrawQuad(const CRect &coords, UTILS::Color color, CD3DTexture *texture, const CRect *texCoords,
    SHADER_METHOD options = SHADER_METHOD_RENDER_TEXTURE_BLEND);

  static void DrawQuad(const CRect &coords, UTILS::Color color, unsigned numViews, ID3D11ShaderResourceView **view, const CRect *texCoords,
    SHADER_METHOD options = SHADER_METHOD_RENDER_TEXTURE_BLEND);

  void OnDestroyDevice(bool fatal) override;
  void OnCreateDevice() override;

protected:
  ID3D11RenderTargetView* GetRenderTargetInternal(unsigned idx = 0);
  unsigned int GetMemoryUsage(unsigned int pitch) const;
  bool CreateInternal(const void* pInitData = nullptr, unsigned int srcPitch = 0);

  void SaveTexture();
  void RestoreTexture();

  // saved data
  BYTE* m_data;
  // creation parameters
  UINT m_width;
  UINT m_height;
  UINT m_mipLevels;
  UINT m_pitch;
  UINT m_bindFlags;
  UINT m_cpuFlags;
  UINT m_viewIdx;
  D3D11_USAGE m_usage;
  DXGI_FORMAT m_format;

  // created texture
  Microsoft::WRL::ComPtr<ID3D11Texture2D> m_texture;
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargets[2];
  // store views in different formats
  std::map<DXGI_FORMAT, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_views;
};

typedef std::map<std::string, std::string> DefinesMap;

class CD3DEffect : public ID3DResource, public ID3DInclude
{
public:
  CD3DEffect();
  virtual ~CD3DEffect();
  bool Create(const std::string &effectString, DefinesMap* defines);
  void Release();
  bool SetFloatArray(LPCSTR handle, const float* val, unsigned int count);
  bool SetMatrix(LPCSTR handle, const float* mat);
  bool SetTechnique(LPCSTR handle);
  bool SetTexture(LPCSTR handle, CD3DTexture &texture);
  bool SetResources(LPCSTR handle, ID3D11ShaderResourceView** ppSRViews, size_t count);
  bool SetConstantBuffer(LPCSTR handle, ID3D11Buffer *buffer);
  bool SetScalar(LPCSTR handle, float value);
  bool Begin(UINT *passes, DWORD flags);
  bool BeginPass(UINT pass);
  bool EndPass();
  bool End();

  ID3DX11Effect* Get() const { return m_effect.Get(); }

  void OnDestroyDevice(bool fatal) override;
  void OnCreateDevice() override;

  // ID3DInclude interface
  __declspec(nothrow) HRESULT __stdcall Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes) override;
  __declspec(nothrow) HRESULT __stdcall Close(LPCVOID pData) override;

private:
  bool CreateEffect();

  std::string m_effectString;
  DefinesMap m_defines;
  Microsoft::WRL::ComPtr<ID3DX11Effect> m_effect;
  Microsoft::WRL::ComPtr<ID3DX11EffectTechnique> m_techniquie;
  Microsoft::WRL::ComPtr<ID3DX11EffectPass> m_currentPass;
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
  unsigned int GetStride() { return m_stride; }
  DXGI_FORMAT GetFormat() { return m_format; }
  ID3D11Buffer* Get() const { return m_buffer.Get(); }

  void OnDestroyDevice(bool fatal) override;
  void OnCreateDevice() override;

private:
  bool CreateBuffer(const void *pData);

  // saved data
  BYTE* m_data;
  UINT m_length;
  UINT m_stride;
  UINT m_cpuFlags;
  DXGI_FORMAT m_format;
  D3D11_USAGE m_usage;
  D3D11_BIND_FLAG m_type;
  Microsoft::WRL::ComPtr<ID3D11Buffer> m_buffer;
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

  void OnDestroyDevice(bool fatal) override;
  void OnCreateDevice() override;

private:
  bool CreateInternal();

  bool m_inited;
  unsigned int m_vertexLayoutSize;
  D3D11_INPUT_ELEMENT_DESC* m_vertexLayout;
  Microsoft::WRL::ComPtr<ID3DBlob> m_VSBuffer;
  Microsoft::WRL::ComPtr<ID3D11VertexShader> m_VS;
  Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
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

  void OnDestroyDevice(bool fatal) override;
  void OnCreateDevice() override;

private:
  bool CreateInternal();

  bool m_inited;
  Microsoft::WRL::ComPtr<ID3DBlob> m_PSBuffer;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> m_PS;
};
