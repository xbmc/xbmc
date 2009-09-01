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
#include "Picture.h"
#include "TextureManager.h"

#ifdef HAS_DX

class CPictureDX : public CPictureBase
{
public:
  CPictureDX(void);
  virtual ~CPictureDX(void);
  virtual CBaseTexture* Load(const CStdString& strFilename, int iMaxWidth = 128, int iMaxHeight = 128);
  bool CreateThumbnailFromSwizzledTexture(CBaseTexture* &texture, int width, int height, const CStdString &thumb);

  // caches a skin image as a thumbnail image
  virtual bool CacheSkinImage(const CStdString &srcFile, const CStdString &destFile);

private:
  struct VERTEX
  {
    D3DXVECTOR4 p;
    D3DCOLOR col;
    FLOAT tu, tv;
  };
  static const DWORD FVF_VERTEX = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;
};

#endif
