/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *      Initial code sponsored by: Voddler Inc (voddler.com)
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "system.h"
#include "OverlayRenderer.h"
#include "OverlayRendererGL.h"
#ifdef HAS_GL
#include "LinuxRendererGL.h"
#elif HAS_GLES == 2
#include "LinuxRendererGLES.h"
#include "MatrixGLES.h"
#endif
#include "RenderManager.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlayImage.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlaySSA.h"
#include "Application.h"
#include "WindowingFactory.h"

#if defined(HAS_GL) || HAS_GLES == 2

#if HAS_GLES == 2
// GLES2.0 cant do CLAMP, but can do CLAMP_TO_EDGE.
#define GL_CLAMP	GL_CLAMP_TO_EDGE
#endif

#define USE_PREMULTIPLIED_ALPHA 1

using namespace OVERLAY;

static void LoadTexture(GLenum target
                      , GLsizei width, GLsizei height, GLsizei stride
                      , GLfloat* u, GLfloat* v
                      , GLenum format, const GLvoid* pixels)
{
  int width2  = NP2(width);
  int height2 = NP2(height);

#ifndef HAS_GLES
  if(format == GL_RGBA)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / 4);
  else if(format == GL_RGB)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / 3);
  else
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
#endif

  glTexImage2D   (target, 0, 4
                , width2, height2, 0
                , format, GL_UNSIGNED_BYTE, NULL);

  glTexSubImage2D(target, 0
                , 0, 0, width, height
                , format, GL_UNSIGNED_BYTE
                , pixels);

  if(height < height2)
    glTexSubImage2D( target, 0
                   , 0, height, width, 1
                   , format, GL_UNSIGNED_BYTE
                   , (unsigned char*)pixels + stride * (height-1));

  if(width  < width2)
    glTexSubImage2D( target, 0
                   , width, 0, 1, height
                   , format, GL_UNSIGNED_BYTE
                   , (unsigned char*)pixels + stride - 1);

#ifndef HAS_GLES
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif

  *u = (GLfloat)width  / width2;
  *v = (GLfloat)height / height2;
}

static uint32_t build_rgba(int a, int r, int g, int b)
{
#if USE_PREMULTIPLIED_ALPHA
    return      a        << PIXEL_ASHIFT
         | (r * a / 255) << PIXEL_RSHIFT
         | (g * a / 255) << PIXEL_GSHIFT
         | (b * a / 255) << PIXEL_BSHIFT;
#else
    return a << PIXEL_ASHIFT
         | r << PIXEL_RSHIFT
         | g << PIXEL_GSHIFT
         | b << PIXEL_BSHIFT;
#endif
}

#define clamp(x) (x) > 255.0 ? 255 : ((x) < 0.0 ? 0 : (int)(x+0.5f))
static uint32_t build_rgba(int yuv[3], int alpha)
{
  int    a = alpha + ( (alpha << 4) & 0xff );
  double r = 1.164 * (yuv[0] - 16)                          + 1.596 * (yuv[2] - 128);
  double g = 1.164 * (yuv[0] - 16) - 0.391 * (yuv[1] - 128) - 0.813 * (yuv[2] - 128);
  double b = 1.164 * (yuv[0] - 16) + 2.018 * (yuv[1] - 128);
  return build_rgba(a, clamp(r), clamp(g), clamp(b));
}


long COverlayGL::Release()
{
  long count = InterlockedDecrement(&m_references);
  if (count == 0)
  {
    if (g_application.IsCurrentThread())
      delete this;
    else
      g_renderManager.AddCleanup(this);
  }
  return count;
}

COverlayTextureGL::COverlayTextureGL(CDVDOverlayImage* o)
{
  m_texture = 0;

  uint32_t* rgba = (uint32_t*)malloc(o->width * o->height * sizeof(uint32_t));

  if(!rgba)
  {
    CLog::Log(LOGERROR, "COverlayTextureGL::COverlayTextureGL - failed to allocate data for rgb buffer");
    return;
  }

  uint32_t palette[256];
  memset(palette, 0, 256 * sizeof(palette[0]));
  for(int i = 0; i < o->palette_colors; i++)
    palette[i] = build_rgba((o->palette[i] >> PIXEL_ASHIFT) & 0xff
                          , (o->palette[i] >> PIXEL_RSHIFT) & 0xff
                          , (o->palette[i] >> PIXEL_GSHIFT) & 0xff
                          , (o->palette[i] >> PIXEL_BSHIFT) & 0xff);

  /* convert to bgra, glTexImage2D should be *
   * able to load paletted formats directly, *
   * not sure how one does that thou         */
  for(int row = 0; row < o->height; row++)
    for(int col = 0; col < o->width; col++)
      rgba[row * o->width + col] = palette[ o->data[row * o->linesize + col] ];

  glGenTextures(1, &m_texture);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  LoadTexture(GL_TEXTURE_2D
            , o->width
            , o->height
            , o->width * 4
            , &m_u, &m_v
            , GL_RGBA
            , rgba);
  free(rgba);

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);

  if(o->source_width && o->source_height)
  {
    float center_x = (float)(0.5f * o->width  + o->x) / o->source_width;
    float center_y = (float)(0.5f * o->height + o->y) / o->source_height;

    m_width  = (float)o->width  / o->source_width;
    m_height = (float)o->height / o->source_height;
    m_pos    = POSITION_RELATIVE;

    if(center_x > 0.4 && center_x < 0.6
    && center_y > 0.8 && center_y < 1.0)
    {
     /* render bottom aligned to subtitle line */
      m_align  = ALIGN_SUBTITLE;
      m_x      = 0.0f;
      m_y      = - 0.5 * m_height;
    }
    else
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

  uint32_t* rgba = (uint32_t*)malloc(o->width * o->height * sizeof(uint32_t));

  if(!rgba)
  {
    CLog::Log(LOGERROR, "COverlayTextureGL::COverlayTextureGL - failed to allocate data for rgb buffer");
    return;
  }

  uint32_t palette[8];
  for(int i = 0; i < 4; i++)
  {
    palette[i]   = build_rgba(o->color[i]          , o->alpha[i]);
    palette[i+4] = build_rgba(o->highlight_color[i], o->highlight_alpha[i]);
  }

  uint32_t  color;
  uint32_t* trg;
  uint16_t* src;

  int len, idx, draw;

  int btn_x_start = 0
    , btn_x_end   = 0
    , btn_y_start = 0
    , btn_y_end   = 0;

  if(o->bForced)
  {
    btn_x_start = o->crop_i_x_start;
    btn_x_end   = o->crop_i_x_end;
    btn_y_start = o->crop_i_y_start;
    btn_y_end   = o->crop_i_y_end;
  }

  int min_x = o->width
    , max_x = 0
    , min_y = o->height
    , max_y = 0;

  trg = rgba;
  src = (uint16_t*)o->result;

  for (int y = 0; y < o->height; y++)
  {
    for (int x = 0; x < o->width ; x += len)
    {
      /* Get the RLE part, then draw the line */
      idx = *src & 0x3;
      len = *src++ >> 2;

      while( len > 0 )
      {
        draw  = len;
        color = palette[idx];

        if (y > btn_y_start && y < btn_y_end)
        {
          if     ( x <  btn_x_start && x + len >= btn_x_start) // starts outside
            draw = btn_x_start - x;
          else if( x >= btn_x_start && x       <= btn_x_end)   // starts inside
          {
            color = palette[idx + 4];
            draw  = btn_x_end - x + 1;
          }
        }
        /* make sure we are not requested to draw to far */
        /* that part will be taken care of in next pass */
        if( draw > len )
          draw = len;

        /* calculate cropping */
        if(color & 0xff000000)
        {
          if(x < min_x)
            min_x = x;
          if(y < min_y)
            min_y = y;
          if(x + draw > max_x)
            max_x = x + draw;
          if(y > max_y)
            max_y = y;
        }

        for(int i = 0; i < draw; i++)
          trg[x + i] = color;

        len -= draw;
        x   += draw;
      }
    }
    trg += o->width;
  }

  glGenTextures(1, &m_texture);
  glEnable(GL_TEXTURE_2D);
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
            , GL_RGBA
            , rgba + min_x + min_y * o->width);

  free(rgba);

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);

  m_align  = ALIGN_VIDEO;
  m_pos    = POSITION_ABSOLUTE;
  m_x      = (float)(min_x + o->x);
  m_y      = (float)(min_y + o->y);
  m_width  = (float)(max_x - min_x);
  m_height = (float)(max_y - min_y);
}

COverlayGlyphGL::COverlayGlyphGL(CDVDOverlaySSA* o, double pts)
{
  m_vertex = NULL;

  m_width  = (float)g_graphicsContext.GetWidth();
  m_height = (float)g_graphicsContext.GetHeight();
  m_align  = ALIGN_SCREEN;
  m_pos    = POSITION_ABSOLUTE;
  m_x      = (float)0.0f;
  m_y      = (float)0.0f;

  ass_image_t* images = o->m_libass->RenderImage((int)m_width, (int)m_height, pts);
  ass_image_t* img;

  m_texture = ~(GLuint)0;

  if (!images)
    return;
    
  // first calculate how many glyph we have and the total x length

  int count  = 0;
  int size_x = 0;
  int size_y = 0;

  for(img = images; img; img = img->next)
  {
    if((img->color & 0xff) == 0xff)
      continue;

    size_x += img->w;
    count++;
  }

  while(size_x > (int)g_Windowing.GetMaxTextureSize())
    size_x /= 2;

  int curr_x = 0;
  int curr_y = 0;

  // calculate the y size of the texture

  for(img = images; img; img = img->next)
  {
    if((img->color & 0xff) == 0xff)
      continue;

    // check if we need to split to new line
    if (curr_x + img->w >= size_x)
    {
      size_y += curr_y + 1;
      curr_x = 0;
      curr_y = 0;
    }

    curr_x += img->w + 1;

    if (img->h > curr_y)
      curr_y = img->h;
  }

  size_y += curr_y + 1;

  // allocate space for the glyph positions and texturedata

  m_count  = count;
  m_vertex = (SVertex*)calloc(count * 4, sizeof(SVertex));
  SVertex* v = m_vertex;

  unsigned char* srca = (unsigned char*)calloc(size_x * size_y, 1);
  unsigned char* data = srca;

  int y = 0;

  curr_x = 0;
  curr_y = 0;

  for(img = images; img; img = img->next)
  {
    unsigned int color = img->color;
    unsigned int alpha = (color & 0xff);
    if(alpha == 0xff)
      continue;

    if (curr_x + img->w >= size_x)
    {
      curr_y += y + 1;
      curr_x  = 0;
      y       = 0;
      data    = srca + curr_y * size_x;
    }

    unsigned int b = ((color >> 24) & 0xff);
    unsigned int g = ((color >> 16) & 0xff);
    unsigned int r = ((color >> 8) & 0xff);

    for(int i = 0; i < 4; i++)
    {
      v[i].a = 255 - alpha;
      v[i].r = r;
      v[i].g = g;
      v[i].b = b;
    }

#ifdef HAS_GL
    v[0].u = (float)(curr_x);
    v[0].v = (float)(curr_y);

    v[1].u = (float)(curr_x + img->w);
    v[1].v = (float)(curr_y);

    v[2].u = (float)(curr_x + img->w);
    v[2].v = (float)(curr_y + img->h);

    v[3].u = (float)(curr_x);
    v[3].v = (float)(curr_y + img->h);

    v[0].x = (float)(img->dst_x);
    v[0].y = (float)(img->dst_y);

    v[1].x = (float)(img->dst_x + img->w);
    v[1].y = (float)(img->dst_y);

    v[2].x = (float)(img->dst_x + img->w);
    v[2].y = (float)(img->dst_y + img->h);

    v[3].x = (float)(img->dst_x);
    v[3].y = (float)(img->dst_y + img->h);
#else
    //TODO: GLES version
#endif

    v += 4;

    for(int i=0; i<img->h; i++)
      memcpy(data        + size_x      * i
           , img->bitmap + img->stride * i
           , img->w);

    if (img->h > y)
      y = img->h;

    curr_x += img->w + 1;
    data   += img->w + 1;
  }
  glGenTextures(1, &m_texture);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  LoadTexture(GL_TEXTURE_2D, size_x, size_y, size_x, &m_u, &m_v, GL_ALPHA, srca);

  free(srca);

  float scale_u = m_u / size_x;
  float scale_v = m_v / size_y;
  for(int i=0; i < count * 4; i++)
  {
    m_vertex[i].u *= scale_u;
    m_vertex[i].v *= scale_v;
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);
}

COverlayGlyphGL::~COverlayGlyphGL()
{
  glDeleteTextures(1, &m_texture);
  free(m_vertex);
}

void COverlayGlyphGL::Render(SRenderState& state)
{
  if (m_texture == ~GLuint(0))
    return;

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);

  glBindTexture(GL_TEXTURE_2D, m_texture);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
#ifdef HAS_GL
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
  glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
  glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

  glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE0);
  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PRIMARY_COLOR);
  glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
  glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslatef(state.x, state.y, 0.0);
  glScalef(state.width / m_width, state.height / m_height, 1.0f);

  VerifyGLState();

  glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

  glColorPointer   (4, GL_UNSIGNED_BYTE, sizeof(SVertex), (char*)m_vertex + offsetof(SVertex, r));
  glVertexPointer  (3, GL_FLOAT        , sizeof(SVertex), (char*)m_vertex + offsetof(SVertex, x));
  glTexCoordPointer(2, GL_FLOAT        , sizeof(SVertex), (char*)m_vertex + offsetof(SVertex, u));
  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glDrawArrays(GL_QUADS, 0, m_count * 4);
  glPopClientAttrib();

  glPopMatrix();
#else
    //TODO: GLES version
#endif

  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, 0);
}


COverlayTextureGL::~COverlayTextureGL()
{
  glDeleteTextures(1, &m_texture);
}

void COverlayTextureGL::Render(SRenderState& state)
{
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);

  glBindTexture(GL_TEXTURE_2D, m_texture);
#if USE_PREMULTIPLIED_ALPHA
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
#else
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
#endif

#if defined(HAS_GL)
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  VerifyGLState();
#endif

  DRAWRECT rd;
  if(m_pos == POSITION_RELATIVE)
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
  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 0.0f);
  glVertex2f(rd.left , rd.top);

  glTexCoord2f(m_u , 0.0f);
  glVertex2f(rd.right, rd.top);

  glTexCoord2f(m_u , m_v);
  glVertex2f(rd.right, rd.bottom);

  glTexCoord2f(0.0f, m_v);
  glVertex2f(rd.left , rd.bottom);
  glEnd();
#elif HAS_GLES == 2
  //TODO: GLES2.0 version!
#endif

  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, 0);
}

#endif // HAS_GL
