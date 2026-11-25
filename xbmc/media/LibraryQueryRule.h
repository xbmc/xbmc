/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dbwrappers/DatabaseQuery.h"
#include "media/LibraryQueryType.h"
#include "media/MediaType.h"
#include "utils/DatabaseUtils.h"
#include "utils/SortUtils.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace KODI::MEDIA
{

class CLibraryQueryReferences;

//! \brief What a library query's rules need to know to be put into SQL.
struct CLibraryQueryContext
{
  //! The type of the query the rules belong to, which says what fields they can have
  LIBRARY_QUERY::TYPE type{LIBRARY_QUERY::TYPE::SONGS};
  //! The kind of library item the rules are asked of
  MEDIA::TYPE itemType{MEDIA::TYPE::NONE};
  //! Whether a year rule on music means the original year, as the music library is set to show
  bool useOriginalDate{false};
  //! The playlists the rules name, found for this listing
  const CLibraryQueryReferences* references{nullptr};
};

class CLibraryQueryRule : public CDatabaseQueryRuleOf<Field, CLibraryQueryContext>
{
public:
  //! What separates a rule's values when they are shown or edited as one text
  static constexpr const char* VALUE_SEPARATOR = " / ";

  CLibraryQueryRule();
  ~CLibraryQueryRule() override = default;

  std::string GetLocalizedRule() const;

  //! The rule's values as one text
  std::string GetParameter() const;
  void SetParameter(const std::string& value);

  std::optional<std::string> GetWhereClause(const CDatabase& db,
                                            const CLibraryQueryContext& context) const override;
  bool IsFilter() const override { return m_field != Field::VIRTUAL_FOLDER; }

  static SortBy TranslateOrder(const char* order);
  static std::string TranslateOrder(SortBy order);

  //! The name a field has in .xsp files and JSON filters
  static std::string GetFieldName(Field field);
  //! The id of the string that labels a field
  static int GetFieldLabel(Field field);
  static std::string GetLocalizedField(Field field);
  //! What a field's values are browsed from; NOTHING if they cannot be
  static LIBRARY_QUERY::BROWSE BrowseOf(Field field);
  //! \param type the type of the query grouped, which decides what the performers are called
  static std::string GetLocalizedGroup(LIBRARY_QUERY::GROUP group, LIBRARY_QUERY::TYPE type);

  //! The fields a query of a type can have rules on, in the order they are offered
  static std::vector<Field> GetFields(LIBRARY_QUERY::TYPE type);
  static bool HasField(LIBRARY_QUERY::TYPE type, Field field);
  static std::vector<SortBy> GetOrders(LIBRARY_QUERY::TYPE type);
  static std::vector<LIBRARY_QUERY::GROUP> GetGroups(LIBRARY_QUERY::TYPE type);
  CDatabaseQueryRule::FieldType GetFieldType(Field field) const override;
  static bool IsFieldBrowseable(Field field);
  //! Whether the field's value names another smart playlist
  static bool IsPlaylistField(Field field);

  //! SQL comparing a date column that may be NULL with a rule's formatted value
  static std::string FormatNullableDate(const std::string& field,
                                        CDatabaseQueryRule::SearchOperator oper,
                                        const std::string& parameter);
  //! SQL comparing a number column that may be NULL with a rule's formatted value
  static std::string FormatNullableNumber(const std::string& field,
                                          CDatabaseQueryRule::SearchOperator oper,
                                          std::string_view param,
                                          const std::string& parameter);

  static bool Validate(const std::string& input, void* data);
  static bool ValidateDate(const std::string& input, void* data);

protected:
  std::string GetField(Field field, const CLibraryQueryContext& context) const override;
  Field TranslateField(const char* field) const override;
  std::string TranslateField(Field field) const override;
  std::optional<std::string> FormatParameter(const std::string& operatorString,
                                             const std::string& param,
                                             const CDatabase& db) const override;
  std::optional<std::string> FormatWhereClause(const std::string& negate,
                                               const std::string& oper,
                                               const std::string& param,
                                               const CDatabase& db,
                                               const CLibraryQueryContext& context) const override;
  SearchOperator GetOperator(const CLibraryQueryContext& context) const override;
  std::string GetBooleanQuery(const std::string& negate,
                              const CLibraryQueryContext& context) const override;
};

using CLibraryQueryRuleGroup = CDatabaseQueryRuleCombination<CLibraryQueryRule>;

} // namespace KODI::MEDIA

extern template class CDatabaseQueryRuleOf<Field, KODI::MEDIA::CLibraryQueryContext>;
extern template class CDatabaseQueryRuleCombination<KODI::MEDIA::CLibraryQueryRule>;
