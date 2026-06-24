/*
 *  Copyright (C) 2005-2026 Team Kodi
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
#include "guishader_checkerboard_left.h"
#include "guishader_checkerboard_right.h"
#include "guishader_clip_vert.h"
#include "guishader_default.h"
#include "guishader_fonts.h"
#include "guishader_interlaced_left.h"
#include "guishader_interlaced_right.h"
#include "guishader_multi_texture_blend.h"
#include "guishader_multi_texture_blend_nearest.h"
#include "guishader_texture.h"
#include "guishader_texture_nearest.h"
#include "guishader_texture_noblend.h"
#include "guishader_vert.h"

#include <algorithm>
#include <array>

#include <d3dcompiler.h>

using namespace DirectX;
using namespace Microsoft::WRL;

// shaders bytecode holder
// clang-format off
constexpr std::array<D3D_SHADER_DATA, SHADER_METHOD_RENDER_COUNT> shaderCode =
{{
  { guishader_default, sizeof(guishader_default) }, // SHADER_METHOD_RENDER_DEFAULT
  { guishader_texture_noblend, sizeof(guishader_texture_noblend) }, // SHADER_METHOD_RENDER_TEXTURE_NOBLEND
  { guishader_fonts, sizeof(guishader_fonts) }, // SHADER_METHOD_RENDER_FONT
  { guishader_fonts, sizeof(guishader_fonts) }, // SHADER_METHOD_RENDER_FONT_SHADER_CLIP
  { guishader_texture, sizeof(guishader_texture) }, // SHADER_METHOD_RENDER_TEXTURE_BLEND
  { guishader_texture_nearest, sizeof(guishader_texture_nearest) }, // SHADER_METHOD_RENDER_TEXTURE_BLEND_NEAREST
  { guishader_multi_texture_blend, sizeof(guishader_multi_texture_blend) }, // SHADER_METHOD_RENDER_MULTI_TEXTURE_BLEND
  { guishader_multi_texture_blend_nearest, sizeof(guishader_multi_texture_blend_nearest) }, // SHADER_METHOD_RENDER_MULTI_TEXTURE_BLEND_NEAREST
  { guishader_interlaced_left, sizeof(guishader_interlaced_left) }, // SHADER_METHOD_RENDER_STEREO_INTERLACED_LEFT
  { guishader_interlaced_right, sizeof(guishader_interlaced_right) }, // SHADER_METHOD_RENDER_STEREO_INTERLACED_RIGHT
  { guishader_checkerboard_left, sizeof(guishader_checkerboard_left) }, // SHADER_METHOD_RENDER_STEREO_CHECKERBOARD_LEFT
  { guishader_checkerboard_right, sizeof(guishader_checkerboard_right) }, // SHADER_METHOD_RENDER_STEREO_CHECKERBOARD_RIGHT
}};
// clang-format on

// Build-time check of the count of shader code entries versus the count of shader methods.
// The default initializer allows missing array initializer elements to slip without this.
static_assert(std::ranges::none_of(shaderCode,
                                   [](const auto& s)
                                   { return s.pBytecode == nullptr || s.BytecodeLength == 0; }));

void XM_CALLCONV CGUIShaderDX::CWorldViewProj::SetWVP(const XMMATRIX& w,
                                                      const XMMATRIX& v,
                                                      const XMMATRIX& p)
{
  m_isWorldDirty = true;
  m_world = w;
  m_isVPDirty = true;
  m_view = v;
  m_projection = p;
}

void CGUIShaderDX::CWorldViewProj::SetWorld(const XMMATRIX& value)
{
  m_isWorldDirty = true;
  m_world = value;
}

void CGUIShaderDX::CWorldViewProj::SetView(const XMMATRIX& value)
{
  m_isVPDirty = true;
  m_view = value;
}

void CGUIShaderDX::CWorldViewProj::SetProjection(const XMMATRIX& value)
{
  m_isVPDirty = true;
  m_projection = value;
}

DirectX::XMMATRIX XM_CALLCONV CGUIShaderDX::CWorldViewProj::GetWVP() const
{
  if (m_isVPDirty)
    m_cachedVP = XMMatrixMultiply(m_view, m_projection);

  if (m_isVPDirty || m_isWorldDirty)
    m_cachedWVP = XMMatrixMultiplyTranspose(m_world, m_cachedVP);

  m_isWorldDirty = false;
  m_isVPDirty = false;

  return m_cachedWVP;
}

CGUIShaderDX::~CGUIShaderDX()
{
  Release();
}

bool CGUIShaderDX::Initialize()
{
  // Create input layout
  D3D11_INPUT_ELEMENT_DESC layout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
  };

  if (!m_vertexShader.Create(guishader_vert, sizeof(guishader_vert), layout, ARRAYSIZE(layout)) ||
      !m_vertexShaderClip.Create(guishader_clip_vert, sizeof(guishader_clip_vert), layout,
                                 ARRAYSIZE(layout)))
  {
    goto error;
  }

  for (std::size_t i = 0; i < SHADER_METHOD_RENDER_COUNT; i++)
  {
    // All methods except the two font ones share guishader_vert.hlsl
    if (i == SHADER_METHOD_RENDER_FONT_SHADER_CLIP)
      m_shaders[i].m_vs = &m_vertexShaderClip;
    else
      m_shaders[i].m_vs = &m_vertexShader;

    if (!m_shaders[i].m_ps.Create(shaderCode[i].pBytecode, shaderCode[i].BytecodeLength))
      goto error;
  }

  if (!CreateBuffers() || !CreateSamplers())
    goto error;

  m_bCreated = true;
  return true;

error:
  Release();

  std::ranges::for_each(m_shaders, [](auto& s) { s.m_ps.Release(); });

  m_vertexShader.Release();
  m_vertexShaderClip.Release();

  return false;
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
    CLog::LogF(LOGERROR, "Failed to create the world constant buffer.");
    return false;
  }
  // class members have reasonable defaults for the initial buffer load
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
  {
    CLog::LogF(LOGERROR, "Failed to create the viewport constant buffer.");
    return false;
  }
  m_bIsVPDirty = false;

  return true;
}

bool CGUIShaderDX::CreateSamplers()
{
  // Linear sampler
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

  // Nearest neighbor sampler
  sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

  if (FAILED(DX::DeviceResources::Get()->GetD3DDevice()->CreateSamplerState(
          &sampDesc, m_pSampNearestNeighbor.ReleaseAndGetAddressOf())))
    return false;

  SetSamplers();

  return true;
}

void CGUIShaderDX::SetSamplers()
{
  // Slot 0: linear sampler
  // Slot 1: nearest neighbor sampler
  ID3D11SamplerState* samplers[] = {m_pSampLinear.Get(), m_pSampNearestNeighbor.Get()};
  DX::DeviceResources::Get()->GetD3DContext()->PSSetSamplers(0, 2, samplers);
}

void CGUIShaderDX::ApplyStateBlock(void)
{
  if (!m_bCreated)
    return;

  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetD3DContext();

  m_shaders[m_currentShader].m_vs->BindShader();
  pContext->VSSetConstantBuffers(0, 1, m_pWVPBuffer.GetAddressOf());

  m_shaders[m_currentShader].m_ps.BindShader();
  pContext->PSSetConstantBuffers(0, 1, m_pWVPBuffer.GetAddressOf());
  pContext->PSSetConstantBuffers(1, 1, m_pVPBuffer.GetAddressOf());

  SetSamplers();

  RestoreBuffers();
}

void CGUIShaderDX::Begin(unsigned int flags)
{
  if (!m_bCreated)
    return;

  if (flags >= SHADER_METHOD_RENDER_COUNT)
  {
    CLog::LogF(LOGERROR, "Invalid shader method count {}.", flags);
    return;
  }

  if (m_currentShader != flags)
  {
    m_currentShader = flags;
    m_shaders[m_currentShader].m_vs->BindShader();
    m_shaders[m_currentShader].m_ps.BindShader();
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
  m_pSampNearestNeighbor = nullptr;
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

void CGUIShaderDX::Project(float& x, float& y, float& z) const
{
  const XMVECTOR V = XMVectorSet(x, y, z, .0f);

  const XMVECTOR screenCoord =
      XMVector3Project(V, m_cbViewPort.TopLeftX, m_cbViewPort.TopLeftY, m_cbViewPort.Width,
                       m_cbViewPort.Height, 0.0f, 1.0f, m_cbWorldViewProj.GetProjection(),
                       m_cbWorldViewProj.GetView(), m_cbWorldViewProj.GetWorld());

  x = XMVectorGetX(screenCoord);
  y = XMVectorGetY(screenCoord);
  z = .0f;
}

void XM_CALLCONV CGUIShaderDX::SetWVP(const XMMATRIX& w, const XMMATRIX& v, const XMMATRIX& p)
{
  m_bIsWVPDirty = true;
  m_cbWorldViewProj.SetWVP(w, v, p);
}

void CGUIShaderDX::SetWorld(const XMMATRIX& value)
{
  m_bIsWVPDirty = true;
  m_cbWorldViewProj.SetWorld(value);
}

void CGUIShaderDX::SetView(const XMMATRIX& value)
{
  m_bIsWVPDirty = true;
  m_cbWorldViewProj.SetView(value);
}

void CGUIShaderDX::SetProjection(const XMMATRIX& value)
{
  m_bIsWVPDirty = true;
  m_cbWorldViewProj.SetProjection(value);
}

void CGUIShaderDX::SetDepth(const float depth)
{
  m_bIsWVPDirty = true;
  m_depth = depth;
}

void CGUIShaderDX::SetShaderClip(float x1, float y1, float x2, float y2)
{
  m_bIsWVPDirty = true;
  m_shaderClip.x = x1;
  m_shaderClip.y = y1;
  m_shaderClip.z = x2;
  m_shaderClip.w = y2;
}

void CGUIShaderDX::SetTexStep(float stepX, float stepY, float stepX2, float stepY2)
{
  m_bIsWVPDirty = true;
  m_texStep.x = stepX;
  m_texStep.y = stepY;
  m_texStep2.x = stepX2;
  m_texStep2.y = stepY2;
}

void CGUIShaderDX::ApplyChanges(void)
{
  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetD3DContext();
  if (pContext == nullptr)
  {
    CLog::LogF(LOGERROR, "Unable to retrieve device context.");
    return;
  }

  D3D11_MAPPED_SUBRESOURCE res;

  if (m_bIsWVPDirty &&
      SUCCEEDED(pContext->Map(m_pWVPBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res)))
  {
    cbWorld* buffer = (cbWorld*)res.pData;

    // Vertex shader constants
    buffer->wvp = m_cbWorldViewProj.GetWVP();

    if (m_currentShader == SHADER_METHOD_RENDER_FONT ||
        m_currentShader == SHADER_METHOD_RENDER_FONT_SHADER_CLIP)
    {
      buffer->m_shaderClip = m_shaderClip;
      buffer->m_texStep = m_texStep;
      buffer->m_texStep2 = m_texStep2;
    }

    // Translate from GL convention (-1 far 1 near) to D3D (0 far 1 near)
    buffer->depth = m_depth / 2.f + 0.5f;

    // Pixel shader constants
    buffer->blackLevel = (DX::Windowing()->UseLimitedColor() ? 16.f / 255.f : 0.f);
    buffer->colorRange = (DX::Windowing()->UseLimitedColor() ? (235.f - 16.f) / 255.f : 1.0f);
    if (DX::Windowing()->IsTransferPQ())
      buffer->sdrPeakLum = 10000.0f / std::max(1.0f, DX::Windowing()->GetGuiSdrPeakLuminance());
    buffer->PQ = (DX::Windowing()->IsTransferPQ() ? 1 : 0);

    pContext->Unmap(m_pWVPBuffer.Get(), 0);

    m_bIsWVPDirty = false;
  }

  // update view port buffer
  if (m_bIsVPDirty &&
      SUCCEEDED(pContext->Map(m_pVPBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res)))
  {
    *(cbViewPort*)res.pData = m_cbViewPort;
    pContext->Unmap(m_pVPBuffer.Get(), 0);
    m_bIsVPDirty = false;
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
  XMStoreFloat4x4(&world, m_cbWorldViewProj.GetWorld());
  XMStoreFloat4x4(&view, m_cbWorldViewProj.GetView());
  XMStoreFloat4x4(&projection, m_cbWorldViewProj.GetProjection());

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

    // projection.m[2][3] is the projection type. 0 = orthographic, 1 or -1 = perspective
    // Orthographic projection: no perspective divide; projection.m[0][0]/m[1][1] already
    // encode the full NDC scale, apply only the viewport mapping.
    // Same treatment for broken transform stacks.
    const float clipW = (guiMatrix.m[2][3] * view.m[2][2] + view.m[3][2]) * projection.m[2][3];
    const float divisorW = (projection.m[2][3] == 0 || clipW == 0) ? 1.0f : clipW;
    const float xMult = (viewPort.x2 - viewPort.x1) / (2 * divisorW);
    const float yMult = (viewPort.y1 - viewPort.y2) /
                        (2 * divisorW); // correct for inverted window coordinate scheme

    m_clipXFactor = m_clipXFactor * xMult;
    m_clipXOffset = m_clipXOffset * xMult + (viewPort.x2 + viewPort.x1) / 2;
    m_clipYFactor = m_clipYFactor * yMult;
    m_clipYOffset = m_clipYOffset * yMult + (viewPort.y2 + viewPort.y1) / 2;
  }
}
