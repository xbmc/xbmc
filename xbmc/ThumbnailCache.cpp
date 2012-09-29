/*
 *      Copyright (C) 2005-2012 Team XBMC
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
#include "utils/StringUtils.h"
#include "filesystem/StackDirectory.h"
#include "settings/AdvancedSettings.h"

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
