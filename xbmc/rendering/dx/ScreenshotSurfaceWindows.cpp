/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ScreenshotSurfaceWindows.h"

#include "guilib/GUIWindowManager.h"
#include "rendering/dx/DeviceResources.h"
#include "utils/Screenshot.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <mutex>

#include <cmath>

#include <wrl/client.h>

using namespace Microsoft::WRL;

namespace
{
// Project a 10-bit sample onto the 16-bit scale so full scale maps to full scale
uint16_t Expand10To16(uint32_t v)
{
  return static_cast<uint16_t>(std::lround(v * 65535.0 / 1023.0));
}
} // namespace

void CScreenshotSurfaceWindows::Register()
{
  CScreenShot::Register(CScreenshotSurfaceWindows::CreateSurface);
}

std::unique_ptr<IScreenshotSurface> CScreenshotSurfaceWindows::CreateSurface()
{
  return std::unique_ptr<CScreenshotSurfaceWindows>(new CScreenshotSurfaceWindows());
}

bool CScreenshotSurfaceWindows::Capture(const ScreenshotContext& ctx)
{
  std::unique_lock lock(ctx.winSystem.GetGfxContext());
  ctx.windowManager.Render();

  auto deviceResources = DX::DeviceResources::Get();
  deviceResources->FinishCommandList();

  ComPtr<ID3D11DeviceContext> pImdContext = deviceResources->GetImmediateContext();
  ComPtr<ID3D11Device> pDevice = deviceResources->GetD3DDevice();
  CD3DTexture& backbuffer = deviceResources->GetBackBuffer();
  if (!backbuffer.Get())
    return false;

  D3D11_TEXTURE2D_DESC desc = {};
  backbuffer.GetDesc(&desc);
  desc.Usage = D3D11_USAGE_STAGING;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  desc.BindFlags = 0;

  ComPtr<ID3D11Texture2D> pCopyTexture = nullptr;
  if (SUCCEEDED(pDevice->CreateTexture2D(&desc, nullptr, &pCopyTexture)))
  {
    // take copy
    pImdContext->CopyResource(pCopyTexture.Get(), backbuffer.Get());

    D3D11_MAPPED_SUBRESOURCE res;
    if (SUCCEEDED(pImdContext->Map(pCopyTexture.Get(), 0, D3D11_MAP_READ, 0, &res)))
    {
      m_width = desc.Width;
      m_height = desc.Height;
      if (desc.Format == DXGI_FORMAT_R10G10B10A2_UNORM)
      {
        // 10-bit swapchain: unpack to RGBA16 instead of decimating to 8-bit
        m_bitDepth = 10;
        m_stride = m_width * 8; // 4 channels x 2 bytes
        m_buffer = new unsigned char[m_height * m_stride];
        for (int y = 0; y < m_height; y++)
        {
          const uint32_t* pixels10 = reinterpret_cast<const uint32_t*>(
              static_cast<const uint8_t*>(res.pData) + y * res.RowPitch);
          uint16_t* pixels16 = reinterpret_cast<uint16_t*>(m_buffer + y * m_stride);

          for (int x = 0; x < m_width; x++, pixels10++, pixels16 += 4)
          {
            // actual bit per channel is A2B10G10R10
            uint32_t pixel = *pixels10;
            pixels16[0] = Expand10To16(pixel & 0x3FF); // R
            pixel >>= 10;
            pixels16[1] = Expand10To16(pixel & 0x3FF); // G
            pixel >>= 10;
            pixels16[2] = Expand10To16(pixel & 0x3FF); // B
            pixels16[3] = 0xFFFF; // A
          }
        }
      }
      else
      {
        m_stride = res.RowPitch;
        m_buffer = new unsigned char[m_height * m_stride];
        memcpy(m_buffer, res.pData, m_height * m_stride);
      }
      pImdContext->Unmap(pCopyTexture.Get(), 0);
    }
    else
      CLog::LogF(LOGERROR, "MAP_READ failed.");
  }

  return m_buffer != nullptr;
}
