/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderCapture.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "cores/IPlayer.h"
#ifdef TARGET_WINDOWS
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"
#else
#include "rendering/RenderSystem.h"
#endif

extern "C" {
#include <libavutil/mem.h>
}

CRenderCaptureBase::CRenderCaptureBase()
{
  m_state          = CAPTURESTATE_FAILED;
  m_userState      = CAPTURESTATE_FAILED;
  m_pixels         = NULL;
  m_width          = 0;
  m_height         = 0;
  m_bufferSize     = 0;
  m_flags          = 0;
  m_asyncSupported = false;
  m_asyncChecked   = false;
}

CRenderCaptureBase::~CRenderCaptureBase() = default;

bool CRenderCaptureBase::UseOcclusionQuery()
{
  if (m_flags & CAPTUREFLAG_IMMEDIATELY)
    return false;
  else if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoCaptureUseOcclusionQuery == 0)
    return false;
  else
    return true;
}

#if defined(TARGET_RASPBERRY_PI)

CRenderCaptureDispmanX::CRenderCaptureDispmanX()
{
  m_pixels = nullptr;
}

CRenderCaptureDispmanX::~CRenderCaptureDispmanX()
{
  delete[] m_pixels;
}

int CRenderCaptureDispmanX::GetCaptureFormat()
{
  return CAPTUREFORMAT_BGRA;
}

void CRenderCaptureDispmanX::BeginRender()
{
}

void CRenderCaptureDispmanX::EndRender()
{
  delete[] m_pixels;
  m_pixels = g_RBP.CaptureDisplay(m_width, m_height, NULL, true);

  SetState(CAPTURESTATE_DONE);
}

void* CRenderCaptureDispmanX::GetRenderBuffer()
{
  return m_pixels;
}

void CRenderCaptureDispmanX::ReadOut()
{
}

#elif defined(HAS_GL) || defined(HAS_GLES)

CRenderCaptureGL::CRenderCaptureGL()
{
  m_pbo   = 0;
  m_query = 0;
  m_occlusionQuerySupported = false;
}

CRenderCaptureGL::~CRenderCaptureGL()
{
#ifndef HAS_GLES
  if (m_asyncSupported)
  {
    if (m_pbo)
    {
      glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo);
      glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
      glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
      glDeleteBuffers(1, &m_pbo);
    }

    if (m_query)
      glDeleteQueries(1, &m_query);
  }
#endif

  delete[] m_pixels;
}

int CRenderCaptureGL::GetCaptureFormat()
{
  return CAPTUREFORMAT_BGRA;
}

void CRenderCaptureGL::BeginRender()
{
  if (!m_asyncChecked)
  {
#ifndef HAS_GLES
    unsigned int major, minor, glversion;
    CServiceBroker::GetRenderSystem()->GetRenderVersion(major, minor);
    glversion = 10 * major + minor;
    if (glversion >= 21)
    {
      m_asyncSupported = true;
      m_occlusionQuerySupported = true;
    }
    else if (glversion > 14)
    {
      m_occlusionQuerySupported = true;
    }
    else
    {
      CLog::Log(LOGWARNING, "CRenderCaptureGL: Occlusion_query not supported, upgrade your GL drivers to support at least GL 2.1");
    }
    if (m_flags & CAPTUREFLAG_CONTINUOUS)
    {
      if (!m_occlusionQuerySupported)
        CLog::Log(LOGWARNING, "CRenderCaptureGL: Occlusion_query not supported, performance might suffer");
      if (!CServiceBroker::GetRenderSystem()->IsExtSupported("GL_ARB_pixel_buffer_object"))
        CLog::Log(LOGWARNING, "CRenderCaptureGL: GL_ARB_pixel_buffer_object not supported, performance might suffer");
      if (!UseOcclusionQuery())
        CLog::Log(LOGWARNING, "CRenderCaptureGL: GL_ARB_occlusion_query disabled, performance might suffer");
    }
#endif
    m_asyncChecked = true;
  }

#ifndef HAS_GLES
  if (m_asyncSupported)
  {
    if (!m_pbo)
      glGenBuffers(1, &m_pbo);

    if (UseOcclusionQuery() && m_occlusionQuerySupported)
    {
      //generate an occlusion query if we don't have one
      if (!m_query)
        glGenQueries(1, &m_query);
    }
    else
    {
      //don't use an occlusion query, clean up any old one
      if (m_query)
      {
        glDeleteQueries(1, &m_query);
        m_query = 0;
      }
    }

    //start the occlusion query
    if (m_query)
      glBeginQuery(GL_SAMPLES_PASSED, m_query);

    //allocate data on the pbo and pixel buffer
    glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo);
    if (m_bufferSize != m_width * m_height * 4)
    {
      m_bufferSize = m_width * m_height * 4;
      glBufferData(GL_PIXEL_PACK_BUFFER, m_bufferSize, 0, GL_STREAM_READ);
      delete[] m_pixels;
      m_pixels = new uint8_t[m_bufferSize];
    }
  }
  else
#endif
  {
    if (m_bufferSize != m_width * m_height * 4)
    {
      delete[] m_pixels;
      m_bufferSize = m_width * m_height * 4;
      m_pixels = new uint8_t[m_bufferSize];
    }
  }
}

void CRenderCaptureGL::EndRender()
{
#ifndef HAS_GLES
  if (m_asyncSupported)
  {
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    if (m_query)
      glEndQuery(GL_SAMPLES_PASSED);

    if (m_flags & CAPTUREFLAG_IMMEDIATELY)
      PboToBuffer();
    else
      SetState(CAPTURESTATE_NEEDSREADOUT);
  }
  else
#endif
  {
    SetState(CAPTURESTATE_DONE);
  }
}

void* CRenderCaptureGL::GetRenderBuffer()
{
#ifndef HAS_GLES
  if (m_asyncSupported)
  {
    return NULL; //offset into the pbo
  }
  else
#endif
  {
    return m_pixels;
  }
}

void CRenderCaptureGL::ReadOut()
{
#ifndef HAS_GLES
  if (m_asyncSupported)
  {
    //we don't care about the occlusion query, we just want to know if the result is available
    //when it is, the write into the pbo is probably done as well,
    //so it can be mapped and read without a busy wait

    GLuint readout = 1;
    if (m_query)
      glGetQueryObjectuiv(m_query, GL_QUERY_RESULT_AVAILABLE, &readout);

    if (readout)
      PboToBuffer();
  }
#endif
}

void CRenderCaptureGL::PboToBuffer()
{
#ifndef HAS_GLES
  glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo);
  GLvoid* pboPtr = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

  if (pboPtr)
  {
    memcpy(m_pixels, pboPtr, m_bufferSize);
    SetState(CAPTURESTATE_DONE);
  }
  else
  {
    CLog::Log(LOGERROR, "CRenderCaptureGL::PboToBuffer: glMapBuffer failed");
    SetState(CAPTURESTATE_FAILED);
  }

  glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
#endif
}

#elif HAS_DX /*HAS_GL*/

CRenderCaptureDX::CRenderCaptureDX()
{
  m_query         = nullptr;
  m_surfaceWidth  = 0;
  m_surfaceHeight = 0;
  DX::Windowing()->Register(this);
}

CRenderCaptureDX::~CRenderCaptureDX()
{
  CleanupDX();
  av_freep(&m_pixels);
  DX::Windowing()->Unregister(this);
}

int CRenderCaptureDX::GetCaptureFormat()
{
  return CAPTUREFORMAT_BGRA;
}

void CRenderCaptureDX::BeginRender()
{
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetD3DContext();
  Microsoft::WRL::ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();
  CD3D11_QUERY_DESC queryDesc(D3D11_QUERY_EVENT);

  if (!m_asyncChecked)
  {
    m_asyncSupported = SUCCEEDED(pDevice->CreateQuery(&queryDesc, nullptr));
    if (m_flags & CAPTUREFLAG_CONTINUOUS)
    {
      if (!m_asyncSupported)
        CLog::Log(LOGWARNING, "%s: D3D11_QUERY_OCCLUSION not supported, performance might suffer.", __FUNCTION__);
      if (!UseOcclusionQuery())
        CLog::Log(LOGWARNING, "%s: D3D11_QUERY_OCCLUSION disabled, performance might suffer.", __FUNCTION__);
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
        CLog::LogF(LOGERROR, "CreateQuery failed %s",
                            DX::GetErrorDescription(result).c_str());
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
    HRESULT result = DX::DeviceResources::Get()->GetImmediateContext()->GetData(m_query.Get(), nullptr, 0, 0);
    if (SUCCEEDED(result))
    {
      if (S_OK == result)
        SurfaceToBuffer();
    }
    else
    {
      CLog::Log(LOGERROR, "%s: GetData failed.", __FUNCTION__);
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
        memcpy(m_pixels + y * m_width * 4, (uint8_t*)lockedRect.pData + y * lockedRect.RowPitch, m_width * 4);
    }
    m_copyTex.UnlockRect(0);
    SetState(CAPTURESTATE_DONE);
  }
  else
  {
    CLog::Log(LOGERROR, "%s: locking m_copySurface failed.", __FUNCTION__);
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

#endif /*HAS_DX*/
