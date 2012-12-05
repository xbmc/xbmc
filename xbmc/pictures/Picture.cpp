/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif

#include "Picture.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "DllImageLib.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "DllSwScale.h"
#include "guilib/JpegIO.h"
#include "guilib/Texture.h"
#if defined(HAS_OMXPLAYER)
#include "cores/omxplayer/OMXImage.h"
#endif

using namespace XFILE;

bool CPicture::CreateThumbnailFromSurface(const unsigned char *buffer, int width, int height, int stride, const CStdString &thumbFile)
{
  CLog::Log(LOGDEBUG, "cached image '%s' size %dx%d", thumbFile.c_str(), width, height);
  if (URIUtils::GetExtension(thumbFile).Equals(".jpg"))
  {
#if defined(HAS_OMXPLAYER)
    COMXImage *omxImage = new COMXImage();
    if (omxImage && omxImage->CreateThumbnailFromSurface((BYTE *)buffer, width, height, XB_FMT_A8R8G8B8, stride, thumbFile.c_str()))
    {
      delete omxImage;
      return true;
    }
    delete omxImage;
#endif
    CJpegIO jpegImage;
    if (jpegImage.CreateThumbnailFromSurface((BYTE *)buffer, width, height, XB_FMT_A8R8G8B8, stride, thumbFile.c_str()))
      return true;
  }
  DllImageLib dll;
  if (!buffer || !dll.Load()) return false;
  return dll.CreateThumbnailFromSurface((BYTE *)buffer, width, height, stride, thumbFile.c_str());
}

CThumbnailWriter::CThumbnailWriter(unsigned char* buffer, int width, int height, int stride, const CStdString& thumbFile)
{
  m_buffer    = buffer;
  m_width     = width;
  m_height    = height;
  m_stride    = stride;
  m_thumbFile = thumbFile;
}

bool CThumbnailWriter::DoWork()
{
  bool success = true;

  if (!CPicture::CreateThumbnailFromSurface(m_buffer, m_width, m_height, m_stride, m_thumbFile))
  {
    CLog::Log(LOGERROR, "CThumbnailWriter::DoWork unable to write %s", m_thumbFile.c_str());
    success = false;
  }

  delete [] m_buffer;

  return success;
}

bool CPicture::CacheTexture(CBaseTexture *texture, uint32_t &dest_width, uint32_t &dest_height, const std::string &dest)
{
  return CacheTexture(texture->GetPixels(), texture->GetWidth(), texture->GetHeight(), texture->GetPitch(),
                      texture->GetOrientation(), dest_width, dest_height, dest);
}

bool CPicture::CacheTexture(uint8_t *pixels, uint32_t width, uint32_t height, uint32_t pitch, int orientation, uint32_t &dest_width, uint32_t &dest_height, const std::string &dest)
{
  // if no max width or height is specified, don't resize
  if (dest_width == 0)
    dest_width = width;
  if (dest_height == 0)
    dest_height = height;

  uint32_t max_height = g_advancedSettings.m_imageRes;
  if (g_advancedSettings.m_fanartRes > g_advancedSettings.m_imageRes)
  { // a separate fanart resolution is specified - check if the image is exactly equal to this res
    if (width * 9 == height * 16 && height >= g_advancedSettings.m_fanartRes)
    { // special case for 16x9 images larger than the fanart res
      max_height = g_advancedSettings.m_fanartRes;
    }
  }
  uint32_t max_width = max_height * 16/9;

  dest_height = std::min(dest_height, max_height);
  dest_width  = std::min(dest_width, max_width);

  if (width > dest_width || height > dest_height || orientation)
  {
    bool success = false;

    dest_width = std::min(width, dest_width);
    dest_height = std::min(height, dest_height);

    // create a buffer large enough for the resulting image
    GetScale(width, height, dest_width, dest_height);
    uint32_t *buffer = new uint32_t[dest_width * dest_height];
    if (buffer)
    {
      if (ScaleImage(pixels, width, height, pitch,
                     (uint8_t *)buffer, dest_width, dest_height, dest_width * 4))
      {
        if (!orientation || OrientateImage(buffer, dest_width, dest_height, orientation))
        {
          success = CreateThumbnailFromSurface((unsigned char*)buffer, dest_width, dest_height, dest_width * 4, dest);
        }
      }
      delete[] buffer;
    }
    return success;
  }
  else
  { // no orientation needed
    dest_width = width;
    dest_height = height;
    return CreateThumbnailFromSurface(pixels, width, height, pitch, dest);
  }
  return false;
}

bool CPicture::CreateTiledThumb(const std::vector<std::string> &files, const std::string &thumb)
{
  if (!files.size())
    return false;

  unsigned int num_across = (unsigned int)ceil(sqrt((float)files.size()));
  unsigned int num_down = (files.size() + num_across - 1) / num_across;

  unsigned int tile_width = g_advancedSettings.GetThumbSize() / num_across;
  unsigned int tile_height = g_advancedSettings.GetThumbSize() / num_down;
  unsigned int tile_gap = 1;

  // create a buffer for the resulting thumb
  uint32_t *buffer = (uint32_t *)calloc(g_advancedSettings.GetThumbSize() * g_advancedSettings.GetThumbSize(), 4);
  for (unsigned int i = 0; i < files.size(); ++i)
  {
    int x = i % num_across;
    int y = i / num_across;
    // load in the image
    unsigned int width = tile_width - 2*tile_gap, height = tile_height - 2*tile_gap;
    CBaseTexture *texture = CTexture::LoadFromFile(files[i], width, height, g_guiSettings.GetBool("pictures.useexifrotation"));
    if (texture && texture->GetWidth() && texture->GetHeight())
    {
      GetScale(texture->GetWidth(), texture->GetHeight(), width, height);

      // scale appropriately
      uint32_t *scaled = new uint32_t[width * height];
      if (ScaleImage(texture->GetPixels(), texture->GetWidth(), texture->GetHeight(), texture->GetPitch(),
                     (uint8_t *)scaled, width, height, width * 4))
      {
        if (!texture->GetOrientation() || OrientateImage(scaled, width, height, texture->GetOrientation()))
        {
          // drop into the texture
          unsigned int posX = x*tile_width + (tile_width - width)/2;
          unsigned int posY = y*tile_height + (tile_height - height)/2;
          uint32_t *dest = buffer + posX + posY*g_advancedSettings.GetThumbSize();
          uint32_t *src = scaled;
          for (unsigned int y = 0; y < height; ++y)
          {
            memcpy(dest, src, width*4);
            dest += g_advancedSettings.GetThumbSize();
            src += width;
          }
        }
      }
      delete[] scaled;
      delete texture;
    }
  }
  // now save to a file
  bool ret = CreateThumbnailFromSurface((uint8_t *)buffer, g_advancedSettings.GetThumbSize(), g_advancedSettings.GetThumbSize(),
                                        g_advancedSettings.GetThumbSize() * 4, thumb);
  free(buffer);
  return ret;
}

void CPicture::GetScale(unsigned int width, unsigned int height, unsigned int &out_width, unsigned int &out_height)
{
  float aspect = (float)width / height;
  if ((unsigned int)(out_width / aspect + 0.5f) > out_height)
    out_width = (unsigned int)(out_height * aspect + 0.5f);
  else
    out_height = (unsigned int)(out_width / aspect + 0.5f);
}

bool CPicture::ScaleImage(uint8_t *in_pixels, unsigned int in_width, unsigned int in_height, unsigned int in_pitch,
                          uint8_t *out_pixels, unsigned int out_width, unsigned int out_height, unsigned int out_pitch)
{
  DllSwScale dllSwScale;
  dllSwScale.Load();
  struct SwsContext *context = dllSwScale.sws_getContext(in_width, in_height, PIX_FMT_BGRA,
                                                         out_width, out_height, PIX_FMT_BGRA,
                                                         SWS_FAST_BILINEAR | SwScaleCPUFlags(), NULL, NULL, NULL);

  uint8_t *src[] = { in_pixels, 0, 0, 0 };
  int     srcStride[] = { (int)in_pitch, 0, 0, 0 };
  uint8_t *dst[] = { out_pixels , 0, 0, 0 };
  int     dstStride[] = { (int)out_pitch, 0, 0, 0 };

  if (context)
  {
    dllSwScale.sws_scale(context, src, srcStride, 0, in_height, dst, dstStride);
    dllSwScale.sws_freeContext(context);
    return true;
  }
  return false;
}

bool CPicture::OrientateImage(uint32_t *&pixels, unsigned int &width, unsigned int &height, int orientation)
{
  // ideas for speeding these functions up: http://cgit.freedesktop.org/pixman/tree/pixman/pixman-fast-path.c
  uint32_t *out = NULL;
  switch (orientation)
  {
    case 1:
      out = FlipHorizontal(pixels, width, height);
      break;
    case 2:
      out = Rotate180CCW(pixels, width, height);
      break;
    case 3:
      out = FlipVertical(pixels, width, height);
      break;
    case 4:
      out = Transpose(pixels, width, height);
      break;
    case 5:
      out = Rotate270CCW(pixels, width, height);
      break;
    case 6:
      out = TransposeOffAxis(pixels, width, height);
      break;
    case 7:
      out = Rotate90CCW(pixels, width, height);
      break;
    default:
      CLog::Log(LOGERROR, "Unknown orientation %i", orientation);
      break;
  }
  if (out)
  {
    pixels = out;
    std::swap(width, height);
    return true;
  }
  return false;
}

uint32_t *CPicture::FlipHorizontal(uint32_t *pixels, unsigned int width, unsigned int height)
{
  // this can be done in-place easily enough
  for (unsigned int y = 0; y < height; ++y)
  {
    uint32_t *line = pixels + y * width;
    for (unsigned int x = 0; x < width / 2; ++x)
      std::swap(line[x], line[width - 1 - x]);
  }
  return NULL;
}

uint32_t *CPicture::FlipVertical(uint32_t *pixels, unsigned int width, unsigned int height)
{
  // this can be done in-place easily enough
  for (unsigned int y = 0; y < height / 2; ++y)
  {
    uint32_t *line1 = pixels + y * width;
    uint32_t *line2 = pixels + (height - 1 - y) * width;
    for (unsigned int x = 0; x < width; ++x)
      std::swap(*line1++, *line2++);
  }
  return NULL;
}

uint32_t *CPicture::Rotate180CCW(uint32_t *pixels, unsigned int width, unsigned int height)
{
  // this can be done in-place easily enough
  for (unsigned int y = 0; y < height / 2; ++y)
  {
    uint32_t *line1 = pixels + y * width;
    uint32_t *line2 = pixels + (height - 1 - y) * width + width - 1;
    for (unsigned int x = 0; x < width; ++x)
      std::swap(*line1++, *line2--);
  }
  if (height % 2)
  { // height is odd, so flip the middle row as well
    uint32_t *line = pixels + (height - 1)/2 * width;
    for (unsigned int x = 0; x < width / 2; ++x)
      std::swap(line[x], line[width - 1 - x]);
  }
  return NULL;
}

uint32_t *CPicture::Rotate90CCW(uint32_t *pixels, unsigned int width, unsigned int height)
{
  uint32_t *dest = new uint32_t[width * height * 4];
  if (dest)
  {
    unsigned int d_height = width, d_width = height;
    for (unsigned int y = 0; y < d_height; y++)
    {
      const uint32_t *src = pixels + (d_height - 1 - y); // y-th col from right, starting at top
      uint32_t *dst = dest + d_width * y;                // y-th row from top, starting at left
      for (unsigned int x = 0; x < d_width; x++)
      {
        *dst++ = *src;
        src += width;
      }
    }
  }
  delete[] pixels;
  return dest;
}

uint32_t *CPicture::Rotate270CCW(uint32_t *pixels, unsigned int width, unsigned int height)
{
  uint32_t *dest = new uint32_t[width * height * 4];
  if (!dest)
    return NULL;

  unsigned int d_height = width, d_width = height;
  for (unsigned int y = 0; y < d_height; y++)
  {
    const uint32_t *src = pixels + width * (d_width - 1) + y; // y-th col from left, starting at bottom
    uint32_t *dst = dest + d_width * y;                       // y-th row from top, starting at left
    for (unsigned int x = 0; x < d_width; x++)
    {
      *dst++ = *src;
      src -= width;
    }
  }

  delete[] pixels;
  return dest;
}

uint32_t *CPicture::Transpose(uint32_t *pixels, unsigned int width, unsigned int height)
{
  uint32_t *dest = new uint32_t[width * height * 4];
  if (!dest)
    return NULL;

  unsigned int d_height = width, d_width = height;
  for (unsigned int y = 0; y < d_height; y++)
  {
    const uint32_t *src = pixels + y;   // y-th col from left, starting at top
    uint32_t *dst = dest + d_width * y; // y-th row from top, starting at left
    for (unsigned int x = 0; x < d_width; x++)
    {
      *dst++ = *src;
      src += width;
    }
  }

  delete[] pixels;
  return dest;
}

uint32_t *CPicture::TransposeOffAxis(uint32_t *pixels, unsigned int width, unsigned int height)
{
  uint32_t *dest = new uint32_t[width * height * 4];
  if (!dest)
    return NULL;

  unsigned int d_height = width, d_width = height;
  for (unsigned int y = 0; y < d_height; y++)
  {
    const uint32_t *src = pixels + width * (d_width - 1) + (d_height - 1 - y); // y-th col from right, starting at bottom
    uint32_t *dst = dest + d_width * y;                                        // y-th row, starting at left
    for (unsigned int x = 0; x < d_width; x++)
    {
      *dst++ = *src;
      src -= width;
    }
  }

  delete[] pixels;
  return dest;
}
