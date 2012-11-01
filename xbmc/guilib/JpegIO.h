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

#ifndef GUILIB_JPEGIO_H
#define GUILIB_JPEGIO_H

#ifdef TARGET_WINDOWS
#define XMD_H
#pragma comment(lib, "turbojpeg-static.lib")
#endif

#include <jpeglib.h>
#include "utils/StdString.h"

class CJpegIO
{

public:
  CJpegIO();
  ~CJpegIO();
  bool           Open(const CStdString& m_texturePath,  unsigned int minx=0, unsigned int miny=0, bool read=true);
  bool           Read(unsigned char* buffer, unsigned int bufSize, unsigned int minx, unsigned int miny);
  bool           Decode(const unsigned char *pixels, unsigned int pitch, unsigned int format);
  bool           CreateThumbnail(const CStdString& sourceFile, const CStdString& destFile, int minx, int miny, bool rotateExif);
  bool           CreateThumbnailFromMemory(unsigned char* buffer, unsigned int bufSize, const CStdString& destFile, unsigned int minx, unsigned int miny);
  bool           CreateThumbnailFromSurface(unsigned char* buffer, unsigned int width, unsigned int height, unsigned int format, unsigned int pitch, const CStdString& destFile);
  void           Close();

  unsigned int   Width()       { return m_width; }
  unsigned int   Height()      { return m_height; }
  unsigned int   Orientation() { return m_orientation; }

protected:
  static  void   jpeg_error_exit(j_common_ptr cinfo);

  unsigned int   GetExifOrientation(unsigned char* exif_data, unsigned int exif_data_size);

  unsigned char  *m_inputBuff;
  unsigned int   m_inputBuffSize;
  struct         jpeg_decompress_struct m_cinfo;
  CStdString     m_texturePath;

  unsigned int   m_width;
  unsigned int   m_height;
  unsigned int   m_orientation;
};

#endif
