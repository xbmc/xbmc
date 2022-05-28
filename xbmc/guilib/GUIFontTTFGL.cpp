/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIFontTTFGL.h"

#include "GUIFont.h"
#include "GUIFontManager.h"
#include "ServiceBroker.h"
#include "Texture.h"
#include "TextureManager.h"
#include "gui3d.h"
#include "utils/GLUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#ifdef HAS_GL
#include "rendering/gl/RenderSystemGL.h"
#elif HAS_GLES
#include "rendering/gles/RenderSystemGLES.h"
#endif
#include "rendering/MatrixGL.h"

#include <cassert>
#include <memory>

// stuff for freetype
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

namespace
{
constexpr size_t ELEMENT_ARRAY_MAX_CHAR_INDEX = 1000;
} /* namespace */

CGUIFontTTF* CGUIFontTTF::CreateGUIFontTTF(const std::string& fontIdent)
{
  return new CGUIFontTTFGL(fontIdent);
}

CGUIFontTTFGL::CGUIFontTTFGL(const std::string& fontIdent) : CGUIFontTTF(fontIdent)
{
}

CGUIFontTTFGL::~CGUIFontTTFGL(void)
{
  // It's important that all the CGUIFontCacheEntry objects are
  // destructed before the CGUIFontTTFGL goes out of scope, because
  // our virtual methods won't be accessible after this point
  m_dynamicCache.Flush();
  DeleteHardwareTexture();
}

bool CGUIFontTTFGL::FirstBegin()
{
#if defined(HAS_GL)
  GLenum pixformat = GL_RED;
  GLenum internalFormat;
  unsigned int major, minor;
  CRenderSystemGL* renderSystem = dynamic_cast<CRenderSystemGL*>(CServiceBroker::GetRenderSystem());
  renderSystem->GetRenderVersion(major, minor);
  if (major >= 3)
    internalFormat = GL_R8;
  else
    internalFormat = GL_LUMINANCE;
  renderSystem->EnableShader(ShaderMethodGL::SM_FONTS);
#else
  CRenderSystemGLES* renderSystem =
      dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
  renderSystem->EnableGUIShader(ShaderMethodGLES::SM_FONTS);
  GLenum pixformat = GL_ALPHA; // deprecated
  GLenum internalFormat = GL_ALPHA;
#endif

  if (m_textureStatus == TEXTURE_REALLOCATED)
  {
    if (glIsTexture(m_nTexture))
      CServiceBroker::GetGUI()->GetTextureManager().ReleaseHwTexture(m_nTexture);
    m_textureStatus = TEXTURE_VOID;
  }

  if (m_textureStatus == TEXTURE_VOID)
  {
    // Have OpenGL generate a texture object handle for us
    glGenTextures(1, static_cast<GLuint*>(&m_nTexture));

    // Bind the texture object
    glBindTexture(GL_TEXTURE_2D, m_nTexture);

    // Set the texture's stretching properties
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Set the texture image -- THIS WORKS, so the pixels must be wrong.
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_texture->GetWidth(), m_texture->GetHeight(), 0,
                 pixformat, GL_UNSIGNED_BYTE, 0);

    VerifyGLState();
    m_textureStatus = TEXTURE_UPDATED;
  }

  if (m_textureStatus == TEXTURE_UPDATED)
  {
    // Copies one more line in case we have to sample from there
    m_updateY2 = std::min(m_updateY2 + 1, m_texture->GetHeight());

    glBindTexture(GL_TEXTURE_2D, m_nTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, m_updateY1, m_texture->GetWidth(), m_updateY2 - m_updateY1,
                    pixformat, GL_UNSIGNED_BYTE,
                    m_texture->GetPixels() + m_updateY1 * m_texture->GetPitch());

    m_updateY1 = m_updateY2 = 0;
    m_textureStatus = TEXTURE_READY;
  }

  // Turn Blending On
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE);
  glEnable(GL_BLEND);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_nTexture);

  return true;
}

void CGUIFontTTFGL::LastEnd()
{
  CWinSystemBase* const winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
    return;

#ifdef HAS_GL
  CRenderSystemGL* renderSystem = dynamic_cast<CRenderSystemGL*>(CServiceBroker::GetRenderSystem());

  GLint posLoc = renderSystem->ShaderGetPos();
  GLint colLoc = renderSystem->ShaderGetCol();
  GLint tex0Loc = renderSystem->ShaderGetCoord0();
  GLint modelLoc = renderSystem->ShaderGetModel();

  CreateStaticVertexBuffers();

  // Enable the attributes used by this shader
  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(colLoc);
  glEnableVertexAttribArray(tex0Loc);

  if (!m_vertex.empty())
  {

    // Deal with vertices that had to use software clipping
    std::vector<SVertex> vecVertices(6 * (m_vertex.size() / 4));
    SVertex* vertices = &vecVertices[0];
    for (size_t i = 0; i < m_vertex.size(); i += 4)
    {
      *vertices++ = m_vertex[i];
      *vertices++ = m_vertex[i + 1];
      *vertices++ = m_vertex[i + 2];

      *vertices++ = m_vertex[i + 1];
      *vertices++ = m_vertex[i + 3];
      *vertices++ = m_vertex[i + 2];
    }
    vertices = &vecVertices[0];

    GLuint VertexVBO;

    glGenBuffers(1, &VertexVBO);
    glBindBuffer(GL_ARRAY_BUFFER, VertexVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SVertex) * vecVertices.size(), &vecVertices[0],
                 GL_STATIC_DRAW);

    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(SVertex),
                          reinterpret_cast<const GLvoid*>(offsetof(SVertex, x)));
    glVertexAttribPointer(colLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(SVertex),
                          reinterpret_cast<const GLvoid*>(offsetof(SVertex, r)));
    glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex),
                          reinterpret_cast<const GLvoid*>(offsetof(SVertex, u)));

    glDrawArrays(GL_TRIANGLES, 0, vecVertices.size());

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &VertexVBO);
  }

#else
  // GLES 2.0 version.
  CRenderSystemGLES* renderSystem =
      dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());

  GLint posLoc = renderSystem->GUIShaderGetPos();
  GLint colLoc = renderSystem->GUIShaderGetCol();
  GLint tex0Loc = renderSystem->GUIShaderGetCoord0();
  GLint modelLoc = renderSystem->GUIShaderGetModel();


  CreateStaticVertexBuffers();

  // Enable the attributes used by this shader
  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(colLoc);
  glEnableVertexAttribArray(tex0Loc);

  if (!m_vertex.empty())
  {
    // Deal with vertices that had to use software clipping
    std::vector<SVertex> vecVertices(6 * (m_vertex.size() / 4));
    SVertex* vertices = &vecVertices[0];

    for (size_t i = 0; i < m_vertex.size(); i += 4)
    {
      *vertices++ = m_vertex[i];
      *vertices++ = m_vertex[i + 1];
      *vertices++ = m_vertex[i + 2];

      *vertices++ = m_vertex[i + 1];
      *vertices++ = m_vertex[i + 3];
      *vertices++ = m_vertex[i + 2];
    }

    vertices = &vecVertices[0];

    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(SVertex),
                          reinterpret_cast<char*>(vertices) + offsetof(SVertex, x));
    glVertexAttribPointer(colLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(SVertex),
                          reinterpret_cast<char*>(vertices) + offsetof(SVertex, r));
    glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex),
                          reinterpret_cast<char*>(vertices) + offsetof(SVertex, u));

    glDrawArrays(GL_TRIANGLES, 0, vecVertices.size());
  }
#endif

  if (!m_vertexTrans.empty())
  {
    // Deal with the vertices that can be hardware clipped and therefore translated

    // Bind our pre-calculated array to GL_ELEMENT_ARRAY_BUFFER
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementArrayHandle);
    // Store current scissor
    CGraphicContext& context = winSystem->GetGfxContext();
    CRect scissor = context.StereoCorrection(context.GetScissors());

    for (size_t i = 0; i < m_vertexTrans.size(); i++)
    {
      if (m_vertexTrans[i].m_vertexBuffer->bufferHandle == 0)
      {
        continue;
      }

      // Apply the clip rectangle
      CRect clip = renderSystem->ClipRectToScissorRect(m_vertexTrans[i].m_clip);
      if (!clip.IsEmpty())
      {
        // intersect with current scissor
        clip.Intersect(scissor);
        // skip empty clip
        if (clip.IsEmpty())
          continue;
        renderSystem->SetScissors(clip);
      }

      // Apply the translation to the currently active (top-of-stack) model view matrix
      glMatrixModview.Push();
      glMatrixModview.Get().Translatef(m_vertexTrans[i].m_translateX, m_vertexTrans[i].m_translateY,
                                       m_vertexTrans[i].m_translateZ);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glMatrixModview.Get());

      // Bind the buffer to the OpenGL context's GL_ARRAY_BUFFER binding point
      glBindBuffer(GL_ARRAY_BUFFER, m_vertexTrans[i].m_vertexBuffer->bufferHandle);

      // Do the actual drawing operation, split into groups of characters no
      // larger than the pre-determined size of the element array
      for (size_t character = 0; m_vertexTrans[i].m_vertexBuffer->size > character;
           character += ELEMENT_ARRAY_MAX_CHAR_INDEX)
      {
        size_t count = m_vertexTrans[i].m_vertexBuffer->size - character;
        count = std::min<size_t>(count, ELEMENT_ARRAY_MAX_CHAR_INDEX);

        // Set up the offsets of the various vertex attributes within the buffer
        // object bound to GL_ARRAY_BUFFER
        glVertexAttribPointer(
            posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(SVertex),
            reinterpret_cast<GLvoid*>(character * sizeof(SVertex) * 4 + offsetof(SVertex, x)));
        glVertexAttribPointer(
            colLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(SVertex),
            reinterpret_cast<GLvoid*>(character * sizeof(SVertex) * 4 + offsetof(SVertex, r)));
        glVertexAttribPointer(
            tex0Loc, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex),
            reinterpret_cast<GLvoid*>(character * sizeof(SVertex) * 4 + offsetof(SVertex, u)));

        glDrawElements(GL_TRIANGLES, 6 * count, GL_UNSIGNED_SHORT, 0);
      }

      glMatrixModview.Pop();
    }
    // Restore the original scissor rectangle
    renderSystem->SetScissors(scissor);
    // Restore the original model view matrix
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glMatrixModview.Get());
    // Unbind GL_ARRAY_BUFFER and GL_ELEMENT_ARRAY_BUFFER
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  // Disable the attributes used by this shader
  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(colLoc);
  glDisableVertexAttribArray(tex0Loc);

#ifdef HAS_GL
  renderSystem->DisableShader();
#else
  renderSystem->DisableGUIShader();
#endif
}

CVertexBuffer CGUIFontTTFGL::CreateVertexBuffer(const std::vector<SVertex>& vertices) const
{
  assert(vertices.size() % 4 == 0);
  GLuint bufferHandle = 0;

  // Do not create empty buffers, leave buffer as 0, it will be ignored in drawing stage
  if (!vertices.empty())
  {
    // Generate a unique buffer object name and put it in bufferHandle
    glGenBuffers(1, &bufferHandle);
    // Bind the buffer to the OpenGL context's GL_ARRAY_BUFFER binding point
    glBindBuffer(GL_ARRAY_BUFFER, bufferHandle);
    // Create a data store for the buffer object bound to the GL_ARRAY_BUFFER
    // binding point (i.e. our buffer object) and initialise it from the
    // specified client-side pointer
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(SVertex), vertices.data(),
                 GL_STATIC_DRAW);
    // Unbind GL_ARRAY_BUFFER
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  return CVertexBuffer(bufferHandle, vertices.size() / 4, this);
}

void CGUIFontTTFGL::DestroyVertexBuffer(CVertexBuffer& buffer) const
{
  if (buffer.bufferHandle != 0)
  {
    // Release the buffer name for reuse
    glDeleteBuffers(1, static_cast<GLuint*>(&buffer.bufferHandle));
    buffer.bufferHandle = 0;
  }
}

std::unique_ptr<CTexture> CGUIFontTTFGL::ReallocTexture(unsigned int& newHeight)
{
  newHeight = CTexture::PadPow2(newHeight);

  std::unique_ptr<CTexture> newTexture =
      CTexture::CreateTexture(m_textureWidth, newHeight, XB_FMT_A8);

  if (!newTexture || !newTexture->GetPixels())
  {
    CLog::Log(LOGERROR, "GUIFontTTFGL::{}: Error creating new cache texture for size {:f}",
              __func__, m_height);
    return nullptr;
  }

  m_textureHeight = newTexture->GetHeight();
  m_textureScaleY = 1.0f / m_textureHeight;
  m_textureWidth = newTexture->GetWidth();
  m_textureScaleX = 1.0f / m_textureWidth;
  if (m_textureHeight < newHeight)
    CLog::Log(LOGWARNING, "GUIFontTTFGL::{}: allocated new texture with height of {}, requested {}",
              __func__, m_textureHeight, newHeight);
  m_staticCache.Flush();
  m_dynamicCache.Flush();

  memset(newTexture->GetPixels(), 0, m_textureHeight * newTexture->GetPitch());
  if (m_texture)
  {
    m_updateY1 = 0;
    m_updateY2 = m_texture->GetHeight();

    unsigned char* src = m_texture->GetPixels();
    unsigned char* dst = newTexture->GetPixels();
    for (unsigned int y = 0; y < m_texture->GetHeight(); y++)
    {
      memcpy(dst, src, m_texture->GetPitch());
      src += m_texture->GetPitch();
      dst += newTexture->GetPitch();
    }
  }

  m_textureStatus = TEXTURE_REALLOCATED;

  return newTexture;
}

bool CGUIFontTTFGL::CopyCharToTexture(
    FT_BitmapGlyph bitGlyph, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
{
  FT_Bitmap bitmap = bitGlyph->bitmap;

  unsigned char* source = bitmap.buffer;
  unsigned char* target = m_texture->GetPixels() + y1 * m_texture->GetPitch() + x1;

  for (unsigned int y = y1; y < y2; y++)
  {
    memcpy(target, source, x2 - x1);
    source += bitmap.width;
    target += m_texture->GetPitch();
  }

  switch (m_textureStatus)
  {
    case TEXTURE_UPDATED:
    {
      m_updateY1 = std::min(m_updateY1, y1);
      m_updateY2 = std::max(m_updateY2, y2);
    }
    break;

    case TEXTURE_READY:
    {
      m_updateY1 = y1;
      m_updateY2 = y2;
      m_textureStatus = TEXTURE_UPDATED;
    }
    break;

    case TEXTURE_REALLOCATED:
    {
      m_updateY2 = std::max(m_updateY2, y2);
    }
    break;

    case TEXTURE_VOID:
    default:
      break;
  }

  return true;
}

void CGUIFontTTFGL::DeleteHardwareTexture()
{
  if (m_textureStatus != TEXTURE_VOID)
  {
    if (glIsTexture(m_nTexture))
      CServiceBroker::GetGUI()->GetTextureManager().ReleaseHwTexture(m_nTexture);

    m_textureStatus = TEXTURE_VOID;
    m_updateY1 = m_updateY2 = 0;
  }
}

void CGUIFontTTFGL::CreateStaticVertexBuffers(void)
{
  if (m_staticVertexBufferCreated)
    return;

  // Bind a new buffer to the OpenGL context's GL_ELEMENT_ARRAY_BUFFER binding point
  glGenBuffers(1, &m_elementArrayHandle);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementArrayHandle);

  // Create an array holding the mesh indices to convert quads to triangles
  GLushort index[ELEMENT_ARRAY_MAX_CHAR_INDEX][6];
  for (size_t i = 0; i < ELEMENT_ARRAY_MAX_CHAR_INDEX; i++)
  {
    index[i][0] = 4 * i;
    index[i][1] = 4 * i + 1;
    index[i][2] = 4 * i + 2;
    index[i][3] = 4 * i + 1;
    index[i][4] = 4 * i + 3;
    index[i][5] = 4 * i + 2;
  }

  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof index, index, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  m_staticVertexBufferCreated = true;
}

void CGUIFontTTFGL::DestroyStaticVertexBuffers(void)
{
  if (!m_staticVertexBufferCreated)
    return;

  glDeleteBuffers(1, &m_elementArrayHandle);
  m_staticVertexBufferCreated = false;
}

GLuint CGUIFontTTFGL::m_elementArrayHandle{0};
bool CGUIFontTTFGL::m_staticVertexBufferCreated{false};
