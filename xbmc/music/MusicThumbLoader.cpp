/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicThumbLoader.h"

#include "FileItem.h"
#include "TextureDatabase.h"
#include "music/infoscanner/MusicInfoScanner.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/StringUtils.h"
#include "video/VideoThumbLoader.h"

#include <utility>

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

  if (pItem->HasMusicInfoTag() && !pItem->GetProperty("libraryartfilled").asBoolean())
  {
    if (FillLibraryArt(*pItem))
      return true;

    if (pItem->GetMusicInfoTag()->GetType() == MediaTypeArtist)
      return false; // No fallback
  }

  if (pItem->HasVideoInfoTag() && !pItem->HasArt("thumb"))
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
  /* Called for any item with MusicInfoTag and no art.
     Items on Genres, Sources and Roles nodes have ID (although items on Years
     node do not) so check for song/album/artist specifically.
     Non-library songs (file view) can also have MusicInfoTag but no ID or type
  */
  bool artfound(false);
  std::vector<ArtForThumbLoader> art;
  CMusicInfoTag &tag = *item.GetMusicInfoTag();
  if (tag.GetDatabaseId() > -1 &&
      (tag.GetType() == MediaTypeSong || tag.GetType() == MediaTypeAlbum ||
       tag.GetType() == MediaTypeArtist))
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
  else if (!tag.GetArtist().empty() &&
           (tag.GetType() == MediaTypeNone || tag.GetType() == MediaTypeSong))
  {
    /*
    Could be non-library song - has musictag but no ID or type (may have
    thumb already). Try to fetch both song artist(s) and album artist(s) art by
    artist name, e.g. "artist.thumb", "artist.fanart", "artist.clearlogo",
    "artist.banner", "artist1.thumb", "artist1.fanart", "artist1.clearlogo",
    "artist1.banner", "albumartist.thumb", "albumartist.fanart" etc.
    Set fanart as fallback.
    */
    CSong song;
    // Try to split song artist names (various tags) into artist credits
    song.SetArtistCredits(tag.GetArtist(), tag.GetMusicBrainzArtistHints(), tag.GetMusicBrainzArtistID());
    if (!song.artistCredits.empty())
    {
      tag.SetType(MediaTypeSong);  // Makes "Information" context menu visible
      m_musicDatabase->Open();
      int iOrder = 0;
      // Song artist art
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
                artitem.prefix = StringUtils::Format("artist{}", iOrder);
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
                  artitem.prefix = StringUtils::Format("albumartist{}", iOrder);
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
  }

  if (artfound)
  {
    std::string fanartfallback;
    std::string artname;
    std::map<std::string, std::string> artmap;
    std::map<std::string, std::string> discartmap;
    for (auto artitem : art)
    {
      /* Add art to artmap, naming according to media type.
      For example: artists have "thumb", "fanart", "poster" etc.,
      albums have "thumb", "artist.thumb", "artist.fanart",... "artist1.thumb", "artist1.fanart" etc.,
      songs have "thumb", "album.thumb", "artist.thumb", "albumartist.thumb", "albumartist1.thumb" etc.
      */
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

      // Pull out album art for this specific disc e.g. "thumb2", skip art for other discs
      if (artitem.mediaType == MediaTypeAlbum && tag.GetDiscNumber() > 0)
      {
        // Find any trailing digits
        size_t startnum = artitem.artType.find_last_not_of("0123456789");
        std::string digits = artitem.artType.substr(startnum + 1);
        int num = atoi(digits.c_str());
        if (num > 0 && startnum < artitem.artType.size())
        {
          if (num == tag.GetDiscNumber())
            discartmap.insert(std::make_pair(artitem.artType.substr(0, startnum + 1), artitem.url));
          continue;
        }
      }

      artmap.insert(std::make_pair(artname, artitem.url));

      // Add fallback art for "thumb" and "fanart" art types only
      // Set album thumb as the fallback used when song thumb is missing
      if (tag.GetType() == MediaTypeSong && artitem.mediaType == MediaTypeAlbum &&
          artitem.artType == "thumb")
      {
        item.SetArtFallback(artitem.artType, artname);
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

    // Process specific disc art when we have some
    for (const auto& discart : discartmap)
    {
      std::map<std::string, std::string>::iterator it;
      if (tag.GetType() == MediaTypeAlbum)
      {
        // Insert or replace album art with specific disc art
        it = artmap.find(discart.first);
        if (it != artmap.end())
          it->second = discart.second;
        else
          artmap.insert(discart);
      }
      else if (tag.GetType() == MediaTypeSong)
      {
        // Use disc thumb rather than album as fallback for song thumb
        // (Fallback approach is used to fill missing thumbs).
        if (discart.first == "thumb")
        {
          it = artmap.find("album.thumb");
          if (it != artmap.end())
            // Replace "album.thumb" already set as fallback
            it->second = discart.second;
          else
          {
            // Insert thumb for album and set as fallback
            artmap.insert(std::make_pair("album.thumb", discart.second));
            item.SetArtFallback("thumb", "album.thumb");
          }
        }
        else
        {
          // Apply disc art as song art when not have that type (fallback does not apply).
          // Art of other types could been set via JSON, or in future read from metadata
          it = artmap.find(discart.first);
          if (it == artmap.end())
            artmap.insert(discart);
        }
      }
    }

    item.AppendArt(artmap);
    item.SetProperty("libraryartfilled", true);
  }

  return artfound;
}
