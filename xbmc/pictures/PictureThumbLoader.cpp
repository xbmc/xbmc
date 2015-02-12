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

#include "PictureThumbLoader.h"
#include "Picture.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "TextureCache.h"
#include "filesystem/Directory.h"
#include "filesystem/MultiPathDirectory.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "utils/URIUtils.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "video/VideoThumbLoader.h"
#include "URL.h"

using namespace XFILE;
using namespace std;

CPictureThumbLoader::CPictureThumbLoader() : CThumbLoader(), CJobQueue(true, 1, CJob::PRIORITY_LOW_PAUSABLE)
{
  m_regenerateThumbs = false;
}

CPictureThumbLoader::~CPictureThumbLoader()
{
  StopThread();
}

void CPictureThumbLoader::OnLoaderFinish()
{
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
  if (pItem->m_bIsShareOrDrive
  ||  pItem->IsParentFolder())
    return false;

  if (pItem->HasArt("thumb") && m_regenerateThumbs)
  {
    CTextureCache::Get().ClearCachedImage(pItem->GetArt("thumb"));
    if (m_textureDatabase->Open())
    {
      m_textureDatabase->ClearTextureForPath(pItem->GetPath(), "thumb");
      m_textureDatabase->Close();
    }
    pItem->SetArt("thumb", "");
  }

  std::string thumb;
  if (pItem->IsPicture() && !pItem->IsZIP() && !pItem->IsRAR() && !pItem->IsCBZ() && !pItem->IsCBR() && !pItem->IsPlayList())
  { // load the thumb from the image file
    thumb = pItem->HasArt("thumb") ? pItem->GetArt("thumb") : CTextureUtils::GetWrappedThumbURL(pItem->GetPath());
  }
  else if (pItem->IsVideo() && !pItem->IsZIP() && !pItem->IsRAR() && !pItem->IsCBZ() && !pItem->IsCBR() && !pItem->IsPlayList())
  { // video
    CVideoThumbLoader loader;
    if (!loader.FillThumb(*pItem))
    {
      std::string thumbURL = CVideoThumbLoader::GetEmbeddedThumbURL(*pItem);
      if (CTextureCache::Get().HasCachedImage(thumbURL))
      {
        thumb = thumbURL;
      }
      else if (CSettings::Get().GetBool("myvideos.extractthumb") && CSettings::Get().GetBool("myvideos.extractflags"))
      {
        CFileItem item(*pItem);
        CThumbExtractor* extract = new CThumbExtractor(item, pItem->GetPath(), true, thumbURL);
        AddJob(extract);
        thumb.clear();
      }
    }
  }
  else if (!pItem->HasArt("thumb"))
  { // folder, zip, cbz, rar, cbr, playlist may have a previously cached image
    thumb = GetCachedImage(*pItem, "thumb");
  }
  if (!thumb.empty())
  {
    CTextureCache::Get().BackgroundCacheImage(thumb);
    pItem->SetArt("thumb", thumb);
  }
  pItem->FillInDefaultIcon();
  return true;
}

bool CPictureThumbLoader::LoadItemLookup(CFileItem* pItem)
{
  return false;
}

void CPictureThumbLoader::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  if (success)
  {
    CThumbExtractor* loader = (CThumbExtractor*)job;
    loader->m_item.SetPath(loader->m_listpath);
    CFileItemPtr pItem(new CFileItem(loader->m_item));
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, pItem);
    g_windowManager.SendThreadMessage(msg);
  }
  CJobQueue::OnJobComplete(jobID, success, job);
}

void CPictureThumbLoader::ProcessFoldersAndArchives(CFileItem *pItem)
{
  if (pItem->HasArt("thumb"))
    return;

  CTextureDatabase db;
  db.Open();
  if (pItem->IsCBR() || pItem->IsCBZ())
  {
    std::string strTBN(URIUtils::ReplaceExtension(pItem->GetPath(),".tbn"));
    if (CFile::Exists(strTBN))
    {
      db.SetTextureForPath(pItem->GetPath(), "thumb", strTBN);
      CTextureCache::Get().BackgroundCacheImage(strTBN);
      pItem->SetArt("thumb", strTBN);
      return;
    }
  }
  if ((pItem->m_bIsFolder || pItem->IsCBR() || pItem->IsCBZ()) && !pItem->m_bIsShareOrDrive
      && !pItem->IsParentFolder() && !pItem->IsPath("add"))
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
    if (CFile::Exists(thumb))
    {
      db.SetTextureForPath(pItem->GetPath(), "thumb", thumb);
      CTextureCache::Get().BackgroundCacheImage(thumb);
      pItem->SetArt("thumb", thumb);
      return;
    }
    if (!pItem->IsPlugin())
    {
      // we load the directory, grab 4 random thumb files (if available) and then generate
      // the thumb.

      CFileItemList items;

      CDirectory::GetDirectory(pathToUrl, items, g_advancedSettings.m_pictureExtensions, DIR_FLAG_NO_FILE_DIRS);
      
      // create the folder thumb by choosing 4 random thumbs within the folder and putting
      // them into one thumb.
      // count the number of images
      for (int i=0; i < items.Size();)
      {
        if (!items[i]->IsPicture() || items[i]->IsZIP() || items[i]->IsRAR() || items[i]->IsPlayList())
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
          CDirectory::GetDirectory(pathToUrl, items, g_advancedSettings.m_pictureExtensions, DIR_FLAG_NO_FILE_DIRS);
          for (int i=0;i<items.Size();++i)
          {
            CFileItemPtr item = items[i];
            if (item->m_bIsFolder)
            {
              ProcessFoldersAndArchives(item.get());
              pItem->SetArt("thumb", items[i]->GetArt("thumb"));
              pItem->SetIconImage(items[i]->GetIconImage());
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
        items.Sort(SortByLabel, SortOrderAscending);
        std::string thumb = CTextureUtils::GetWrappedThumbURL(items[0]->GetPath());
        db.SetTextureForPath(pItem->GetPath(), "thumb", thumb);
        CTextureCache::Get().BackgroundCacheImage(thumb);
        pItem->SetArt("thumb", thumb);
      }
      else
      {
        // ok, now we've got the files to get the thumbs from, lets create it...
        // we basically load the 4 images and combine them
        vector<string> files;
        for (int thumb = 0; thumb < 4; thumb++)
          files.push_back(items[thumb]->GetPath());
        std::string thumb = CTextureUtils::GetWrappedImageURL(pItem->GetPath(), "picturefolder");
        std::string relativeCacheFile = CTextureCache::GetCacheFile(thumb) + ".png";
        if (CPicture::CreateTiledThumb(files, CTextureCache::GetCachedPath(relativeCacheFile)))
        {
          CTextureDetails details;
          details.file = relativeCacheFile;
          details.width = g_advancedSettings.GetThumbSize();
          details.height = g_advancedSettings.GetThumbSize();
          CTextureCache::Get().AddCachedTexture(thumb, details);
          db.SetTextureForPath(pItem->GetPath(), "thumb", thumb);
          pItem->SetArt("thumb", CTextureCache::GetCachedPath(relativeCacheFile));
        }
      }
    }
    // refill in the icon to get it to update
    pItem->FillInDefaultIcon();
  }
}
