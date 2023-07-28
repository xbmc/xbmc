/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIShaderDX.h"
#include "windowing/GraphicContext.h"
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"
#include "utils/log.h"

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

#include <d3dcompiler.h>

using namespace DirectX;
using namespace Microsoft::WRL;

// shaders bytecode holder
// clang-format off
static const D3D_SHADER_DATA cbPSShaderCode[SHADER_METHOD_RENDER_COUNT] =
{
  { guishader_default, sizeof(guishader_default) }, // SHADER_METHOD_RENDER_DEFAULT
  { guishader_texture_noblend, sizeof(guishader_texture_noblend) }, // SHADER_METHOD_RENDER_TEXTURE_NOBLEND
  { guishader_fonts, sizeof(guishader_fonts) }, // SHADER_METHOD_RENDER_FONT
  { guishader_texture, sizeof(guishader_texture) }, // SHADER_METHOD_RENDER_TEXTURE_BLEND
  { guishader_multi_texture_blend, sizeof(guishader_multi_texture_blend) }, // SHADER_METHOD_RENDER_MULTI_TEXTURE_BLEND
  { guishader_interlaced_left, sizeof(guishader_interlaced_left) }, // SHADER_METHOD_RENDER_STEREO_INTERLACED_LEFT
  { guishader_interlaced_right, sizeof(guishader_interlaced_right) }, // SHADER_METHOD_RENDER_STEREO_INTERLACED_RIGHT
  { guishader_checkerboard_left, sizeof(guishader_checkerboard_left) }, // SHADER_METHOD_RENDER_STEREO_CHECKERBOARD_LEFT
  { guishader_checkerboard_right, sizeof(guishader_checkerboard_right) }, // SHADER_METHOD_RENDER_STEREO_CHECKERBOARD_RIGHT
};
// clang-format on

CGUIShaderDX::CGUIShaderDX() :
    m_pSampLinear(nullptr),
    m_pVPBuffer(nullptr),
    m_pWVPBuffer(nullptr),
    m_pVertexBuffer(nullptr),
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
  ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();

  // create vertex buffer
  CD3D11_BUFFER_DESC bufferDesc(sizeof(Vertex) * 4, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
  if (FAILED(pDevice->CreateBuffer(&bufferDesc, NULL, m_pVertexBuffer.ReleaseAndGetAddressOf())))
  {
    CLog::LogF(LOGERROR, "Failed to create GUI vertex buffer.");
    return false;
  }

  // Create the constant buffer for WVP
  size_t buffSize = (sizeof(cbWorld) + 15) & ~15;
  CD3D11_BUFFER_DESC cbbd(buffSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE); // it can change very frequently
  if (FAILED(pDevice->CreateBuffer(&cbbd, NULL, m_pWVPBuffer.ReleaseAndGetAddressOf())))
  {
    CLog::LogF(LOGERROR, "Failed to create the constant buffer.");
    return false;
  }
  m_bIsWVPDirty = true;

  CRect viewPort;
  DX::Windowing()->GetViewPort(viewPort);

  // initial data for viewport buffer
  m_cbViewPort.TopLeftX = viewPort.x1;
  m_cbViewPort.TopLeftY = viewPort.y1;
  m_cbViewPort.Width = viewPort.Width();
  m_cbViewPort.Height = viewPort.Height();

  cbbd.ByteWidth = sizeof(cbViewPort);
  D3D11_SUBRESOURCE_DATA initData = { &m_cbViewPort, 0, 0 };
  // create viewport buffer
  if (FAILED(pDevice->CreateBuffer(&cbbd, &initData, m_pVPBuffer.ReleaseAndGetAddressOf())))
    return false;

  return true;
}

bool CGUIShaderDX::CreateSamplers()
{
  // Describe the Sampler State
  D3D11_SAMPLER_DESC sampDesc = {};
  sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  sampDesc.MinLOD = 0;
  sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

  if (FAILED(DX::DeviceResources::Get()->GetD3DDevice()->CreateSamplerState(&sampDesc, m_pSampLinear.ReleaseAndGetAddressOf())))
    return false;

  DX::DeviceResources::Get()->GetD3DContext()->PSSetSamplers(0, 1, m_pSampLinear.GetAddressOf());

  return true;
}

void CGUIShaderDX::ApplyStateBlock(void)
{
  if (!m_bCreated)
    return;

  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetD3DContext();

  m_vertexShader.BindShader();
  pContext->VSSetConstantBuffers(0, 1, m_pWVPBuffer.GetAddressOf());

  m_pixelShader[m_currentShader].BindShader();
  pContext->PSSetConstantBuffers(0, 1, m_pWVPBuffer.GetAddressOf());
  pContext->PSSetConstantBuffers(1, 1, m_pVPBuffer.GetAddressOf());

  pContext->PSSetSamplers(0, 1, m_pSampLinear.GetAddressOf());

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

  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetD3DContext();

  // update vertex buffer
  D3D11_MAPPED_SUBRESOURCE resource;
  if (SUCCEEDED(pContext->Map(m_pVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource)))
  {
    // we are using strip topology
    Vertex vertices[4] = { v2, v3, v1, v4 };
    memcpy(resource.pData, &vertices, sizeof(Vertex) * 4);
    pContext->Unmap(m_pVertexBuffer.Get(), 0);
    // Draw primitives
    pContext->Draw(4, 0);
  }
}

void CGUIShaderDX::DrawIndexed(unsigned int indexCount, unsigned int startIndex, unsigned int startVertex)
{
  if (!m_bCreated)
    return;

  ApplyChanges();
  DX::DeviceResources::Get()->GetD3DContext()->DrawIndexed(indexCount, startIndex, startVertex);
}

void CGUIShaderDX::Draw(unsigned int vertexCount, unsigned int startVertex)
{
  if (!m_bCreated)
    return;

  ApplyChanges();
  DX::DeviceResources::Get()->GetD3DContext()->Draw(vertexCount, startVertex);
}

void CGUIShaderDX::SetShaderViews(unsigned int numViews, ID3D11ShaderResourceView** views)
{
  if (!m_bCreated)
    return;

  DX::DeviceResources::Get()->GetD3DContext()->PSSetShaderResources(0, numViews, views);
}

void CGUIShaderDX::Release()
{
  m_pVertexBuffer = nullptr;
  m_pWVPBuffer = nullptr;
  m_pVPBuffer = nullptr;
  m_pSampLinear = nullptr;
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
#if defined(_XM_SSE_INTRINSICS_) && !defined(_XM_NO_INTRINSICS_)
  XMVECTOR vLocation = { x, y, z };
#elif defined(_XM_ARM_NEON_INTRINSICS_) && !defined(_XM_NO_INTRINSICS_)
  XMVECTOR vLocation = { x, y };
#endif
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
  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetD3DContext();
  D3D11_MAPPED_SUBRESOURCE res;

  if (m_bIsWVPDirty)
  {
    if (SUCCEEDED(pContext->Map(m_pWVPBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res)))
    {
      XMMATRIX worldView = XMMatrixMultiply(m_cbWorldViewProj.world, m_cbWorldViewProj.view);
      XMMATRIX worldViewProj = XMMatrixMultiplyTranspose(worldView, m_cbWorldViewProj.projection);

      cbWorld* buffer = (cbWorld*)res.pData;
      buffer->wvp = worldViewProj;
      buffer->blackLevel = (DX::Windowing()->UseLimitedColor() ? 16.f / 255.f : 0.f);
      buffer->colorRange = (DX::Windowing()->UseLimitedColor() ? (235.f - 16.f) / 255.f : 1.0f);
      if (DX::Windowing()->IsTransferPQ())
        buffer->sdrPeakLum = 10000.0f / DX::Windowing()->GetGuiSdrPeakLuminance();
      buffer->PQ = (DX::Windowing()->IsTransferPQ() ? 1 : 0);

      pContext->Unmap(m_pWVPBuffer.Get(), 0);
      m_bIsWVPDirty = false;
    }
  }

  // update view port buffer
  if (m_bIsVPDirty)
  {
    if (SUCCEEDED(pContext->Map(m_pVPBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res)))
    {
      *(cbViewPort*)res.pData = m_cbViewPort;
      pContext->Unmap(m_pVPBuffer.Get(), 0);
      m_bIsVPDirty = false;
    }
  }
}

void CGUIShaderDX::RestoreBuffers(void)
{
  const unsigned stride = sizeof(Vertex);
  const unsigned offset = 0;

  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetD3DContext();
  // Set the vertex buffer to active in the input assembler so it can be rendered.
  pContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
  // Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}

void CGUIShaderDX::ClipToScissorParams(void)
{
  CRect viewPort; // absolute positions of corners
  DX::Windowing()->GetViewPort(viewPort);

  // get current GUI transform
  const TransformMatrix &guiMatrix = CServiceBroker::GetWinSystem()->GetGfxContext().GetGUIMatrix();
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
