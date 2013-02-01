/*
 *      Copyright (C) 2012 Team XBMC
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

#include "MusicThumbLoader.h"
#include "FileItem.h"
#include "TextureCache.h"
#include "music/tags/MusicInfoTag.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "music/infoscanner/MusicInfoScanner.h"
#include "music/Artist.h"
#include "video/VideoThumbLoader.h"

using namespace std;
using namespace MUSIC_INFO;

CMusicThumbLoader::CMusicThumbLoader() : CThumbLoader(1)
{
  m_database = new CMusicDatabase;
}

CMusicThumbLoader::~CMusicThumbLoader()
{
  delete m_database;
}

void CMusicThumbLoader::Initialize()
{
  m_database->Open();
  m_albumArt.clear();
}

void CMusicThumbLoader::Deinitialize()
{
  m_database->Close();
  m_albumArt.clear();
}

void CMusicThumbLoader::OnLoaderStart()
{
  Initialize();
}

void CMusicThumbLoader::OnLoaderFinish()
{
  Deinitialize();
}

bool CMusicThumbLoader::LoadItem(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive)
    return true;

  if (pItem->HasMusicInfoTag() && pItem->GetArt().empty())
  {
    if (FillLibraryArt(*pItem))
      return true;
    if (pItem->GetMusicInfoTag()->GetType() == "artist")
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
    if (pItem->HasMusicInfoTag() && !pItem->GetMusicInfoTag()->GetArtist().empty())
    {
      std::string artist = pItem->GetMusicInfoTag()->GetArtist()[0];
      m_database->Open();
      int idArtist = m_database->GetArtistByName(artist);
      if (idArtist >= 0)
      {
        string fanart = m_database->GetArtForItem(idArtist, "artist", "fanart");
        if (!fanart.empty())
        {
          pItem->SetArt("artist.fanart", fanart);
          pItem->SetArtFallback("fanart", "artist.fanart");
        }
      }
      m_database->Close();
    }
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

bool CMusicThumbLoader::FillThumb(CFileItem &item, bool folderThumbs /* = true */)
{
  if (item.HasArt("thumb"))
    return true;
  CStdString thumb = GetCachedImage(item, "thumb");
  if (thumb.IsEmpty())
  {
    thumb = item.GetUserMusicThumb(false, folderThumbs);
    if (!thumb.IsEmpty())
      SetCachedImage(item, "thumb", thumb);
  }
  item.SetArt("thumb", thumb);
  return !thumb.IsEmpty();
}

bool CMusicThumbLoader::FillLibraryArt(CFileItem &item)
{
  CMusicInfoTag &tag = *item.GetMusicInfoTag();
  if (tag.GetDatabaseId() > -1 && !tag.GetType().empty())
  {
    m_database->Open();
    map<string, string> artwork;
    if (m_database->GetArtForItem(tag.GetDatabaseId(), tag.GetType(), artwork))
      item.SetArt(artwork);
    else if (tag.GetType() == "song")
    { // no art for the song, try the album
      ArtCache::const_iterator i = m_albumArt.find(tag.GetAlbumId());
      if (i == m_albumArt.end())
      {
        m_database->GetArtForItem(tag.GetAlbumId(), "album", artwork);
        i = m_albumArt.insert(make_pair(tag.GetAlbumId(), artwork)).first;
      }
      if (i != m_albumArt.end())
      {
        item.AppendArt(i->second, "album");
        for (map<string, string>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
          item.SetArtFallback(j->first, "album." + j->first);
      }
    }
    if (tag.GetType() == "song" || tag.GetType() == "album")
    { // fanart from the artist
      string fanart = m_database->GetArtistArtForItem(tag.GetDatabaseId(), tag.GetType(), "fanart");
      if (!fanart.empty())
      {
        item.SetArt("artist.fanart", fanart);
        item.SetArtFallback("fanart", "artist.fanart");
      }
    }
    m_database->Close();
  }
  return !item.GetArt().empty();
}

bool CMusicThumbLoader::GetEmbeddedThumb(const std::string &path, EmbeddedArt &art)
{
  auto_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(path));
  CMusicInfoTag tag;
  if (NULL != pLoader.get())
    pLoader->Load(path, tag, &art);

  return !art.empty();
}
