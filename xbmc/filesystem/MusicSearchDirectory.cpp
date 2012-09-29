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

#include "threads/SystemClock.h"
#include "MusicSearchDirectory.h"
#include "music/MusicDatabase.h"
#include "URL.h"
#include "FileItem.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "guilib/LocalizeStrings.h"

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

  if (search.IsEmpty())
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
