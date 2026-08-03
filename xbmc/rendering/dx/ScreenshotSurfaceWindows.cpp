/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ScreenshotSurfaceWindows.h"

#include "rendering/dx/DeviceResources.h"
#include "utils/Screenshot.h"
#include "utils/log.h"

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

bool CScreenshotSurfaceWindows::Read(const ScreenshotContext&)
{
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
      // no CPU unpack: swscale on the consumer expands the packed 10-bit; D3D
      // is top-left origin so the rows are already top-down
      m_stride = static_cast<int>(res.RowPitch);
      m_format =
          (desc.Format == DXGI_FORMAT_R10G10B10A2_UNORM) ? AV_PIX_FMT_X2BGR10LE : AV_PIX_FMT_BGRA;
      m_buffer = new unsigned char[static_cast<size_t>(m_height) * m_stride];
      memcpy(m_buffer, res.pData, static_cast<size_t>(m_height) * m_stride);
      pImdContext->Unmap(pCopyTexture.Get(), 0);
    }
    else
      CLog::LogF(LOGERROR, "MAP_READ failed.");
  }

  return m_buffer != nullptr;
}
