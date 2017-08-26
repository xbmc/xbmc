/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "RenderCapture.h"
#include "utils/log.h"
#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"
#include "cores/IPlayer.h"
#ifdef HAS_DX
#include "rendering/dx/DirectXHelper.h"
#endif

extern "C" {
#include "libavutil/mem.h"
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
  else if ((g_advancedSettings.m_videoCaptureUseOcclusionQuery == 0) ||
           (g_advancedSettings.m_videoCaptureUseOcclusionQuery == -1 &&
            g_Windowing.GetRenderQuirks() & RENDER_QUIRKS_BROKEN_OCCLUSION_QUERY))
    return false;
  else
    return true;
}


#if defined(HAS_IMXVPU)
CRenderCaptureIMX::CRenderCaptureIMX()
{
}

CRenderCaptureIMX::~CRenderCaptureIMX()
{
}

int CRenderCaptureIMX::GetCaptureFormat()
{
  return CAPTUREFORMAT_BGRA;
}

void CRenderCaptureIMX::BeginRender()
{
}

void CRenderCaptureIMX::EndRender()
{
  if (g_IMXContext.CaptureDisplay(m_pixels, m_width, m_height))
    SetState(CAPTURESTATE_DONE);
  else
    SetState(CAPTURESTATE_FAILED);
}

void* CRenderCaptureIMX::GetRenderBuffer()
{
  return m_pixels;
}

void CRenderCaptureIMX::ReadOut()
{
}

#elif defined(TARGET_RASPBERRY_PI)

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
      glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, m_pbo);
      glUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
      glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
      glDeleteBuffersARB(1, &m_pbo);
    }

    if (m_query)
      glDeleteQueriesARB(1, &m_query);
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
    m_asyncSupported = g_Windowing.IsExtSupported("GL_ARB_pixel_buffer_object");
    m_occlusionQuerySupported = g_Windowing.IsExtSupported("GL_ARB_occlusion_query");

    if (m_flags & CAPTUREFLAG_CONTINUOUS)
    {
      if (!m_occlusionQuerySupported)
        CLog::Log(LOGWARNING, "CRenderCaptureGL: GL_ARB_occlusion_query not supported, performance might suffer");
      if (!g_Windowing.IsExtSupported("GL_ARB_pixel_buffer_object"))
        CLog::Log(LOGWARNING, "CRenderCaptureGL: GL_ARB_pixel_buffer_object not supported, performance might suffer");
      if (UseOcclusionQuery())
        CLog::Log(LOGWARNING, "CRenderCaptureGL: GL_ARB_occlusion_query disabled, performance might suffer");
    }
#endif
    m_asyncChecked = true;
  }

#ifndef HAS_GLES
  if (m_asyncSupported)
  {
    if (!m_pbo)
      glGenBuffersARB(1, &m_pbo);

    if (UseOcclusionQuery() && m_occlusionQuerySupported)
    {
      //generate an occlusion query if we don't have one
      if (!m_query)
        glGenQueriesARB(1, &m_query);
    }
    else
    {
      //don't use an occlusion query, clean up any old one
      if (m_query)
      {
        glDeleteQueriesARB(1, &m_query);
        m_query = 0;
      }
    }

    //start the occlusion query
    if (m_query)
      glBeginQueryARB(GL_SAMPLES_PASSED_ARB, m_query);

    //allocate data on the pbo and pixel buffer
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, m_pbo);
    if (m_bufferSize != m_width * m_height * 4)
    {
      m_bufferSize = m_width * m_height * 4;
      glBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, m_bufferSize, 0, GL_STREAM_READ_ARB);
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
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);

    if (m_query)
      glEndQueryARB(GL_SAMPLES_PASSED_ARB);

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
      glGetQueryObjectuivARB(m_query, GL_QUERY_RESULT_AVAILABLE_ARB, &readout);

    if (readout)
      PboToBuffer();
  }
#endif
}

void CRenderCaptureGL::PboToBuffer()
{
#ifndef HAS_GLES
  glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, m_pbo);
  GLvoid* pboPtr = glMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);

  if (pboPtr)
  {
    memcpy(m_pixels, pboPtr, m_bufferSize);
    SetState(CAPTURESTATE_DONE);
  }
  else
  {
    CLog::Log(LOGERROR, "CRenderCaptureGL::PboToBuffer: glMapBufferARB failed");
    SetState(CAPTURESTATE_FAILED);
  }

  glUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
  glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
#endif
}

#elif HAS_DX /*HAS_GL*/

CRenderCaptureDX::CRenderCaptureDX()
{
  m_query         = nullptr;
  m_surfaceWidth  = 0;
  m_surfaceHeight = 0;
  g_Windowing.Register(this);
}

CRenderCaptureDX::~CRenderCaptureDX()
{
  CleanupDX();
  av_freep(&m_pixels);
  g_Windowing.Unregister(this);
}

int CRenderCaptureDX::GetCaptureFormat()
{
  return CAPTUREFORMAT_BGRA;
}

void CRenderCaptureDX::BeginRender()
{
  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  ID3D11Device* pDevice = g_Windowing.Get3D11Device();
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
      result = pDevice->CreateQuery(&queryDesc, &m_query);
      if (FAILED(result))
      {
        CLog::LogF(LOGERROR, "CreateQuery failed %s",
                            DX::GetErrorDescription(result).c_str());
        m_asyncSupported = false;
        SAFE_RELEASE(m_query);
      }
    }
  }
  else
  {
    //don't use an occlusion query, clean up any old one
    SAFE_RELEASE(m_query);
  }
}

void CRenderCaptureDX::EndRender()
{
  // send commands to the GPU queue
  auto deviceResources = DX::DeviceResources::Get();
  deviceResources->FinishCommandList();
  ID3D11DeviceContext* pContext = deviceResources->GetImmediateContext();

  pContext->CopyResource(m_copyTex.Get(), m_renderTex.Get());

  if (m_query)
  {
    pContext->End(m_query);
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
    HRESULT result = g_Windowing.GetImmediateContext()->GetData(m_query, nullptr, 0, 0);
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

void CRenderCaptureDX::OnLostDevice()
{
  CleanupDX();
  SetState(CAPTURESTATE_FAILED);
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
  SAFE_RELEASE(m_query);
  m_surfaceWidth = 0;
  m_surfaceHeight = 0;
}

#endif /*HAS_DX*/
