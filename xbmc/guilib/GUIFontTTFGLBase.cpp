/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIFontTTFGLBase.h"

#include "GUIFont.h"
#include "GUIFontManager.h"
#include "ServiceBroker.h"
#include "Texture.h"
#include "TextureManager.h"
#include "gui3d.h"
#include "rendering/MatrixGL.h"
#include "utils/GLUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include <cassert>

// stuff for freetype
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

namespace
{
constexpr int ELEMENT_ARRAY_MAX_CHAR_INDEX{1000};
}

CGUIFontTTFGLBase::CGUIFontTTFGLBase(const std::string& strFileName) : CGUIFontTTFBase(strFileName)
{
  m_updateY1 = 0;
  m_updateY2 = 0;
  m_textureStatus = TEXTURE_VOID;
}

CGUIFontTTFGLBase::~CGUIFontTTFGLBase()
{
  // It's important that all the CGUIFontCacheEntry objects are
  // destructed before the CGUIFontTTFGLBase goes out of scope, because
  // our virtual methods won't be accessible after this point
  m_dynamicCache.Flush();
  DeleteHardwareTexture();
}

CVertexBuffer CGUIFontTTFGLBase::CreateVertexBuffer(const std::vector<SVertex>& vertices) const
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

void CGUIFontTTFGLBase::DestroyVertexBuffer(CVertexBuffer& buffer) const
{
  if (buffer.bufferHandle != 0)
  {
    // Release the buffer name for reuse
    glDeleteBuffers(1, (GLuint*)&buffer.bufferHandle);
    buffer.bufferHandle = 0;
  }
}

CTexture* CGUIFontTTFGLBase::ReallocTexture(unsigned int& newHeight)
{
  newHeight = CTexture::PadPow2(newHeight);

  CTexture* newTexture = CTexture::GetTexture(m_textureWidth, newHeight, XB_FMT_A8);

  if (!newTexture || newTexture->GetPixels() == NULL)
  {
    CLog::Log(LOGERROR,
              "GUIFontTTFGL::CacheCharacter: Error creating new cache texture for size %f",
              m_height);
    delete newTexture;
    return NULL;
  }
  m_textureHeight = newTexture->GetHeight();
  m_textureScaleY = 1.0f / m_textureHeight;
  m_textureWidth = newTexture->GetWidth();
  m_textureScaleX = 1.0f / m_textureWidth;
  if (m_textureHeight < newHeight)
    CLog::Log(LOGWARNING, "%s: allocated new texture with height of %d, requested %d", __FUNCTION__,
              m_textureHeight, newHeight);
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
    delete m_texture;
  }

  m_textureStatus = TEXTURE_REALLOCATED;

  return newTexture;
}

bool CGUIFontTTFGLBase::CopyCharToTexture(
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

void CGUIFontTTFGLBase::DeleteHardwareTexture()
{
  if (m_textureStatus != TEXTURE_VOID)
  {
    if (glIsTexture(m_nTexture))
      CServiceBroker::GetGUI()->GetTextureManager().ReleaseHwTexture(m_nTexture);

    m_textureStatus = TEXTURE_VOID;
    m_updateY1 = m_updateY2 = 0;
  }
}

void CGUIFontTTFGLBase::CreateStaticVertexBuffers()
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
    index[i][0] = 4*i;
    index[i][1] = 4*i+1;
    index[i][2] = 4*i+2;
    index[i][3] = 4*i+1;
    index[i][4] = 4*i+3;
    index[i][5] = 4*i+2;
  }

  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index), index, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  m_staticVertexBufferCreated = true;
}

void CGUIFontTTFGLBase::DestroyStaticVertexBuffers()
{
  if (!m_staticVertexBufferCreated)
    return;
  glDeleteBuffers(1, &m_elementArrayHandle);
  m_staticVertexBufferCreated = false;
}

void CGUIFontTTFGLBase::RenderCharacter(CRect* texture, SVertex* v, float* x, float* y, float* z)
{
  for (int i = 0; i < 4; i++)
  {
    v[i].r = GET_R(m_color);
    v[i].g = GET_G(m_color);
    v[i].b = GET_B(m_color);
    v[i].a = GET_A(m_color);
  }

  // GL / GLES uses triangle strips, not quads, so have to rearrange the vertex order
  v[0].u = texture->x1 * m_textureScaleX;
  v[0].v = texture->y1 * m_textureScaleY;
  v[0].x = x[0];
  v[0].y = y[0];
  v[0].z = z[0];

  v[1].u = texture->x1 * m_textureScaleX;
  v[1].v = texture->y2 * m_textureScaleY;
  v[1].x = x[3];
  v[1].y = y[3];
  v[1].z = z[3];

  v[2].u = texture->x2 * m_textureScaleX;
  v[2].v = texture->y1 * m_textureScaleY;
  v[2].x = x[1];
  v[2].y = y[1];
  v[2].z = z[1];

  v[3].u = texture->x2 * m_textureScaleX;
  v[3].v = texture->y2 * m_textureScaleY;
  v[3].x = x[2];
  v[3].y = y[2];
  v[3].z = z[2];
}
