/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartPlaylistFileItemListModifier.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "playlists/SmartPlayList.h"
#include "utils/StringUtils.h"

#include <string>

#define URL_OPTION_XSP              "xsp"
#define PROPERTY_SORT_ORDER         "sort.order"
#define PROPERTY_SORT_ASCENDING     "sort.ascending"

bool CSmartPlaylistFileItemListModifier::CanModify(const CFileItemList &items) const
{
  return !GetUrlOption(items.GetPath(), URL_OPTION_XSP).empty();
}

bool CSmartPlaylistFileItemListModifier::Modify(CFileItemList &items) const
{
  if (items.HasProperty(PROPERTY_SORT_ORDER))
    return false;

  std::string xspOption = GetUrlOption(items.GetPath(), URL_OPTION_XSP);
  if (xspOption.empty())
    return false;

  // check for smartplaylist-specific sorting information
  CSmartPlaylist xsp;
  if (!xsp.LoadFromJson(xspOption))
    return false;

  items.SetProperty(PROPERTY_SORT_ORDER, (int)xsp.GetOrder());
  items.SetProperty(PROPERTY_SORT_ASCENDING, xsp.GetOrderDirection() == SortOrderAscending);

  return true;
}

std::string CSmartPlaylistFileItemListModifier::GetUrlOption(const std::string &path, const std::string &option)
{
  if (path.empty() || option.empty())
    return StringUtils::Empty;

  CURL url(path);
  return url.GetOption(option);
}
