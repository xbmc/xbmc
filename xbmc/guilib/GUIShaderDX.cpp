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

#include <d3dcompiler.h>
#include "GUIShaderDX.h"
#include "guilib/GraphicContext.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "windowing/WindowingFactory.h"

// shaders bytecode includes
#include "guishader_vert.h"
#include "guishader_checkerboard_right.h"
#include "guishader_checkerboard_left.h"
#include "guishader_default.h"
#include "guishader_fonts.h"
#include "guishader_interlaced_right.h"
#include "guishader_interlaced_left.h"
#include "guishader_multi_texture_blend.h"
#include "guishader_texture.h"
#include "guishader_texture_noblend.h"
#include "guishader_video.h"
#include "guishader_video_control.h"

// shaders bytecode holder
static const D3D_SHADER_DATA cbPSShaderCode[SHADER_METHOD_RENDER_COUNT] =
{
  { guishader_default,             sizeof(guishader_default)              }, // SHADER_METHOD_RENDER_DEFAULT
  { guishader_texture_noblend,     sizeof(guishader_texture_noblend)      }, // SHADER_METHOD_RENDER_TEXTURE_NOBLEND
  { guishader_fonts,               sizeof(guishader_fonts)                }, // SHADER_METHOD_RENDER_FONT
  { guishader_texture,             sizeof(guishader_texture)              }, // SHADER_METHOD_RENDER_TEXTURE_BLEND
  { guishader_multi_texture_blend, sizeof(guishader_multi_texture_blend)  }, // SHADER_METHOD_RENDER_MULTI_TEXTURE_BLEND
  { guishader_video,               sizeof(guishader_video)                }, // SHADER_METHOD_RENDER_VIDEO
  { guishader_video_control,       sizeof(guishader_video_control)        }, // SHADER_METHOD_RENDER_VIDEO_CONTROL
  { guishader_interlaced_left,     sizeof(guishader_interlaced_left)      }, // SHADER_METHOD_RENDER_STEREO_INTERLACED_LEFT
  { guishader_interlaced_right,    sizeof(guishader_interlaced_right)     }, // SHADER_METHOD_RENDER_STEREO_INTERLACED_RIGHT
  { guishader_checkerboard_left,   sizeof(guishader_checkerboard_left)    }, // SHADER_METHOD_RENDER_STEREO_CHECKERBOARD_LEFT
  { guishader_checkerboard_right,  sizeof(guishader_checkerboard_right)   }, // SHADER_METHOD_RENDER_STEREO_CHECKERBOARD_RIGHT
};

CGUIShaderDX::CGUIShaderDX() :
    m_pSampLinear(NULL),
    m_pSampPoint(NULL),
    m_pVPBuffer(NULL),
    m_pWVPBuffer(NULL),
    m_pVertexBuffer(NULL),
    m_clipXFactor(0.0f),
    m_clipXOffset(0.0f),
    m_clipYFactor(0.0f),
    m_clipYOffset(0.0f),
    m_bIsWVPDirty(false),
    m_bIsVPDirty(false),
    m_bCreated(false),
    m_currentShader(0),
    m_clipPossible(false)
{
  ZeroMemory(&m_cbViewPort, sizeof(m_cbViewPort));
  ZeroMemory(&m_cbWorldViewProj, sizeof(m_cbWorldViewProj));
}

CGUIShaderDX::~CGUIShaderDX()
{
  Release();
}

bool CGUIShaderDX::Initialize()
{
  // Create input layout
  D3D11_INPUT_ELEMENT_DESC layout[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,       0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };

  if (!m_vertexShader.Create(guishader_vert, sizeof(guishader_vert), layout, ARRAYSIZE(layout)))
    return false;

  size_t i;
  bool bSuccess = true;
  for (i = 0; i < SHADER_METHOD_RENDER_COUNT; i++)
  {
    if (!m_pixelShader[i].Create(cbPSShaderCode[i].pBytecode, cbPSShaderCode[i].BytecodeLength))
    {
      bSuccess = false;
      break;
    }
  }

  if (!bSuccess)
  {
    m_vertexShader.Release();
    for (size_t j = 0; j < i; j++)
      m_pixelShader[j].Release();
  }

  if (!bSuccess || !CreateBuffers() || !CreateSamplers())
    return false;

  m_bCreated = true;
  return true;
}

bool CGUIShaderDX::CreateBuffers()
{
  ID3D11Device* pDevice = g_Windowing.Get3D11Device();

  // create vertex buffer
  CD3D11_BUFFER_DESC bufferDesc(sizeof(Vertex) * 4, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
  if (FAILED(pDevice->CreateBuffer(&bufferDesc, NULL, &m_pVertexBuffer)))
  {
    CLog::Log(LOGERROR, __FUNCTION__ " - Failed to create GUI vertex buffer.");
    return false;
  }

  // Create the constant buffer for WVP
  size_t buffSize = (sizeof(cbWorld) + 15) & ~15;
  CD3D11_BUFFER_DESC cbbd(buffSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE); // it can change very frequently
  if (FAILED(pDevice->CreateBuffer(&cbbd, NULL, &m_pWVPBuffer)))
  {
    CLog::Log(LOGERROR, __FUNCTION__ " - Failed to create the constant buffer.");
    return false;
  }
  m_bIsWVPDirty = true;

  CRect viewPort;
  g_Windowing.GetViewPort(viewPort);

  // initial data for viewport buffer
  m_cbViewPort.TopLeftX = viewPort.x1;
  m_cbViewPort.TopLeftY = viewPort.y1;
  m_cbViewPort.Width = viewPort.Width();
  m_cbViewPort.Height = viewPort.Height();

  cbbd.ByteWidth = sizeof(cbViewPort);
  D3D11_SUBRESOURCE_DATA initData = { &m_cbViewPort, 0, 0 };
  // create viewport buffer
  if (FAILED(pDevice->CreateBuffer(&cbbd, &initData, &m_pVPBuffer)))
    return false;

  return true;
}

bool CGUIShaderDX::CreateSamplers()
{
  // Describe the Sampler State
  D3D11_SAMPLER_DESC sampDesc;
  ZeroMemory(&sampDesc, sizeof(D3D11_SAMPLER_DESC));
  sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  sampDesc.MinLOD = 0;
  sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

  if (FAILED(g_Windowing.Get3D11Device()->CreateSamplerState(&sampDesc, &m_pSampLinear)))
    return false;

  sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  if (FAILED(g_Windowing.Get3D11Device()->CreateSamplerState(&sampDesc, &m_pSampPoint)))
    return false;

  ID3D11SamplerState* samplers[] = { m_pSampLinear, m_pSampPoint };
  g_Windowing.Get3D11Context()->PSSetSamplers(0, ARRAYSIZE(samplers), samplers);

  return true;
}

void CGUIShaderDX::ApplyStateBlock(void)
{
  if (!m_bCreated)
    return;

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();

  m_vertexShader.BindShader();
  pContext->VSSetConstantBuffers(0, 1, &m_pWVPBuffer);

  m_pixelShader[m_currentShader].BindShader();
  pContext->PSSetConstantBuffers(0, 1, &m_pWVPBuffer);
  pContext->PSSetConstantBuffers(1, 1, &m_pVPBuffer);

  ID3D11SamplerState* samplers[] = { m_pSampLinear, m_pSampPoint };
  pContext->PSSetSamplers(0, ARRAYSIZE(samplers), samplers);

  RestoreBuffers();
}

void CGUIShaderDX::Begin(unsigned int flags)
{
  if (!m_bCreated)
    return;

  if (m_currentShader != flags)
  {
    m_currentShader = flags;
    m_pixelShader[m_currentShader].BindShader();
  }
  ClipToScissorParams();
}

void CGUIShaderDX::End()
{
  if (!m_bCreated)
    return;
}

void CGUIShaderDX::DrawQuad(Vertex& v1, Vertex& v2, Vertex& v3, Vertex& v4)
{
  if (!m_bCreated)
    return;

  ApplyChanges();

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();

  // update vertex buffer
  D3D11_MAPPED_SUBRESOURCE resource;
  if (SUCCEEDED(pContext->Map(m_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource)))
  {
    // we are using strip topology
    Vertex verticies[4] = { v2, v3, v1, v4 };
    memcpy(resource.pData, &verticies, sizeof(Vertex) * 4);
    pContext->Unmap(m_pVertexBuffer, 0);
    // Draw primitives
    pContext->Draw(4, 0);
  }
}

void CGUIShaderDX::DrawIndexed(unsigned int indexCount, unsigned int startIndex, unsigned int startVertex)
{
  if (!m_bCreated)
    return;

  ApplyChanges();
  g_Windowing.Get3D11Context()->DrawIndexed(indexCount, startIndex, startVertex);
}

void CGUIShaderDX::Draw(unsigned int vertexCount, unsigned int startVertex)
{
  if (!m_bCreated)
    return;

  ApplyChanges();
  g_Windowing.Get3D11Context()->Draw(vertexCount, startVertex);
}

void CGUIShaderDX::SetShaderViews(unsigned int numViews, ID3D11ShaderResourceView** views)
{
  if (!m_bCreated)
    return;

  g_Windowing.Get3D11Context()->PSSetShaderResources(0, numViews, views);
}

void CGUIShaderDX::SetSampler(SHADER_SAMPLER sampler)
{
  if (!m_bCreated)
    return;

  g_Windowing.Get3D11Context()->PSSetSamplers(1, 1, sampler == SHADER_SAMPLER_POINT ? &m_pSampPoint : &m_pSampLinear);
}

void CGUIShaderDX::Release()
{
  SAFE_RELEASE(m_pVertexBuffer);
  SAFE_RELEASE(m_pWVPBuffer);
  SAFE_RELEASE(m_pVPBuffer);
  SAFE_RELEASE(m_pSampLinear);
  SAFE_RELEASE(m_pSampPoint);
  m_bCreated = false;
}

void CGUIShaderDX::SetViewPort(D3D11_VIEWPORT viewPort)
{
  if (!m_pVPBuffer)
    return;

  if ( viewPort.TopLeftX != m_cbViewPort.TopLeftX
    || viewPort.TopLeftY != m_cbViewPort.TopLeftY
    || viewPort.Width    != m_cbViewPort.Width
    || viewPort.Height   != m_cbViewPort.Height)
  {
    m_cbViewPort.TopLeftX = viewPort.TopLeftX;
    m_cbViewPort.TopLeftY = viewPort.TopLeftY;
    m_cbViewPort.Width = viewPort.Width;
    m_cbViewPort.Height = viewPort.Height;
    m_bIsVPDirty = true;
  }
}

void CGUIShaderDX::Project(float &x, float &y, float &z)
{
  XMVECTOR vLocation = { x, y, z };
  XMVECTOR vScreenCoord = XMVector3Project(vLocation, m_cbViewPort.TopLeftX, m_cbViewPort.TopLeftY, 
                                           m_cbViewPort.Width, m_cbViewPort.Height, 0, 1, 
                                           m_cbWorldViewProj.projection, m_cbWorldViewProj.view, m_cbWorldViewProj.world);
  x = XMVectorGetX(vScreenCoord);
  y = XMVectorGetY(vScreenCoord);
  z = 0;
}

void XM_CALLCONV CGUIShaderDX::SetWVP(const XMMATRIX &w, const XMMATRIX &v, const XMMATRIX &p)
{
  m_bIsWVPDirty = true;
  m_cbWorldViewProj.world = w;
  m_cbWorldViewProj.view = v;
  m_cbWorldViewProj.projection = p;
}

void CGUIShaderDX::SetWorld(const XMMATRIX &value)
{
  m_bIsWVPDirty = true;
  m_cbWorldViewProj.world = value;
}

void CGUIShaderDX::SetView(const XMMATRIX &value)
{
  m_bIsWVPDirty = true;
  m_cbWorldViewProj.view = value;
}

void CGUIShaderDX::SetProjection(const XMMATRIX &value)
{
  m_bIsWVPDirty = true;
  m_cbWorldViewProj.projection = value;
}

void CGUIShaderDX::ApplyChanges(void)
{
  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  D3D11_MAPPED_SUBRESOURCE res;

  if (m_bIsWVPDirty)
  {
    if (SUCCEEDED(pContext->Map(m_pWVPBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res)))
    {
      XMMATRIX worldView = XMMatrixMultiply(m_cbWorldViewProj.world, m_cbWorldViewProj.view);
      XMMATRIX worldViewProj = XMMatrixMultiplyTranspose(worldView, m_cbWorldViewProj.projection);

      cbWorld* buffer = (cbWorld*)res.pData;
      buffer->wvp = worldViewProj;
      buffer->blackLevel = (g_Windowing.UseLimitedColor() ? 16.f / 255.f : 0.f);
      buffer->colorRange = (g_Windowing.UseLimitedColor() ? (235.f - 16.f) / 255.f : 1.0f);

      pContext->Unmap(m_pWVPBuffer, 0);
      m_bIsWVPDirty = false;
    }
  }

  // update view port buffer
  if (m_bIsVPDirty)
  {
    if (SUCCEEDED(pContext->Map(m_pVPBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res)))
    {
      *(cbViewPort*)res.pData = m_cbViewPort;
      pContext->Unmap(m_pVPBuffer, 0);
      m_bIsVPDirty = false;
    }
  }
}

void CGUIShaderDX::RestoreBuffers(void)
{
  const unsigned stride = sizeof(Vertex);
  const unsigned offset = 0;

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  // Set the vertex buffer to active in the input assembler so it can be rendered.
  pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
  // Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}

void CGUIShaderDX::ClipToScissorParams(void)
{
  CRect viewPort; // absolute positions of corners
  g_Windowing.GetViewPort(viewPort);

  // get current GUI transform
  const TransformMatrix &guiMatrix = g_graphicsContext.GetGUIMatrix();
  // get current GPU transforms
  XMFLOAT4X4 world, view, projection;
  XMStoreFloat4x4(&world, m_cbWorldViewProj.world);
  XMStoreFloat4x4(&view, m_cbWorldViewProj.view);
  XMStoreFloat4x4(&projection, m_cbWorldViewProj.projection);

  m_clipPossible = guiMatrix.m[0][1] == 0 &&
    guiMatrix.m[1][0]  == 0 &&
    guiMatrix.m[2][0]  == 0 &&
    guiMatrix.m[2][1]  == 0 &&
    view.m[0][1]       == 0 &&
    view.m[0][2]       == 0 &&
    view.m[1][0]       == 0 &&
    view.m[1][2]       == 0 &&
    view.m[2][0]       == 0 &&
    view.m[2][1]       == 0 &&
    projection.m[0][1] == 0 &&
    projection.m[0][2] == 0 &&
    projection.m[0][3] == 0 &&
    projection.m[1][0] == 0 &&
    projection.m[1][2] == 0 &&
    projection.m[1][3] == 0 &&
    projection.m[3][0] == 0 &&
    projection.m[3][1] == 0 &&
    projection.m[3][3] == 0;

  m_clipXFactor = 0.0f;
  m_clipXOffset = 0.0f;
  m_clipYFactor = 0.0f;
  m_clipYOffset = 0.0f;

  if (m_clipPossible)
  {
    m_clipXFactor = guiMatrix.m[0][0] * view.m[0][0] * projection.m[0][0];
    m_clipXOffset = (guiMatrix.m[0][3] * view.m[0][0] + view.m[3][0]) * projection.m[0][0];
    m_clipYFactor = guiMatrix.m[1][1] * view.m[1][1] * projection.m[1][1];
    m_clipYOffset = (guiMatrix.m[1][3] * view.m[1][1] + view.m[3][1]) * projection.m[1][1];

    float clipW = (guiMatrix.m[2][3] * view.m[2][2] + view.m[3][2]) * projection.m[2][3];
    float xMult = (viewPort.x2 - viewPort.x1) / (2 * clipW);
    float yMult = (viewPort.y1 - viewPort.y2) / (2 * clipW); // correct for inverted window coordinate scheme

    m_clipXFactor = m_clipXFactor * xMult;
    m_clipXOffset = m_clipXOffset * xMult + (viewPort.x2 + viewPort.x1) / 2;
    m_clipYFactor = m_clipYFactor * yMult;
    m_clipYOffset = m_clipYOffset * yMult + (viewPort.y2 + viewPort.y1) / 2;
  }
}

#endif