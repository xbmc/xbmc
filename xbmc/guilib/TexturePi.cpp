/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "Texture.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "guilib/TextureManager.h"
#include "utils/URIUtils.h"

#if defined(HAS_OMXPLAYER)
#include "cores/omxplayer/OMXImage.h"

/************************************************************************/
/*    CPiTexture                                                       */
/************************************************************************/

CPiTexture::CPiTexture(unsigned int width, unsigned int height, unsigned int format)
: CGLTexture(width, height, format)
{
  m_egl_image = NULL;
}

CPiTexture::~CPiTexture()
{
  if (m_egl_image)
  {
    g_OMXImage.DestroyTexture(m_egl_image);
    m_egl_image = NULL;
  }
}

void CPiTexture::Allocate(unsigned int width, unsigned int height, unsigned int format)
{
  if (m_egl_image)
  {
    m_imageWidth = m_originalWidth = width;
    m_imageHeight = m_originalHeight = height;
    m_format = format;
    m_orientation = 0;

    m_textureWidth = m_imageWidth;
    m_textureHeight = m_imageHeight;
    return;
  }
  return CGLTexture::Allocate(width, height, format);
}

void CPiTexture::CreateTextureObject()
{
  if (m_egl_image && !m_texture)
  {
    g_OMXImage.GetTexture(m_egl_image, &m_texture);
    return;
  }
  CGLTexture::CreateTextureObject();
}

void CPiTexture::LoadToGPU()
{
  if (m_egl_image)
  {
    if (m_loadedToGPU)
    {
      // nothing to load - probably same image (no change)
      return;
    }
    if (m_texture == 0)
    {
      // Have OpenGL generate a texture object handle for us
      // this happens only one time - the first time the texture is loaded
      CreateTextureObject();
    }

    // Bind the texture object
    glBindTexture(GL_TEXTURE_2D, m_texture);

    if (IsMipmapped()) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glGenerateMipmap(GL_TEXTURE_2D);
    }
    m_loadedToGPU = true;
    return;
  }
  CGLTexture::LoadToGPU();
}

void CPiTexture::Update(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, const unsigned char *pixels, bool loadToGPU)
{
  if (m_egl_image)
  {
    if (loadToGPU)
      LoadToGPU();
    return;
  }
  CGLTexture::Update(width, height, pitch, format, pixels, loadToGPU);
}

bool CPiTexture::LoadFromFileInternal(const std::string& texturePath, unsigned int maxWidth, unsigned int maxHeight, bool requirePixels, const std::string& strMimeType)
{
  if (URIUtils::HasExtension(texturePath, ".jpg|.tbn"))
  {
    COMXImageFile *file = g_OMXImage.LoadJpeg(texturePath);
    if (file)
    {
      bool okay = false;
      int orientation = file->GetOrientation();
      // limit the sizes of jpegs (even if we fail to decode)
      g_OMXImage.ClampLimits(maxWidth, maxHeight, file->GetWidth(), file->GetHeight(), orientation & 4);

      if (requirePixels)
      {
        Allocate(maxWidth, maxHeight, XB_FMT_A8R8G8B8);
        if (m_pixels && COMXImage::DecodeJpeg(file, maxWidth, GetRows(), GetPitch(), (void *)m_pixels))
          okay = true;
      }
      else
      {
        if (g_OMXImage.DecodeJpegToTexture(file, maxWidth, maxHeight, &m_egl_image) && m_egl_image)
        {
          Allocate(maxWidth, maxHeight, XB_FMT_A8R8G8B8);
          okay = true;
        }
      }
      g_OMXImage.CloseJpeg(file);
      if (okay)
      {
        m_hasAlpha = false;
        m_orientation = orientation;
        return true;
      }
    }
  }
  return CGLTexture::LoadFromFileInternal(texturePath, maxWidth, maxHeight, requirePixels);
}

#endif
