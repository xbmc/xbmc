/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartPlaylistDirectory.h"

#include "DbUrl.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "filesystem/File.h"
#include "filesystem/FileDirectoryFactory.h"
#include "media/LibraryQuery.h"
#include "media/LibraryQueryDatabase.h"
#include "media/LibraryQueryReferences.h"
#include "media/LibraryQueryType.h"
#include "playlists/PlayListTypes.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ItemProperties.h"
#include "utils/SortUtils.h"
#include "utils/log.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace KODI;
namespace LIBRARY_QUERY = KODI::MEDIA::LIBRARY_QUERY;

namespace
{
/*!
 * \brief The library path that lists what a query matches of one of its sources, carrying the
 * query as its xsp or filter option.
 * \param baseDir the path to list, or empty for the path of the source's items or of the
 * query's groups of them
 * \return nullopt if the library has no such path or the query cannot be written
 */
std::optional<std::string> PathOf(const LIBRARY_QUERY::Source& source,
                                  const MEDIA::CLibraryQuery& query,
                                  const std::string& baseDir,
                                  bool filter)
{
  std::string path{baseDir};
  if (path.empty())
  {
    const LIBRARY_QUERY::GROUP group{query.IsGrouped() ? query.GetGroup()
                                                       : LIBRARY_QUERY::GROUP::DEFAULT};
    const std::optional<std::string_view> listing{LIBRARY_QUERY::ListingPath(source, group)};
    if (!listing)
    {
      CLog::Log(LOGERROR,
                "Smart playlist \"{}\" cannot be listed: the library does not list the {} of {}",
                query.GetName(), LIBRARY_QUERY::NameOf(group, query.GetType()),
                PluralNameOf(source.itemType));
      return std::nullopt;
    }
    path = *listing;
  }

  const std::unique_ptr<CDbUrl> url{LIBRARY_QUERY::UrlOf(source.section)};
  if (!url->FromString(path))
  {
    return std::nullopt;
  }

  std::string json;
  if (!query.IsEmpty(filter) && !query.SaveAsJson(json, !filter))
  {
    return std::nullopt;
  }
  const char* const option{filter ? "filter" : "xsp"};
  if (!json.empty())
  {
    url->AddOption(option, json);
  }
  else
  {
    url->RemoveOption(option);
  }
  return url->ToString();
}
} // unnamed namespace

namespace XFILE
{
CSmartPlaylistDirectory::CSmartPlaylistDirectory() = default;

CSmartPlaylistDirectory::~CSmartPlaylistDirectory() = default;

bool CSmartPlaylistDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  // Load in the SmartPlaylist and get the WHERE query
  MEDIA::CLibraryQuery playlist;
  if (!playlist.Load(url))
    return false;
  bool result = GetDirectory(playlist, items);
  if (result)
    items.SetProperty(ITEM::PROPERTY::LIBRARY_SMARTPLAYLIST, true);

  return result;
}

bool CSmartPlaylistDirectory::GetDirectory(const MEDIA::CLibraryQuery& playlist,
                                           CFileItemList& items,
                                           const std::string& strBaseDir /* = "" */,
                                           bool filter /* = false */)
{
  const auto settings{CServiceBroker::GetSettingsComponent()->GetSettings()};

  const std::optional<MEDIA::CLibraryQueryReferences> references{
      MEDIA::CLibraryQueryReferences::Resolve(playlist)};
  if (!references)
  {
    return false;
  }

  // A path to list is of one library, and only the source in that library is asked
  const LIBRARY_QUERY::TYPE type{playlist.GetType()};
  std::vector<LIBRARY_QUERY::Source> sources;
  for (const LIBRARY_QUERY::Source& source : LIBRARY_QUERY::SourcesOf(type))
  {
    if (strBaseDir.empty() || LIBRARY_QUERY::LibraryOf(strBaseDir) == source.section)
    {
      sources.push_back(source);
    }
  }
  if (sources.empty())
  {
    return false;
  }

  // Each library lists up to the limit; what more than one lists is merged, and an ordered merge
  // is then cut to the limit
  const bool merge{sources.size() > 1};

  SortDescription sorting;
  if (playlist.GetLimit() > 0)
    sorting.limitEnd = playlist.GetLimit();
  sorting.sortBy = playlist.GetOrder();
  sorting.sortOrder = playlist.GetOrderAscending() ? SortOrder::ASCENDING : SortOrder::DESCENDING;
  sorting.sortAttributes = playlist.GetOrderAttributes();
  if (settings->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
    sorting.sortAttributes = (SortAttribute)(sorting.sortAttributes | SortAttributeIgnoreArticle);
  LIBRARY_QUERY::AddSortAttributes(type, sorting);
  items.SetSortIgnoreFolders((sorting.sortAttributes & SortAttributeIgnoreFolders) ==
                             SortAttributeIgnoreFolders);

  for (const std::string& virtualFolder : references->GetVirtualFolders())
  {
    CFileItemPtr pItem = std::make_shared<CFileItem>(virtualFolder, true);
    IFileDirectory* dir = CFileDirectoryFactory::Create(pItem->GetURL(), pItem.get());

    if (dir != NULL)
    {
      pItem->SetSpecialSort(SortSpecial::TOP);
      items.Add(pItem);
      delete dir;
    }
  }

  bool success{false};
  CFileItemList merged;
  for (const LIBRARY_QUERY::Source& source : sources)
  {
    const std::optional<std::string> path{PathOf(source, playlist, strBaseDir, filter)};
    if (!path)
    {
      continue;
    }

    if (source.itemType == MEDIA::TYPE::MUSIC_VIDEO && items.Size() <= 0 && merged.Size() <= 0)
    {
      items.SetPath(*path);
    }
    if (LIBRARY_QUERY::ListItems(source, *path, sorting, merge ? merged : items))
    {
      success = true;
    }
    items.SetProperty(ITEM::PROPERTY::PATH_DB, *path);
  }

  if (merge)
  {
    const bool ordered{playlist.GetOrder() != SortBy::NONE};
    if (ordered)
    {
      merged.Sort(sorting.sortBy, sorting.sortOrder, sorting.sortAttributes);
    }
    const int limit{ordered && playlist.GetLimit() > 0 ? static_cast<int>(playlist.GetLimit())
                                                       : merged.Size()};
    for (int i = 0; i < merged.Size() && i < limit; ++i)
    {
      items.Add(merged[i]);
    }
  }

  const LIBRARY_QUERY::GROUP group{playlist.GetGroup()};
  const std::string groupName{LIBRARY_QUERY::NameOf(group, type)};

  items.SetLabel(playlist.GetName());
  if (playlist.IsGrouped())
    items.SetContent(groupName);
  else
    items.SetContent(std::string{LIBRARY_QUERY::NameOf(type)});

  items.SetProperty(ITEM::PROPERTY::SORT_ORDER, (int)playlist.GetOrder());
  items.SetProperty(ITEM::PROPERTY::SORT_ASCENDING,
                    playlist.GetOrderDirection() == SortOrder::ASCENDING);
  if (group != LIBRARY_QUERY::GROUP::DEFAULT)
  {
    items.SetProperty(ITEM::PROPERTY::GROUP_BY, groupName);
    items.SetProperty(ITEM::PROPERTY::GROUP_MIXED, playlist.IsGroupMixed());
  }

  // sort grouped list by label unless random was specified for musicvideo artists
  if (items.Size() > 1 && group != LIBRARY_QUERY::GROUP::DEFAULT)
  {
    if (playlist.GetOrder() == SortBy::RANDOM && group == LIBRARY_QUERY::GROUP::PERFORMER &&
        type == LIBRARY_QUERY::TYPE::MUSIC_VIDEOS)
      items.Sort(SortBy::RANDOM, SortOrder::ASCENDING,
                 settings->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING)
                     ? SortAttributeIgnoreArticle
                     : SortAttributeNone);
    else
      items.Sort(SortBy::LABEL, SortOrder::ASCENDING,
                 settings->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING)
                     ? SortAttributeIgnoreArticle
                     : SortAttributeNone);
  }

  if (auto watchedMode = playlist.GetWatchedMode(); watchedMode.has_value())
    items.SetProperty(ITEM::PROPERTY::WATCHED_MODE, static_cast<int>(watchedMode.value()));

  // Hint for playlist files like STRM
  const PLAYLIST::Id playlistTypeHint{sources.back().section == MEDIA::MediaSection::MUSIC
                                          ? PLAYLIST::Id::TYPE_MUSIC
                                          : PLAYLIST::Id::TYPE_VIDEO};

  // go through and set the playlist order
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items[i];
    item->SetProgramCount(i); //! @todo remove this hack for playlist order
    item->SetProperty("playlist_type_hint", static_cast<int>(playlistTypeHint));
  }

  return success;
}

bool CSmartPlaylistDirectory::ContainsFiles(const CURL& url)
{
  return true;
}

bool CSmartPlaylistDirectory::Remove(const CURL& url)
{
  return XFILE::CFile::Delete(url);
}
} // namespace XFILE
