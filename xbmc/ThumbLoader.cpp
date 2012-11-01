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

#include "ThumbLoader.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "TextureCache.h"

using namespace std;
using namespace XFILE;

CThumbLoader::CThumbLoader(int nThreads) :
  CBackgroundInfoLoader(nThreads)
{
}

CThumbLoader::~CThumbLoader()
{
}

CStdString CThumbLoader::GetCachedImage(const CFileItem &item, const CStdString &type)
{
  CTextureDatabase db;
  if (!item.GetPath().empty() && db.Open())
    return db.GetTextureForPath(item.GetPath(), type);
  return "";
}

void CThumbLoader::SetCachedImage(const CFileItem &item, const CStdString &type, const CStdString &image)
{
  CTextureDatabase db;
  if (!item.GetPath().empty() && db.Open())
    db.SetTextureForPath(item.GetPath(), type, image);
}

CProgramThumbLoader::CProgramThumbLoader()
{
}

CProgramThumbLoader::~CProgramThumbLoader()
{
}

bool CProgramThumbLoader::LoadItem(CFileItem *pItem)
{
  if (pItem->IsParentFolder()) return true;
  return FillThumb(*pItem);
}

bool CProgramThumbLoader::FillThumb(CFileItem &item)
{
  // no need to do anything if we already have a thumb set
  CStdString thumb = item.GetArt("thumb");

  if (thumb.IsEmpty())
  { // see whether we have a cached image for this item
    thumb = GetCachedImage(item, "thumb");
    if (thumb.IsEmpty())
    {
      thumb = GetLocalThumb(item);
      if (!thumb.IsEmpty())
        SetCachedImage(item, "thumb", thumb);
    }
  }

  if (!thumb.IsEmpty())
  {
    CTextureCache::Get().BackgroundCacheImage(thumb);
    item.SetArt("thumb", thumb);
  }
  return true;
}

CStdString CProgramThumbLoader::GetLocalThumb(const CFileItem &item)
{
  // look for the thumb
  if (item.m_bIsFolder)
  {
    CStdString folderThumb = item.GetFolderThumb();
    if (CFile::Exists(folderThumb))
      return folderThumb;
  }
  else
  {
    CStdString fileThumb(item.GetTBNFile());
    if (CFile::Exists(fileThumb))
      return fileThumb;
  }
  return "";
}
