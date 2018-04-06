/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LinuxRendererGLES3.h"

#include "RenderFactory.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "ServiceBroker.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

CBaseRenderer* CLinuxRendererGLES3::Create(CVideoBuffer *buffer)
{
  CLog::Log(LOGDEBUG, "CLinuxRendererGLES3::%s - Using the OpenGLES3 renderer", __FUNCTION__);
  return new CLinuxRendererGLES3();
}

bool CLinuxRendererGLES3::Register()
{
  int renderVersionMajor = 0;
  glGetIntegerv(GL_MAJOR_VERSION, &renderVersionMajor);

  if (renderVersionMajor >= 3)
  {
    VIDEOPLAYER::CRendererFactory::RegisterRenderer("default", CLinuxRendererGLES3::Create);
    return true;
  }

  return false;
}

bool CLinuxRendererGLES3::CreateYV12Texture(int index)
{
  CPictureBuffer &buf = m_buffers[index];
  YuvImage &im = m_buffers[index].image;

  DeleteYV12Texture(index);

  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;
  im.cshift_x = 1;
  im.cshift_y = 1;

  switch (m_format)
  {
    case AV_PIX_FMT_YUV420P16:
      buf.m_srcTextureBits = 16;
      break;
    case AV_PIX_FMT_YUV420P14:
      buf.m_srcTextureBits = 14;
      break;
    case AV_PIX_FMT_YUV420P12:
      buf.m_srcTextureBits = 12;
      break;
    case AV_PIX_FMT_YUV420P10:
      buf.m_srcTextureBits = 10;
      break;
    case AV_PIX_FMT_YUV420P9:
      buf.m_srcTextureBits = 9;
      break;
    default:
      break;
  }

  if (buf.m_srcTextureBits > 8)
  {
    im.bpp = 2;
  }
  else
  {
    im.bpp = 1;
  }

  im.stride[0] = im.bpp * im.width;
  im.stride[1] = im.bpp * (im.width >> im.cshift_x);
  im.stride[2] = im.bpp * (im.width >> im.cshift_x);

  im.planesize[0] = im.stride[0] * im.height;
  im.planesize[1] = im.stride[1] * (im.height >> im.cshift_y);
  im.planesize[2] = im.stride[2] * (im.height >> im.cshift_y);

  for (int i = 0; i < 3; i++)
  {
    im.plane[i] = new uint8_t[im.planesize[i]];
  }

  for (int f = 0; f < MAX_FIELDS; f++)
  {
    for (int p = 0; p < YuvImage::MAX_PLANES; p++)
    {
      if (!glIsTexture(m_buffers[index].fields[f][p].id))
      {
        glGenTextures(1, &m_buffers[index].fields[f][p].id);
        VerifyGLState();
      }
    }
  }

  // YUV
  for (int f = FIELD_FULL; f <= FIELD_BOT; f++)
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

      if (im.bpp >= 2)
      {
        glTexImage2D(m_textureTarget, 0, GL_R16_EXT, plane.texwidth, plane.texheight, 0, GL_RED, GL_UNSIGNED_SHORT, nullptr);
      }
      else
      {
        glTexImage2D(m_textureTarget, 0, GL_R8, plane.texwidth, plane.texheight, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
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

bool CLinuxRendererGLES3::UploadYV12Texture(int source)
{
  CPictureBuffer& buf = m_buffers[source];
  YuvImage* im = &buf.image;

  VerifyGLState();

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  //Load Y plane
  LoadPlane(buf.fields[FIELD_FULL][0], GL_RED,
            im->width, im->height,
            im->stride[0], im->bpp, im->plane[0]);

  //load U plane
  LoadPlane(buf.fields[FIELD_FULL][1], GL_RED,
            im->width >> im->cshift_x, im->height >> im->cshift_y,
            im->stride[1], im->bpp, im->plane[1]);

  //load V plane
  LoadPlane(buf.fields[FIELD_FULL][2], GL_RED,
            im->width >> im->cshift_x, im->height >> im->cshift_y,
            im->stride[2], im->bpp, im->plane[2]);

  VerifyGLState();

  CalculateTextureSourceRects(source, 3);

  return true;
}

bool CLinuxRendererGLES3::CreateNV12Texture(int index)
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

  for (int f = 0; f < MAX_FIELDS; f++)
  {
    for (int p = 0; p < 2; p++)
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
        glTexImage2D(m_textureTarget, 0, GL_RG, plane.texwidth, plane.texheight, 0, GL_RG, GL_UNSIGNED_BYTE, nullptr);
      }
      else
      {
        glTexImage2D(m_textureTarget, 0, GL_RED, plane.texwidth, plane.texheight, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
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

bool CLinuxRendererGLES3::UploadNV12Texture(int source)
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
    LoadPlane(buf.fields[FIELD_TOP][0], GL_RED,
              im->width, im->height >> 1,
              im->stride[0]*2, im->bpp, im->plane[0]);

    // Load Even Y field
    LoadPlane(buf.fields[FIELD_BOT][0], GL_RED,
              im->width, im->height >> 1,
              im->stride[0]*2, im->bpp, im->plane[0] + im->stride[0]);

    // Load Odd UV Fields
    LoadPlane(buf.fields[FIELD_TOP][1], GL_RG,
              im->width >> im->cshift_x, im->height >> (im->cshift_y + 1),
              im->stride[1]*2, im->bpp, im->plane[1]);

    // Load Even UV Fields
    LoadPlane(buf.fields[FIELD_BOT][1], GL_RG,
              im->width >> im->cshift_x, im->height >> (im->cshift_y + 1),
              im->stride[1]*2, im->bpp, im->plane[1] + im->stride[1]);

  }
  else
  {
    // Load Y plane
    LoadPlane(buf. fields[FIELD_FULL][0], GL_RED,
              im->width, im->height,
              im->stride[0], im->bpp, im->plane[0]);

    // Load UV plane
    LoadPlane(buf.fields[FIELD_FULL][1], GL_RG,
              im->width >> im->cshift_x, im->height >> im->cshift_y,
              im->stride[1], im->bpp, im->plane[1]);
  }

  VerifyGLState();

  CalculateTextureSourceRects(source, 3);

  return true;
}

void CLinuxRendererGLES3::LoadPlane(CYuvPlane& plane, int type,
                                    unsigned width, unsigned height,
                                    int stride, int bpp, void* data)
{
  const GLvoid *pixelData = data;
  int bps = bpp * glFormatElementByteCount(type);

  GLint dataType = GL_UNSIGNED_BYTE;
  if (bpp >= 2)
  {
    dataType = GL_UNSIGNED_SHORT;
  }

  glBindTexture(m_textureTarget, plane.id);

  GLint pixelStore = -1;
  unsigned int pixelStoreKey = -1;

  if (stride != static_cast<int>(width * bps))
  {
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &pixelStore);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
    pixelStoreKey = GL_UNPACK_ROW_LENGTH;
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
