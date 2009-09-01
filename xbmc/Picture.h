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
#include "DllImageLib.h"

class CBaseTexture;

class CPicture
{
public:
  CPicture(void);
  virtual ~CPicture(void);
  virtual bool Load(const CStdString& strFilename, CBaseTexture* texture, int iMaxWidth = 128, int iMaxHeight = 128);

  bool CreateThumbnailFromMemory(const BYTE* pBuffer, int nBufSize, const CStdString& strExtension, const CStdString& strThumbFileName);
  bool CreateThumbnailFromSurface(BYTE* pBuffer, int width, int height, int stride, const CStdString &strThumbFileName);
  int ConvertFile(const CStdString& srcFile, const CStdString& destFile, float rotateDegrees, int width, int height, unsigned int quality, bool mirror=false);

  ImageInfo GetInfo() const { return m_info; };
  unsigned int GetWidth() const { return m_info.width; };
  unsigned int GetHeight() const { return m_info.height; };
  unsigned int GetOriginalWidth() const { return m_info.originalwidth; };
  unsigned int GetOriginalHeight() const { return m_info.originalheight; };
  const EXIFINFO *GetExifInfo() const { return &m_info.exifInfo; };

  void CreateFolderThumb(const CStdString *strThumbs, const CStdString &folderThumbnail);
  bool DoCreateThumbnail(const CStdString& strFileName, const CStdString& strThumbFileName, bool checkExistence = false);
  bool CacheImage(const CStdString& sourceFileName, const CStdString& destFileName);

protected:
  DllImageLib m_dll;
  ImageInfo m_info;
};
