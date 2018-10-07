/*
 *      Initial code sponsored by: Voddler Inc (voddler.com)
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OverlayRenderer.h"
#include "OverlayRendererUtil.h"
#include "OverlayRendererGL.h"
#ifdef HAS_GL
#include "LinuxRendererGL.h"
#include "rendering/gl/RenderSystemGL.h"
#elif HAS_GLES >= 2
#include "LinuxRendererGLES.h"
#include "rendering/gles/RenderSystemGLES.h"
#endif
#include "rendering/MatrixGL.h"
#include "RenderManager.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayImage.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlaySSA.h"
#include "windowing/WinSystem.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "utils/GLUtils.h"

#if HAS_GLES >= 2
// GLES2.0 cant do CLAMP, but can do CLAMP_TO_EDGE.
#define GL_CLAMP	GL_CLAMP_TO_EDGE
#endif

#define USE_PREMULTIPLIED_ALPHA 1

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace OVERLAY;

static void LoadTexture(GLenum target
                      , GLsizei width, GLsizei height, GLsizei stride
                      , GLfloat* u, GLfloat* v
                      , bool alpha, const GLvoid* pixels)
{
  int width2  = width;
  int height2 = height;
  char *pixelVector = NULL;
  const GLvoid *pixelData = pixels;

#ifdef HAS_GLES
  /** OpenGL ES does not support BGR so use RGB and swap later **/
  GLenum internalFormat = alpha ? GL_ALPHA : GL_RGBA;
  GLenum externalFormat = alpha ? GL_ALPHA : GL_RGBA;
#else
  GLenum internalFormat = alpha ? GL_RED : GL_RGBA;
  GLenum externalFormat = alpha ? GL_RED : GL_BGRA;
#endif

  int bytesPerPixel = glFormatElementByteCount(externalFormat);

#ifdef HAS_GLES

  /** OpenGL ES does not support BGR **/
  if (!alpha)
  {
    int bytesPerLine = bytesPerPixel * width;

    pixelVector = (char *)malloc(bytesPerLine * height);

    const char *src = (const char*)pixels;
    char *dst = pixelVector;
    for (int y = 0;y < height;++y)
    {
      src = (const char*)pixels + y * stride;
      dst = pixelVector + y * bytesPerLine;

      for (GLsizei i = 0; i < width; i++, src+=4, dst+=4)
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
  else if (stride != width)
  {
    int bytesPerLine = bytesPerPixel * width;

    pixelVector = (char *)malloc(bytesPerLine * height);

    const char *src = (const char*)pixels;
    char *dst = pixelVector;
    for (int y = 0;y < height;++y)
    {
      memcpy(dst, src, bytesPerLine);
      src += stride;
      dst += bytesPerLine;
    }

    pixelData = pixelVector;
    stride = width;
  }
#else
  glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / bytesPerPixel);
#endif

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexImage2D   (target, 0, internalFormat
                , width2, height2, 0
                , externalFormat, GL_UNSIGNED_BYTE, NULL);

  glTexSubImage2D(target, 0
                , 0, 0, width, height
                , externalFormat, GL_UNSIGNED_BYTE
                , pixelData);

  if(height < height2)
    glTexSubImage2D( target, 0
                   , 0, height, width, 1
                   , externalFormat, GL_UNSIGNED_BYTE
                   , (const unsigned char*)pixelData + stride * (height-1));

  if(width  < width2)
    glTexSubImage2D( target, 0
                   , width, 0, 1, height
                   , externalFormat, GL_UNSIGNED_BYTE
                   , (const unsigned char*)pixelData + bytesPerPixel * (width-1));

#ifndef HAS_GLES
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif

  free(pixelVector);

  *u = (GLfloat)width  / width2;
  *v = (GLfloat)height / height2;
}

COverlayTextureGL::COverlayTextureGL(CDVDOverlayImage* o)
{
  m_texture = 0;

  uint32_t* rgba;
  int stride;
  if(o->palette)
  {
    m_pma  = !!USE_PREMULTIPLIED_ALPHA;
    rgba   = convert_rgba(o, m_pma);
    stride = o->width * 4;
  }
  else
  {
    m_pma  = false;
    rgba   = (uint32_t*)o->data;
    stride = o->linesize;
  }

  if(!rgba)
  {
    CLog::Log(LOGERROR, "COverlayTextureGL::COverlayTextureGL - failed to convert overlay to rgb");
    return;
  }

  glGenTextures(1, &m_texture);
  glBindTexture(GL_TEXTURE_2D, m_texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  LoadTexture(GL_TEXTURE_2D
            , o->width
            , o->height
            , stride
            , &m_u, &m_v
            , false
            , rgba);
  if(reinterpret_cast<uint8_t*>(rgba) != o->data)
    free(rgba);

  glBindTexture(GL_TEXTURE_2D, 0);

  if(o->source_width && o->source_height)
  {
    float center_x = (0.5f * o->width  + o->x) / o->source_width;
    float center_y = (0.5f * o->height + o->y) / o->source_height;

    m_width  = (float)o->width  / o->source_width;
    m_height = (float)o->height / o->source_height;
    m_pos    = POSITION_RELATIVE;

    {
      /* render aligned to screen to avoid cropping problems */
      m_align  = ALIGN_SCREEN;
      m_x      = center_x;
      m_y      = center_y;
    }
  }
  else
  {
    m_align  = ALIGN_VIDEO;
    m_pos    = POSITION_ABSOLUTE;
    m_x      = (float)o->x;
    m_y      = (float)o->y;
    m_width  = (float)o->width;
    m_height = (float)o->height;
  }
}

COverlayTextureGL::COverlayTextureGL(CDVDOverlaySpu* o)
{
  m_texture = 0;

  int min_x, max_x, min_y, max_y;
  uint32_t* rgba = convert_rgba(o, USE_PREMULTIPLIED_ALPHA
                              , min_x, max_x, min_y, max_y);

  if (!rgba)
  {
    CLog::Log(LOGERROR, "COverlayTextureGL::COverlayTextureGL - failed to convert overlay to rgb");
    return;
  }

  glGenTextures(1, &m_texture);
  glBindTexture(GL_TEXTURE_2D, m_texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  LoadTexture(GL_TEXTURE_2D
            , max_x - min_x
            , max_y - min_y
            , o->width * 4
            , &m_u, &m_v
            , false
            , rgba + min_x + min_y * o->width);

  free(rgba);

  glBindTexture(GL_TEXTURE_2D, 0);

  m_align  = ALIGN_VIDEO;
  m_pos    = POSITION_ABSOLUTE;
  m_x      = (float)(min_x + o->x);
  m_y      = (float)(min_y + o->y);
  m_width  = (float)(max_x - min_x);
  m_height = (float)(max_y - min_y);
  m_pma    = !!USE_PREMULTIPLIED_ALPHA;
}

COverlayGlyphGL::COverlayGlyphGL(ASS_Image* images, int width, int height)
{
  m_vertex = NULL;
  m_width  = 1.0;
  m_height = 1.0;
  m_align  = ALIGN_VIDEO;
  m_pos    = POSITION_RELATIVE;
  m_x      = 0.0f;
  m_y      = 0.0f;
  m_texture = 0;

  SQuads quads;
  if(!convert_quad(images, quads, width))
    return;

  glGenTextures(1, &m_texture);
  glBindTexture(GL_TEXTURE_2D, m_texture);

  LoadTexture(GL_TEXTURE_2D
            , quads.size_x
            , quads.size_y
            , quads.size_x
            , &m_u, &m_v
            , true
            , quads.data);


  float scale_u = m_u / quads.size_x;
  float scale_v = m_v / quads.size_y;

  float scale_x = 1.0f / width;
  float scale_y = 1.0f / height;

  m_count  = quads.count;
  m_vertex = (VERTEX*)calloc(m_count * 4, sizeof(VERTEX));

  VERTEX* vt = m_vertex;
  SQuad*  vs = quads.quad;

  for(int i=0; i < quads.count; i++)
  {
    for(int s = 0; s < 4; s++)
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

COverlayGlyphGL::~COverlayGlyphGL()
{
  glDeleteTextures(1, &m_texture);
  free(m_vertex);
}

void COverlayGlyphGL::Render(SRenderState& state)
{
  if ((m_texture == 0) || (m_count == 0))
    return;

  glEnable(GL_BLEND);

  glBindTexture(GL_TEXTURE_2D, m_texture);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glMatrixModview.Push();
  glMatrixModview->Translatef(state.x, state.y, 0.0f);
  glMatrixModview->Scalef(state.width, state.height, 1.0f);
  glMatrixModview.Load();

#ifdef HAS_GL
  CRenderSystemGL* renderSystem = dynamic_cast<CRenderSystemGL*>(CServiceBroker::GetRenderSystem());
  renderSystem->EnableShader(SM_FONTS);
  GLint posLoc  = renderSystem->ShaderGetPos();
  GLint colLoc  = renderSystem->ShaderGetCol();
  GLint tex0Loc = renderSystem->ShaderGetCoord0();

  std::vector<VERTEX> vecVertices( 6 * m_count);
  VERTEX *vertices = &vecVertices[0];

  for (int i=0; i<m_count*4; i+=4)
  {
    *vertices++ = m_vertex[i];
    *vertices++ = m_vertex[i+1];
    *vertices++ = m_vertex[i+2];

    *vertices++ = m_vertex[i+1];
    *vertices++ = m_vertex[i+3];
    *vertices++ = m_vertex[i+2];
  }
  GLuint VertexVBO;

  glGenBuffers(1, &VertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, VertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX)*vecVertices.size(), &vecVertices[0], GL_STATIC_DRAW);

  glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(VERTEX), BUFFER_OFFSET(offsetof(VERTEX, x)));
  glVertexAttribPointer(colLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VERTEX), BUFFER_OFFSET(offsetof(VERTEX, r)));
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, GL_FALSE, sizeof(VERTEX), BUFFER_OFFSET(offsetof(VERTEX, u)));

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(colLoc);
  glEnableVertexAttribArray(tex0Loc);

  glDrawArrays(GL_TRIANGLES, 0, vecVertices.size());

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(colLoc);
  glDisableVertexAttribArray(tex0Loc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &VertexVBO);

  renderSystem->DisableShader();

#else
  CRenderSystemGLES* renderSystem = dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
  renderSystem->EnableGUIShader(SM_FONTS);
  GLint posLoc  = renderSystem->GUIShaderGetPos();
  GLint colLoc  = renderSystem->GUIShaderGetCol();
  GLint tex0Loc = renderSystem->GUIShaderGetCoord0();

  // stack object until VBOs will be used
  std::vector<VERTEX> vecVertices( 6 * m_count);
  VERTEX *vertices = &vecVertices[0];

  for (int i=0; i<m_count*4; i+=4)
  {
    *vertices++ = m_vertex[i];
    *vertices++ = m_vertex[i+1];
    *vertices++ = m_vertex[i+2];

    *vertices++ = m_vertex[i+1];
    *vertices++ = m_vertex[i+3];
    *vertices++ = m_vertex[i+2];
  }

  vertices = &vecVertices[0];

  glVertexAttribPointer(posLoc,  3, GL_FLOAT,         GL_FALSE, sizeof(VERTEX), (char*)vertices + offsetof(VERTEX, x));
  glVertexAttribPointer(colLoc,  4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(VERTEX), (char*)vertices + offsetof(VERTEX, r));
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT,         GL_FALSE, sizeof(VERTEX), (char*)vertices + offsetof(VERTEX, u));

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(colLoc);
  glEnableVertexAttribArray(tex0Loc);

  glDrawArrays(GL_TRIANGLES, 0, vecVertices.size());

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(colLoc);
  glDisableVertexAttribArray(tex0Loc);

  renderSystem->DisableGUIShader();
#endif

  glMatrixModview.PopLoad();

  glDisable(GL_BLEND);

  glBindTexture(GL_TEXTURE_2D, 0);
}


COverlayTextureGL::~COverlayTextureGL()
{
  glDeleteTextures(1, &m_texture);
}

void COverlayTextureGL::Render(SRenderState& state)
{
  glEnable(GL_BLEND);

  glBindTexture(GL_TEXTURE_2D, m_texture);
  if(m_pma)
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  else
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  DRAWRECT rd;
  if (m_pos == POSITION_RELATIVE)
  {
    rd.top     = state.y - state.height * 0.5;
    rd.bottom  = state.y + state.height * 0.5;
    rd.left    = state.x - state.width  * 0.5;
    rd.right   = state.x + state.width  * 0.5;
  }
  else
  {
    rd.top     = state.y;
    rd.bottom  = state.y + state.height;
    rd.left    = state.x;
    rd.right   = state.x + state.width;
  }

#if defined(HAS_GL)
  CRenderSystemGL* renderSystem = dynamic_cast<CRenderSystemGL*>(CServiceBroker::GetRenderSystem());
  renderSystem->EnableShader(SM_TEXTURE_LIM);
  GLint posLoc = renderSystem->ShaderGetPos();
  GLint tex0Loc = renderSystem->ShaderGetCoord0();
  GLint uniColLoc = renderSystem->ShaderGetUniCol();

  GLfloat col[4] = {1.0f, 1.0f, 1.0f, 1.0f};

  struct PackedVertex
  {
    float x, y, z;
    float u1, v1;
  } vertex[4];
  GLubyte idx[4] = {0, 1, 3, 2};  //determines order of the vertices
  GLuint vertexVBO;
  GLuint indexVBO;

  glUniform4f(uniColLoc,(col[0]), (col[1]), (col[2]), (col[3]));

  // Setup vertex position values
  vertex[0].x = rd.left;
  vertex[0].y = rd.top;
  vertex[0].z = 0;
  vertex[0].u1 = 0.0f;
  vertex[0].v1 = 0.0;

  vertex[1].x = rd.right;
  vertex[1].y = rd.top;
  vertex[1].z = 0;
  vertex[1].u1 = m_u;
  vertex[1].v1 = 0.0f;

  vertex[2].x = rd.right;
  vertex[2].y = rd.bottom;
  vertex[2].z = 0;
  vertex[2].u1 = m_u;
  vertex[2].v1 = m_v;

  vertex[3].x = rd.left;
  vertex[3].y = rd.bottom;
  vertex[3].z = 0;
  vertex[3].u1 = 0.0f;
  vertex[3].v1 = m_v;

  glGenBuffers(1, &vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex)*4, &vertex[0], GL_STATIC_DRAW);

  glVertexAttribPointer(posLoc, 2, GL_FLOAT, 0, sizeof(PackedVertex), BUFFER_OFFSET(offsetof(PackedVertex, x)));
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, sizeof(PackedVertex), BUFFER_OFFSET(offsetof(PackedVertex, u1)));

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(tex0Loc);

  glGenBuffers(1, &indexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte)*4, idx, GL_STATIC_DRAW);

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(tex0Loc);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vertexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &indexVBO);

  renderSystem->DisableShader();

#else
  CRenderSystemGLES* renderSystem = dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
  renderSystem->EnableGUIShader(SM_TEXTURE);
  GLint posLoc = renderSystem->GUIShaderGetPos();
  GLint colLoc = renderSystem->GUIShaderGetCol();
  GLint tex0Loc = renderSystem->GUIShaderGetCoord0();
  GLint uniColLoc = renderSystem->GUIShaderGetUniCol();

  GLfloat col[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  GLfloat ver[4][2];
  GLfloat tex[4][2];
  GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip

  glVertexAttribPointer(posLoc, 2, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(colLoc, 4, GL_FLOAT, 0, 0, col);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, 0, tex);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(colLoc);
  glEnableVertexAttribArray(tex0Loc);

  glUniform4f(uniColLoc,(col[0]), (col[1]), (col[2]), (col[3]));
  // Setup vertex position values
  ver[0][0] = ver[3][0] = rd.left;
  ver[0][1] = ver[1][1] = rd.top;
  ver[1][0] = ver[2][0] = rd.right;
  ver[2][1] = ver[3][1] = rd.bottom;

  // Setup texture coordinates
  tex[0][0] = tex[0][1] = tex[1][1] = tex[3][0] = 0.0f;
  tex[1][0] = tex[2][0] = m_u;
  tex[2][1] = tex[3][1] = m_v;

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(colLoc);
  glDisableVertexAttribArray(tex0Loc);

  renderSystem->DisableGUIShader();
#endif

  glDisable(GL_BLEND);

  glBindTexture(GL_TEXTURE_2D, 0);
}
