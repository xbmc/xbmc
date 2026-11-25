/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "media/LibraryQueryType.h"
#include "utils/DatabaseUtils.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class CDbUrl;
class CFileItemList;
class CMediaSource;
struct SortDescription;

namespace KODI::MEDIA
{
class CLibraryQuery;
class CLibraryQueryReferences;
} // namespace KODI::MEDIA

//! \brief What a library query asks of the libraries it lists from: their databases, paths,
//! settings and media sources.
namespace KODI::MEDIA::LIBRARY_QUERY
{

//! \brief A column whose lowest and highest values are a field's range, and the view it is in.
struct RangeColumn
{
  std::string_view view;
  std::string column;
  //! The rows the range is read from; empty for all of them
  std::string where;
};

//! \brief The library a library path is in. Nullopt for a path of neither library.
std::optional<MediaSection> LibraryOf(std::string_view path);

//! \brief An empty URL of a library's paths. Null for a section that is not a library.
std::unique_ptr<CDbUrl> UrlOf(MediaSection library);

//! \brief The content name of what a URL of a library's paths lists: "movies", "genres" and so on.
std::string ListedContentOf(MediaSection library, const CDbUrl& url);

/*!
 * \brief The path that lists the items of a source, or the groups of the items a query matches.
 * \param group DEFAULT or NONE for the items themselves
 * \return nullopt if the library has no such listing
 */
std::optional<std::string_view> ListingPath(const Source& source, GROUP group);

//! \brief Adds the sort attributes the settings of a type's libraries ask for.
void AddSortAttributes(TYPE type, SortDescription& sorting);

//! \brief The media sources the paths of a type's items are in.
std::vector<CMediaSource> PathSourcesOf(TYPE type);

//! \brief Lists a path of the library a source is listed from.
bool ListItems(const Source& source,
               const std::string& path,
               const SortDescription& sorting,
               CFileItemList& items);

/*!
 * \brief Lists the values of a kind there are among the items of a source, for a field to be given
 * one.
 * \param baseDir the listing whose values are wanted; empty for those of the whole library
 * \param query narrows the items; null for all of them
 * \param countOnly asks for one item that carries the number of values as its "total" property
 * \return false if the library has no values of that kind, or the query or the listing failed
 */
bool BrowseValues(const Source& source,
                  BROWSE browse,
                  const std::string& baseDir,
                  const CLibraryQuery* query,
                  bool countOnly,
                  CFileItemList& items);

/*!
 * \brief The lowest and highest value a field has among the items a path of a source lists.
 * \return nullopt if the library keeps no range of the field, or it could not be read
 */
std::optional<std::pair<int, int>> RangeOf(const Source& source,
                                           Field field,
                                           const std::string& path);

/*!
 * \brief The ids of the items of a source a query matches.
 * \param query null for all the items
 * \param references the playlists the query's rules name, found already; null to find them
 * \param sorting the order of the ids; SortBy::RANDOM shuffles them
 * \return nullopt if the database could not be opened or the query could not be put into SQL
 */
std::optional<std::vector<int>> IdsOf(const Source& source,
                                      const CLibraryQuery* query,
                                      const CLibraryQueryReferences* references,
                                      const SortDescription& sorting);

/*!
 * \brief Adds the items of a source with these ids to \p items, in no particular order.
 * \return false if the source is not of songs or music videos, or the database could not be opened
 */
bool ItemsById(const Source& source, const std::vector<int>& ids, CFileItemList& items);

} // namespace KODI::MEDIA::LIBRARY_QUERY
