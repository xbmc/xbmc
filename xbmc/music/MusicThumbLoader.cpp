/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
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

  if (pItem->HasMusicInfoTag() && (pItem->GetArt().empty() ||
    (pItem->GetArt().size() == 1 && pItem->HasArt("thumb"))))
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

  // Fallback to folder thumb when path has one cached
  if (!pItem->HasArt("thumb"))
  {
    std::string art = GetCachedImage(*pItem, "thumb");
    if (!art.empty())
      pItem->SetArt("thumb", art);
  }

  // Fallback to folder fanart when path has one cached
  //! @todo Remove as "fanart" is never been cached for music folders (only for
  // artists) or start caching fanart for folders?
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
  bool artfound(false);
  std::vector<ArtForThumbLoader> art;
  CMusicInfoTag &tag = *item.GetMusicInfoTag();
  if (tag.GetDatabaseId() > -1 && !tag.GetType().empty())
  {
    // Item in music library, fetch the art
    m_musicDatabase->Open();
    if (tag.GetType() == MediaTypeSong)
      artfound = m_musicDatabase->GetArtForItem(tag.GetDatabaseId(), tag.GetAlbumId(), -1, false, art);
    else if (tag.GetType() == MediaTypeAlbum)
      artfound = m_musicDatabase->GetArtForItem(-1, tag.GetDatabaseId(), -1, false, art);
    else //Artist
      artfound = m_musicDatabase->GetArtForItem(-1, -1, tag.GetDatabaseId(), true, art);

    m_musicDatabase->Close();
  }
  else
  {
    // Non-library song (has musictag but no ID, and may have thumb already loaded),
    // try to fetch both song artist(s) and album artist(s) art by artist name,
    // e.g. "artist.thumb", "artist.fanart", "artist.clearlogo", "artist.banner",
    // "artist1.thumb", "artist1.fanart", "artist1.clearlogo", "artist1.banner",
    // "albumartist.thumb", "albumartist.fanart" etc. Set fanart as fallback.
    tag.SetType(MediaTypeSong);

    // Song artist art
    // Split song artist names correctly into artist credits from various tag arrays
    CSong song;
    song.SetArtistCredits(tag.GetArtist(), tag.GetMusicBrainzArtistHints(), tag.GetMusicBrainzArtistID());
    m_musicDatabase->Open();
    int iOrder = 0;
    for (const auto& artistCredit : song.artistCredits)
    {
      int idArtist = m_musicDatabase->GetArtistByName(artistCredit.GetArtist());
      if (idArtist > 0)
      {
        std::vector<ArtForThumbLoader> artistart;
        if (m_musicDatabase->GetArtForItem(-1, -1, idArtist, true, artistart))
        {
          for (auto& artitem : artistart)
          {
            if (iOrder > 0)
              artitem.prefix = StringUtils::Format("artist%i", iOrder);
            else
              artitem.prefix = "artist";
          }
          art.insert(art.end(), artistart.begin(), artistart.end());
        }
      }
      ++iOrder;
    }

    // Album artist art
    if (!tag.GetAlbumArtist().empty() && tag.GetArtistString().compare(tag.GetAlbumArtistString()) != 0)
    {
      // Split song artist names correctly into artist credits from various tag
      // arrays, inc. fallback to song artist names
      CAlbum album;
      album.SetArtistCredits(tag.GetAlbumArtist(), tag.GetMusicBrainzAlbumArtistHints(), tag.GetMusicBrainzAlbumArtistID(),
        tag.GetArtist(), tag.GetMusicBrainzArtistHints(), tag.GetMusicBrainzArtistID());

      iOrder = 0;
      for (const auto& artistCredit : album.artistCredits)
      {
        int idArtist = m_musicDatabase->GetArtistByName(artistCredit.GetArtist());
        if (idArtist > 0)
        {
          std::vector<ArtForThumbLoader> artistart;
          if (m_musicDatabase->GetArtForItem(-1, -1, idArtist, true, artistart))
          {
            for (auto& artitem : artistart)
            {
              if (iOrder > 0)
                artitem.prefix = StringUtils::Format("albumartist%i", iOrder);
              else
                artitem.prefix = "albumartist";
            }
            art.insert(art.end(), artistart.begin(), artistart.end());
          }
        }
        ++iOrder;
      }
    }
    else
    {
      // Replicate the artist art as album artist art
      std::vector<ArtForThumbLoader> artistart;
      for (const auto& artitem : art)
      {
        ArtForThumbLoader newart;
        newart.artType = artitem.artType;
        newart.mediaType = artitem.mediaType;
        newart.prefix = "album" + artitem.prefix;
        newart.url = artitem.url;
        artistart.emplace_back(newart);
      }
      art.insert(art.end(), artistart.begin(), artistart.end());
    }
    artfound = !art.empty();
    m_musicDatabase->Close();
  }

  if (artfound)
  {
    std::string fanartfallback;
    bool bDiscSetThumbSet = false;
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
      // or use extra album thumb when part of disc set
      if (tag.GetType() == MediaTypeSong && artitem.mediaType == MediaTypeAlbum)
      {
        if (artitem.artType == "thumb" && !bDiscSetThumbSet)
          item.SetArtFallback(artitem.artType, artname);
        else if (StringUtils::StartsWith(artitem.artType, "thumb"))
        {
          int number = atoi(artitem.artType.substr(5).c_str());
          if (number > 0 && tag.GetDiscNumber() == number)
          {
            item.SetArtFallback("thumb", artname);
            bDiscSetThumbSet = true;
          }
        }
      }

      // For albums and songs set fallback fanart from the artist.
      // For songs prefer primary song artist over primary albumartist fanart as fallback fanart
      if (artitem.prefix == "artist" && artitem.artType == "fanart")
        fanartfallback = artname;
      if (artitem.prefix == "albumartist" && artitem.artType == "fanart" && fanartfallback.empty())
        fanartfallback = artname;
    }
    if (!fanartfallback.empty())
      item.SetArtFallback("fanart", fanartfallback);

    item.AppendArt(artmap);
  }

  return artfound;
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
