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

#include "Texture.h"

class CAutoTexBuffer;

class CTextureBundle
{
  struct FileHeader_t
  {
    DWORD Offset;
    DWORD UnpackedSize;
    DWORD PackedSize;
  };

  FILE*  m_hFile;
  time_t m_TimeStamp;

  std::map<CStdString, FileHeader_t> m_FileHeaders;
  typedef std::map<CStdString, FileHeader_t>::iterator iFiles;

  bool m_themeBundle;

  bool OpenBundle();
  HRESULT LoadFile(const CStdString& Filename, CAutoTexBuffer& UnpackedBuf);

public:
  CTextureBundle(void);
  ~CTextureBundle(void);

  void Cleanup();

  void SetThemeBundle(bool themeBundle);
  bool HasFile(const CStdString& Filename);
  void GetTexturesFromPath(const CStdString &path, std::vector<CStdString> &textures);
  static CStdString Normalize(const CStdString &name);

  HRESULT LoadTexture(const CStdString& Filename, CBaseTexture** ppTexture,
                       XBMC::PalettePtr* ppPalette);

  int LoadAnim(const CStdString& Filename, CBaseTexture** ppTextures,
                XBMC::PalettePtr* ppPalette, int& nLoops, int** ppDelays);
};

