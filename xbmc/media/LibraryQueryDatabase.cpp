/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LibraryQueryDatabase.h"

#include "FileItemList.h"
#include "MediaSource.h"
#include "ServiceBroker.h"
#include "media/LibraryQuery.h"
#include "media/LibraryQueryReferences.h"
#include "music/MusicDatabase.h"
#include "music/MusicDbPaths.h"
#include "music/MusicDbUrl.h"
#include "music/MusicLibraryQuery.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoDbPaths.h"
#include "video/VideoDbUrl.h"
#include "video/VideoLibraryQuery.h"

#include <cstdlib>
#include <type_traits>
#include <utility>

namespace KODI::MEDIA::LIBRARY_QUERY
{

namespace
{
// Each library's functions take its own database, so a call on a database picks its library's.
using MUSIC::LIBRARY_QUERY::BrowseValues;
using MUSIC::LIBRARY_QUERY::ItemsById;
using VIDEO::LIBRARY_QUERY::BrowseValues;
using VIDEO::LIBRARY_QUERY::ItemsById;

template<typename DATABASE>
constexpr bool IS_MUSIC_DATABASE{std::is_same_v<std::remove_cvref_t<DATABASE>, CMusicDatabase>};

bool IsMusic(const Source& source)
{
  return source.section == MediaSection::MUSIC;
}

//! The ORDER BY of a sort for items of a kind; empty for none
std::string OrderByOf(MEDIA::TYPE itemType, const SortDescription& sorting)
{
  if (sorting.sortBy == SortBy::NONE)
  {
    return {};
  }
  if (sorting.sortBy == SortBy::RANDOM)
  {
    return DatabaseUtils::GetField(Field::RANDOM, itemType, DatabaseQueryPart::ORDER_BY);
  }

  FieldList fields;
  SortUtils::GetFieldsForSQLSort(itemType, sorting.sortBy, fields);
  std::vector<std::string> columns;
  for (const Field field : fields)
  {
    std::string column{DatabaseUtils::GetField(field, itemType, DatabaseQueryPart::ORDER_BY)};
    if (!column.empty())
    {
      if (sorting.sortOrder == SortOrder::DESCENDING)
      {
        column += " DESC";
      }
      columns.push_back(std::move(column));
    }
  }
  return StringUtils::Join(columns, ", ");
}

/*!
 * \brief Opens the database a source is listed from and calls \p use with it.
 * \return what \p use returns, or an empty result if the database could not be opened
 */
template<typename USE>
auto WithDatabase(const Source& source, USE&& use) -> decltype(use(std::declval<CMusicDatabase&>()))
{
  using Result = decltype(use(std::declval<CMusicDatabase&>()));
  if (IsMusic(source))
  {
    CMusicDatabase db;
    if (!db.Open())
    {
      CLog::LogF(LOGERROR, "could not open the music database");
      return Result{};
    }
    return use(db);
  }
  CVideoDatabase db;
  if (!db.Open())
  {
    CLog::LogF(LOGERROR, "could not open the video database");
    return Result{};
  }
  return use(db);
}

/*!
 * \brief The WHERE clause of a query for a source's items; empty without a query. A year rule on
 * music means the original year when the music library's original date setting is on.
 */
std::optional<std::string> WhereOf(const CDatabase& db,
                                   const Source& source,
                                   const CLibraryQuery* query,
                                   const CLibraryQueryReferences* references)
{
  if (!query)
  {
    return std::string{};
  }
  const bool useOriginalDate{IsMusic(source) &&
                             CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                                 CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE)};
  return references ? query->GetWhereClause(db, source.itemType, useOriginalDate, *references)
                    : query->GetWhereClause(db, source.itemType, useOriginalDate);
}

std::string IdList(const std::vector<int>& ids)
{
  std::string list;
  for (const int id : ids)
  {
    list += StringUtils::Format("{},", id);
  }
  if (!list.empty())
  {
    list.pop_back();
  }
  return list;
}
} // unnamed namespace

std::optional<MediaSection> LibraryOf(std::string_view path)
{
  if (path.starts_with(MUSIC::DB_PATH::ROOT))
  {
    return MediaSection::MUSIC;
  }
  if (path.starts_with(VIDEO::DB_PATH::ROOT))
  {
    return MediaSection::VIDEO;
  }
  return std::nullopt;
}

std::unique_ptr<CDbUrl> UrlOf(MediaSection library)
{
  switch (library)
  {
    case MediaSection::MUSIC:
      return std::make_unique<CMusicDbUrl>();
    case MediaSection::VIDEO:
      return std::make_unique<CVideoDbUrl>();
    default:
      return nullptr;
  }
}

std::string ListedContentOf(MediaSection library, const CDbUrl& url)
{
  return library == MediaSection::VIDEO ? static_cast<const CVideoDbUrl&>(url).GetItemType()
                                        : url.GetType();
}

std::optional<std::string_view> ListingPath(const Source& source, GROUP group)
{
  return IsMusic(source) ? MUSIC::LIBRARY_QUERY::ListingPath(source.itemType, group)
                         : VIDEO::LIBRARY_QUERY::ListingPath(source.itemType, group);
}

void AddSortAttributes(TYPE type, SortDescription& sorting)
{
  if (ListsFrom(type, MediaSection::MUSIC) &&
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_MUSICLIBRARY_USEARTISTSORTNAME))
  {
    sorting.sortAttributes =
        static_cast<SortAttribute>(sorting.sortAttributes | SortAttributeUseArtistSortName);
  }
}

std::vector<CMediaSource> PathSourcesOf(TYPE type)
{
  std::vector<CMediaSource> sources;
  for (const Source& source : SourcesOf(type))
  {
    // Of music, only songs have a path of their own
    if (IsMusic(source) && source.itemType != MEDIA::TYPE::SONG)
    {
      continue;
    }
    const std::vector<CMediaSource> sectionSources{
        CMediaSourceSettings::GetInstance().GetSources(source.section)};
    sources.insert(sources.end(), sectionSources.begin(), sectionSources.end());
  }
  return sources;
}

bool ListItems(const Source& source,
               const std::string& path,
               const SortDescription& sorting,
               CFileItemList& items)
{
  return WithDatabase(source,
                      [&](auto& db) -> bool
                      {
                        // The two databases take the filter and the sort in opposite orders
                        if constexpr (IS_MUSIC_DATABASE<decltype(db)>)
                          return db.GetItems(path, items, sorting, CDatabase::Filter{});
                        else
                          return db.GetItems(path, items, CDatabase::Filter{}, sorting);
                      });
}

bool BrowseValues(const Source& source,
                  BROWSE browse,
                  const std::string& baseDir,
                  const CLibraryQuery* query,
                  bool countOnly,
                  CFileItemList& items)
{
  return WithDatabase(source,
                      [&](auto& db) -> bool
                      {
                        const std::optional<std::string> where{WhereOf(db, source, query, nullptr)};
                        return where && BrowseValues(db, browse, source.itemType, baseDir,
                                                     CDatabase::Filter(*where), countOnly, items);
                      });
}

std::optional<std::pair<int, int>> RangeOf(const Source& source,
                                           Field field,
                                           const std::string& path)
{
  const std::optional<RangeColumn> range{
      IsMusic(source) ? MUSIC::LIBRARY_QUERY::RangeColumnOf(field, source.itemType)
                      : VIDEO::LIBRARY_QUERY::RangeColumnOf(field, source.itemType)};
  if (!range)
  {
    return std::nullopt;
  }

  return WithDatabase(
      source,
      [&](CDatabase& db) -> std::optional<std::pair<int, int>>
      {
        CDatabase::Filter filter(range->where);
        std::string sqlExtra;
        const std::unique_ptr<CDbUrl> url{UrlOf(source.section)};
        if (!db.BuildSQL(path, sqlExtra, filter, sqlExtra, *url))
        {
          return std::nullopt;
        }

        const std::string column{db.PrepareSQL(range->column)};
        const std::string view{range->view};
        const auto aggregate = [&db, &column, &view, &sqlExtra](const std::string& function)
        {
          const std::string sql{db.PrepareSQL("SELECT %s FROM %s ",
                                              (function + "(" + column + ")").c_str(),
                                              view.c_str()) +
                                sqlExtra};
          return static_cast<int>(strtol(db.GetSingleValue(sql).c_str(), nullptr, 0));
        };
        return std::make_pair(aggregate("MIN"), aggregate("MAX"));
      });
}

std::optional<std::vector<int>> IdsOf(const Source& source,
                                      const CLibraryQuery* query,
                                      const CLibraryQueryReferences* references,
                                      const SortDescription& sorting)
{
  const std::string_view view{DatabaseUtils::ViewNameOf(source.itemType)};
  const std::string id{
      DatabaseUtils::GetField(Field::ID, source.itemType, DatabaseQueryPart::SELECT)};
  if (view.empty() || id.empty())
  {
    return std::nullopt;
  }

  return WithDatabase(
      source,
      [&](CDatabase& db) -> std::optional<std::vector<int>>
      {
        const std::optional<std::string> where{WhereOf(db, source, query, references)};
        if (!where)
        {
          return std::nullopt;
        }

        std::string sql{StringUtils::Format("SELECT {} FROM {}", id, view)};
        if (!where->empty())
        {
          sql += " WHERE " + *where;
        }
        if (const std::string order{OrderByOf(source.itemType, sorting)}; !order.empty())
        {
          sql += " ORDER BY " + order;
        }
        CLog::LogF(LOGDEBUG, "{}", sql);

        std::vector<int> ids;
        if (!db.GetIntValues(sql, ids))
        {
          return std::nullopt;
        }
        return ids;
      });
}

bool ItemsById(const Source& source, const std::vector<int>& ids, CFileItemList& items)
{
  return WithDatabase(source, [&](auto& db) -> bool
                      { return ItemsById(db, source.itemType, IdList(ids), items); });
}

} // namespace KODI::MEDIA::LIBRARY_QUERY
