/*
*      Copyright (C) 2005-2015 Team Kodi
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
#ifdef HAS_DX

#pragma once

#include "Geometry.h"
#include "Texture.h"
#include "D3DResource.h"
#include <DirectXMath.h>

using namespace DirectX;

struct Vertex {
  Vertex() {}

  Vertex(XMFLOAT3 p, XMFLOAT4 c)
    : pos(p), color(c) {}

  Vertex(XMFLOAT3 p, XMFLOAT4 c, XMFLOAT2 t1, XMFLOAT2 t2)
    : pos(p), color(c), texCoord(t1), texCoord2(t2) {}

  XMFLOAT3 pos;
  XMFLOAT4 color;
  XMFLOAT2 texCoord;
  XMFLOAT2 texCoord2;
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
  void SetSampler(SHADER_SAMPLER sampler);

  XMMATRIX XM_CALLCONV GetWorld()      const { return m_cbWorldViewProj.world; }
  XMMATRIX XM_CALLCONV GetView()       const { return m_cbWorldViewProj.view; }
  XMMATRIX XM_CALLCONV GetProjection() const { return m_cbWorldViewProj.projection; }
  void     XM_CALLCONV SetWorld(const XMMATRIX &value);
  void     XM_CALLCONV SetView(const XMMATRIX &value);
  void     XM_CALLCONV SetProjection(const XMMATRIX &value);
  void                 Project(float &x, float &y, float &z);

  void DrawQuad(Vertex& v1, Vertex& v2, Vertex& v3, Vertex& v4);
  void DrawIndexed(unsigned int indexCount, unsigned int startIndex, unsigned int startVertex);
  void Draw(unsigned int vertexCount, unsigned int startVertex);
  
  bool  HardwareClipIsPossible(void) { return m_clipPossible; }
  float GetClipXFactor(void)         { return m_clipXFactor;  }
  float GetClipXOffset(void)         { return m_clipXOffset;  }
  float GetClipYFactor(void)         { return m_clipYFactor;  }
  float GetClipYOffset(void)         { return m_clipYOffset;  }

  // need to use aligned allocation bacause we use XMMATRIX in structures.
  static void* operator new (size_t size)
  {
    void* ptr = _aligned_malloc(size, __alignof(CGUIShaderDX));
    if (!ptr)
      throw std::bad_alloc();
    return ptr;
  }
  // free aligned memory.
  static void operator delete (void* ptr)
  {
    _aligned_free(ptr);
  }

private:
  struct cbWorldViewProj
  {
    XMMATRIX world;
    XMMATRIX view;
    XMMATRIX projection;
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
    XMMATRIX wvp;
    float blackLevel;
    float colorRange;
  };

  void Release(void);
  bool CreateBuffers(void);
  bool CreateSamplers(void);
  void ApplyChanges(void);
  void ClipToScissorParams(void);

  // GUI constants
  cbViewPort          m_cbViewPort;
  cbWorldViewProj     m_cbWorldViewProj;

  bool                m_bCreated;
  ID3D11SamplerState* m_pSampLinear;
  ID3D11SamplerState* m_pSampPoint;
  CD3DVertexShader    m_vertexShader;
  CD3DPixelShader     m_pixelShader[SHADER_METHOD_RENDER_COUNT];
  size_t              m_currentShader;

  // GUI buffers
  ID3D11Buffer*       m_pWVPBuffer;
  ID3D11Buffer*       m_pVPBuffer;
  ID3D11Buffer*       m_pVertexBuffer;
  bool                m_bIsWVPDirty;
  bool                m_bIsVPDirty;

  // clip to scissors params
  bool                m_clipPossible;
  float               m_clipXFactor;
  float               m_clipXOffset;
  float               m_clipYFactor;
  float               m_clipYOffset;
};

#endif