/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PictureThumbLoader.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "Picture.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/MultiPathDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "imagefiles/ImageFileURL.h"
#include "playlists/PlayListFileItemClassify.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ArtTypes.h"
#include "utils/ArtUtils.h"
#include "utils/FileExtensionProvider.h"
#include "utils/FileUtils.h"
#include "utils/PlaceholderPaths.h"
#include "utils/URIUtils.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoThumbLoader.h"

using namespace KODI;
using namespace XFILE;

CPictureThumbLoader::CPictureThumbLoader() : CThumbLoader()
{
  m_regenerateThumbs = false;
}

CPictureThumbLoader::~CPictureThumbLoader()
{
  StopThread();
}

void CPictureThumbLoader::OnLoaderFinish()
{
  if (m_regenerateThumbs)
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  }
  m_regenerateThumbs = false;
  CThumbLoader::OnLoaderFinish();
}

bool CPictureThumbLoader::LoadItem(CFileItem* pItem)
{
  bool result  = LoadItemCached(pItem);
       result |= LoadItemLookup(pItem);

  return result;
}

bool CPictureThumbLoader::LoadItemCached(CFileItem* pItem)
{
  if (pItem->IsShareOrDrive() || pItem->IsParentFolder())
    return false;

  if (pItem->HasArt(ART::TYPE::THUMB) && m_regenerateThumbs)
  {
    CServiceBroker::GetTextureCache()->ClearCachedImage(pItem->GetArt(ART::TYPE::THUMB));
    if (m_textureDatabase->Open())
    {
      m_textureDatabase->ClearTextureForPath(pItem->GetPath(), ART::TYPE::THUMB);
      m_textureDatabase->Close();
    }
    pItem->SetArt(ART::TYPE::THUMB, "");
  }

  std::string thumb;
  if (pItem->IsPicture() && !pItem->IsZIP() && !pItem->IsRAR() && !pItem->IsCBZ() &&
      !pItem->IsCBR() && !PLAYLIST::IsPlayList(*pItem))
  { // load the thumb from the image file
    thumb = pItem->HasArt(ART::TYPE::THUMB) ? pItem->GetArt(ART::TYPE::THUMB)
                                            : IMAGE_FILES::URLFromFile(pItem->GetPath());
  }
  else if (VIDEO::IsVideo(*pItem) && !pItem->IsZIP() && !pItem->IsRAR() && !pItem->IsCBZ() &&
           !pItem->IsCBR() && !PLAYLIST::IsPlayList(*pItem))
  { // video
    CVideoThumbLoader loader;
    loader.LoadItem(pItem);
  }
  else if (!pItem->HasArt(ART::TYPE::THUMB))
  { // folder, zip, cbz, rar, cbr, playlist may have a previously cached image
    thumb = GetCachedImage(*pItem, ART::TYPE::THUMB);
  }
  if (!thumb.empty())
  {
    CServiceBroker::GetTextureCache()->BackgroundCacheImage(thumb);
    pItem->SetArt(ART::TYPE::THUMB, thumb);
  }
  ART::FillInDefaultIcon(*pItem);
  return true;
}

bool CPictureThumbLoader::LoadItemLookup(CFileItem* pItem)
{
  return false;
}

void CPictureThumbLoader::ProcessFoldersAndArchives(CFileItem *pItem)
{
  if (pItem->HasArt(ART::TYPE::THUMB))
    return;

  CTextureDatabase db;
  db.Open();
  if (pItem->IsCBR() || pItem->IsCBZ())
  {
    std::string strTBN(URIUtils::ReplaceExtension(pItem->GetPath(),".tbn"));
    if (CFileUtils::Exists(strTBN))
    {
      db.SetTextureForPath(pItem->GetPath(), ART::TYPE::THUMB, strTBN);
      CServiceBroker::GetTextureCache()->BackgroundCacheImage(strTBN);
      pItem->SetArt(ART::TYPE::THUMB, strTBN);
      return;
    }
  }
  if ((pItem->IsFolder() || pItem->IsCBR() || pItem->IsCBZ()) && !pItem->IsShareOrDrive() &&
      !pItem->IsParentFolder() && !pItem->IsPath(PLACEHOLDER::ADD_SOURCE))
  {
    // first check for a folder.jpg
    std::string thumb = "folder.jpg";
    CURL pathToUrl = pItem->GetURL();
    if (pItem->IsCBR())
    {
      pathToUrl = URIUtils::CreateArchivePath("rar",pItem->GetURL(),"");
      thumb = "cover.jpg";
    }
    if (pItem->IsCBZ())
    {
      pathToUrl = URIUtils::CreateArchivePath("zip",pItem->GetURL(),"");
      thumb = "cover.jpg";
    }
    if (pItem->IsMultiPath())
      pathToUrl = CURL(CMultiPathDirectory::GetFirstPath(pItem->GetPath()));
    thumb = URIUtils::AddFileToFolder(pathToUrl.Get(), thumb);
    if (CFileUtils::Exists(thumb))
    {
      db.SetTextureForPath(pItem->GetPath(), ART::TYPE::THUMB, thumb);
      CServiceBroker::GetTextureCache()->BackgroundCacheImage(thumb);
      pItem->SetArt(ART::TYPE::THUMB, thumb);
      return;
    }
    if (!pItem->IsPlugin())
    {
      // we load the directory, grab 4 random thumb files (if available) and then generate
      // the thumb.

      CFileItemList items;

      CDirectory::GetDirectory(pathToUrl, items, CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(), DIR_FLAG_NO_FILE_DIRS);

      // create the folder thumb by choosing 4 random thumbs within the folder and putting
      // them into one thumb.
      // count the number of images
      for (int i=0; i < items.Size();)
      {
        if (!items[i]->IsPicture() || items[i]->IsZIP() || items[i]->IsRAR() ||
            PLAYLIST::IsPlayList(*items[i]))
        {
          items.Remove(i);
        }
        else
          i++;
      }

      if (items.IsEmpty())
      {
        if (pItem->IsCBZ() || pItem->IsCBR())
        {
          CDirectory::GetDirectory(pathToUrl, items, CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(), DIR_FLAG_NO_FILE_DIRS);
          for (int i=0;i<items.Size();++i)
          {
            CFileItemPtr item = items[i];
            if (item->IsFolder())
            {
              ProcessFoldersAndArchives(item.get());
              pItem->SetArt(ART::TYPE::THUMB, items[i]->GetArt(ART::TYPE::THUMB));
              pItem->SetArt(ART::TYPE::ICON, items[i]->GetArt(ART::TYPE::ICON));
              return;
            }
          }
        }
        return; // no images in this folder
      }

      // randomize them
      items.Randomize();

      if (items.Size() < 4 || pItem->IsCBR() || pItem->IsCBZ())
      { // less than 4 items, so just grab the first thumb
        items.Sort(SortBy::LABEL, SortOrder::ASCENDING);
        std::string thumb = IMAGE_FILES::URLFromFile(items[0]->GetPath());
        db.SetTextureForPath(pItem->GetPath(), ART::TYPE::THUMB, thumb);
        CServiceBroker::GetTextureCache()->BackgroundCacheImage(thumb);
        pItem->SetArt(ART::TYPE::THUMB, thumb);
      }
      else
      {
        std::string thumb = IMAGE_FILES::URLFromFile(pItem->GetPath(), "picturefolder");
        db.SetTextureForPath(pItem->GetPath(), ART::TYPE::THUMB, thumb);
        pItem->SetArt(ART::TYPE::THUMB, thumb);
      }
    }
    // refill in the icon to get it to update
    ART::FillInDefaultIcon(*pItem);
  }
}
