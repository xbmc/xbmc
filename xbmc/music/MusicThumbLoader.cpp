/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "MusicThumbLoader.h"

#include <utility>

#include "FileItem.h"
#include "music/infoscanner/MusicInfoScanner.h"
#include "music/tags/MusicInfoTag.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "TextureDatabase.h"
#include "video/VideoThumbLoader.h"

using namespace MUSIC_INFO;

CMusicThumbLoader::CMusicThumbLoader() : CThumbLoader()
{
  m_musicDatabase = new CMusicDatabase;
}

CMusicThumbLoader::~CMusicThumbLoader()
{
  delete m_musicDatabase;
}

void CMusicThumbLoader::OnLoaderStart()
{
  m_musicDatabase->Open();
  m_albumArt.clear();
  CThumbLoader::OnLoaderStart();
}

void CMusicThumbLoader::OnLoaderFinish()
{
  m_musicDatabase->Close();
  m_albumArt.clear();
  CThumbLoader::OnLoaderFinish();
}

bool CMusicThumbLoader::LoadItem(CFileItem* pItem)
{
  bool result  = LoadItemCached(pItem);
       result |= LoadItemLookup(pItem);

  return result;
}

bool CMusicThumbLoader::LoadItemCached(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive)
    return false;

  if (pItem->HasMusicInfoTag() && pItem->GetArt().empty())
  {
    if (FillLibraryArt(*pItem))
      return true;
      
    if (pItem->GetMusicInfoTag()->GetType() == MediaTypeArtist)
      return false; // No fallback
  }

  if (pItem->HasVideoInfoTag() && pItem->GetArt().empty())
  { // music video
    CVideoThumbLoader loader;
    if (loader.LoadItemCached(pItem))
      return true;
  }

  if (!pItem->HasArt("thumb"))
  {
    std::string art = GetCachedImage(*pItem, "thumb");
    if (!art.empty())
      pItem->SetArt("thumb", art);
  }

  if (!pItem->HasArt("fanart"))
  {
    std::string art = GetCachedImage(*pItem, "fanart");
    if (!art.empty())
    {
      pItem->SetArt("fanart", art);
    }
  }

  return false;
}

bool CMusicThumbLoader::LoadItemLookup(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive)
    return false;

  if (pItem->HasMusicInfoTag() && pItem->GetMusicInfoTag()->GetType() == MediaTypeArtist) // No fallback for artist
    return false;

  if (pItem->HasVideoInfoTag())
  { // music video
    CVideoThumbLoader loader;
    if (loader.LoadItemLookup(pItem))
      return true;
  }

  if (!pItem->HasArt("thumb"))
  {
    // Look for embedded art
    if (pItem->HasMusicInfoTag() && !pItem->GetMusicInfoTag()->GetCoverArtInfo().Empty())
    {
      // The item has got embedded art but user thumbs overrule, so check for those first
      if (!FillThumb(*pItem, false)) // Check for user thumbs but ignore folder thumbs
      {
        // No user thumb, use embedded art
        std::string thumb = CTextureUtils::GetWrappedImageURL(pItem->GetPath(), "music");
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

bool CMusicThumbLoader::FillThumb(CFileItem &item, bool folderThumbs /* = true */)
{
  if (item.HasArt("thumb"))
    return true;
  std::string thumb = GetCachedImage(item, "thumb");
  if (thumb.empty())
  {
    thumb = item.GetUserMusicThumb(false, folderThumbs);
    if (!thumb.empty())
      SetCachedImage(item, "thumb", thumb);
  }
  if (!thumb.empty())
    item.SetArt("thumb", thumb);
  return !thumb.empty();
}

bool CMusicThumbLoader::FillLibraryArt(CFileItem &item)
{
  CMusicInfoTag &tag = *item.GetMusicInfoTag();
  if (tag.GetDatabaseId() > -1 && !tag.GetType().empty())
  {
    m_musicDatabase->Open();
    std::vector<ArtForThumbLoader> art;
    bool artfound;
    if (tag.GetType() == MediaTypeSong)
      artfound = m_musicDatabase->GetArtForItem(tag.GetDatabaseId(), tag.GetAlbumId(), -1, false, art);
    else if (tag.GetType() == MediaTypeAlbum)
      artfound = m_musicDatabase->GetArtForItem(-1, tag.GetDatabaseId(), -1, false, art);
    else //Artist
      artfound = m_musicDatabase->GetArtForItem(-1, -1, tag.GetDatabaseId(), true, art);
    
    m_musicDatabase->Close();
    if (artfound)
    {
      std::string fanartfallback;
      std::map<std::string, std::string> artmap;
      for (auto artitem : art)
      {
        /* Add art to artmap, naming according to media type. 
           For example: artists have "thumb", "fanart", "poster" etc.,
           albums have "thumb", "artist.thumb", "artist.fanart",... "artist1.thumb", "artist1.fanart" etc.,
           songs have "thumb", "album.thumb", "artist.thumb", "albumartist.thumb", "albumartist1.thumb" etc.
        */
        std::string artname;
        if (tag.GetType() == artitem.mediaType)
          artname = artitem.artType;
        else if (artitem.prefix.empty())
          artname = artitem.mediaType + "." + artitem.artType;
        else
        {
          if (tag.GetType() == MediaTypeAlbum)
            StringUtils::Replace(artitem.prefix, "albumartist", "artist");
          artname = artitem.prefix + "." + artitem.artType;
        }

        artmap.insert(std::make_pair(artname, artitem.url));

        // Add fallback art for "thumb" and "fanart" art types only
        // Set album thumb as the fallback used when song thumb is missing
        if (tag.GetType() == MediaTypeSong && artitem.mediaType == MediaTypeAlbum && artitem.artType == "thumb")
          item.SetArtFallback(artitem.artType, artname);

        // For albums and songs set fallback fanart from the artist.
        // For songs prefer primary song artist over primary albumartist fanart as fallback fanart 
        if (artitem.prefix == "artist" && artitem.artType == "fanart")
          fanartfallback = artname;
        if (artitem.prefix == "albumartist" && artitem.artType == "fanart" && fanartfallback.empty())
          fanartfallback = artname;
      }
      if (!fanartfallback.empty())
        item.SetArtFallback("fanart", fanartfallback);

      item.SetArt(artmap);
    }       
  }
  return !item.GetArt().empty();
}

bool CMusicThumbLoader::GetEmbeddedThumb(const std::string &path, EmbeddedArt &art)
{
  CFileItem item(path, false);
  std::unique_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(item));
  CMusicInfoTag tag;
  if (NULL != pLoader.get())
    pLoader->Load(path, tag, &art);

  return !art.Empty();
}
