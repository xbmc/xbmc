#pragma once
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

#include "utils/StdString.h"

#include <map>

class CCriticalSection;
class CVideoInfoTag;
namespace MUSIC_INFO 
{
  class CMusicInfoTag;
}
class CAlbum;
class CArtist;
class CFileItem;

class CThumbnailCache
{
private:
  CThumbnailCache();
public:
  virtual ~CThumbnailCache();

  static CThumbnailCache* GetThumbnailCache();
  bool ThumbExists(const CStdString& strFileName, bool bAddCache = false);
  void Add(const CStdString& strFileName, bool bExists);
  void Clear();
  bool IsCached(const CStdString& strFileName);

  static CStdString GetMusicThumb(const CStdString &path);
  static CStdString GetAlbumThumb(const CFileItem &item);
  static CStdString GetAlbumThumb(const MUSIC_INFO::CMusicInfoTag *musicInfo);
  static CStdString GetAlbumThumb(const CAlbum &album);
  static CStdString GetAlbumThumb(const CStdString &album, const CStdString &artist);
  static CStdString GetArtistThumb(const CArtist &artist);
  static CStdString GetArtistThumb(const CFileItem &item);
  static CStdString GetArtistThumb(const CStdString &label);
  static CStdString GetActorThumb(const CFileItem &item);
  static CStdString GetActorThumb(const CStdString &label);
  static CStdString GetSeasonThumb(const CFileItem &item);
  static CStdString GetSeasonThumb(const CStdString &label, const CVideoInfoTag *videoInfo = NULL);
  static CStdString GetEpisodeThumb(const CFileItem &item);
  static CStdString GetEpisodeThumb(const CVideoInfoTag *videoInfo);
  static CStdString GetVideoThumb(const CFileItem &item);
  static CStdString GetFanart(const CFileItem &item);
  static CStdString GetThumb(const CStdString &path, const CStdString &path2, bool split = false);
protected:

  static CThumbnailCache* m_pCacheInstance;

  std::map<CStdString, bool> m_Cache;

  static CCriticalSection m_cs;
};
