/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "ContactThumbLoader.h"
#include "TextureCache.h"
#include "ContactDatabase.h"
/*
#include "music/tags/ContactInfoTag.h"
#include "music/tags/ContactInfoTagLoaderFactory.h"
#include "music/infoscanner/ContactInfoScanner.h"
 */

#include "video/VideoThumbLoader.h"

using namespace std;
using namespace PICTURE_INFO;

CContactThumbLoader::CContactThumbLoader() : CThumbLoader(1)
{
  m_database = new CContactDatabase;
}

CContactThumbLoader::~CContactThumbLoader()
{
  delete m_database;
}

void CContactThumbLoader::Initialize()
{
  m_database->Open();
  m_albumArt.clear();
}

void CContactThumbLoader::Deinitialize()
{
  m_database->Close();
  m_albumArt.clear();
}

void CContactThumbLoader::OnLoaderStart()
{
  Initialize();
}

void CContactThumbLoader::OnLoaderFinish()
{
  Deinitialize();
}

bool CContactThumbLoader::LoadItem(CContactFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive)
    return true;
  
  if (pItem->HasContactInfoTag() && pItem->GetArt().empty())
  {
    if (FillLibraryArt(*pItem))
      return true;
    if (pItem->GetContactInfoTag()->GetType() == "artist")
      return true; // no fallback
  }
  
  if (pItem->HasVideoInfoTag() && pItem->GetArt().empty())
  { // music video
    CVideoThumbLoader loader;
    if (loader.LoadItem(pItem))
      return true;
  }
  /*
  
  if (!pItem->HasArt("thumb"))
  {
    // Look for embedded art
    if (pItem->HasContactInfoTag() && !pItem->GetContactInfoTag()->GetCoverArtInfo().empty())
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
   */
  
  return true;
}

bool CContactThumbLoader::FillThumb(CContactFileItem &item, bool folderThumbs /* = true */)
{
  /*
  if (item.HasArt("thumb"))
    return true;
  CStdString thumb = GetCachedImage(item, "thumb");
  if (thumb.IsEmpty())
  {
    thumb = item.GetUserContactThumb(false, folderThumbs);
    if (!thumb.IsEmpty())
      SetCachedImage(item, "thumb", thumb);
  }
  item.SetArt("thumb", thumb);
  return !thumb.IsEmpty();
   */
  return false;
}

bool CContactThumbLoader::FillLibraryArt(CContactFileItem &item)
{
  /*
  CContactInfoTag &tag = *item.GetContactInfoTag();
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
    m_database->Close();
  }
  return !item.GetArt().empty();
   */
  return false;
}

