/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "D3DResource.h"
#include "Texture.h"
#include "utils/Geometry.h"
#include "utils/MemUtils.h"

#include <DirectXMath.h>
#include <wrl/client.h>

struct Vertex {
  Vertex() {}

  Vertex(DirectX::XMFLOAT3 p, DirectX::XMFLOAT4 c)
    : pos(p), color(c) {}

  Vertex(DirectX::XMFLOAT3 p, DirectX::XMFLOAT4 c, DirectX::XMFLOAT2 t1, DirectX::XMFLOAT2 t2)
    : pos(p), color(c), texCoord(t1), texCoord2(t2) {}

  DirectX::XMFLOAT3 pos;
  DirectX::XMFLOAT4 color;
  DirectX::XMFLOAT2 texCoord;
  DirectX::XMFLOAT2 texCoord2;
};

class ID3DResource;

class CGUIShaderDX
{
public:
  CGUIShaderDX();
  ~CGUIShaderDX();

  bool Initialize();
  void Begin(unsigned int flags);
  void End(void);
  void ApplyStateBlock(void);
  void RestoreBuffers(void);

  void SetShaderViews(unsigned int numViews, ID3D11ShaderResourceView** views);
  void SetViewPort(D3D11_VIEWPORT viewPort);

  void XM_CALLCONV GetWVP(DirectX::XMMATRIX &w, DirectX::XMMATRIX &v, DirectX::XMMATRIX &p)
  {
    w = m_cbWorldViewProj.world;
    v = m_cbWorldViewProj.view;
    p = m_cbWorldViewProj.projection;
  }
  DirectX::XMMATRIX XM_CALLCONV GetWorld() const { return m_cbWorldViewProj.world; }
  DirectX::XMMATRIX XM_CALLCONV GetView() const { return m_cbWorldViewProj.view; }
  DirectX::XMMATRIX XM_CALLCONV GetProjection() const { return m_cbWorldViewProj.projection; }
  void XM_CALLCONV SetWVP(const DirectX::XMMATRIX &w, const DirectX::XMMATRIX &v, const DirectX::XMMATRIX &p);
  void XM_CALLCONV SetWorld(const DirectX::XMMATRIX &value);
  void XM_CALLCONV SetView(const DirectX::XMMATRIX &value);
  void XM_CALLCONV SetProjection(const DirectX::XMMATRIX &value);
  void Project(float &x, float &y, float &z);

  void DrawQuad(Vertex& v1, Vertex& v2, Vertex& v3, Vertex& v4);
  void DrawIndexed(unsigned int indexCount, unsigned int startIndex, unsigned int startVertex);
  void Draw(unsigned int vertexCount, unsigned int startVertex);

  bool  HardwareClipIsPossible(void) const { return m_clipPossible; }
  float GetClipXFactor(void) const { return m_clipXFactor;  }
  float GetClipXOffset(void) const { return m_clipXOffset;  }
  float GetClipYFactor(void) const { return m_clipYFactor;  }
  float GetClipYOffset(void) const { return m_clipYOffset;  }

  // need to use aligned allocation because we use XMMATRIX in structures.
  void* operator new (size_t size)
  {
    void* ptr = KODI::MEMORY::AlignedMalloc(size, __alignof(CGUIShaderDX));
    if (!ptr)
      throw std::bad_alloc();
    return ptr;
  }
  // free aligned memory.
  void operator delete (void* ptr)
  {
    KODI::MEMORY::AlignedFree(ptr);
  }

private:
  struct cbWorldViewProj
  {
    DirectX::XMMATRIX world;
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
  };
  struct cbViewPort
  {
    float TopLeftX;
    float TopLeftY;
    float Width;
    float Height;
  };
  struct cbWorld
  {
    DirectX::XMMATRIX wvp;
    float blackLevel;
    float colorRange;
    float sdrPeakLum;
    int PQ;
  };

  void Release(void);
  bool CreateBuffers(void);
  bool CreateSamplers(void);
  void ApplyChanges(void);
  void ClipToScissorParams(void);

  // GUI constants
  cbViewPort m_cbViewPort = {};
  cbWorldViewProj m_cbWorldViewProj = {};

  bool  m_bCreated;
  size_t m_currentShader;
  CD3DVertexShader m_vertexShader;
  CD3DPixelShader m_pixelShader[SHADER_METHOD_RENDER_COUNT];
  Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSampLinear;

  // GUI buffers
  bool m_bIsWVPDirty;
  bool m_bIsVPDirty;
  Microsoft::WRL::ComPtr<ID3D11Buffer> m_pWVPBuffer;
  Microsoft::WRL::ComPtr<ID3D11Buffer> m_pVPBuffer;
  Microsoft::WRL::ComPtr<ID3D11Buffer> m_pVertexBuffer;

  // clip to scissors params
  bool m_clipPossible;
  float m_clipXFactor;
  float m_clipXOffset;
  float m_clipYFactor;
  float m_clipYOffset;
};
