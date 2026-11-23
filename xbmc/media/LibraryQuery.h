/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "media/LibraryQueryRule.h"
#include "media/LibraryQueryType.h"
#include "utils/SortUtils.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class CURL;
class CVariant;
class CXBMCTinyXML2;
enum class WatchedMode;

namespace tinyxml2
{
class XMLNode;
} // namespace tinyxml2

namespace KODI::MEDIA
{

class CLibraryQueryReferences;

class CLibraryQuery
{
public:
  CLibraryQuery();

  bool Load(const CURL& url);
  bool Load(const std::string& path);
  bool Load(const CVariant& obj);
  bool LoadFromJson(const std::string& json);
  bool Save(const std::string& path) const;
  bool Save(CVariant& obj, bool full = true) const;
  bool SaveAsJson(std::string& json, bool full = true) const;

  bool OpenAndReadName(const CURL& url);
  bool LoadFromXML(const tinyxml2::XMLNode* root);

  void Reset();

  void SetName(const std::string& name);
  void SetType(LIBRARY_QUERY::TYPE type) { m_playlistType = type; }
  const std::string& GetName() const { return m_playlistName; }
  LIBRARY_QUERY::TYPE GetType() const { return m_playlistType; }

  void SetMatchAllRules(bool matchAll)
  {
    m_ruleCombination.SetType(matchAll ? CLibraryQueryRuleGroup::Type::COMBINATION_AND
                                       : CLibraryQueryRuleGroup::Type::COMBINATION_OR);
  }

  bool GetMatchAllRules() const
  {
    return m_ruleCombination.GetType() == CLibraryQueryRuleGroup::Type::COMBINATION_AND;
  }

  const CLibraryQueryRuleGroup::Rules& GetRules() const { return m_ruleCombination.GetRules(); }
  void AddRule(const std::shared_ptr<CLibraryQueryRule>& rule) { m_ruleCombination.AddRule(rule); }
  void RemoveRule(const std::shared_ptr<CLibraryQueryRule>& rule)
  {
    m_ruleCombination.RemoveRule(rule);
  }
  void RemoveRule(int index) { m_ruleCombination.RemoveRule(index); }

  void SetLimit(unsigned int limit) { m_limit = limit; }
  unsigned int GetLimit() const { return m_limit; }

  void SetOrder(SortBy order) { m_orderField = order; }
  SortBy GetOrder() const { return m_orderField; }
  void SetOrderAscending(bool orderAscending)
  {
    m_orderDirection = orderAscending ? SortOrder::ASCENDING : SortOrder::DESCENDING;
  }
  bool GetOrderAscending() const { return m_orderDirection != SortOrder::DESCENDING; }
  SortOrder GetOrderDirection() const { return m_orderDirection; }
  void SetOrderAttributes(SortAttribute attributes) { m_orderAttributes = attributes; }
  SortAttribute GetOrderAttributes() const { return m_orderAttributes; }
  /*!
   * \brief Puts the limit and the order the query states into the sorting of a listing. What it
   * does not state stays as the caller had it.
   * \param ignoreArticle whether the listing sorts without leading articles
   */
  void ApplyOrder(SortDescription& sorting, bool ignoreArticle) const;

  void SetGroup(LIBRARY_QUERY::GROUP group) { m_group = group; }
  LIBRARY_QUERY::GROUP GetGroup() const { return m_group; }
  void SetGroupMixed(bool mixed) { m_groupMixed = mixed; }
  bool IsGroupMixed() const { return m_groupMixed; }
  //! Whether the groups of what the query matches are listed in place of the items
  bool IsGrouped() const;

  /*!
   * \brief Whether the query applies to a listing of a library. Every reader of a library path
   * that carries a query asks this.
   * \param section the library that lists
   * \param content the content name of what is listed, as the library's URL class gives it
   * \param asFilter true for a query that came as the "filter" option, which narrows a listing of
   * its own kind of item only; false for one that came as "xsp", which also applies to the
   * listing of its groups and, for episodes, to the TV show listing their path starts from
   */
  bool AppliesTo(MediaSection section, std::string_view content, bool asFilter) const;

  std::optional<WatchedMode> GetWatchedMode() const { return m_watchedMode; }
  void SetWatchedMode(std::optional<WatchedMode> mode) { m_watchedMode = mode; }

  //! The rules, with the groups nested in them
  const CLibraryQueryRuleGroup& GetRuleGroup() const { return m_ruleCombination; }

  /*!
   * \brief The WHERE clause of the rules, asked of one of the kinds of item the type lists. The
   * playlists the rules name are found first.
   * \param db the database that formats the clause
   * \param itemType the kind of item, one of the type's sources
   * \param useOriginalDate whether a year rule on music means the original year, which is the
   * music library's original date setting; the caller reads it
   * \return the clause, or nullopt if a rule's value cannot go into it or the playlist a rule
   * names cannot be resolved
   */
  std::optional<std::string> GetWhereClause(const CDatabase& db,
                                            MEDIA::TYPE itemType,
                                            bool useOriginalDate) const;
  //! \brief The same for a listing that has already found the playlists the rules name.
  std::optional<std::string> GetWhereClause(const CDatabase& db,
                                            MEDIA::TYPE itemType,
                                            bool useOriginalDate,
                                            const CLibraryQueryReferences& references) const;

  static void GetAvailableFields(LIBRARY_QUERY::TYPE type, std::vector<std::string>& fieldList);

  bool IsEmpty(bool ignoreSortAndLimit = true) const;

private:
  //! \brief Takes the type a name names. Logs and fails if there is no name or it names none.
  bool SetTypeByName(const char* name, const std::string& source);
  //! \brief Takes the group a name names. Logs and fails if it names none.
  bool SetGroupByName(const std::string& name);
  const tinyxml2::XMLNode* readName(const tinyxml2::XMLNode* root, const std::string& file);
  const tinyxml2::XMLNode* readNameFromPath(const CURL& url, CXBMCTinyXML2& xmlDoc);
  bool load(const tinyxml2::XMLNode* root);

  CLibraryQueryRuleGroup m_ruleCombination;
  std::string m_playlistName;
  LIBRARY_QUERY::TYPE m_playlistType;

  // order information
  unsigned int m_limit;
  SortBy m_orderField;
  SortOrder m_orderDirection;
  SortAttribute m_orderAttributes;
  LIBRARY_QUERY::GROUP m_group;
  bool m_groupMixed;
  std::optional<WatchedMode> m_watchedMode;
};

} // namespace KODI::MEDIA
