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

#include "RPRendererOpenGLES.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

#include <cstring>
#include <stddef.h>

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace KODI;
using namespace RETRO;

// --- CRendererFactoryOpenGLES ------------------------------------------------

std::string CRendererFactoryOpenGLES::RenderSystemName() const
{
  return "OpenGLES";
}

CRPBaseRenderer *CRendererFactoryOpenGLES::CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool)
{
  return new CRPRendererOpenGLES(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CRendererFactoryOpenGLES::CreateBufferPools(CRenderContext &context)
{
  return { std::make_shared<CRenderBufferPoolOpenGLES>(context) };
}

// --- CRenderBufferOpenGLES ---------------------------------------------------

CRenderBufferOpenGLES::CRenderBufferOpenGLES(CRenderContext &context,
                                             GLuint pixeltype,
                                             GLuint internalformat,
                                             GLuint pixelformat,
                                             GLuint bpp) :
  m_context(context),
  m_pixeltype(pixeltype),
  m_internalformat(internalformat),
  m_pixelformat(pixelformat),
  m_bpp(bpp)
{
}

CRenderBufferOpenGLES::~CRenderBufferOpenGLES()
{
  DeleteTexture();
}

void CRenderBufferOpenGLES::CreateTexture()
{
  glGenTextures(1, &m_textureId);

  glBindTexture(m_textureTarget, m_textureId);

  glTexImage2D(m_textureTarget, 0, m_internalformat, m_width, m_height, 0, m_pixelformat, m_pixeltype, NULL);

  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(m_textureTarget, 0);
}

bool CRenderBufferOpenGLES::UploadTexture()
{
  if (!glIsTexture(m_textureId))
    CreateTexture();

  glBindTexture(m_textureTarget, m_textureId);

  const int stride = GetFrameSize() / m_height;

  glPixelStorei(GL_UNPACK_ALIGNMENT, m_bpp);

  if (m_bpp == 4 && m_pixelformat == GL_RGBA)
  {
    // XOR Swap RGBA -> BGRA
    // GLES 2.0 doesn't support strided textures (unless GL_UNPACK_ROW_LENGTH_EXT is supported)
    uint8_t* pixels = const_cast<uint8_t*>(m_data.data());
    for (unsigned int y = 0; y < m_height; ++y, pixels += stride)
    {
      for (int x = 0; x < stride; x += 4)
        std::swap(pixels[x], pixels[x + 2]);
      glTexSubImage2D(m_textureTarget, 0, 0, y, m_width, 1, m_pixelformat, m_pixeltype, pixels);
    }
  }
#ifdef GL_UNPACK_ROW_LENGTH_EXT
  else if (m_context.IsExtSupported("GL_EXT_unpack_subimage"))
  {
    glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, stride / m_bpp);
    glTexSubImage2D(m_textureTarget, 0, 0, 0, m_width, m_height, m_pixelformat, m_pixeltype, m_data.data());
    glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, 0);
  }
#endif
  else
  {
    uint8_t* pixels = const_cast<uint8_t*>(m_data.data());
    for (unsigned int y = 0; y < m_height; ++y, pixels += stride)
      glTexSubImage2D(m_textureTarget, 0, 0, y, m_width, 1, m_pixelformat, m_pixeltype, pixels);
  }

  glBindTexture(m_textureTarget, 0);

  return true;
}

void CRenderBufferOpenGLES::DeleteTexture()
{
  if (glIsTexture(m_textureId))
    glDeleteTextures(1, &m_textureId);

  m_textureId = 0;
}

// --- CRenderBufferPoolOpenGLES -----------------------------------------------

CRenderBufferPoolOpenGLES::CRenderBufferPoolOpenGLES(CRenderContext &context)
  : m_context(context)
{
}

bool CRenderBufferPoolOpenGLES::IsCompatible(const CRenderVideoSettings &renderSettings) const
{
  if (!CRPRendererOpenGLES::SupportsScalingMethod(renderSettings.GetScalingMethod()))
    return false;

  return true;
}

IRenderBuffer *CRenderBufferPoolOpenGLES::CreateRenderBuffer(void *header /* = nullptr */)
{
  return new CRenderBufferOpenGLES(m_context,
                                   m_pixeltype,
                                   m_internalformat,
                                   m_pixelformat,
                                   m_bpp);
}

bool CRenderBufferPoolOpenGLES::ConfigureInternal()
{
  switch (m_format)
  {
  case AV_PIX_FMT_0RGB32:
  {
    m_pixeltype = GL_UNSIGNED_BYTE;
    if (m_context.IsExtSupported("GL_EXT_texture_format_BGRA8888") ||
        m_context.IsExtSupported("GL_IMG_texture_format_BGRA8888"))
    {
      m_internalformat = GL_BGRA_EXT;
      m_pixelformat = GL_BGRA_EXT;
    }
    else if (m_context.IsExtSupported("GL_APPLE_texture_format_BGRA8888"))
    {
      // Apple's implementation does not conform to spec. Instead, they require
      // differing format/internalformat, more like GL.
      m_internalformat = GL_RGBA;
      m_pixelformat = GL_BGRA_EXT;
    }
    else
    {
      m_internalformat = GL_RGBA;
      m_pixelformat = GL_RGBA;
    }
    m_bpp = sizeof(uint32_t);
    return true;
  }
  case AV_PIX_FMT_RGB555:
  {
    m_pixeltype = GL_UNSIGNED_SHORT_5_5_5_1;
    m_internalformat = GL_RGB;
    m_pixelformat = GL_RGB;
    m_bpp = sizeof(uint16_t);
    return true;
  }
  case AV_PIX_FMT_RGB565:
  {
    m_pixeltype = GL_UNSIGNED_SHORT_5_6_5;
    m_internalformat = GL_RGB;
    m_pixelformat = GL_RGB;
    m_bpp = sizeof(uint16_t);
    return true;
  }
  default:
    break;
  }

  return false;
}

// --- CRPRendererOpenGLES -----------------------------------------------------

CRPRendererOpenGLES::CRPRendererOpenGLES(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) :
  CRPBaseRenderer(renderSettings, context, std::move(bufferPool))
{
}

CRPRendererOpenGLES::~CRPRendererOpenGLES()
{
  Deinitialize();
}

void CRPRendererOpenGLES::RenderInternal(bool clear, uint8_t alpha)
{
  if (clear)
  {
    if (alpha == 255)
      DrawBlackBars();
    else
      ClearBackBuffer();
  }

  if (alpha < 255)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else
  {
    glDisable(GL_BLEND);
  }

  Render(alpha);

  glEnable(GL_BLEND);
  glFlush();
}

void CRPRendererOpenGLES::FlushInternal()
{
  if (!m_bConfigured)
    return;

  glFinish();
}

bool CRPRendererOpenGLES::Supports(RENDERFEATURE feature) const
{
  if (feature == RENDERFEATURE::STRETCH         ||
      feature == RENDERFEATURE::ZOOM            ||
      feature == RENDERFEATURE::PIXEL_RATIO     ||
      feature == RENDERFEATURE::ROTATION)
  {
    return true;
  }

  return false;
}

bool CRPRendererOpenGLES::SupportsScalingMethod(SCALINGMETHOD method)
{
  if (method == SCALINGMETHOD::NEAREST ||
      method == SCALINGMETHOD::LINEAR)
  {
    return true;
  }

  return false;
}

void CRPRendererOpenGLES::ClearBackBuffer()
{
  glClearColor(m_clearColour, m_clearColour, m_clearColour, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void CRPRendererOpenGLES::DrawBlackBars()
{
  glDisable(GL_BLEND);

  struct Svertex
  {
    float x;
    float y;
    float z;
  };
  Svertex vertices[24];
  GLubyte count = 0;

  m_context.EnableGUIShader(GL_SHADER_METHOD::DEFAULT);
  GLint posLoc = m_context.GUIShaderGetPos();
  GLint uniCol = m_context.GUIShaderGetUniCol();

  glUniform4f(uniCol, m_clearColour / 255.0f, m_clearColour / 255.0f, m_clearColour / 255.0f, 1.0f);

  //top quad
  if (m_rotatedDestCoords[0].y > 0.0)
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = 0.0;
    vertices[quad].z = 0;
    vertices[quad+1].x = m_context.GetScreenWidth();
    vertices[quad+1].y = 0;
    vertices[quad+1].z = 0;
    vertices[quad+2].x = m_context.GetScreenWidth();
    vertices[quad+2].y = m_rotatedDestCoords[0].y;
    vertices[quad+2].z = 0;
    vertices[quad+3] = vertices[quad+2];
    vertices[quad+4].x = 0;
    vertices[quad+4].y = m_rotatedDestCoords[0].y;
    vertices[quad+4].z = 0;
    vertices[quad+5] = vertices[quad];
    count += 6;
  }

  //bottom quad
  if (m_rotatedDestCoords[2].y < m_context.GetScreenHeight())
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = m_rotatedDestCoords[2].y;
    vertices[quad].z = 0;
    vertices[quad+1].x = m_context.GetScreenWidth();
    vertices[quad+1].y = m_rotatedDestCoords[2].y;
    vertices[quad+1].z = 0;
    vertices[quad+2].x = m_context.GetScreenWidth();
    vertices[quad+2].y = m_context.GetScreenHeight();
    vertices[quad+2].z = 0;
    vertices[quad+3] = vertices[quad+2];
    vertices[quad+4].x = 0;
    vertices[quad+4].y = m_context.GetScreenHeight();
    vertices[quad+4].z = 0;
    vertices[quad+5] = vertices[quad];
    count += 6;
  }

  //left quad
  if (m_rotatedDestCoords[0].x > 0.0)
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = m_rotatedDestCoords[0].y;
    vertices[quad].z = 0;
    vertices[quad+1].x = m_rotatedDestCoords[0].x;
    vertices[quad+1].y = m_rotatedDestCoords[0].y;
    vertices[quad+1].z = 0;
    vertices[quad+2].x = m_rotatedDestCoords[3].x;
    vertices[quad+2].y = m_rotatedDestCoords[3].y;
    vertices[quad+2].z = 0;
    vertices[quad+3] = vertices[quad+2];
    vertices[quad+4].x = 0;
    vertices[quad+4].y = m_rotatedDestCoords[3].y;
    vertices[quad+4].z = 0;
    vertices[quad+5] = vertices[quad];
    count += 6;
  }

  //right quad
  if (m_rotatedDestCoords[2].x < m_context.GetScreenWidth())
  {
    GLubyte quad = count;
    vertices[quad].x = m_rotatedDestCoords[1].x;
    vertices[quad].y = m_rotatedDestCoords[1].y;
    vertices[quad].z = 0;
    vertices[quad+1].x = m_context.GetScreenWidth();
    vertices[quad+1].y = m_rotatedDestCoords[1].y;
    vertices[quad+1].z = 0;
    vertices[quad+2].x = m_context.GetScreenWidth();
    vertices[quad+2].y = m_rotatedDestCoords[2].y;
    vertices[quad+2].z = 0;
    vertices[quad+3] = vertices[quad+2];
    vertices[quad+4].x = m_rotatedDestCoords[1].x;
    vertices[quad+4].y = m_rotatedDestCoords[2].y;
    vertices[quad+4].z = 0;
    vertices[quad+5] = vertices[quad];
    count += 6;
  }

  GLuint vertexVBO;

  glGenBuffers(1, &vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Svertex)*count, &vertices[0], GL_STATIC_DRAW);

  glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Svertex), 0);
  glEnableVertexAttribArray(posLoc);

  glDrawArrays(GL_TRIANGLES, 0, count);

  glDisableVertexAttribArray(posLoc);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vertexVBO);

  m_context.DisableGUIShader();
}

void CRPRendererOpenGLES::Render(uint8_t alpha)
{
  CRenderBufferOpenGLES *renderBuffer = static_cast<CRenderBufferOpenGLES*>(m_renderBuffer);

  if (renderBuffer == nullptr)
    return;

  CRect rect = m_sourceRect;

  rect.x1 /= m_sourceWidth;
  rect.x2 /= m_sourceWidth;
  rect.y1 /= m_sourceHeight;
  rect.y2 /= m_sourceHeight;

  const uint32_t color = (alpha << 24) | 0xFFFFFF;

  glBindTexture(m_textureTarget, renderBuffer->TextureID());

  GLint filter = GL_NEAREST;
  if (GetRenderSettings().VideoSettings().GetScalingMethod() == SCALINGMETHOD::LINEAR)
    filter = GL_LINEAR;
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  m_context.EnableGUIShader(GL_SHADER_METHOD::TEXTURE);

  GLubyte colour[4];
  GLubyte idx[4] = {0, 1, 3, 2}; // Determines order of triangle strip
  GLuint vertexVBO;
  GLuint indexVBO;
  struct PackedVertex
  {
    float x, y, z;
    float u1, v1;
  } vertex[4];

  GLint vertLoc = m_context.GUIShaderGetPos();
  GLint loc = m_context.GUIShaderGetCoord0();
  GLint uniColLoc = m_context.GUIShaderGetUniCol();

  // Setup color values
  colour[0] = static_cast<GLubyte>(GET_R(color));
  colour[1] = static_cast<GLubyte>(GET_G(color));
  colour[2] = static_cast<GLubyte>(GET_B(color));
  colour[3] = static_cast<GLubyte>(GET_A(color));

  for (unsigned int i = 0; i < 4; i++)
  {
    // Setup vertex position values
    vertex[i].x = m_rotatedDestCoords[i].x;
    vertex[i].y = m_rotatedDestCoords[i].y;
    vertex[i].z = 0.0f;
  }

  // Setup texture coordinates
  vertex[0].u1 = vertex[3].u1 = rect.x1;
  vertex[0].v1 = vertex[1].v1 = rect.y1;
  vertex[1].u1 = vertex[2].u1 = rect.x2;
  vertex[2].v1 = vertex[3].v1 = rect.y2;

  glGenBuffers(1, &vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex)*4, &vertex[0], GL_STATIC_DRAW);

  glVertexAttribPointer(vertLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex), BUFFER_OFFSET(offsetof(PackedVertex, x)));
  glVertexAttribPointer(loc, 2, GL_FLOAT, 0, sizeof(PackedVertex), BUFFER_OFFSET(offsetof(PackedVertex, u1)));

  glEnableVertexAttribArray(vertLoc);
  glEnableVertexAttribArray(loc);

  glGenBuffers(1, &indexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte)*4, idx, GL_STATIC_DRAW);

  glUniform4f(uniColLoc,(colour[0] / 255.0f), (colour[1] / 255.0f), (colour[2] / 255.0f), (colour[3] / 255.0f));
  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);

  glDisableVertexAttribArray(vertLoc);
  glDisableVertexAttribArray(loc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vertexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &indexVBO);

  m_context.DisableGUIShader();
}
