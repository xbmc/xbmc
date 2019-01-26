/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureBundle.h"

CTextureBundle::CTextureBundle()
  : m_tbXBT{false}
	, m_useXBT{false}
{
}

CTextureBundle::CTextureBundle(bool useXBT)
  : m_tbXBT{useXBT}
	, m_useXBT{useXBT}
{
}

bool CTextureBundle::HasFile(const std::string& Filename)
{
  if (m_useXBT)
  {
    return m_tbXBT.HasFile(Filename);
  }

  if (m_tbXBT.HasFile(Filename))
  {
    m_useXBT = true;
    return true;
  }

  return false;
}

void CTextureBundle::GetTexturesFromPath(const std::string &path, std::vector<std::string> &textures)
{
  if (m_useXBT)
  {
    m_tbXBT.GetTexturesFromPath(path, textures);
  }
}

bool CTextureBundle::LoadTexture(const std::string& Filename, CBaseTexture** ppTexture,
                                     int &width, int &height)
{
  if (m_useXBT)
  {
    return m_tbXBT.LoadTexture(Filename, ppTexture, width, height);
  }

  return false;
}

int CTextureBundle::LoadAnim(const std::string& Filename, CBaseTexture*** ppTextures,
                              int &width, int &height, int& nLoops, int** ppDelays)
{
  if (m_useXBT)
  {
    return m_tbXBT.LoadAnim(Filename, ppTextures, width, height, nLoops, ppDelays);
  }

  return 0;
}

void CTextureBundle::Close()
{
  m_tbXBT.CloseBundle();
}

void CTextureBundle::SetThemeBundle(bool themeBundle)
{
  m_tbXBT.SetThemeBundle(themeBundle);
}

std::string CTextureBundle::Normalize(const std::string &name)
{
  return CTextureBundleXBT::Normalize(name);
}
