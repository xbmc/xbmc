/*
 *  Copyright (C) 2005-2026 Team Kodi
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
  CGUIShaderDX() {}
  CGUIShaderDX(const CGUIShaderDX&) = delete;
  CGUIShaderDX& operator=(const CGUIShaderDX&) = delete;
  ~CGUIShaderDX();

  bool Initialize();

  /*!
   * \brief Activate shaders and recalculate clipping information.
   * \param flags Shader method to activate
   */
  void Begin(unsigned int flags);
  void End(void);
  void ApplyStateBlock(void);
  void RestoreBuffers(void);

  void SetShaderViews(unsigned int numViews, ID3D11ShaderResourceView** views);
  void SetViewPort(D3D11_VIEWPORT viewPort);

  void XM_CALLCONV GetWVP(DirectX::XMMATRIX& w, DirectX::XMMATRIX& v, DirectX::XMMATRIX& p)
  {
    w = m_cbWorldViewProj.GetWorld();
    v = m_cbWorldViewProj.GetView();
    p = m_cbWorldViewProj.GetProjection();
  }
  DirectX::XMMATRIX XM_CALLCONV GetWorld() const { return m_cbWorldViewProj.GetWorld(); }
  DirectX::XMMATRIX XM_CALLCONV GetView() const { return m_cbWorldViewProj.GetView(); }
  DirectX::XMMATRIX XM_CALLCONV GetProjection() const { return m_cbWorldViewProj.GetProjection(); }
  void XM_CALLCONV SetWVP(const DirectX::XMMATRIX& w,
                          const DirectX::XMMATRIX& v,
                          const DirectX::XMMATRIX& p);
  void XM_CALLCONV SetWorld(const DirectX::XMMATRIX& value);
  void XM_CALLCONV SetView(const DirectX::XMMATRIX& value);
  void XM_CALLCONV SetProjection(const DirectX::XMMATRIX& value);
  void Project(float& x, float& y, float& z) const;

  /*!
   * \brief Sets the depth value of the primitives to be drawn (overrides z of the vertices)
   * \param[in] depth value -1=far to 1=near (GL convention).
   */
  void SetDepth(float depth);

  void SetShaderClip(float x1, float y1, float x2, float y2);
  void SetTexStep(float stepX, float stepY, float stepX2, float stepY2);

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
  class CWorldViewProj
  {
  public:
    void XM_CALLCONV GetWVP(DirectX::XMMATRIX& w, DirectX::XMMATRIX& v, DirectX::XMMATRIX& p) const
    {
      w = m_world;
      v = m_view;
      p = m_projection;
    }
    DirectX::XMMATRIX XM_CALLCONV GetWVP() const;
    DirectX::XMMATRIX XM_CALLCONV GetWorld() const { return m_world; }
    DirectX::XMMATRIX XM_CALLCONV GetView() const { return m_view; }
    DirectX::XMMATRIX XM_CALLCONV GetProjection() const { return m_projection; }

    void XM_CALLCONV SetWVP(const DirectX::XMMATRIX& w,
                            const DirectX::XMMATRIX& v,
                            const DirectX::XMMATRIX& p);
    void XM_CALLCONV SetWorld(const DirectX::XMMATRIX& value);
    void XM_CALLCONV SetView(const DirectX::XMMATRIX& value);
    void XM_CALLCONV SetProjection(const DirectX::XMMATRIX& value);

  private:
    DirectX::XMMATRIX m_world = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX m_view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX m_projection = DirectX::XMMatrixIdentity();
    mutable DirectX::XMMATRIX m_cachedVP = DirectX::XMMatrixIdentity();
    mutable DirectX::XMMATRIX m_cachedWVP = DirectX::XMMatrixIdentity();
    mutable bool m_isVPDirty = false;
    mutable bool m_isWorldDirty = false;
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
    DirectX::XMFLOAT4 m_shaderClip;
    DirectX::XMFLOAT2 m_texStep;
    DirectX::XMFLOAT2 m_texStep2;
    float blackLevel;
    float colorRange;
    float sdrPeakLum;
    int PQ;
    float depth;
  };

  void Release(void);
  bool CreateBuffers(void);
  bool CreateSamplers(void);
  void SetSamplers(void);
  void ApplyChanges(void);
  void ClipToScissorParams(void);

  // GUI constants
  cbViewPort m_cbViewPort = {};
  CWorldViewProj m_cbWorldViewProj;
  float m_depth = 1.f;
  // Font vertex shader constants
  DirectX::XMFLOAT4 m_shaderClip; // clip rect (x1,y1,x2,y2) in font-local coords
  DirectX::XMFLOAT2 m_texStep; // step (1/resolution) for the input texture 1
  DirectX::XMFLOAT2 m_texStep2; // step (1/resolution) for the input texture 2

  bool m_bCreated = false;
  size_t m_currentShader = 0;

  CD3DVertexShader m_vertexShader;
  CD3DVertexShader m_vertexShaderClip;

  struct ShaderPair
  {
    CD3DVertexShader* m_vs = nullptr;
    CD3DPixelShader m_ps;
  };
  ShaderPair m_shaders[SHADER_METHOD_RENDER_COUNT];

  Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSampLinear;
  Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSampNearestNeighbor;

  // GUI buffers
  bool m_bIsWVPDirty = true;
  bool m_bIsVPDirty = true;
  Microsoft::WRL::ComPtr<ID3D11Buffer> m_pWVPBuffer;
  Microsoft::WRL::ComPtr<ID3D11Buffer> m_pVPBuffer;
  Microsoft::WRL::ComPtr<ID3D11Buffer> m_pVertexBuffer;

  // clip to scissors params
  bool m_clipPossible = false;
  float m_clipXFactor = 0.0f;
  float m_clipXOffset = 0.0f;
  float m_clipYFactor = 0.0f;
  float m_clipYOffset = 0.0f;
};
