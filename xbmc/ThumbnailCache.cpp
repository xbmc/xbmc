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

#include "ThumbnailCache.h"
#include "filesystem/File.h"
#include "threads/SingleLock.h"

#include "FileItem.h"
#include "video/VideoInfoTag.h"
#include "video/VideoDatabase.h"
#include "music/tags/MusicInfoTag.h"
#include "music/Album.h"
#include "music/Artist.h"
#include "settings/Settings.h"
#include "utils/URIUtils.h"
#include "utils/Crc32.h"
#include "filesystem/StackDirectory.h"

using namespace std;
using namespace XFILE;
using namespace MUSIC_INFO;

CThumbnailCache* CThumbnailCache::m_pCacheInstance = NULL;

CCriticalSection CThumbnailCache::m_cs;

CThumbnailCache::~CThumbnailCache()
{}

CThumbnailCache::CThumbnailCache()
{
}

CThumbnailCache* CThumbnailCache::GetThumbnailCache()
{
  CSingleLock lock (m_cs);

  if (m_pCacheInstance == NULL)
    m_pCacheInstance = new CThumbnailCache;

  return m_pCacheInstance;
}

bool CThumbnailCache::ThumbExists(const CStdString& strFileName, bool bAddCache /*=false*/)
{
  CSingleLock lock (m_cs);

  if (strFileName.size() == 0) return false;
  map<CStdString, bool>::iterator it;
  it = m_Cache.find(strFileName);
  if (it != m_Cache.end())
    return it->second;

  bool bExists = CFile::Exists(strFileName);

  if (bAddCache)
    Add(strFileName, bExists);
  return bExists;
}

bool CThumbnailCache::IsCached(const CStdString& strFileName)
{
  CSingleLock lock (m_cs);

  map<CStdString, bool>::iterator it;
  it = m_Cache.find(strFileName);
  return it != m_Cache.end();
}

void CThumbnailCache::Clear()
{
  CSingleLock lock (m_cs);

  if (m_pCacheInstance != NULL)
  {
    m_Cache.erase(m_Cache.begin(), m_Cache.end());
    delete m_pCacheInstance;
  }

  m_pCacheInstance = NULL;
}

void CThumbnailCache::Add(const CStdString& strFileName, bool bExists)
{
  CSingleLock lock (m_cs);

  map<CStdString, bool>::iterator it;
  it = m_Cache.find(strFileName);
  if (it != m_Cache.end())
    it->second = bExists;
  else
    m_Cache.insert(pair<CStdString, bool>(strFileName, bExists));
}

CStdString CThumbnailCache::GetAlbumThumb(const CFileItem &item)
{
  return GetAlbumThumb(item.GetMusicInfoTag());
}

CStdString CThumbnailCache::GetAlbumThumb(const CMusicInfoTag *musicInfo)
{
  if (!musicInfo)
    return CStdString();

  return GetAlbumThumb(musicInfo->GetAlbum(), !musicInfo->GetAlbumArtist().empty() ? musicInfo->GetAlbumArtist() : musicInfo->GetArtist());
}

CStdString CThumbnailCache::GetAlbumThumb(const CAlbum &album)
{
  return GetAlbumThumb(album.strAlbum, album.strArtist);
}

CStdString CThumbnailCache::GetAlbumThumb(const CStdString& album, const CStdString& artist)
{
  if (album.IsEmpty())
    return GetMusicThumb("unknown" + artist);
  if (artist.IsEmpty())
    return GetMusicThumb(album + "unknown");
  return GetMusicThumb(album + artist);
}

CStdString CThumbnailCache::GetArtistThumb(const CFileItem &item)
{
  return GetArtistThumb(item.GetLabel());
}

CStdString CThumbnailCache::GetArtistThumb(const CArtist &artist)
{
  return GetArtistThumb(artist.strArtist);
}

CStdString CThumbnailCache::GetArtistThumb(const CStdString &label)
{
  return GetThumb("artist" + label, g_settings.GetMusicArtistThumbFolder());
}

CStdString CThumbnailCache::GetActorThumb(const CFileItem &item)
{
  return GetActorThumb(item.GetLabel());
}

CStdString CThumbnailCache::GetActorThumb(const CStdString &label)
{
  return GetThumb("actor" + label, g_settings.GetVideoThumbFolder(), true);
}

CStdString CThumbnailCache::GetSeasonThumb(const CFileItem &item)
{
  return GetSeasonThumb(item.GetLabel(), item.GetVideoInfoTag());
}

CStdString CThumbnailCache::GetSeasonThumb(const CStdString &label, const CVideoInfoTag *videoInfo /* = NULL */)
{
  CStdString seasonPath;
  if (videoInfo)
    seasonPath = videoInfo->m_strPath;

  return GetThumb("season" + seasonPath + label, g_settings.GetVideoThumbFolder(), true);
}

CStdString CThumbnailCache::GetEpisodeThumb(const CFileItem &item)
{
  if (!item.HasVideoInfoTag())
    return CStdString();

  return GetEpisodeThumb(item.GetVideoInfoTag());
}

CStdString CThumbnailCache::GetEpisodeThumb(const CVideoInfoTag* videoInfo)
{
  // get the locally cached thumb
  CStdString strCRC;
  strCRC.Format("%sepisode%i", videoInfo->m_strFileNameAndPath.c_str(), videoInfo->m_iEpisode);
  return GetThumb(strCRC, g_settings.GetVideoThumbFolder(), true);
}

CStdString CThumbnailCache::GetVideoThumb(const CFileItem &item)
{
  if (item.IsStack())
    return GetThumb(CStackDirectory::GetFirstStackedFile(item.GetPath()), g_settings.GetVideoThumbFolder(), true);
  else if (item.IsVideoDb() && item.HasVideoInfoTag())
  {
    if (item.m_bIsFolder && !item.GetVideoInfoTag()->m_strPath.IsEmpty())
      return GetThumb(item.GetVideoInfoTag()->m_strPath, g_settings.GetVideoThumbFolder(), true);
    else if (!item.GetVideoInfoTag()->m_strFileNameAndPath.IsEmpty())
      return GetThumb(item.GetVideoInfoTag()->m_strFileNameAndPath, g_settings.GetVideoThumbFolder(), true);
  }
  return GetThumb(item.GetPath(), g_settings.GetVideoThumbFolder(), true);
}

CStdString CThumbnailCache::GetFanart(const CFileItem &item)
{
  // get the locally cached thumb
  if (item.IsVideoDb() || (item.HasVideoInfoTag() && !item.GetVideoInfoTag()->IsEmpty()))
  {
    if (!item.HasVideoInfoTag())
      return "";
    if (!item.GetVideoInfoTag()->m_strArtist.IsEmpty())
      return GetThumb(item.GetVideoInfoTag()->m_strArtist,g_settings.GetMusicFanartFolder());
    if (!item.m_bIsFolder && !item.GetVideoInfoTag()->m_strShowTitle.IsEmpty())
    {
      CStdString showPath;
      if (!item.GetVideoInfoTag()->m_strShowPath.IsEmpty())
        showPath = item.GetVideoInfoTag()->m_strShowPath;
      else
      {
        CVideoDatabase database;
        database.Open();
        int iShowId = item.GetVideoInfoTag()->m_iIdShow;
        if (iShowId <= 0)
          iShowId = database.GetTvShowId(item.GetVideoInfoTag()->m_strPath);
        CStdString showPath;
        database.GetFilePathById(iShowId,showPath,VIDEODB_CONTENT_TVSHOWS);
      }
      return GetThumb(showPath,g_settings.GetVideoFanartFolder());
    }
    CStdString path = item.GetVideoInfoTag()->GetPath();
    if (path.empty())
      return "";
    return GetThumb(path,g_settings.GetVideoFanartFolder());
  }
  if (item.HasMusicInfoTag())
    return GetThumb(item.GetMusicInfoTag()->GetArtist(),g_settings.GetMusicFanartFolder());

  return GetThumb(item.GetPath(),g_settings.GetVideoFanartFolder());
}

CStdString CThumbnailCache::GetThumb(const CStdString &path, const CStdString &path2, bool split /* = false */)
{
  // get the locally cached thumb
  Crc32 crc;
  crc.ComputeFromLowerCase(path);

  CStdString thumb;
  if (split)
  {
    CStdString hex;
    hex.Format("%08x", (__int32)crc);
    thumb.Format("%c\\%08x.tbn", hex[0], (unsigned __int32)crc);
  }
  else
    thumb.Format("%08x.tbn", (unsigned __int32)crc);

  return URIUtils::AddFileToFolder(path2, thumb);
}

CStdString CThumbnailCache::GetMusicThumb(const CStdString& path)
{
  Crc32 crc;
  CStdString noSlashPath(path);
  URIUtils::RemoveSlashAtEnd(noSlashPath);
  crc.ComputeFromLowerCase(noSlashPath);
  CStdString hex;
  hex.Format("%08x", (unsigned __int32) crc);
  CStdString thumb;
  thumb.Format("%c/%s.tbn", hex[0], hex.c_str());
  return URIUtils::AddFileToFolder(g_settings.GetMusicThumbFolder(), thumb);
}
