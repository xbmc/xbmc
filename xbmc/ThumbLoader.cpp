/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ThumbLoader.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "utils/FileUtils.h"

CThumbLoader::CThumbLoader() :
  CBackgroundInfoLoader()
{
  m_textureDatabase = new CTextureDatabase();
}

CThumbLoader::~CThumbLoader()
{
  delete m_textureDatabase;
}

void CThumbLoader::OnLoaderStart()
{
  m_textureDatabase->Open();
}

void CThumbLoader::OnLoaderFinish()
{
  m_textureDatabase->Close();
}

std::string CThumbLoader::GetCachedImage(const CFileItem &item, const std::string &type)
{
  if (!item.GetPath().empty() && m_textureDatabase->Open())
  {
    std::string image = m_textureDatabase->GetTextureForPath(item.GetPath(), type);
    m_textureDatabase->Close();
    return image;
  }
  return "";
}

void CThumbLoader::SetCachedImage(const CFileItem &item, const std::string &type, const std::string &image)
{
  if (!item.GetPath().empty() && m_textureDatabase->Open())
  {
    m_textureDatabase->SetTextureForPath(item.GetPath(), type, image);
    m_textureDatabase->Close();
  }
}

CProgramThumbLoader::CProgramThumbLoader() = default;

CProgramThumbLoader::~CProgramThumbLoader() = default;

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
    CServiceBroker::GetTextureCache()->BackgroundCacheImage(thumb);
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
    if (CFileUtils::Exists(folderThumb))
      return folderThumb;
  }
  else
  {
    std::string fileThumb(item.GetTBNFile());
    if (CFileUtils::Exists(fileThumb))
      return fileThumb;
  }
  return "";
}
