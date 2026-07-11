/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIFontTTFDX.h"

#include "GUIFontManager.h"
#include "GUIShaderDX.h"
#include "TextureDX.h"
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"
#include "utils/log.h"

#include <cassert>
#include <limits>

// stuff for freetype
#include <ft2build.h>

using namespace Microsoft::WRL;

#ifdef TARGET_WINDOWS_STORE
#define generic GenericFromFreeTypeLibrary
#endif

#include FT_FREETYPE_H
#include FT_GLYPH_H

namespace
{
constexpr size_t ELEMENT_ARRAY_MAX_CHAR_INDEX = 2000;
} /* namespace */

CGUIFontTTF* CGUIFontTTF::CreateGUIFontTTF(const std::string& fontIdent)
{
  return new CGUIFontTTFDX(fontIdent);
}

CGUIFontTTFDX::CGUIFontTTFDX(const std::string& fontIdent) : CGUIFontTTF(fontIdent)
{
  DX::Windowing()->Register(this);
}

CGUIFontTTFDX::~CGUIFontTTFDX(void)
{
  DX::Windowing()->Unregister(this);
}

bool CGUIFontTTFDX::FirstBegin()
{
  if (!DX::DeviceResources::Get()->GetD3DContext())
    return false;

  CGUIShaderDX* pGUIShader = DX::Windowing()->GetGUIShader();
  if (pGUIShader == nullptr)
    return false;

  // Set a shader first to trigger the internal clipping recalculations. The cached clipping data
  // contains stale information from the previous draw otherwise.
  // Mirrors the GL/GLES workaround
  pGUIShader->Begin(SHADER_METHOD_RENDER_FONT);

  if (DX::Windowing()->ScissorsCanEffectClipping())
  {
    m_scissorClip = true;
    // SHADER_METHOD_RENDER_FONT already activated
  }
  else
  {
    m_scissorClip = false;
    DX::Windowing()->ResetScissors();
    pGUIShader->Begin(SHADER_METHOD_RENDER_FONT_SHADER_CLIP);
  }

  return true;
}

void CGUIFontTTFDX::LastEnd()
{
  // static vertex arrays are not supported anymore
  assert(m_vertex.empty());

  CWinSystemBase* const winSystem = CServiceBroker::GetWinSystem();
  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetD3DContext();
  if (!pContext || !winSystem)
    return;

  typedef CGUIFontTTF::CTranslatedVertices trans;
  // no chars to render
  if (std::all_of(m_vertexTrans.begin(), m_vertexTrans.end(),
                  [](trans& _) { return _.m_vertexBuffer->size <= 0; }))
    return;

  CreateStaticIndexBuffer();
  if (m_staticIndexBuffer == nullptr)
    return;

  CGUIShaderDX* pGUIShader = DX::Windowing()->GetGUIShader();

  if (pGUIShader == nullptr)
    return;

  pGUIShader->SetDepth(CServiceBroker::GetWinSystem()->GetGfxContext().GetTransformDepth());

  // Set font texture as shader resource
  if (m_speedupTexture == nullptr)
    return;
  pGUIShader->SetShaderViews(1, m_speedupTexture->GetAddressOfSRV());
  // Enable alpha blend
  DX::Windowing()->SetAlphaBlendEnable(true);
  // Set our static index buffer
  pContext->IASetIndexBuffer(m_staticIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // Store current scissors
  CGraphicContext& context = winSystem->GetGfxContext();
  CRect scissor;
  if (m_scissorClip)
    scissor = context.StereoCorrection(context.GetScissors());

  for (size_t i = 0; i < m_vertexTrans.size(); i++)
  {
    // ignore empty buffers
    if (m_vertexTrans[i].m_vertexBuffer->size == 0)
      continue;

    const CD3DBuffer* vBuffer =
        reinterpret_cast<const CD3DBuffer*>(m_vertexTrans[i].m_vertexBuffer->bufferHandle);
    if (vBuffer == nullptr)
      continue;

    if (m_scissorClip)
    {
      // Apply the clip rectangle
      CRect clip = DX::Windowing()->ClipRectToScissorRect(m_vertexTrans[i].m_clip);
      clip.Intersect(scissor);

      // skip empty clip, a little improvement to not render invisible text
      if (clip.IsEmpty())
        continue;
      DX::Windowing()->SetScissors(clip);
    }
    else
    {
      // clip using vertex shader
      const float scaleX = context.GetGUIScaleX();
      const float scaleY = context.GetGUIScaleY();

      if (scaleX == 0 || scaleY == 0)
      {
        CLog::LogF(LOGERROR, "Invalid GUI scaling ({}x{}).", scaleX, scaleY);
        continue;
      }

      const float x1 = (m_vertexTrans[i].m_clip.x1 - m_vertexTrans[i].m_translateX -
                        m_vertexTrans[i].m_offsetX) /
                       scaleX;
      const float y1 = (m_vertexTrans[i].m_clip.y1 - m_vertexTrans[i].m_translateY -
                        m_vertexTrans[i].m_offsetY) /
                       scaleY;
      const float x2 = (m_vertexTrans[i].m_clip.x2 - m_vertexTrans[i].m_translateX -
                        m_vertexTrans[i].m_offsetX) /
                       scaleX;
      const float y2 = (m_vertexTrans[i].m_clip.y2 - m_vertexTrans[i].m_translateY -
                        m_vertexTrans[i].m_offsetY) /
                       scaleY;

      pGUIShader->SetShaderClip(x1, y1, x2, y2);

      // Texture steps
      if (m_textureWidth == 0 || m_textureHeight == 0)
      {
        CLog::LogF(LOGERROR, "Invalid texture dimensions ({}x{}).", m_textureWidth,
                   m_textureHeight);
        continue;
      }

      const float stepX = 1.f / static_cast<float>(m_textureWidth);
      const float stepY = 1.f / static_cast<float>(m_textureHeight);

      pGUIShader->SetTexStep(stepX, stepY, 1, 1);
    }

    // calculate the fractional offset to the ideal position
    float fractX =
        context.ScaleFinalXCoord(m_vertexTrans[i].m_translateX, m_vertexTrans[i].m_translateY);
    float fractY =
        context.ScaleFinalYCoord(m_vertexTrans[i].m_translateX, m_vertexTrans[i].m_translateY);
    fractX = -fractX + std::round(fractX);
    fractY = -fractY + std::round(fractY);

    // The multiplication order below is important and the reverse of the GL/GLES chain because of
    // column-major vs row-major differences
    // proj * model * gui * scroll * translation * scaling * correction factor
    const XMMATRIX world = pGUIShader->GetWorld();

    const XMMATRIX correction = XMMatrixTranslation(fractX, fractY, 0.0f);
    const XMMATRIX scale = XMMatrixScaling(context.GetGUIScaleX(), context.GetGUIScaleY(), 1.0f);
    const XMMATRIX translation =
        XMMatrixTranslation(m_vertexTrans[i].m_translateX, m_vertexTrans[i].m_translateY, 0.0f);
    const XMMATRIX offset =
        XMMatrixTranslation(m_vertexTrans[i].m_offsetX, m_vertexTrans[i].m_offsetY, 0.0f);
    const XMMATRIX gui =
        XMLoadFloat3x4(reinterpret_cast<const XMFLOAT3X4*>(&context.GetGUIMatrix().m));

    XMMATRIX matrix = XMMatrixMultiply(world, correction);
    matrix = XMMatrixMultiply(matrix, scale);
    matrix = XMMatrixMultiply(matrix, translation);
    matrix = XMMatrixMultiply(matrix, offset);
    matrix = XMMatrixMultiply(matrix, gui);

    pGUIShader->SetWorld(matrix);

    // Set the static vertex buffer to active in the input assembler
    ID3D11Buffer* buffers[1] = {vBuffer->Get()};

    unsigned int offsets = 0;
    unsigned int strides = sizeof(SVertex);
    pContext->IASetVertexBuffers(0, 1, buffers, &strides, &offsets);

    // Do the actual drawing operation, split into groups of characters no
    // larger than the pre-determined size of the element array
    for (size_t character = 0; m_vertexTrans[i].m_vertexBuffer->size > character;
         character += ELEMENT_ARRAY_MAX_CHAR_INDEX)
    {
      size_t count = m_vertexTrans[i].m_vertexBuffer->size - character;
      count = std::min<size_t>(count, ELEMENT_ARRAY_MAX_CHAR_INDEX);

      // 6 indices and 4 vertices per character
      if (count > std::numeric_limits<unsigned int>::max() / 6 ||
          character > std::numeric_limits<unsigned int>::max() / 4)
      {
        CLog::LogF(LOGERROR, "Character index too large: {}", character);
        break;
      }
      pGUIShader->DrawIndexed(count * 6, 0, character * 4);
    }
    pGUIShader->SetWorld(world);
  }

  // restore scissor
  if (m_scissorClip)
    DX::Windowing()->SetScissors(scissor);

  pGUIShader->RestoreBuffers();
}

CVertexBuffer CGUIFontTTFDX::CreateVertexBuffer(const std::vector<SVertex>& vertices) const
{
  CD3DBuffer* buffer = nullptr;
  // do not create empty buffers, leave buffer as nullptr, it will be ignored on drawing stage
  if (!vertices.empty())
  {
    buffer = new CD3DBuffer();
    if (!buffer->Create(D3D11_BIND_VERTEX_BUFFER, vertices.size(), sizeof(SVertex),
                        DXGI_FORMAT_UNKNOWN, D3D11_USAGE_IMMUTABLE, &vertices[0]))
    {
      CLog::LogF(LOGERROR, "Failed to create vertex buffer.");
      delete buffer;
      buffer = nullptr;
    }
    else
    {
      AddReference((CGUIFontTTFDX*)this, buffer);
    }
  }

  return CVertexBuffer(reinterpret_cast<void*>(buffer), buffer ? vertices.size() / 4 : 0, this);
}

void CGUIFontTTFDX::AddReference(CGUIFontTTFDX* font, CD3DBuffer* pBuffer)
{
  font->m_buffers.emplace_back(pBuffer);
}

void CGUIFontTTFDX::DestroyVertexBuffer(CVertexBuffer& buffer) const
{
  if (nullptr != buffer.bufferHandle)
  {
    CD3DBuffer* vbuffer = reinterpret_cast<CD3DBuffer*>(buffer.bufferHandle);
    ClearReference((CGUIFontTTFDX*)this, vbuffer);
    if (vbuffer)
      delete vbuffer;
    buffer.bufferHandle = 0;
  }
}

void CGUIFontTTFDX::ClearReference(CGUIFontTTFDX* font, CD3DBuffer* pBuffer)
{
  std::list<CD3DBuffer*>::iterator it =
      std::find(font->m_buffers.begin(), font->m_buffers.end(), pBuffer);
  if (it != font->m_buffers.end())
    font->m_buffers.erase(it);
}

std::unique_ptr<CTexture> CGUIFontTTFDX::ReallocTexture(unsigned int& newHeight)
{
  assert(newHeight != 0);
  assert(m_textureWidth != 0);
  if (m_textureHeight == 0)
  {
    m_texture.reset();
    m_speedupTexture.reset();
  }
  m_staticCache.Flush();
  m_dynamicCache.Flush();

  std::unique_ptr<CDXTexture> pNewTexture =
      std::make_unique<CDXTexture>(m_textureWidth, newHeight, XB_FMT_A8);
  std::unique_ptr<CD3DTexture> newSpeedupTexture = std::make_unique<CD3DTexture>();
  if (!newSpeedupTexture->Create(m_textureWidth, newHeight, 1, D3D11_USAGE_DEFAULT,
                                 DXGI_FORMAT_R8_UNORM))
  {
    return nullptr;
  }

  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetImmediateContext();
  if (!pContext)
    return nullptr;

  // There might be data to copy from the previous texture
  if (m_speedupTexture)
  {
    CD3D11_BOX rect(0, 0, 0, m_textureWidth, m_textureHeight, 1);
    pContext->CopySubresourceRegion(newSpeedupTexture->Get(), 0, 0, 0, 0, m_speedupTexture->Get(),
                                    0, &rect);
  }

  // Zero the previously unused part of the resource
  //! @todo avoid the CPU/GPU synchronization with a GPU clear
  if (newHeight > m_textureHeight)
  {
    const unsigned int startRow = m_speedupTexture ? m_textureHeight : 0;
    CD3D11_BOX rect(0, startRow, 0, m_textureWidth, newHeight, 1);
    std::vector<uint8_t> black(m_textureWidth * (newHeight - startRow), 0);
    pContext->UpdateSubresource(newSpeedupTexture->Get(), 0, &rect, black.data(), m_textureWidth,
                                0);
  }
  m_texture.reset();

  m_textureHeight = newHeight;
  m_textureScaleY = 1.0f / m_textureHeight;
  m_speedupTexture = std::move(newSpeedupTexture);

  return pNewTexture;
}

bool CGUIFontTTFDX::CopyCharToTexture(
    FT_BitmapGlyph bitGlyph, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
{
  FT_Bitmap bitmap = bitGlyph->bitmap;

  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetImmediateContext();
  if (m_speedupTexture && m_speedupTexture->Get() && pContext && bitmap.buffer)
  {
    CD3D11_BOX dstBox(x1, y1, 0, x2, y2, 1);
    pContext->UpdateSubresource(m_speedupTexture->Get(), 0, &dstBox, bitmap.buffer, bitmap.pitch,
                                0);
    return true;
  }

  return false;
}

void CGUIFontTTFDX::DeleteHardwareTexture()
{
}

void CGUIFontTTFDX::CreateStaticIndexBuffer(void)
{
  // Fast path no mutex
  if (m_staticIndexBufferCreated.load(std::memory_order_acquire))
    return;

  std::unique_lock lock(m_staticIndexBufferSection);

  // Second protected check for thread safety
  if (m_staticIndexBufferCreated.load(std::memory_order_relaxed))
    return;

  ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();
  if (!pDevice)
    return;

  uint16_t index[ELEMENT_ARRAY_MAX_CHAR_INDEX][6];
  for (size_t i = 0; i < ELEMENT_ARRAY_MAX_CHAR_INDEX; i++)
  {
    index[i][0] = 4 * i;
    index[i][1] = 4 * i + 1;
    index[i][2] = 4 * i + 2;
    index[i][3] = 4 * i + 2;
    index[i][4] = 4 * i + 3;
    index[i][5] = 4 * i + 0;
  }

  CD3D11_BUFFER_DESC desc(sizeof(index), D3D11_BIND_INDEX_BUFFER, D3D11_USAGE_IMMUTABLE);
  D3D11_SUBRESOURCE_DATA initData = {};
  initData.pSysMem = index;

  if (SUCCEEDED(
          pDevice->CreateBuffer(&desc, &initData, m_staticIndexBuffer.ReleaseAndGetAddressOf())))
    m_staticIndexBufferCreated.store(true, std::memory_order_release);
}

CCriticalSection CGUIFontTTFDX::m_staticIndexBufferSection;
std::atomic<bool> CGUIFontTTFDX::m_staticIndexBufferCreated = false;
ComPtr<ID3D11Buffer> CGUIFontTTFDX::m_staticIndexBuffer = nullptr;

void CGUIFontTTFDX::OnDestroyDevice(bool fatal)
{
  std::unique_lock lock(m_staticIndexBufferSection);

  m_staticIndexBuffer = nullptr;
  m_staticIndexBufferCreated.store(false, std::memory_order_relaxed);
}

void CGUIFontTTFDX::OnCreateDevice(void)
{
}
