/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ScreenshotSurfaceWindows.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "rendering/dx/DeviceResources.h"
#include "threads/SingleLock.h"
#include "utils/Screenshot.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include <wrl/client.h>

using namespace Microsoft::WRL;

void CScreenshotSurfaceWindows::Register()
{
  CScreenShot::Register(CScreenshotSurfaceWindows::CreateSurface);
}

std::unique_ptr<IScreenshotSurface> CScreenshotSurfaceWindows::CreateSurface()
{
  return std::unique_ptr<CScreenshotSurfaceWindows>(new CScreenshotSurfaceWindows());
}

bool CScreenshotSurfaceWindows::Capture()
{
  CWinSystemBase* winsystem = CServiceBroker::GetWinSystem();
  if (!winsystem)
    return false;

  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (!gui)
    return false;

  CSingleLock lock(winsystem->GetGfxContext());
  gui->GetWindowManager().Render();

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
      m_stride = res.RowPitch;
      m_buffer = new unsigned char[m_height * m_stride];
      if (desc.Format == DXGI_FORMAT_R10G10B10A2_UNORM)
      {
        // convert R10G10B10A2 -> B8G8R8A8
        for (int y = 0; y < m_height; y++)
        {
          uint32_t* pixels10 = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(res.pData) + y * res.RowPitch);
          uint8_t* pixels8 = m_buffer + y * m_stride;

          for (int x = 0; x < m_width; x++, pixels10++, pixels8 += 4)
          {
            // actual bit per channel is A2B10G10R10
            uint32_t pixel = *pixels10;
            // R
            pixels8[2] = static_cast<uint8_t>((pixel & 0x3FF) * 255 / 1023);
            // G
            pixel >>= 10;
            pixels8[1] = static_cast<uint8_t>((pixel & 0x3FF) * 255 / 1023);
            // B
            pixel >>= 10;
            pixels8[0] = static_cast<uint8_t>((pixel & 0x3FF) * 255 / 1023);
            // A
            pixels8[3] = 0xFF;
          }
        }
      }
      else
        memcpy(m_buffer, res.pData, m_height * m_stride);
      pImdContext->Unmap(pCopyTexture.Get(), 0);
    }
    else
      CLog::LogF(LOGERROR, "MAP_READ failed.");
  }

  return m_buffer != nullptr;
}
