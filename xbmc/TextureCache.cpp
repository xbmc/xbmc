/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "TextureCache.h"
#include "FileSystem/File.h"
#include "Util.h"
#include "AdvancedSettings.h"

#include "Texture.h"
#include "DDSImage.h"

CTextureCache &CTextureCache::Get()
{
  static CTextureCache s_cache;
  return s_cache;
}

CTextureCache::CTextureCache()
{
}

CTextureCache::~CTextureCache()
{
}

CStdString CTextureCache::CheckAndCacheImage(const CStdString &url)
{
  if (CUtil::IsSpecial(url))
  { // already cached
    if (CUtil::GetExtension(url).Equals(".tbn"))
    { // check to see whether we have a .dds version
      CStdString ddsURL = CUtil::ReplaceExtension(url, ".dds");
      if (XFILE::CFile::Exists(ddsURL))
        return ddsURL;
      else if (g_advancedSettings.m_useDDSFanart && CacheDDS(url, ddsURL))
        return ddsURL;
    }
  }
  return url;
}

bool CTextureCache::CacheDDS(const CStdString &image, const CStdString &ddsImage, double maxMSE) const
{
  CTexture texture;
  if (texture.LoadFromFile(image))
  { // convert to DDS
    CDDSImage image;
    if (image.Compress(texture.GetWidth(), texture.GetHeight(), texture.GetPitch(), texture.GetPixels(), maxMSE))
    {
      image.WriteFile(ddsImage);
      return true;
    }
  }
  return false;
}
