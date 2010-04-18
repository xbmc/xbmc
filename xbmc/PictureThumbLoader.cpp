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
#include "FileSystem/File.h"
#include "FileItem.h"
#include "VideoInfoTag.h"
#include "TextureManager.h"
#include "TextureCache.h"
#include "FileSystem/Directory.h"
#include "FileSystem/MultiPathDirectory.h"
#include "Util.h"
#include "Settings.h"

using namespace XFILE;

CPictureThumbLoader::CPictureThumbLoader()
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

  if (pItem->HasThumbnail())
  {
    CStdString thumb(pItem->GetThumbnailImage());

    // look for remote thumbs
    if (!g_TextureManager.CanLoad(thumb))
    {
      thumb = CTextureCache::Get().CheckAndCacheImage(thumb);
      pItem->SetThumbnailImage(thumb);
      pItem->FillInDefaultIcon();
      return !thumb.IsEmpty();
    }
    else if (m_regenerateThumbs)
    {
      CTextureCache::Get().ClearCachedImage(thumb);
      pItem->SetThumbnailImage("");
    }
  }

  if (pItem->IsPicture() && !pItem->IsZIP() && !pItem->IsRAR() && !pItem->IsCBZ() && !pItem->IsCBR() && !pItem->IsPlayList())
  { // load the thumb from the image file
    CStdString image = pItem->HasThumbnail() ? pItem->GetThumbnailImage() : CTextureCache::GetWrappedThumbURL(pItem->m_strPath);
    CStdString thumb = CTextureCache::Get().CheckAndCacheImage(image);
    if (!thumb.IsEmpty())
      pItem->SetThumbnailImage(thumb);
    pItem->FillInDefaultIcon();
  }
  else
  { // folder, zip, cbz, rar, cbr, playlist
  }
  return true;
}

void CPictureThumbLoader::OnLoaderFinish()
{
  m_regenerateThumbs = false;
}

void CPictureThumbLoader::ProcessFoldersAndArchives(CFileItem *pItem)
{
  if (pItem->IsCBR() || pItem->IsCBZ())
  {
    CStdString strTBN(CUtil::ReplaceExtension(pItem->m_strPath,".tbn"));
    if (CFile::Exists(strTBN))
    {
      if (CPicture::CreateThumbnail(strTBN, pItem->GetCachedPictureThumb(),true))
      {
        pItem->SetCachedPictureThumb();
        pItem->FillInDefaultIcon();
        return;
      }
    }
  }
  if ((pItem->m_bIsFolder || pItem->IsCBR() || pItem->IsCBZ()) && !pItem->m_bIsShareOrDrive && !pItem->HasThumbnail() && !pItem->IsParentFolder())
  {
    // first check for a folder.jpg
    CStdString thumb = "folder.jpg";
    CStdString strPath = pItem->m_strPath;
    if (pItem->IsCBR())
    {
      CUtil::CreateArchivePath(strPath,"rar",pItem->m_strPath,"");
      thumb = "cover.jpg";
    }
    if (pItem->IsCBZ())
    {
      CUtil::CreateArchivePath(strPath,"zip",pItem->m_strPath,"");
      thumb = "cover.jpg";
    }
    if (pItem->IsMultiPath())
      strPath = CMultiPathDirectory::GetFirstPath(pItem->m_strPath);
    thumb = CUtil::AddFileToFolder(strPath, thumb);
    if (CFile::Exists(thumb))
      CPicture::CreateThumbnail(thumb, pItem->GetCachedPictureThumb(),true);
    else if (!pItem->IsPlugin())
    {
      // we load the directory, grab 4 random thumb files (if available) and then generate
      // the thumb.

      CFileItemList items;

      CDirectory::GetDirectory(strPath, items, g_settings.m_pictureExtensions, false, false);
      
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
          CDirectory::GetDirectory(strPath, items, g_settings.m_pictureExtensions, false, false);
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
        CStdString folderThumb(pItem->GetCachedPictureThumb());
        items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
        CPicture::CreateThumbnail(items[0]->m_strPath, folderThumb);
      }
      else
      {
        // ok, now we've got the files to get the thumbs from, lets create it...
        // we basically load the 4 thumbs, resample to 62x62 pixels, and add them
        CStdString strFiles[4];
        for (int thumb = 0; thumb < 4; thumb++)
          strFiles[thumb] = CTextureCache::Get().CheckAndCacheImage(CTextureCache::GetWrappedThumbURL(items[thumb]->m_strPath), false);
        CPicture::CreateFolderThumb(strFiles, pItem->GetCachedPictureThumb());
      }
    }
    // refill in the icon to get it to update
    pItem->SetCachedPictureThumb();
    pItem->FillInDefaultIcon();
  }
}
