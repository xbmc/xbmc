/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#ifndef ROOT_THUMBNAILCACHE_H_INCLUDED
#define ROOT_THUMBNAILCACHE_H_INCLUDED
#include "ThumbnailCache.h"
#endif

#ifndef ROOT_FILESYSTEM_FILE_H_INCLUDED
#define ROOT_FILESYSTEM_FILE_H_INCLUDED
#include "filesystem/File.h"
#endif

#ifndef ROOT_THREADS_SINGLELOCK_H_INCLUDED
#define ROOT_THREADS_SINGLELOCK_H_INCLUDED
#include "threads/SingleLock.h"
#endif


#ifndef ROOT_FILEITEM_H_INCLUDED
#define ROOT_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif

#ifndef ROOT_VIDEO_VIDEOINFOTAG_H_INCLUDED
#define ROOT_VIDEO_VIDEOINFOTAG_H_INCLUDED
#include "video/VideoInfoTag.h"
#endif

#ifndef ROOT_VIDEO_VIDEODATABASE_H_INCLUDED
#define ROOT_VIDEO_VIDEODATABASE_H_INCLUDED
#include "video/VideoDatabase.h"
#endif

#ifndef ROOT_MUSIC_TAGS_MUSICINFOTAG_H_INCLUDED
#define ROOT_MUSIC_TAGS_MUSICINFOTAG_H_INCLUDED
#include "music/tags/MusicInfoTag.h"
#endif

#ifndef ROOT_MUSIC_ALBUM_H_INCLUDED
#define ROOT_MUSIC_ALBUM_H_INCLUDED
#include "music/Album.h"
#endif

#ifndef ROOT_MUSIC_ARTIST_H_INCLUDED
#define ROOT_MUSIC_ARTIST_H_INCLUDED
#include "music/Artist.h"
#endif

#ifndef ROOT_UTILS_URIUTILS_H_INCLUDED
#define ROOT_UTILS_URIUTILS_H_INCLUDED
#include "utils/URIUtils.h"
#endif

#ifndef ROOT_UTILS_CRC32_H_INCLUDED
#define ROOT_UTILS_CRC32_H_INCLUDED
#include "utils/Crc32.h"
#endif

#ifndef ROOT_UTILS_STRINGUTILS_H_INCLUDED
#define ROOT_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif

#ifndef ROOT_FILESYSTEM_STACKDIRECTORY_H_INCLUDED
#define ROOT_FILESYSTEM_STACKDIRECTORY_H_INCLUDED
#include "filesystem/StackDirectory.h"
#endif

#ifndef ROOT_SETTINGS_ADVANCEDSETTINGS_H_INCLUDED
#define ROOT_SETTINGS_ADVANCEDSETTINGS_H_INCLUDED
#include "settings/AdvancedSettings.h"
#endif


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
