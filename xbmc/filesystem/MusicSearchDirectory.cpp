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

#ifndef FILESYSTEM_THREADS_SYSTEMCLOCK_H_INCLUDED
#define FILESYSTEM_THREADS_SYSTEMCLOCK_H_INCLUDED
#include "threads/SystemClock.h"
#endif

#ifndef FILESYSTEM_MUSICSEARCHDIRECTORY_H_INCLUDED
#define FILESYSTEM_MUSICSEARCHDIRECTORY_H_INCLUDED
#include "MusicSearchDirectory.h"
#endif

#ifndef FILESYSTEM_MUSIC_MUSICDATABASE_H_INCLUDED
#define FILESYSTEM_MUSIC_MUSICDATABASE_H_INCLUDED
#include "music/MusicDatabase.h"
#endif

#ifndef FILESYSTEM_URL_H_INCLUDED
#define FILESYSTEM_URL_H_INCLUDED
#include "URL.h"
#endif

#ifndef FILESYSTEM_FILEITEM_H_INCLUDED
#define FILESYSTEM_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif

#ifndef FILESYSTEM_UTILS_LOG_H_INCLUDED
#define FILESYSTEM_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif

#ifndef FILESYSTEM_UTILS_TIMEUTILS_H_INCLUDED
#define FILESYSTEM_UTILS_TIMEUTILS_H_INCLUDED
#include "utils/TimeUtils.h"
#endif

#ifndef FILESYSTEM_GUILIB_LOCALIZESTRINGS_H_INCLUDED
#define FILESYSTEM_GUILIB_LOCALIZESTRINGS_H_INCLUDED
#include "guilib/LocalizeStrings.h"
#endif


using namespace XFILE;

CMusicSearchDirectory::CMusicSearchDirectory(void)
{
}

CMusicSearchDirectory::~CMusicSearchDirectory(void)
{
}

bool CMusicSearchDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  // break up our path
  // format is:  musicsearch://<url encoded search string>
  CURL url(strPath);
  CStdString search(url.GetHostName());

  if (search.empty())
    return false;

  // and retrieve the search details
  items.SetPath(strPath);
  unsigned int time = XbmcThreads::SystemClockMillis();
  CMusicDatabase db;
  db.Open();
  db.Search(search, items);
  db.Close();
  CLog::Log(LOGDEBUG, "%s (%s) took %u ms",
            __FUNCTION__, strPath.c_str(), XbmcThreads::SystemClockMillis() - time);
  items.SetLabel(g_localizeStrings.Get(137)); // Search
  return true;
}

bool CMusicSearchDirectory::Exists(const char* strPath)
{
  return true;
}
