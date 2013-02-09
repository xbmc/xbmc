#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "iimage.h"
#include "pictures/DllImageLib.h"

class CXImage : public IImage
{
public:
  CXImage(const std::string& strMimeType);
  ~CXImage();

  virtual bool LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize, unsigned int width, unsigned int height);
  virtual bool Decode(const unsigned char *pixels, unsigned int pitch, unsigned int format);
  virtual bool CreateThumbnailFromSurface(unsigned char* bufferin, unsigned int width, unsigned int height, unsigned int format, unsigned int pitch, const CStdString& destFile, 
                                          unsigned char* &bufferout, unsigned int &bufferoutSize);
  virtual void ReleaseThumbnailBuffer();

private:
  DllImageLib m_dll;
  ImageInfo m_image;
  std::string m_strMimeType;
  BYTE* m_thumbnailbuffer;
};