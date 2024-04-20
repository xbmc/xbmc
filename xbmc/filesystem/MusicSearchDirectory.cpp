/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicSearchDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "guilib/LocalizeStrings.h"
#include "music/MusicDatabase.h"
#include "utils/log.h"

using namespace XFILE;

CMusicSearchDirectory::CMusicSearchDirectory(void) = default;

CMusicSearchDirectory::~CMusicSearchDirectory(void) = default;

bool CMusicSearchDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  // break up our path
  // format is:  musicsearch://<url encoded search string>
  const std::string& search(url.GetHostName());

  if (search.empty())
    return false;

  // and retrieve the search details
  items.SetURL(url);
  auto start = std::chrono::steady_clock::now();
  CMusicDatabase db;
  db.Open();
  db.Search(search, items);
  db.Close();

  auto end = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  CLog::Log(LOGDEBUG, "{} ({}) took {} ms", __FUNCTION__, url.GetRedacted(), duration.count());

  items.SetLabel(g_localizeStrings.Get(137)); // Search
  return true;
}

bool CMusicSearchDirectory::Exists(const CURL& url)
{
  return true;
}
