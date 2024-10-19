/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderCaptureDX.h"

#include "cores/IPlayer.h"
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"
#include "utils/log.h"

#include "platform/win32/WIN32Util.h"

extern "C"
{
#include <libavutil/mem.h>
}

CRenderCaptureDX::CRenderCaptureDX() : CRenderCapture()
{
  DX::Windowing()->Register(this);
}

CRenderCaptureDX::~CRenderCaptureDX()
{
  CleanupDX();
  av_freep(&m_pixels);
  DX::Windowing()->Unregister(this);
}

void CRenderCaptureDX::BeginRender()
{
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> pContext =
      DX::DeviceResources::Get()->GetD3DContext();
  Microsoft::WRL::ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();
  CD3D11_QUERY_DESC queryDesc(D3D11_QUERY_EVENT);

  if (!m_asyncChecked)
  {
    m_asyncSupported = SUCCEEDED(pDevice->CreateQuery(&queryDesc, nullptr));
    if (m_flags & CAPTUREFLAG_CONTINUOUS)
    {
      if (!m_asyncSupported)
        CLog::Log(LOGWARNING, "{}: D3D11_QUERY_OCCLUSION not supported, performance might suffer.",
                  __FUNCTION__);
      if (!UseOcclusionQuery())
        CLog::Log(LOGWARNING, "{}: D3D11_QUERY_OCCLUSION disabled, performance might suffer.",
                  __FUNCTION__);
    }
    m_asyncChecked = true;
  }

  HRESULT result;

  if (m_surfaceWidth != m_width || m_surfaceHeight != m_height)
  {
    m_renderTex.Release();
    m_copyTex.Release();

    if (!m_renderTex.Create(m_width, m_height, 1, D3D11_USAGE_DEFAULT, DXGI_FORMAT_B8G8R8A8_UNORM))
    {
      CLog::LogF(LOGERROR, "CreateTexture2D (RENDER_TARGET) failed.");
      SetState(CAPTURESTATE_FAILED);
      return;
    }

    if (!m_copyTex.Create(m_width, m_height, 1, D3D11_USAGE_STAGING, DXGI_FORMAT_B8G8R8A8_UNORM))
    {
      CLog::LogF(LOGERROR, "CreateRenderTargetView failed.");
      SetState(CAPTURESTATE_FAILED);
      return;
    }

    m_surfaceWidth = m_width;
    m_surfaceHeight = m_height;
  }

  if (m_bufferSize != m_width * m_height * 4)
  {
    m_bufferSize = m_width * m_height * 4;
    av_freep(&m_pixels);
    m_pixels = (uint8_t*)av_malloc(m_bufferSize);
  }

  if (m_asyncSupported && UseOcclusionQuery())
  {
    //generate an occlusion query if we don't have one
    if (!m_query)
    {
      result = pDevice->CreateQuery(&queryDesc, m_query.ReleaseAndGetAddressOf());
      if (FAILED(result))
      {
        CLog::LogF(LOGERROR, "CreateQuery failed {}", CWIN32Util::FormatHRESULT(result));
        m_asyncSupported = false;
        m_query = nullptr;
      }
    }
  }
  else
  {
    //don't use an occlusion query, clean up any old one
    m_query = nullptr;
  }
}

void CRenderCaptureDX::EndRender()
{
  // send commands to the GPU queue
  auto deviceResources = DX::DeviceResources::Get();
  deviceResources->FinishCommandList();
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> pContext = deviceResources->GetImmediateContext();

  pContext->CopyResource(m_copyTex.Get(), m_renderTex.Get());

  if (m_query)
  {
    pContext->End(m_query.Get());
  }

  if (m_flags & CAPTUREFLAG_IMMEDIATELY)
    SurfaceToBuffer();
  else
    SetState(CAPTURESTATE_NEEDSREADOUT);
}

void CRenderCaptureDX::ReadOut()
{
  if (m_query)
  {
    //if the result of the occlusion query is available, the data is probably also written into m_copySurface
    HRESULT result =
        DX::DeviceResources::Get()->GetImmediateContext()->GetData(m_query.Get(), nullptr, 0, 0);
    if (SUCCEEDED(result))
    {
      if (S_OK == result)
        SurfaceToBuffer();
    }
    else
    {
      CLog::Log(LOGERROR, "{}: GetData failed.", __FUNCTION__);
      SurfaceToBuffer();
    }
  }
  else
  {
    SurfaceToBuffer();
  }
}

void CRenderCaptureDX::SurfaceToBuffer()
{
  D3D11_MAPPED_SUBRESOURCE lockedRect;
  if (m_copyTex.LockRect(0, &lockedRect, D3D11_MAP_READ))
  {
    //if pitch is same, do a direct copy, otherwise copy one line at a time
    if (lockedRect.RowPitch == m_width * 4)
    {
      memcpy(m_pixels, lockedRect.pData, m_width * m_height * 4);
    }
    else
    {
      for (unsigned int y = 0; y < m_height; y++)
        memcpy(m_pixels + y * m_width * 4, (uint8_t*)lockedRect.pData + y * lockedRect.RowPitch,
               m_width * 4);
    }
    m_copyTex.UnlockRect(0);
    SetState(CAPTURESTATE_DONE);
  }
  else
  {
    CLog::Log(LOGERROR, "{}: locking m_copySurface failed.", __FUNCTION__);
    SetState(CAPTURESTATE_FAILED);
  }
}

void CRenderCaptureDX::OnDestroyDevice(bool fatal)
{
  CleanupDX();
  SetState(CAPTURESTATE_FAILED);
}

void CRenderCaptureDX::CleanupDX()
{
  m_renderTex.Release();
  m_copyTex.Release();
  m_query = nullptr;
  m_surfaceWidth = 0;
  m_surfaceHeight = 0;
}
