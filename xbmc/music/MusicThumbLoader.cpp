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
#include "FileItem.h"
#include "TextureDatabase.h"
#include "music/tags/MusicInfoTag.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "music/infoscanner/MusicInfoScanner.h"
#include "video/VideoThumbLoader.h"

using namespace std;
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
    else if (pItem->HasMusicInfoTag() && !pItem->GetMusicInfoTag()->GetArtist().empty())
    {
      std::string artist = pItem->GetMusicInfoTag()->GetArtist()[0];
      m_musicDatabase->Open();
      int idArtist = m_musicDatabase->GetArtistByName(artist);
      if (idArtist >= 0)
      {
        string fanart = m_musicDatabase->GetArtForItem(idArtist, MediaTypeArtist, "fanart");
        if (!fanart.empty())
        {
          pItem->SetArt("artist.fanart", fanart);
          pItem->SetArtFallback("fanart", "artist.fanart");
        }
        else if (!pItem->GetMusicInfoTag()->GetAlbumArtist().empty() &&
                 pItem->GetMusicInfoTag()->GetAlbumArtist()[0] != artist)
        {
          // If no artist fanart and the album artist is different to the artist,
          // try to get fanart from the album artist
          artist = pItem->GetMusicInfoTag()->GetAlbumArtist()[0];
          idArtist = m_musicDatabase->GetArtistByName(artist);
          if (idArtist >= 0)
          {
            fanart = m_musicDatabase->GetArtForItem(idArtist, MediaTypeArtist, "fanart");
            if (!fanart.empty())
            {
              pItem->SetArt("albumartist.fanart", fanart);
              pItem->SetArtFallback("fanart", "albumartist.fanart");
            }
          }
        }
      }
      m_musicDatabase->Close();
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
    if (pItem->HasMusicInfoTag() && !pItem->GetMusicInfoTag()->GetCoverArtInfo().empty())
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
    map<string, string> artwork;
    if (m_musicDatabase->GetArtForItem(tag.GetDatabaseId(), tag.GetType(), artwork))
      item.SetArt(artwork);
    else if (tag.GetType() == MediaTypeSong)
    { // no art for the song, try the album
      ArtCache::const_iterator i = m_albumArt.find(tag.GetAlbumId());
      if (i == m_albumArt.end())
      {
        m_musicDatabase->GetArtForItem(tag.GetAlbumId(), MediaTypeAlbum, artwork);
        i = m_albumArt.insert(make_pair(tag.GetAlbumId(), artwork)).first;
      }
      if (i != m_albumArt.end())
      {
        item.AppendArt(i->second, MediaTypeAlbum);
        for (map<string, string>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
          item.SetArtFallback(j->first, "album." + j->first);
      }
    }
    if (tag.GetType() == MediaTypeSong || tag.GetType() == MediaTypeAlbum)
    { // fanart from the artist
      string fanart = m_musicDatabase->GetArtistArtForItem(tag.GetDatabaseId(), tag.GetType(), "fanart");
      if (!fanart.empty())
      {
        item.SetArt("artist.fanart", fanart);
        item.SetArtFallback("fanart", "artist.fanart");
      }
      else if (tag.GetType() == MediaTypeSong)
      {
        // If no artist fanart, try for album artist fanart
        fanart = m_musicDatabase->GetArtistArtForItem(tag.GetAlbumId(), MediaTypeAlbum, "fanart");
        if (!fanart.empty())
        {
          item.SetArt("albumartist.fanart", fanart);
          item.SetArtFallback("fanart", "albumartist.fanart");
        }
      }
    }
    m_musicDatabase->Close();
  }
  return !item.GetArt().empty();
}

bool CMusicThumbLoader::GetEmbeddedThumb(const std::string &path, EmbeddedArt &art)
{
  CFileItem item(path, false);
  unique_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(item));
  CMusicInfoTag tag;
  if (NULL != pLoader.get())
    pLoader->Load(path, tag, &art);

  return !art.empty();
}
