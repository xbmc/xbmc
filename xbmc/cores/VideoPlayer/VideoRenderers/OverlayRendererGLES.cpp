/*
 *      Initial code sponsored by: Voddler Inc (voddler.com)
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OverlayRendererGLES.h"

#include "LinuxRendererGLES.h"
#include "OverlayRenderer.h"
#include "OverlayRendererUtil.h"
#include "RenderManager.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayImage.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlaySSA.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "rendering/MatrixGL.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/GLUtils.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"

#include <cmath>

// GLES2.0 cant do CLAMP, but can do CLAMP_TO_EDGE.
#define GL_CLAMP GL_CLAMP_TO_EDGE

#define USE_PREMULTIPLIED_ALPHA 1

using namespace OVERLAY;

static void LoadTexture(GLenum target,
                        GLsizei width,
                        GLsizei height,
                        GLsizei stride,
                        GLfloat* u,
                        GLfloat* v,
                        bool alpha,
                        const GLvoid* pixels)
{
  int width2 = width;
  int height2 = height;
  char* pixelVector = NULL;
  const GLvoid* pixelData = pixels;

  GLenum internalFormat = alpha ? GL_ALPHA : GL_RGBA;
  GLenum externalFormat = alpha ? GL_ALPHA : GL_RGBA;

  int bytesPerPixel = KODI::UTILS::GL::glFormatElementByteCount(externalFormat);

  bool bgraSupported = false;
  CRenderSystemGLES* renderSystem =
      dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());

  if (!alpha)
  {
    if (renderSystem->IsExtSupported("GL_EXT_texture_format_BGRA8888") ||
        renderSystem->IsExtSupported("GL_IMG_texture_format_BGRA8888"))
    {
      bgraSupported = true;
      internalFormat = externalFormat = GL_BGRA_EXT;
    }
    else if (renderSystem->IsExtSupported("GL_APPLE_texture_format_BGRA8888"))
    {
      // Apple's implementation does not conform to spec. Instead, they require
      // differing format/internalformat, more like GL.
      bgraSupported = true;
      externalFormat = GL_BGRA_EXT;
    }
  }

  int bytesPerLine = bytesPerPixel * width;

  if (!alpha && !bgraSupported)
  {
    pixelVector = (char*)malloc(bytesPerLine * height);

    const char* src = (const char*)pixels;
    char* dst = pixelVector;
    for (int y = 0; y < height; ++y)
    {
      src = (const char*)pixels + y * stride;
      dst = pixelVector + y * bytesPerLine;

      for (GLsizei i = 0; i < width; i++, src += 4, dst += 4)
      {
        dst[0] = src[2];
        dst[1] = src[1];
        dst[2] = src[0];
        dst[3] = src[3];
      }
    }

    pixelData = pixelVector;
    stride = width;
  }
  /** OpenGL ES does not support strided texture input. Make a copy without stride **/
  else if (stride != bytesPerLine)
  {
    pixelVector = (char*)malloc(bytesPerLine * height);

    const char* src = (const char*)pixels;
    char* dst = pixelVector;
    for (int y = 0; y < height; ++y)
    {
      memcpy(dst, src, bytesPerLine);
      src += stride;
      dst += bytesPerLine;
    }

    pixelData = pixelVector;
    stride = bytesPerLine;
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexImage2D(target, 0, internalFormat, width2, height2, 0, externalFormat, GL_UNSIGNED_BYTE,
               NULL);

  glTexSubImage2D(target, 0, 0, 0, width, height, externalFormat, GL_UNSIGNED_BYTE, pixelData);

  if (height < height2)
    glTexSubImage2D(target, 0, 0, height, width, 1, externalFormat, GL_UNSIGNED_BYTE,
                    (const unsigned char*)pixelData + stride * (height - 1));

  if (width < width2)
    glTexSubImage2D(target, 0, width, 0, 1, height, externalFormat, GL_UNSIGNED_BYTE,
                    (const unsigned char*)pixelData + bytesPerPixel * (width - 1));

  free(pixelVector);

  *u = (GLfloat)width / width2;
  *v = (GLfloat)height / height2;
}

std::shared_ptr<COverlay> COverlay::Create(const CDVDOverlayImage& o, CRect& rSource)
{
  return std::make_shared<COverlayTextureGLES>(o, rSource);
}

COverlayTextureGLES::COverlayTextureGLES(const CDVDOverlayImage& o, CRect& rSource)
{
  glGenTextures(1, &m_texture);
  glBindTexture(GL_TEXTURE_2D, m_texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  if (o.palette.empty())
  {
    m_pma = false;
    const uint32_t* rgba = reinterpret_cast<const uint32_t*>(o.pixels.data());
    LoadTexture(GL_TEXTURE_2D, o.width, o.height, o.linesize, &m_u, &m_v, false, rgba);
  }
  else
  {
    std::vector<uint32_t> rgba(o.width * o.height);
    m_pma = !!USE_PREMULTIPLIED_ALPHA;
    convert_rgba(o, m_pma, rgba);
    LoadTexture(GL_TEXTURE_2D, o.width, o.height, o.width * 4, &m_u, &m_v, false, rgba.data());
  }

  glBindTexture(GL_TEXTURE_2D, 0);

  if (o.source_width > 0 && o.source_height > 0)
  {
    m_pos = POSITION_RELATIVE;
    m_x = (0.5f * o.width + o.x) / o.source_width;
    m_y = (0.5f * o.height + o.y) / o.source_height;

    const float subRatio{static_cast<float>(o.source_width) / o.source_height};
    const float vidRatio{rSource.Width() / rSource.Height()};

    // We always consider aligning 4/3 subtitles to the video,
    // for example SD DVB subtitles (4/3) must be stretched on fullhd video

    if (std::fabs(subRatio - vidRatio) < 0.001f || IsSquareResolution(subRatio))
    {
      m_align = ALIGN_VIDEO;
      m_width = static_cast<float>(o.width) / o.source_width;
      m_height = static_cast<float>(o.height) / o.source_height;
    }
    else
    {
      // We should have a re-encoded/cropped (removed black bars) video source.
      // Then we cannot align to video otherwise the subtitles will be deformed
      // better align to screen by keeping the aspect-ratio.
      m_align = ALIGN_SCREEN_AR;
      m_width = static_cast<float>(o.width);
      m_height = static_cast<float>(o.height);
      m_source_width = static_cast<float>(o.source_width);
      m_source_height = static_cast<float>(o.source_height);
    }
  }
  else
  {
    m_align = ALIGN_VIDEO;
    m_pos = POSITION_ABSOLUTE;
    m_x = static_cast<float>(o.x);
    m_y = static_cast<float>(o.y);
    m_width = static_cast<float>(o.width);
    m_height = static_cast<float>(o.height);
  }
}

std::shared_ptr<COverlay> COverlay::Create(const CDVDOverlaySpu& o)
{
  return std::make_shared<COverlayTextureGLES>(o);
}

COverlayTextureGLES::COverlayTextureGLES(const CDVDOverlaySpu& o)
{
  int min_x, max_x, min_y, max_y;
  std::vector<uint32_t> rgba(o.width * o.height);

  convert_rgba(o, USE_PREMULTIPLIED_ALPHA, min_x, max_x, min_y, max_y, rgba);

  glGenTextures(1, &m_texture);
  glBindTexture(GL_TEXTURE_2D, m_texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  LoadTexture(GL_TEXTURE_2D, max_x - min_x, max_y - min_y, o.width * 4, &m_u, &m_v, false,
              rgba.data() + min_x + min_y * o.width);

  glBindTexture(GL_TEXTURE_2D, 0);

  m_align = ALIGN_VIDEO;
  m_pos = POSITION_ABSOLUTE;
  m_x = static_cast<float>(min_x + o.x);
  m_y = static_cast<float>(min_y + o.y);
  m_width = static_cast<float>(max_x - min_x);
  m_height = static_cast<float>(max_y - min_y);
  m_pma = !!USE_PREMULTIPLIED_ALPHA;
}

std::shared_ptr<COverlay> COverlay::Create(ASS_Image* images, float width, float height)
{
  return std::make_shared<COverlayGlyphGLES>(images, width, height);
}

COverlayGlyphGLES::COverlayGlyphGLES(ASS_Image* images, float width, float height)
{
  m_width = 1.0;
  m_height = 1.0;
  m_align = ALIGN_SCREEN;
  m_pos = POSITION_RELATIVE;
  m_x = 0.0f;
  m_y = 0.0f;

  SQuads quads;
  if (!convert_quad(images, quads, static_cast<int>(width)))
    return;

  glGenTextures(1, &m_texture);
  glBindTexture(GL_TEXTURE_2D, m_texture);

  LoadTexture(GL_TEXTURE_2D, quads.size_x, quads.size_y, quads.size_x, &m_u, &m_v, true,
              quads.texture.data());

  float scale_u = m_u / quads.size_x;
  float scale_v = m_v / quads.size_y;

  float scale_x = 1.0f / width;
  float scale_y = 1.0f / height;

  m_vertex.resize(quads.quad.size() * 4);

  VERTEX* vt = m_vertex.data();
  SQuad* vs = quads.quad.data();

  for (size_t i = 0; i < quads.quad.size(); i++)
  {
    for (int s = 0; s < 4; s++)
    {
      vt[s].a = vs->a;
      vt[s].r = vs->r;
      vt[s].g = vs->g;
      vt[s].b = vs->b;

      vt[s].x = scale_x;
      vt[s].y = scale_y;
      vt[s].z = 0.0f;
      vt[s].u = scale_u;
      vt[s].v = scale_v;
    }

    vt[0].x *= vs->x;
    vt[0].u *= vs->u;
    vt[0].y *= vs->y;
    vt[0].v *= vs->v;

    vt[1].x *= vs->x;
    vt[1].u *= vs->u;
    vt[1].y *= vs->y + vs->h;
    vt[1].v *= vs->v + vs->h;

    vt[2].x *= vs->x + vs->w;
    vt[2].u *= vs->u + vs->w;
    vt[2].y *= vs->y;
    vt[2].v *= vs->v;

    vt[3].x *= vs->x + vs->w;
    vt[3].u *= vs->u + vs->w;
    vt[3].y *= vs->y + vs->h;
    vt[3].v *= vs->v + vs->h;

    vs += 1;
    vt += 4;
  }

  glBindTexture(GL_TEXTURE_2D, 0);
}

COverlayGlyphGLES::~COverlayGlyphGLES()
{
  glDeleteTextures(1, &m_texture);
}

void COverlayGlyphGLES::Render(SRenderState& state)
{
  if ((m_texture == 0) || (m_vertex.size() == 0))
    return;

  glEnable(GL_BLEND);

  glBindTexture(GL_TEXTURE_2D, m_texture);
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glMatrixModview.Push();
  glMatrixModview->Translatef(state.x, state.y, 0.0f);
  glMatrixModview->Scalef(state.width, state.height, 1.0f);
  glMatrixModview.Load();

  CRenderSystemGLES* renderSystem =
      dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
  renderSystem->EnableGUIShader(ShaderMethodGLES::SM_FONTS);
  GLint posLoc = renderSystem->GUIShaderGetPos();
  GLint colLoc = renderSystem->GUIShaderGetCol();
  GLint tex0Loc = renderSystem->GUIShaderGetCoord0();
  GLint depthLoc = renderSystem->GUIShaderGetDepth();
  GLint matrixUniformLoc = renderSystem->GUIShaderGetMatrix();

  CMatrixGL matrix = glMatrixProject.Get();
  matrix.MultMatrixf(glMatrixModview.Get());
  glUniformMatrix4fv(matrixUniformLoc, 1, GL_FALSE, matrix);

  // stack object until VBOs will be used
  std::vector<VERTEX> vecVertices(6 * m_vertex.size() / 4);
  VERTEX* vertices = vecVertices.data();

  for (size_t i = 0; i < m_vertex.size(); i += 4)
  {
    *vertices++ = m_vertex[i];
    *vertices++ = m_vertex[i + 1];
    *vertices++ = m_vertex[i + 2];

    *vertices++ = m_vertex[i + 1];
    *vertices++ = m_vertex[i + 3];
    *vertices++ = m_vertex[i + 2];
  }

  vertices = vecVertices.data();

  glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(VERTEX),
                        (char*)vertices + offsetof(VERTEX, x));
  glVertexAttribPointer(colLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VERTEX),
                        (char*)vertices + offsetof(VERTEX, r));
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, GL_FALSE, sizeof(VERTEX),
                        (char*)vertices + offsetof(VERTEX, u));

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(colLoc);
  glEnableVertexAttribArray(tex0Loc);

  glUniform1f(depthLoc, -1.0f);

  glDrawArrays(GL_TRIANGLES, 0, vecVertices.size());

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(colLoc);
  glDisableVertexAttribArray(tex0Loc);

  renderSystem->DisableGUIShader();

  glMatrixModview.PopLoad();

  glDisable(GL_BLEND);

  glBindTexture(GL_TEXTURE_2D, 0);
}

COverlayTextureGLES::~COverlayTextureGLES()
{
  glDeleteTextures(1, &m_texture);
}

void COverlayTextureGLES::Render(SRenderState& state)
{
  glEnable(GL_BLEND);

  glBindTexture(GL_TEXTURE_2D, m_texture);
  if (m_pma)
    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  else
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  CRect rd;
  if (m_pos == POSITION_RELATIVE)
  {
    float top = state.y - state.height * 0.5f;
    float bottom = state.y + state.height * 0.5f;
    float left = state.x - state.width * 0.5f;
    float right = state.x + state.width * 0.5f;

    rd.SetRect(left, top, right, bottom);
  }
  else
  {
    float top = state.y;
    float bottom = state.y + state.height;
    float left = state.x;
    float right = state.x + state.width;

    rd.SetRect(left, top, right, bottom);
  }

  CRenderSystemGLES* renderSystem =
      dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
  renderSystem->EnableGUIShader(ShaderMethodGLES::SM_TEXTURE);
  GLint posLoc = renderSystem->GUIShaderGetPos();
  GLint colLoc = renderSystem->GUIShaderGetCol();
  GLint tex0Loc = renderSystem->GUIShaderGetCoord0();
  GLint uniColLoc = renderSystem->GUIShaderGetUniCol();
  GLint depthLoc = renderSystem->GUIShaderGetDepth();

  GLfloat col[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  GLfloat ver[4][2];
  GLfloat tex[4][2];
  GLubyte idx[4] = {0, 1, 3, 2}; //determines order of triangle strip

  glVertexAttribPointer(posLoc, 2, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(colLoc, 4, GL_FLOAT, 0, 0, col);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, 0, tex);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(colLoc);
  glEnableVertexAttribArray(tex0Loc);

  glUniform4f(uniColLoc, (col[0]), (col[1]), (col[2]), (col[3]));
  glUniform1f(depthLoc, 1.0f);
  // Setup vertex position values
  ver[0][0] = ver[3][0] = rd.x1;
  ver[0][1] = ver[1][1] = rd.y1;
  ver[1][0] = ver[2][0] = rd.x2;
  ver[2][1] = ver[3][1] = rd.y2;

  // Setup texture coordinates
  tex[0][0] = tex[0][1] = tex[1][1] = tex[3][0] = 0.0f;
  tex[1][0] = tex[2][0] = m_u;
  tex[2][1] = tex[3][1] = m_v;

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(colLoc);
  glDisableVertexAttribArray(tex0Loc);

  renderSystem->DisableGUIShader();

  glDisable(GL_BLEND);

  glBindTexture(GL_TEXTURE_2D, 0);
}
