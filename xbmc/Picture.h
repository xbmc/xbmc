#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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
#include "StdString.h"
#include "Job.h"

class CPicture
{
public:
  static bool CreateThumbnailFromMemory(const unsigned char* buffer, int bufSize, const CStdString& extension, const CStdString& thumbFile);
  static bool CreateThumbnailFromSurface(const unsigned char* buffer, int width, int height, int stride, const CStdString &thumbFile);
  static int ConvertFile(const CStdString& srcFile, const CStdString& destFile, float rotateDegrees, int width, int height, unsigned int quality, bool mirror=false);

  static void CreateFolderThumb(const CStdString *thumbs, const CStdString &folderThumb);
  static bool CreateThumbnail(const CStdString& file, const CStdString& thumbFile, bool checkExistence = false);
  static bool CacheThumb(const CStdString& sourceUrl, const CStdString& destFile);
  static bool CacheFanart(const CStdString& SourceUrl, const CStdString& destFile);

private:
  static bool CacheImage(const CStdString& sourceUrl, const CStdString& destFile, int width, int height);
};

//this class calls CreateThumbnailFromSurface in a CJob, so a png file can be written without halting the render thread
class CThumbnailWriter : public CJob
{
  public:
    //WARNING: buffer is deleted from DoWork()
    CThumbnailWriter(unsigned char* buffer, int width, int height, int stride, const CStdString& thumbFile);
    bool DoWork();

  private:
    unsigned char* m_buffer;
    int            m_width;
    int            m_height;
    int            m_stride;
    CStdString     m_thumbFile;
};

