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

#pragma once

#include <map>
#include "ThumbLoader.h"

class CFileItem;
class CMusicDatabase;
class EmbeddedArt;

class CMusicThumbLoader : public CThumbLoader
{
public:
  CMusicThumbLoader();
  ~CMusicThumbLoader() override;

  void OnLoaderStart() override;
  void OnLoaderFinish() override;

  bool LoadItem(CFileItem* pItem) override;
  bool LoadItemCached(CFileItem* pItem) override;
  bool LoadItemLookup(CFileItem* pItem) override;

  /*! \brief Helper function to fill all the art for a music library item
  This fetches the original url for each type of art, and sets fallback thumb and fanart.
  For songs the art for the related album and artist(s) is also set, and for albums that
  of the related artist(s). Art type is named according to media type of the item,
  for example:
  artists may have "thumb", "fanart", "logo", "poster" etc.,
  albums may have "thumb", "spine" etc. and "artist.thumb", "artist.fanart" etc.,
  songs may have "thumb", "album.thumb", "artist.thumb", "artist.fanart", "artist.logo",...
  "artist1.thumb", "artist1.fanart",... "albumartist.thumb", "albumartist1.thumb" etc.
   \param item a music CFileItem
   \return true if we fill art, false if there is no art found
   */
  bool FillLibraryArt(CFileItem &item) override;

  /*! \brief Fill the thumb of a music file/folder item
   First uses a cached thumb from a previous run, then checks for a local thumb
   and caches it for the next run
   \param item the CFileItem object to fill
   \return true if we fill the thumb, false otherwise
   */
  virtual bool FillThumb(CFileItem &item, bool folderThumbs = true);

  static bool GetEmbeddedThumb(const std::string &path, EmbeddedArt &art);

protected:
  CMusicDatabase *m_musicDatabase;
  typedef std::map<int, std::map<std::string, std::string> > ArtCache;
  ArtCache m_albumArt;
};
