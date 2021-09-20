/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderCaptureGL.h"

#include "ServiceBroker.h"
#include "cores/IPlayer.h"
#include "rendering/RenderSystem.h"
#include "utils/log.h"

CRenderCaptureGL::~CRenderCaptureGL()
{
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

  delete[] m_pixels;
}

void CRenderCaptureGL::BeginRender()
{
  if (!m_asyncChecked)
  {
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
      CLog::Log(LOGWARNING, "CRenderCaptureGL: Occlusion_query not supported, upgrade your GL "
                            "drivers to support at least GL 2.1");
    }
    if (m_flags & CAPTUREFLAG_CONTINUOUS)
    {
      if (!m_occlusionQuerySupported)
        CLog::Log(LOGWARNING,
                  "CRenderCaptureGL: Occlusion_query not supported, performance might suffer");
      if (!CServiceBroker::GetRenderSystem()->IsExtSupported("GL_ARB_pixel_buffer_object"))
        CLog::Log(
            LOGWARNING,
            "CRenderCaptureGL: GL_ARB_pixel_buffer_object not supported, performance might suffer");
      if (!UseOcclusionQuery())
        CLog::Log(LOGWARNING,
                  "CRenderCaptureGL: GL_ARB_occlusion_query disabled, performance might suffer");
    }

    m_asyncChecked = true;
  }

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
  {
    SetState(CAPTURESTATE_DONE);
  }
}

void* CRenderCaptureGL::GetRenderBuffer()
{
  if (m_asyncSupported)
  {
    return nullptr; //offset into the pbo
  }
  else
  {
    return m_pixels;
  }
}

void CRenderCaptureGL::ReadOut()
{
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
}

void CRenderCaptureGL::PboToBuffer()
{
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
}
