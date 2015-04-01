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

#include "ThumbLoader.h"
#include "DatabaseManager.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "TextureCache.h"

using namespace std;
using namespace XFILE;

CThumbLoader::CThumbLoader() :
  CBackgroundInfoLoader()
{
}

std::string CThumbLoader::GetCachedImage(const CFileItem &item, const std::string &type)
{
  if (!item.GetPath().empty())
  {
    CTextureDatabase *database = CDatabaseManager::Get().GetTextureDatabase();
    std::string image = database->GetTextureForPath(item.GetPath(), type);
    return image;
  }
  return "";
}

void CThumbLoader::SetCachedImage(const CFileItem &item, const std::string &type, const std::string &image)
{
  if (!item.GetPath().empty())
  {
    CTextureDatabase *database = CDatabaseManager::Get().GetTextureDatabase();
    database->SetTextureForPath(item.GetPath(), type, image);
  }
}

CProgramThumbLoader::CProgramThumbLoader()
{
}

CProgramThumbLoader::~CProgramThumbLoader()
{
}

bool CProgramThumbLoader::LoadItem(CFileItem *pItem)
{
  bool result  = LoadItemCached(pItem);
       result |= LoadItemLookup(pItem);

  return result;
}

bool CProgramThumbLoader::LoadItemCached(CFileItem *pItem)
{
  if (pItem->IsParentFolder())
    return false;

  return FillThumb(*pItem);
}

bool CProgramThumbLoader::LoadItemLookup(CFileItem *pItem)
{
  return false;
}

bool CProgramThumbLoader::FillThumb(CFileItem &item)
{
  // no need to do anything if we already have a thumb set
  std::string thumb = item.GetArt("thumb");

  if (thumb.empty())
  { // see whether we have a cached image for this item
    thumb = GetCachedImage(item, "thumb");
    if (thumb.empty())
    {
      thumb = GetLocalThumb(item);
      if (!thumb.empty())
        SetCachedImage(item, "thumb", thumb);
    }
  }

  if (!thumb.empty())
  {
    CTextureCache::Get().BackgroundCacheImage(thumb);
    item.SetArt("thumb", thumb);
  }
  return true;
}

std::string CProgramThumbLoader::GetLocalThumb(const CFileItem &item)
{
  if (item.IsAddonsPath())
    return "";

  // look for the thumb
  if (item.m_bIsFolder)
  {
    std::string folderThumb = item.GetFolderThumb();
    if (CFile::Exists(folderThumb))
      return folderThumb;
  }
  else
  {
    std::string fileThumb(item.GetTBNFile());
    if (CFile::Exists(fileThumb))
      return fileThumb;
  }
  return "";
}
