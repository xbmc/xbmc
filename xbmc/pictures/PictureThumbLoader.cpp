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

#include "PictureThumbLoader.h"
#include "Picture.h"
#include "URL.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "TextureCache.h"
#include "filesystem/Directory.h"
#include "filesystem/MultiPathDirectory.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "settings/GUISettings.h"
#include "utils/URIUtils.h"
#include "settings/Settings.h"

using namespace XFILE;
using namespace std;

CPictureThumbLoader::CPictureThumbLoader() : CThumbLoader(1), CJobQueue(true)
{
  m_regenerateThumbs = false;
}

CPictureThumbLoader::~CPictureThumbLoader()
{
  StopThread();
}

bool CPictureThumbLoader::LoadItem(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive) return true;
  if (pItem->IsParentFolder()) return true;

  if (pItem->HasThumbnail() && m_regenerateThumbs)
  {
    CTextureCache::Get().ClearCachedImage(pItem->GetThumbnailImage());
    CTextureDatabase db;
    if (db.Open())
      db.ClearTextureForPath(pItem->GetPath(), "thumb");
    pItem->SetThumbnailImage("");
  }

  CStdString thumb;
  if (pItem->IsPicture() && !pItem->IsZIP() && !pItem->IsRAR() && !pItem->IsCBZ() && !pItem->IsCBR() && !pItem->IsPlayList())
  { // load the thumb from the image file
    thumb = pItem->HasThumbnail() ? pItem->GetThumbnailImage() : CTextureCache::GetWrappedThumbURL(pItem->GetPath());
  }
  else if (pItem->IsVideo() && !pItem->IsZIP() && !pItem->IsRAR() && !pItem->IsCBZ() && !pItem->IsCBR() && !pItem->IsPlayList())
  { // video
    thumb = pItem->GetCachedVideoThumb();
    if (!CFile::Exists(thumb))
    {
      CStdString strPath, strFileName;
      URIUtils::Split(thumb, strPath, strFileName);

      CStdString autoThumb = strPath + "auto-" + strFileName;

      // this is abit of a hack to avoid loading zero sized images
      // which we know will fail. They will just display empty image
      // we should really have some way for the texture loader to
      // do fallbacks to default images for a failed image instead
      if (CFile::Exists(autoThumb))
      {
        thumb = autoThumb;
      }
      else if (g_guiSettings.GetBool("myvideos.extractthumb") && g_guiSettings.GetBool("myvideos.extractflags"))
      {
        CFileItem item(*pItem);
        CThumbExtractor* extract = new CThumbExtractor(item, pItem->GetPath(), true, autoThumb);
        AddJob(extract);
        thumb.clear();
      }
    }
  }
  else if (!pItem->HasThumbnail())
  { // folder, zip, cbz, rar, cbr, playlist may have a previously cached image
    thumb = GetCachedImage(*pItem, "thumb");
  }
  if (!thumb.IsEmpty())
  {
    CTextureCache::Get().BackgroundCacheImage(thumb);
    pItem->SetThumbnailImage(thumb);
  }
  pItem->FillInDefaultIcon();
  return true;
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


void CPictureThumbLoader::OnLoaderFinish()
{
  m_regenerateThumbs = false;
}

void CPictureThumbLoader::ProcessFoldersAndArchives(CFileItem *pItem)
{
  if (pItem->HasThumbnail())
    return;

  CTextureDatabase db;
  db.Open();
  if (pItem->IsCBR() || pItem->IsCBZ())
  {
    CStdString strTBN(URIUtils::ReplaceExtension(pItem->GetPath(),".tbn"));
    if (CFile::Exists(strTBN))
    {
      db.SetTextureForPath(pItem->GetPath(), "thumb", strTBN);
      CTextureCache::Get().BackgroundCacheImage(strTBN);
      pItem->SetThumbnailImage(strTBN);
      return;
    }
  }
  if ((pItem->m_bIsFolder || pItem->IsCBR() || pItem->IsCBZ()) && !pItem->m_bIsShareOrDrive && !pItem->IsParentFolder())
  {
    // first check for a folder.jpg
    CStdString thumb = "folder.jpg";
    CStdString strPath = pItem->GetPath();
    if (pItem->IsCBR())
    {
      URIUtils::CreateArchivePath(strPath,"rar",pItem->GetPath(),"");
      thumb = "cover.jpg";
    }
    if (pItem->IsCBZ())
    {
      URIUtils::CreateArchivePath(strPath,"zip",pItem->GetPath(),"");
      thumb = "cover.jpg";
    }
    if (pItem->IsMultiPath())
      strPath = CMultiPathDirectory::GetFirstPath(pItem->GetPath());
    thumb = URIUtils::AddFileToFolder(strPath, thumb);
    if (CFile::Exists(thumb))
    {
      db.SetTextureForPath(pItem->GetPath(), "thumb", thumb);
      CTextureCache::Get().BackgroundCacheImage(thumb);
      pItem->SetThumbnailImage(thumb);
      return;
    }
    if (!pItem->IsPlugin())
    {
      // we load the directory, grab 4 random thumb files (if available) and then generate
      // the thumb.

      CFileItemList items;

      CDirectory::GetDirectory(strPath, items, g_settings.m_pictureExtensions, DIR_FLAG_NO_FILE_DIRS);
      
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
          CDirectory::GetDirectory(strPath, items, g_settings.m_pictureExtensions, DIR_FLAG_NO_FILE_DIRS);
          for (int i=0;i<items.Size();++i)
          {
            CFileItemPtr item = items[i];
            if (item->m_bIsFolder)
            {
              ProcessFoldersAndArchives(item.get());
              pItem->SetThumbnailImage(items[i]->GetThumbnailImage());
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
        items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
        CStdString thumb = CTextureCache::GetWrappedThumbURL(items[0]->GetPath());
        db.SetTextureForPath(pItem->GetPath(), "thumb", thumb);
        CTextureCache::Get().BackgroundCacheImage(thumb);
        pItem->SetThumbnailImage(thumb);
      }
      else
      {
        // ok, now we've got the files to get the thumbs from, lets create it...
        // we basically load the 4 images and combine them
        vector<string> files;
        for (int thumb = 0; thumb < 4; thumb++)
          files.push_back(items[thumb]->GetPath());
        CStdString thumb = CTextureCache::GetWrappedImageURL(pItem->GetPath(), "picturefolder");
        CStdString relativeCacheFile = CTextureCache::GetCacheFile(thumb) + ".png";
        if (CPicture::CreateTiledThumb(files, CTextureCache::GetCachedPath(relativeCacheFile)))
        {
          CTextureCache::Get().AddCachedTexture(thumb, relativeCacheFile, "");
          db.SetTextureForPath(pItem->GetPath(), "thumb", thumb);
          pItem->SetThumbnailImage(CTextureCache::GetCachedPath(relativeCacheFile));
        }
      }
    }
    // refill in the icon to get it to update
    pItem->FillInDefaultIcon();
  }
}
