/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LinuxRendererGLES2.h"

#include "RenderFactory.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "ServiceBroker.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

CBaseRenderer* CLinuxRendererGLES2::Create(CVideoBuffer *buffer)
{
  CLog::Log(LOGDEBUG, "CLinuxRendererGLES2::%s - Using the OpenGLES2 renderer", __FUNCTION__);
  return new CLinuxRendererGLES2();
}

bool CLinuxRendererGLES2::Register()
{
  VIDEOPLAYER::CRendererFactory::RegisterRenderer("default", CLinuxRendererGLES2::Create);
  return true;
}

bool CLinuxRendererGLES2::CreateYV12Texture(int index)
{
  // since we also want the field textures, pitch must be texture aligned
  unsigned p;
  YuvImage &im = m_buffers[index].image;

  DeleteYV12Texture(index);

  im.height = m_sourceHeight;
  im.width = m_sourceWidth;
  im.cshift_x = 1;
  im.cshift_y = 1;
  im.bpp = 1;

  im.stride[0] = im.bpp * im.width;
  im.stride[1] = im.bpp * (im.width >> im.cshift_x);
  im.stride[2] = im.bpp * (im.width >> im.cshift_x);

  im.planesize[0] = im.stride[0] * im.height;
  im.planesize[1] = im.stride[1] * (im.height >> im.cshift_y);
  im.planesize[2] = im.stride[2] * (im.height >> im.cshift_y);

  m_planeBuffer = static_cast<unsigned char*>(realloc(m_planeBuffer, m_sourceHeight * m_sourceWidth * im.bpp));

  for (int i = 0; i < 3; i++)
  {
    im.plane[i] = new uint8_t[im.planesize[i]];
  }

  for(int f = 0; f < MAX_FIELDS; f++)
  {
    for(p = 0; p < YuvImage::MAX_PLANES; p++)
    {
      if (!glIsTexture(m_buffers[index].fields[f][p].id))
      {
        glGenTextures(1, &m_buffers[index].fields[f][p].id);
        VerifyGLState();
      }
    }
  }

  // YUV
  for (int f = FIELD_FULL; f <= FIELD_BOT ; f++)
  {
    int fieldshift = (f == FIELD_FULL) ? 0 : 1;
    CYuvPlane (&planes)[YuvImage::MAX_PLANES] = m_buffers[index].fields[f];

    planes[0].texwidth  = im.width;
    planes[0].texheight = im.height >> fieldshift;

    planes[1].texwidth  = planes[0].texwidth  >> im.cshift_x;
    planes[1].texheight = planes[0].texheight >> im.cshift_y;
    planes[2].texwidth  = planes[0].texwidth  >> im.cshift_x;
    planes[2].texheight = planes[0].texheight >> im.cshift_y;

    for (int p = 0; p < 3; p++)
    {
      planes[p].pixpertex_x = 1;
      planes[p].pixpertex_y = 1;
    }

    for(int p = 0; p < 3; p++)
    {
      CYuvPlane &plane = planes[p];
      if (plane.texwidth * plane.texheight == 0)
      {
        continue;
      }

      glBindTexture(m_textureTarget, plane.id);

      GLint format;
      if (p == 2) // V plane needs an alpha texture
      {
        format = GL_ALPHA;
      }
      else
      {
        format = GL_LUMINANCE;
      }

      glTexImage2D(m_textureTarget, 0, format, plane.texwidth, plane.texheight, 0, format, GL_UNSIGNED_BYTE, nullptr);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      VerifyGLState();
    }
  }
  return true;
}

bool CLinuxRendererGLES2::UploadYV12Texture(int source)
{
  CPictureBuffer& buf = m_buffers[source];
  YuvImage* im = &buf.image;

  VerifyGLState();

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

  // load Y plane
  LoadPlane(buf.fields[FIELD_FULL][0], GL_LUMINANCE,
            im->width, im->height,
            im->stride[0], im->bpp, im->plane[0]);

  // load U plane
  LoadPlane(buf.fields[FIELD_FULL][1], GL_LUMINANCE,
            im->width >> im->cshift_x, im->height >> im->cshift_y,
            im->stride[1], im->bpp, im->plane[1]);

  // load V plane
  LoadPlane(buf.fields[FIELD_FULL][2], GL_ALPHA,
            im->width >> im->cshift_x, im->height >> im->cshift_y,
            im->stride[2], im->bpp, im->plane[2]);

  VerifyGLState();

  CalculateTextureSourceRects(source, 3);

  return true;
}

bool CLinuxRendererGLES2::CreateNV12Texture(int index)
{
  // since we also want the field textures, pitch must be texture aligned
  CPictureBuffer& buf = m_buffers[index];
  YuvImage &im = buf.image;

  // Delete any old texture
  DeleteNV12Texture(index);

  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;
  im.cshift_x = 1;
  im.cshift_y = 1;
  im.bpp = 1;

  im.stride[0] = im.width;
  im.stride[1] = im.width;
  im.stride[2] = 0;

  im.plane[0] = nullptr;
  im.plane[1] = nullptr;
  im.plane[2] = nullptr;

  // Y plane
  im.planesize[0] = im.stride[0] * im.height;
  // packed UV plane
  im.planesize[1] = im.stride[1] * im.height / 2;
  // third plane is not used
  im.planesize[2] = 0;

  for (int i = 0; i < 2; i++)
  {
    im.plane[i] = new uint8_t[im.planesize[i]];
  }

  for(int f = 0; f < MAX_FIELDS; f++)
  {
    for(int p = 0; p < 2; p++)
    {
      if (!glIsTexture(buf.fields[f][p].id))
      {
        glGenTextures(1, &buf.fields[f][p].id);
        VerifyGLState();
      }
    }

    buf.fields[f][2].id = buf.fields[f][1].id;
  }

  // YUV
  for (int f = FIELD_FULL; f <= FIELD_BOT; f++)
  {
    int fieldshift = (f == FIELD_FULL) ? 0 : 1;
    CYuvPlane (&planes)[YuvImage::MAX_PLANES] = buf.fields[f];

    planes[0].texwidth  = im.width;
    planes[0].texheight = im.height >> fieldshift;

    planes[1].texwidth  = planes[0].texwidth  >> im.cshift_x;
    planes[1].texheight = planes[0].texheight >> im.cshift_y;
    planes[2].texwidth  = planes[1].texwidth;
    planes[2].texheight = planes[1].texheight;

    for (int p = 0; p < 3; p++)
    {
      planes[p].pixpertex_x = 1;
      planes[p].pixpertex_y = 1;
    }

    for(int p = 0; p < 2; p++)
    {
      CYuvPlane &plane = planes[p];
      if (plane.texwidth * plane.texheight == 0)
      {
        continue;
      }

      glBindTexture(m_textureTarget, plane.id);

      if (p == 1)
      {
        glTexImage2D(m_textureTarget, 0, GL_LUMINANCE_ALPHA, plane.texwidth, plane.texheight, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, nullptr);
      }
      else
      {
        glTexImage2D(m_textureTarget, 0, GL_LUMINANCE, plane.texwidth, plane.texheight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);
      }

      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      VerifyGLState();
    }
  }

  return true;
}

bool CLinuxRendererGLES2::UploadNV12Texture(int source)
{
  CPictureBuffer& buf = m_buffers[source];
  YuvImage* im = &buf.image;

  bool deinterlacing;
  if (m_currentField == FIELD_FULL)
  {
    deinterlacing = false;
  }
  else
  {
    deinterlacing = true;
  }

  VerifyGLState();

  glPixelStorei(GL_UNPACK_ALIGNMENT, im->bpp);

  if (deinterlacing)
  {
    // Load Odd Y field
    LoadPlane(buf.fields[FIELD_TOP][0] , GL_LUMINANCE,
              im->width, im->height >> 1,
              im->stride[0]*2, im->bpp, im->plane[0]);

    // Load Even Y field
    LoadPlane(buf.fields[FIELD_BOT][0], GL_LUMINANCE,
              im->width, im->height >> 1,
              im->stride[0]*2, im->bpp, im->plane[0] + im->stride[0]) ;

    // Load Odd UV Fields
    LoadPlane(buf.fields[FIELD_TOP][1], GL_LUMINANCE_ALPHA,
              im->width >> im->cshift_x, im->height >> (im->cshift_y + 1),
              im->stride[1]*2, im->bpp, im->plane[1]);

    // Load Even UV Fields
    LoadPlane(buf.fields[FIELD_BOT][1], GL_LUMINANCE_ALPHA,
              im->width >> im->cshift_x, im->height >> (im->cshift_y + 1),
              im->stride[1]*2, im->bpp, im->plane[1] + im->stride[1]);

  }
  else
  {
    // Load Y plane
    LoadPlane(buf. fields[FIELD_FULL][0], GL_LUMINANCE,
              im->width, im->height,
              im->stride[0], im->bpp, im->plane[0]);

    // Load UV plane
    LoadPlane(buf.fields[FIELD_FULL][1], GL_LUMINANCE_ALPHA,
              im->width >> im->cshift_x, im->height >> im->cshift_y,
              im->stride[1], im->bpp, im->plane[1]);
  }

  VerifyGLState();

  CalculateTextureSourceRects(source, 3);

  return true;
}

void CLinuxRendererGLES2::LoadPlane(CYuvPlane& plane, int type, unsigned width, unsigned height, int stride, int bpp, void* data)
{
  const GLvoid *pixelData = data;
  int bps = bpp * glFormatElementByteCount(type);

  GLint dataType = GL_UNSIGNED_BYTE;
  if (bpp >= 2)
  {
    dataType = GL_UNSIGNED_SHORT;
  }

  glBindTexture(m_textureTarget, plane.id);

  // OpenGL ES does not support strided texture input.
  GLint pixelStore = -1;
  unsigned int pixelStoreKey = -1;

  if (stride != static_cast<int>(width * bps))
  {
#if defined (GL_UNPACK_ROW_LENGTH_EXT)
    if (m_renderSystem->IsExtSupported("GL_EXT_unpack_subimage"))
    {
      glGetIntegerv(GL_UNPACK_ROW_LENGTH_EXT, &pixelStore);
      glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, stride);
      pixelStoreKey = GL_UNPACK_ROW_LENGTH_EXT;
    }
    else
#endif
    {
      unsigned char *src(static_cast<unsigned char*>(data)),
                    *dst(m_planeBuffer);

      for (unsigned int y = 0; y < height; ++y, src += stride, dst += width * bpp)
        memcpy(dst, src, width * bpp);

      pixelData = m_planeBuffer;
    }
  }

  glTexSubImage2D(m_textureTarget, 0, 0, 0, width, height, type, GL_UNSIGNED_BYTE, pixelData);

  if (pixelStore >= 0)
    glPixelStorei(pixelStoreKey, pixelStore);

  // check if we need to load any border pixels
  if (height < plane.texheight)
  {
    glTexSubImage2D(m_textureTarget, 0,
                    0, height, width, 1,
                    type, dataType,
                    static_cast<const unsigned char*>(pixelData) + stride * (height - 1));
  }

  if (width  < plane.texwidth)
  {
    glTexSubImage2D(m_textureTarget, 0,
                    width, 0, 1, height,
                    type, dataType,
                    static_cast<const unsigned char*>(pixelData) + bps * (width - 1));
  }

  glBindTexture(m_textureTarget, 0);
}
