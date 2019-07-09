/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicSearchDirectory.h"

#include "FileItem.h"
#include "URL.h"
#include "guilib/LocalizeStrings.h"
#include "music/MusicDatabase.h"
#include "threads/SystemClock.h"
#include "utils/log.h"

using namespace XFILE;

CMusicSearchDirectory::CMusicSearchDirectory(void) = default;

CMusicSearchDirectory::~CMusicSearchDirectory(void) = default;

bool CMusicSearchDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  // break up our path
  // format is:  musicsearch://<url encoded search string>
  std::string search(url.GetHostName());

  if (search.empty())
    return false;

  // and retrieve the search details
  items.SetURL(url);
  unsigned int time = XbmcThreads::SystemClockMillis();
  CMusicDatabase db;
  db.Open();
  db.Search(search, items);
  db.Close();
  CLog::Log(LOGDEBUG, "%s (%s) took %u ms",
            __FUNCTION__, url.GetRedacted().c_str(), XbmcThreads::SystemClockMillis() - time);
  items.SetLabel(g_localizeStrings.Get(137)); // Search
  return true;
}

bool CMusicSearchDirectory::Exists(const CURL& url)
{
  return true;
}
