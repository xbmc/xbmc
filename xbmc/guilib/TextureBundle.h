#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#ifndef GUILIB_UTILS_STDSTRING_H_INCLUDED
#define GUILIB_UTILS_STDSTRING_H_INCLUDED
#include "utils/StdString.h"
#endif

#ifndef GUILIB_TEXTUREBUNDLEXPR_H_INCLUDED
#define GUILIB_TEXTUREBUNDLEXPR_H_INCLUDED
#include "TextureBundleXPR.h"
#endif

#ifndef GUILIB_TEXTUREBUNDLEXBT_H_INCLUDED
#define GUILIB_TEXTUREBUNDLEXBT_H_INCLUDED
#include "TextureBundleXBT.h"
#endif


class CTextureBundle
{
public:
  CTextureBundle(void);
  ~CTextureBundle(void);

  void Cleanup();

  void SetThemeBundle(bool themeBundle);
  bool HasFile(const CStdString& Filename);
  void GetTexturesFromPath(const CStdString &path, std::vector<CStdString> &textures);
  static CStdString Normalize(const CStdString &name);

  bool LoadTexture(const CStdString& Filename, CBaseTexture** ppTexture, int &width, int &height);

  int LoadAnim(const CStdString& Filename, CBaseTexture*** ppTextures, int &width, int &height, int& nLoops, int** ppDelays);

private:
  CTextureBundleXPR m_tbXPR;
  CTextureBundleXBT m_tbXBT;

  bool m_useXPR;
  bool m_useXBT;
};


