/*
 *      Copyright (C) 2012-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "cximage.h"
#include "utils/log.h"

CXImage::CXImage(const std::string& strMimeType): m_strMimeType(strMimeType), m_thumbnailbuffer(NULL)
{
  m_hasAlpha = false;
  memset(&m_image, 0, sizeof(m_image));
  m_dll.Load();
}

CXImage::~CXImage()
{
  if (m_dll.IsLoaded()) 
  {
    m_dll.FreeMemory(m_thumbnailbuffer);
    m_dll.ReleaseImage(&m_image);
    m_dll.Unload();
  }
}

bool CXImage::LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize, unsigned int width, unsigned int height)
{
  if (!m_dll.IsLoaded())
    return false;

  memset(&m_image, 0, sizeof(m_image));

  std::string strExt = m_strMimeType;
  size_t nPos = strExt.find('/');
  if (nPos != std::string::npos)
    strExt.erase(0, nPos + 1);

  if(!m_dll.LoadImageFromMemory(buffer, bufSize, strExt.c_str(), width, height, &m_image))
  {
    CLog::Log(LOGERROR, "Texture manager unable to load image from memory");
    return false;
  }

  m_hasAlpha = NULL != m_image.alpha;
  m_width = m_image.width;
  m_height = m_image.height;
  m_orientation = m_image.exifInfo.Orientation;
  m_originalWidth = m_image.originalwidth;
  m_originalHeight = m_image.originalheight;
  return true;
}

bool CXImage::Decode(const unsigned char *pixels, unsigned int pitch, unsigned int format)
{
  if (m_image.width == 0 || m_image.height == 0 || !m_dll.IsLoaded())
    return false;

  unsigned int dstPitch = pitch;
  unsigned int srcPitch = ((m_image.width + 1)* 3 / 4) * 4; // bitmap row length is aligned to 4 bytes

  unsigned char *dst = (unsigned char*)pixels;
  unsigned char *src = m_image.texture + (m_height - 1) * srcPitch;

  for (unsigned int y = 0; y < m_height; y++)
  {
    unsigned char *dst2 = dst;
    unsigned char *src2 = src;
    for (unsigned int x = 0; x < m_width; x++, dst2 += 4, src2 += 3)
    {
      dst2[0] = src2[0];
      dst2[1] = src2[1];
      dst2[2] = src2[2];
      dst2[3] = 0xff;
    }
    src -= srcPitch;
    dst += dstPitch;
  }

  if(m_image.alpha)
  {
    dst = (unsigned char*)pixels + 3;
    src = m_image.alpha + (m_height - 1) * m_width;

    for (unsigned int y = 0; y < m_height; y++)
    {
      unsigned char *dst2 = dst;
      unsigned char *src2 = src;

      for (unsigned int x = 0; x < m_width; x++,  dst2+=4, src2++)
        *dst2 = *src2;
      src -= m_width;
      dst += dstPitch;
    }
  }

  m_dll.ReleaseImage(&m_image);
  memset(&m_image, 0, sizeof(m_image));
  return true;
}

bool CXImage::CreateThumbnailFromSurface(unsigned char* bufferin, unsigned int width, unsigned int height, unsigned int format, unsigned int pitch, const std::string& destFile, 
                                         unsigned char* &bufferout, unsigned int &bufferoutSize)
{
  if (!bufferin || !m_dll.IsLoaded()) 
    return false;

  bool ret = m_dll.CreateThumbnailFromSurface2((BYTE *)bufferin, width, height, pitch, destFile.c_str(), m_thumbnailbuffer, bufferoutSize);
  bufferout = m_thumbnailbuffer;
  return ret;
}

void CXImage::ReleaseThumbnailBuffer()
{
  if (!m_dll.IsLoaded())
    return;

  m_dll.FreeMemory(m_thumbnailbuffer);
  m_thumbnailbuffer = NULL;
}
