/*
 *      Copyright (C) 2013 Team XBMC
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

#include <map>

#ifndef PLAYLISTS_SMARTPLAYLISTFILEITEMLISTMODIFIER_H_INCLUDED
#define PLAYLISTS_SMARTPLAYLISTFILEITEMLISTMODIFIER_H_INCLUDED
#include "SmartPlaylistFileItemListModifier.h"
#endif

#ifndef PLAYLISTS_FILEITEM_H_INCLUDED
#define PLAYLISTS_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif

#ifndef PLAYLISTS_URL_H_INCLUDED
#define PLAYLISTS_URL_H_INCLUDED
#include "URL.h"
#endif

#ifndef PLAYLISTS_PLAYLISTS_SMARTPLAYLIST_H_INCLUDED
#define PLAYLISTS_PLAYLISTS_SMARTPLAYLIST_H_INCLUDED
#include "playlists/SmartPlayList.h"
#endif

#ifndef PLAYLISTS_UTILS_STRINGUTILS_H_INCLUDED
#define PLAYLISTS_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif


#define URL_OPTION_XSP              "xsp"
#define PROPERTY_SORT_ORDER         "sort.order"
#define PROPERTY_SORT_ASCENDING     "sort.ascending"
using namespace std;

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
