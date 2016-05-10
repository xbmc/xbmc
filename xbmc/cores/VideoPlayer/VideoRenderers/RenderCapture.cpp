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

CRenderCaptureBase::~CRenderCaptureBase()
{
}

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
  m_asyncChecked = true;
  m_asyncSupported = true;
}

void CRenderCaptureIMX::EndRender()
{
  if (m_flags & CAPTUREFLAG_IMMEDIATELY)
    ReadOut();
  else
    SetState(CAPTURESTATE_NEEDSREADOUT);
}

void* CRenderCaptureIMX::GetRenderBuffer()
{
    return m_pixels;
}

void CRenderCaptureIMX::ReadOut()
{
  g_IMXContext.WaitCapture();
  m_pixels = reinterpret_cast<uint8_t*>(g_IMXContext.GetCaptureBuffer());
  SetState(CAPTURESTATE_DONE);
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
  m_renderTexture = nullptr;
  m_renderSurface = nullptr;
  m_copySurface   = nullptr;
  m_query         = nullptr;
  m_surfaceWidth  = 0;
  m_surfaceHeight = 0;

  g_Windowing.Register(this);
}

CRenderCaptureDX::~CRenderCaptureDX()
{
  CleanupDX();
  delete[] m_pixels;

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
  CD3D11_QUERY_DESC queryDesc(D3D11_QUERY_OCCLUSION);

  if (!m_asyncChecked)
  {
    m_asyncSupported = S_OK == pDevice->CreateQuery(&queryDesc, nullptr);
    if (m_flags & CAPTUREFLAG_CONTINUOUS)
    {
      if (!m_asyncSupported)
        CLog::Log(LOGWARNING, "CRenderCaptureDX: D3D11_QUERY_OCCLUSION not supported, performance might suffer");
      if (!UseOcclusionQuery())
        CLog::Log(LOGWARNING, "CRenderCaptureDX: D3D11_QUERY_OCCLUSION disabled, performance might suffer");
    }

    m_asyncChecked = true;
  }

  HRESULT result;

  if (m_surfaceWidth != m_width || m_surfaceHeight != m_height)
  {
    if (m_renderSurface)
    {
      while(m_renderSurface->Release() > 0) {}
      m_renderSurface = nullptr;
    }

    if (m_copySurface)
    {
      while (m_copySurface->Release() > 0) {}
      m_copySurface = nullptr;
    }

    CD3D11_TEXTURE2D_DESC texDesc(DXGI_FORMAT_B8G8R8A8_UNORM, m_width, m_height, 1, 1, D3D11_BIND_RENDER_TARGET);
    result = pDevice->CreateTexture2D(&texDesc, nullptr, &m_renderTexture);
    if (S_OK != result)
    {
      CLog::Log(LOGERROR, "CRenderCaptureDX::BeginRender: CreateTexture2D (RENDER_TARGET) failed %s",
                g_Windowing.GetErrorDescription(result).c_str());
      SetState(CAPTURESTATE_FAILED);
      return;
    }

    CD3D11_RENDER_TARGET_VIEW_DESC rtDesc(D3D11_RTV_DIMENSION_TEXTURE2D);
    result = pDevice->CreateRenderTargetView(m_renderTexture, &rtDesc, &m_renderSurface);
    if (S_OK != result)
    {
      CLog::Log(LOGERROR, "CRenderCaptureDX::BeginRender: CreateRenderTargetView failed %s",
        g_Windowing.GetErrorDescription(result).c_str());
      SetState(CAPTURESTATE_FAILED);
      return;
    }

    texDesc.BindFlags = 0;
    texDesc.Usage = D3D11_USAGE_STAGING;
    texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    result = pDevice->CreateTexture2D(&texDesc, nullptr, &m_copySurface);
    if (S_OK != result)
    {
      CLog::Log(LOGERROR, "CRenderCaptureDX::BeginRender: CreateTexture2D (USAGE_STAGING) failed %s",
                g_Windowing.GetErrorDescription(result).c_str());
      SetState(CAPTURESTATE_FAILED);
      return;
    }

    m_surfaceWidth = m_width;
    m_surfaceHeight = m_height;
  }

  if (m_bufferSize != m_width * m_height * 4)
  {
    m_bufferSize = m_width * m_height * 4;
    delete[] m_pixels;
    m_pixels = new uint8_t[m_bufferSize];
  }

  pContext->OMSetRenderTargets(1, &m_renderSurface, nullptr);

  if (m_asyncSupported && UseOcclusionQuery())
  {
    //generate an occlusion query if we don't have one
    if (!m_query)
    {
      result = pDevice->CreateQuery(&queryDesc, &m_query);
      if (S_OK != result)
      {
        CLog::Log(LOGERROR, "CRenderCaptureDX::BeginRender: CreateQuery failed %s",
                  g_Windowing.GetErrorDescription(result).c_str());
        m_asyncSupported = false;
        if (m_query)
        {
          while (m_query->Release() > 0) {}
          m_query = nullptr;
        }
      }
    }
  }
  else
  {
    //don't use an occlusion query, clean up any old one
    if (m_query)
    {
      while (m_query->Release() > 0) {}
      m_query = nullptr;
    }
  }

  if (m_query)
    g_Windowing.GetImmediateContext()->Begin(m_query);
}

void CRenderCaptureDX::EndRender()
{
  g_Windowing.FinishCommandList();
  ID3D11DeviceContext* pContext = g_Windowing.GetImmediateContext();

  pContext->CopyResource(m_copySurface, m_renderTexture);

  if (m_query)
  {
    pContext->End(m_query);
    pContext->GetData(m_query, nullptr, 0, 0); //flush the query request
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
    if (S_OK == result)
    {
      SurfaceToBuffer();
    }
    else if (S_FALSE != result)
    {
      CLog::Log(LOGERROR, "CRenderCaptureDX::ReadOut: GetData failed");
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
  ID3D11DeviceContext* pContext = g_Windowing.GetImmediateContext();

  D3D11_MAPPED_SUBRESOURCE lockedRect;
  if (pContext->Map(m_copySurface, 0, D3D11_MAP_READ, 0, &lockedRect) == S_OK)
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
    pContext->Unmap(m_copySurface, 0);
    SetState(CAPTURESTATE_DONE);
  }
  else
  {
    CLog::Log(LOGERROR, "CRenderCaptureDX::SurfaceToBuffer: locking m_copySurface failed");
    SetState(CAPTURESTATE_FAILED);
  }
}

void CRenderCaptureDX::OnLostDevice()
{
  CleanupDX();
  SetState(CAPTURESTATE_FAILED);
}

void CRenderCaptureDX::OnDestroyDevice()
{
  CleanupDX();
  SetState(CAPTURESTATE_FAILED);
}

void CRenderCaptureDX::CleanupDX()
{
  if (m_renderSurface)
  {
    while (m_renderSurface->Release() > 0) {}
    m_renderSurface = nullptr;
  }

  if (m_renderTexture)
  {
    while (m_renderTexture->Release() > 0) {}
    m_renderTexture = nullptr;
  }

  if (m_copySurface)
  {
    while (m_copySurface->Release() > 0) {}
    m_copySurface = nullptr;
  }

  if (m_query)
  {
    while (m_query->Release() > 0) {}
    m_query = nullptr;
  }

  m_surfaceWidth = 0;
  m_surfaceHeight = 0;
}

#endif /*HAS_DX*/
