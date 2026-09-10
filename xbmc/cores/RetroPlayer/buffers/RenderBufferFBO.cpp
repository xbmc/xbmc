/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RenderBufferFBO.h"

#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

CRenderBufferFBO::CRenderBufferFBO(
    CRenderContext& context, bool depth, bool stencil, bool bottomLeftOrigin, Type type)
  : m_context(context),
    m_depth(depth),
    m_stencil(stencil),
    m_bottomLeftOrigin(type == Type::CLIENT && bottomLeftOrigin),
    m_type(type)
{
}

CRenderBufferFBO::~CRenderBufferFBO() = default;

void CRenderBufferFBO::Resources::Destroy()
{
  std::unique_lock lock(mutex);
  if (rendered)
  {
    glWaitSync(rendered, 0, GL_TIMEOUT_IGNORED);
    glDeleteSync(rendered);
    rendered = nullptr;
  }
  if (ready)
  {
    glDeleteSync(ready);
    ready = nullptr;
  }
  glDeleteFramebuffers(1, &framebuffer);
  glDeleteTextures(1, &texture);
  glDeleteRenderbuffers(1, &depthStencil);
  framebuffer = texture = depthStencil = 0;
}

bool CRenderBufferFBO::Allocate(AVPixelFormat format, unsigned int width, unsigned int height)
{
  GLint maxTextureSize = 0;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
  if (width == 0 || height == 0 || maxTextureSize <= 0 ||
      width > static_cast<unsigned int>(maxTextureSize) ||
      height > static_cast<unsigned int>(maxTextureSize))
    return false;

  GLint readFramebuffer, drawFramebuffer, texture, renderbuffer, unpackBuffer;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFramebuffer);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
  glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbuffer);
  glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpackBuffer);

  GLuint newTexture = 0, newDepthStencil = 0, newFramebuffer = 0;
  glGenTextures(1, &newTexture);
  glBindTexture(GL_TEXTURE_2D, newTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  // RGB capture alpha is one for both texture sampling and framebuffer blits.
  glTexImage2D(GL_TEXTURE_2D, 0, IsCapture() ? GL_RGB8 : GL_RGBA8, width, height, 0,
               IsCapture() ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

  if (m_depth)
  {
    glGenRenderbuffers(1, &newDepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, newDepthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, m_stencil ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT24,
                          width, height);
  }

  const auto attach = [&]
  {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, newTexture, 0);
    if (m_depth)
    {
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                newDepthStencil);
      if (m_stencil)
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                  newDepthStencil);
    }
  };
  glGenFramebuffers(1, &newFramebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, newFramebuffer);
  attach();
  const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  const bool complete = status == GL_FRAMEBUFFER_COMPLETE && newTexture != 0 &&
                        newFramebuffer != 0 && (!m_depth || newDepthStencil != 0);
  if (complete)
  {
    // Validate replacement attachments before changing the framebuffer name
    // that a core may have cached during context reset.
    if (m_resources->framebuffer != 0)
    {
      glBindFramebuffer(GL_FRAMEBUFFER, m_resources->framebuffer);
      attach();
      glDeleteFramebuffers(1, &newFramebuffer);
    }
    else
      m_resources->framebuffer = newFramebuffer;

    if (m_resources->texture != 0 && texture == static_cast<GLint>(m_resources->texture))
      texture = newTexture;
    if (m_resources->depthStencil != 0 &&
        renderbuffer == static_cast<GLint>(m_resources->depthStencil))
      renderbuffer = newDepthStencil;
    glDeleteTextures(1, &m_resources->texture);
    glDeleteRenderbuffers(1, &m_resources->depthStencil);
    m_resources->texture = newTexture;
    m_resources->depthStencil = newDepthStencil;
    m_format = format;
    m_width = m_textureWidth = width;
    m_height = m_textureHeight = height;
  }
  else
  {
    glDeleteFramebuffers(1, &newFramebuffer);
    glDeleteTextures(1, &newTexture);
    glDeleteRenderbuffers(1, &newDepthStencil);
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Framebuffer is incomplete, status {:#x}", status);
  }

  glBindFramebuffer(GL_READ_FRAMEBUFFER, readFramebuffer);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFramebuffer);
  glBindTexture(GL_TEXTURE_2D, texture);
  glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpackBuffer);
  return complete;
}

uintptr_t CRenderBufferFBO::GetCurrentFramebuffer()
{
  return m_resources->framebuffer;
}

void CRenderBufferFBO::PrepareForCapture()
{
  if (m_resources->rendered)
  {
    glWaitSync(m_resources->rendered, 0, GL_TIMEOUT_IGNORED);
    glDeleteSync(m_resources->rendered);
    m_resources->rendered = nullptr;
  }
  if (m_resources->ready)
  {
    glDeleteSync(m_resources->ready);
    m_resources->ready = nullptr;
  }
}

bool CRenderBufferFBO::SetReady()
{
  m_resources->ready = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  glFlush();
  return m_resources->ready != nullptr;
}

void CRenderBufferFBO::WaitForCapture()
{
  if (m_resources->ready)
    glWaitSync(m_resources->ready, 0, GL_TIMEOUT_IGNORED);
}

void CRenderBufferFBO::FinishRender()
{
  if (m_resources->rendered)
    glDeleteSync(m_resources->rendered);
  m_resources->rendered = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  if (!m_resources->rendered)
    m_resources->retired = true;
  glFlush();
}
