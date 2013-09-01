/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "Picture.h"
#include "PictureThumbLoader.h"
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
#include "pictures/tags/PictureInfoTagLoaderFactory.h"

using namespace XFILE;
using namespace std;
using namespace PICTURE_INFO;

#ifdef _USE_OLD_LOADER_
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

  if (pItem->HasArt("thumb") && m_regenerateThumbs)
  {
    CTextureCache::Get().ClearCachedImage(pItem->GetArt("thumb"));
    CTextureDatabase db;
    if (db.Open())
      db.ClearTextureForPath(pItem->GetPath(), "thumb");
    pItem->SetArt("thumb", "");
  }

  CStdString thumb;
  if (pItem->IsPicture() && !pItem->IsZIP() && !pItem->IsRAR() && !pItem->IsCBZ() && !pItem->IsCBR() && !pItem->IsPlayList())
  { // load the thumb from the image file
    thumb = pItem->HasArt("thumb") ? pItem->GetArt("thumb") : CTextureCache::GetWrappedThumbURL(pItem->GetPath());
  }
  else if (pItem->IsVideo() && !pItem->IsZIP() && !pItem->IsRAR() && !pItem->IsCBZ() && !pItem->IsCBR() && !pItem->IsPlayList())
  { // video
    if (!CVideoThumbLoader::FillThumb(*pItem))
    {
      CStdString thumbURL = CVideoThumbLoader::GetEmbeddedThumbURL(*pItem);
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
  if (!thumb.IsEmpty())
  {
    CTextureCache::Get().BackgroundCacheImage(thumb);
    pItem->SetArt("thumb", thumb);
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


#else
void CPictureThumbLoader::ProcessFoldersAndArchives(CFileItem *pItem)
{
  if (pItem->HasArt("thumb"))
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
      pItem->SetArt("thumb", strTBN);
      return;
    }
  }
  if ((pItem->m_bIsFolder || pItem->IsCBR() || pItem->IsCBZ()) && !pItem->m_bIsShareOrDrive
      && !pItem->IsParentFolder() && !pItem->GetPath().Equals("add"))
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
      pItem->SetArt("thumb", thumb);
      return;
    }
    if (!pItem->IsPlugin())
    {
      // we load the directory, grab 4 random thumb files (if available) and then generate
      // the thumb.
      
      CFileItemList items;
      
      CDirectory::GetDirectory(strPath, items, g_advancedSettings.m_pictureExtensions, DIR_FLAG_NO_FILE_DIRS);
      
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
          CDirectory::GetDirectory(strPath, items, g_advancedSettings.m_pictureExtensions, DIR_FLAG_NO_FILE_DIRS);
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
        items.Sort(SORT_METHOD_LABEL, SortOrderAscending);
        CStdString thumb = CTextureCache::GetWrappedThumbURL(items[0]->GetPath());
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
        CStdString thumb = CTextureCache::GetWrappedImageURL(pItem->GetPath(), "picturefolder");
        CStdString relativeCacheFile = CTextureCache::GetCacheFile(thumb) + ".png";
        
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

CPictureThumbLoader::CPictureThumbLoader() : CThumbLoader(1)
{
  m_database = new CPictureDatabase;
}

CPictureThumbLoader::~CPictureThumbLoader()
{
  delete m_database;
}

void CPictureThumbLoader::Initialize()
{
  m_database->Open();
  m_albumArt.clear();
}

void CPictureThumbLoader::Deinitialize()
{
  m_database->Close();
  m_albumArt.clear();
}

void CPictureThumbLoader::OnLoaderStart()
{
  Initialize();
}

void CPictureThumbLoader::OnLoaderFinish()
{
  Deinitialize();
}

bool CPictureThumbLoader::LoadItem(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive)
    return true;
  
  if (pItem->HasPictureInfoTag() && pItem->GetArt().empty())
  {
    if (FillLibraryArt(*pItem))
      return true;
    if (pItem->GetPictureInfoTag()->GetType() == "face")
      return true; // no fallback
  }
  
  if (pItem->HasVideoInfoTag() && pItem->GetArt().empty())
  { // music video
    CVideoThumbLoader loader;
    if (loader.LoadItem(pItem))
      return true;
  }
  
  if (!pItem->HasArt("fanart"))
  {
    if (pItem->HasPictureInfoTag() && !pItem->GetPictureInfoTag()->GetFace().empty())
    {
      std::string face = pItem->GetPictureInfoTag()->GetFace()[0];
      m_database->Open();
      int idFace = m_database->GetFaceByName(face);
      if (idFace >= 0)
      {
        string fanart = m_database->GetArtForItem(idFace, "face", "fanart");
        if (!fanart.empty())
        {
          pItem->SetArt("face.fanart", fanart);
          pItem->SetArtFallback("fanart", "face.fanart");
        }
      }
      m_database->Close();
    }
  }
  
  if (!pItem->HasArt("thumb"))
  {
    // Look for embedded art
    if (pItem->HasPictureInfoTag() && !pItem->GetPictureInfoTag()->GetCoverArtInfo().empty())
    {
      // The item has got embedded art but user thumbs overrule, so check for those first
      if (!FillThumb(*pItem, false)) // Check for user thumbs but ignore folder thumbs
      {
        // No user thumb, use embedded art
        CStdString thumb = CTextureCache::GetWrappedImageURL(pItem->GetPath(), "music");
        pItem->SetArt("thumb", thumb);
      }
    }
    else
    {
      // Check for user thumbs
      FillThumb(*pItem, true);
    }
  }
  
  return true;
}

bool CPictureThumbLoader::FillThumb(CFileItem &item, bool folderThumbs /* = true */)
{
  if (item.HasArt("thumb"))
    return true;
  CStdString thumb = GetCachedImage(item, "thumb");
  if (thumb.IsEmpty())
  {
    thumb = item.GetUserPictureThumb(false, folderThumbs);
    if (!thumb.IsEmpty())
      SetCachedImage(item, "thumb", thumb);
  }
  item.SetArt("thumb", thumb);
  return !thumb.IsEmpty();
}

bool CPictureThumbLoader::FillLibraryArt(CFileItem &item)
{
  CPictureInfoTag &tag = *item.GetPictureInfoTag();
  if (tag.GetDatabaseId() > -1 && !tag.GetType().empty())
  {
    m_database->Open();
    map<string, string> artwork;
    if (m_database->GetArtForItem(tag.GetDatabaseId(), tag.GetType(), artwork))
      item.SetArt(artwork);
    else if (tag.GetType() == "picture")
    { // no art for the song, try the album
      ArtCache::const_iterator i = m_albumArt.find(tag.GetPictureAlbumId());
      if (i == m_albumArt.end())
      {
        m_database->GetArtForItem(tag.GetPictureAlbumId(), "album", artwork);
        i = m_albumArt.insert(make_pair(tag.GetPictureAlbumId(), artwork)).first;
      }
      if (i != m_albumArt.end())
      {
        item.AppendArt(i->second, "album");
        for (map<string, string>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
          item.SetArtFallback(j->first, "album." + j->first);
      }
    }
    if (tag.GetType() == "picture" || tag.GetType() == "album")
    { // fanart from the face
      string fanart = m_database->GetFaceArtForItem(tag.GetDatabaseId(), tag.GetType(), "fanart");
      if (!fanart.empty())
      {
        item.SetArt("face.fanart", fanart);
        item.SetArtFallback("fanart", "face.fanart");
      }
    }
    m_database->Close();
  }
  return !item.GetArt().empty();
}

bool CPictureThumbLoader::GetEmbeddedThumb(const std::string &path, EmbeddedArt &art)
{
  auto_ptr<IPictureInfoTagLoader> pLoader (CPictureInfoTagLoaderFactory::CreateLoader(path));
  CPictureInfoTag tag;
  if (NULL != pLoader.get())
    pLoader->Load(path, tag, &art);
  
  return !art.empty();
}
#endif