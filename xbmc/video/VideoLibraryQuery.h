/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dbwrappers/Database.h"
#include "dbwrappers/DatabaseQuery.h"
#include "media/LibraryQueryDatabase.h"
#include "media/LibraryQueryType.h"
#include "media/MediaType.h"

#include <optional>
#include <string>
#include <string_view>

class CFileItemList;
class CVideoDatabase;

namespace KODI::MEDIA
{
class CLibraryQueryRule;
}

//! \brief The video database's side of a library query: the SQL its rules take against the movie,
//! TV show, episode and music video views, and its listings, values and ranges.
namespace KODI::VIDEO::LIBRARY_QUERY
{

/*!
 * \brief The clause of a rule the views have a form of their own for.
 * \param type a movie, a TV show, an episode or a music video
 * \param param one of the rule's values
 * \param parameter the value as the comparison the rule's operator makes of it
 * \return the clause, or nothing where comparing the field's column is all the rule needs
 */
std::string RuleClause(const MEDIA::CLibraryQueryRule& rule,
                       MEDIA::TYPE type,
                       const std::string& negate,
                       const std::string& param,
                       const std::string& parameter,
                       const CDatabase& db);

//! \return the clause of a rule that is true or false, or nothing if the field is not one
std::string BooleanClause(const MEDIA::CLibraryQueryRule& rule,
                          MEDIA::TYPE type,
                          const std::string& negate);

//! \return the operator the rule compares with against these views
CDatabaseQueryRule::SearchOperator OperatorOf(const MEDIA::CLibraryQueryRule& rule,
                                              MEDIA::TYPE type);

/*!
 * \brief The path that lists the items of a kind in the video library, or the groups of the items
 * a query matches.
 * \param group DEFAULT or NONE for the items themselves
 * \return nullopt if the library has no such listing
 */
std::optional<std::string_view> ListingPath(MEDIA::TYPE itemType,
                                            MEDIA::LIBRARY_QUERY::GROUP group);

//! \brief The content name of the list of a group in the video library. Empty if it has none.
std::string_view GroupContent(MEDIA::LIBRARY_QUERY::GROUP group);

/*!
 * \brief Lists the values there are of a kind in the video library, for a field to be given one.
 * \param browse the kind of value
 * \param itemType the kind of item the field is of
 * \param baseDir the listing whose values are wanted; empty for those of the whole library
 * \param filter narrows the listing
 * \param countOnly asks for one item that carries the number of values as its "total" property
 * \return false if the library has no values of that kind or could not list them
 */
bool BrowseValues(CVideoDatabase& db,
                  MEDIA::LIBRARY_QUERY::BROWSE browse,
                  MEDIA::TYPE itemType,
                  const std::string& baseDir,
                  const CDatabase::Filter& filter,
                  bool countOnly,
                  CFileItemList& items);

//! \brief The column a field's range is read from, for items of a kind. Nullopt if it has none.
std::optional<MEDIA::LIBRARY_QUERY::RangeColumn> RangeColumnOf(Field field, MEDIA::TYPE itemType);

/*!
 * \brief Adds the items of a kind with these ids to \p items.
 * \param ids the ids, separated by commas
 * \return false for a kind other than music videos
 */
bool ItemsById(CVideoDatabase& db,
               MEDIA::TYPE itemType,
               const std::string& ids,
               CFileItemList& items);

} // namespace KODI::VIDEO::LIBRARY_QUERY
