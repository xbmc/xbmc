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

class CAutoTexBuffer;

class CTextureBundle
{
  struct FileHeader_t
  {
    DWORD Offset;
    DWORD UnpackedSize;
    DWORD PackedSize;
  };

#ifndef _LINUX
  HANDLE m_hFile;
  FILETIME m_TimeStamp;
  OVERLAPPED m_Ovl[2];
#else
  FILE*  m_hFile;
  time_t m_TimeStamp;
#endif  
  std::map<CStdString, FileHeader_t> m_FileHeaders;
  std::map<CStdString, FileHeader_t>::iterator m_CurFileHeader[2];
  BYTE* m_PreLoadBuffer[2];
  int m_PreloadIdx;
  int m_LoadIdx;
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
  bool PreloadFile(const CStdString& Filename);
  static CStdString Normalize(const CStdString &name);

#ifndef HAS_SDL
  HRESULT LoadTexture(LPDIRECT3DDEVICE8 pDevice, const CStdString& Filename, D3DXIMAGE_INFO* pInfo, LPDIRECT3DTEXTURE8* ppTexture,
                      LPDIRECT3DPALETTE8* ppPalette);

  int LoadAnim(LPDIRECT3DDEVICE8 pDevice, const CStdString& Filename, D3DXIMAGE_INFO* pInfo, LPDIRECT3DTEXTURE8** ppTextures,
               LPDIRECT3DPALETTE8* ppPalette, int& nLoops, int** ppDelays);
#else
  HRESULT LoadTexture(const CStdString& Filename, D3DXIMAGE_INFO* pInfo, SDL_Surface** ppTexture,
                      SDL_Palette** ppPalette);

  int LoadAnim(const CStdString& Filename, D3DXIMAGE_INFO* pInfo, SDL_Surface*** ppTextures,
               SDL_Palette** ppPalette, int& nLoops, int** ppDelays);
#endif
};

