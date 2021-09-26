/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dbwrappers/DatabaseQuery.h"
#include "utils/SortUtils.h"
#include "utils/XBMCTinyXML.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

class CURL;
class CVariant;

class CSmartPlaylistRule : public CDatabaseQueryRule
{
public:
  CSmartPlaylistRule();
  ~CSmartPlaylistRule() override = default;

  std::string GetLocalizedRule() const;

  static SortBy TranslateOrder(const char *order);
  static std::string TranslateOrder(SortBy order);
  static Field TranslateGroup(const char *group);
  static std::string TranslateGroup(Field group);

  static std::string GetLocalizedField(int field);
  static std::string GetLocalizedGroup(Field group);
  static bool CanGroupMix(Field group);

  static std::vector<Field> GetFields(const std::string &type);
  static std::vector<SortBy> GetOrders(const std::string &type);
  static std::vector<Field> GetGroups(const std::string &type);
  FIELD_TYPE GetFieldType(int field) const override;
  static bool IsFieldBrowseable(int field);

  static bool Validate(const std::string &input, void *data);
  static bool ValidateRating(const std::string &input, void *data);
  static bool ValidateMyRating(const std::string &input, void *data);

protected:
  std::string GetField(int field, const std::string& type) const override;
  int TranslateField(const char *field) const override;
  std::string TranslateField(int field) const override;
  std::string FormatParameter(const std::string &negate,
                              const std::string &oper,
                              const CDatabase &db,
                              const std::string &type) const override;
  std::string FormatWhereClause(const std::string &negate,
                                const std::string& oper,
                                const std::string &param,
                                const CDatabase &db,
                                const std::string &type) const override;
  SEARCH_OPERATOR GetOperator(const std::string &type) const override;
  std::string GetBooleanQuery(const std::string &negate,
                              const std::string &strType) const override;

private:
  std::string GetVideoResolutionQuery(const std::string &parameter) const;
  static std::string FormatLinkQuery(const char *field, const char *table, const MediaType& mediaType, const std::string& mediaField, const std::string& parameter);
  std::string FormatYearQuery(const std::string& field,
                              const std::string& param,
                              const std::string& parameter) const;
};

class CSmartPlaylistRuleCombination : public CDatabaseQueryRuleCombination
{
public:
  CSmartPlaylistRuleCombination() = default;
  ~CSmartPlaylistRuleCombination() override = default;

  std::string GetWhereClause(const CDatabase &db,
                             const std::string& strType,
                             std::set<std::string> &referencedPlaylists) const;
  void GetVirtualFolders(const std::string& strType,
                         std::vector<std::string> &virtualFolders) const;

  void AddRule(const CSmartPlaylistRule &rule);
};

class CSmartPlaylist : public IDatabaseQueryRuleFactory
{
public:
  CSmartPlaylist();
  ~CSmartPlaylist() override = default;

  bool Load(const CURL& url);
  bool Load(const std::string &path);
  bool Load(const CVariant &obj);
  bool LoadFromXml(const std::string &xml);
  bool LoadFromJson(const std::string &json);
  bool Save(const std::string &path) const;
  bool Save(CVariant &obj, bool full = true) const;
  bool SaveAsJson(std::string &json, bool full = true) const;

  bool OpenAndReadName(const CURL &url);
  bool LoadFromXML(const TiXmlNode *root, const std::string &encoding = "UTF-8");

  void Reset();

  void SetName(const std::string &name);
  void SetType(const std::string &type); // music, video, mixed
  const std::string& GetName() const { return m_playlistName; }
  const std::string& GetType() const { return m_playlistType; }
  bool IsVideoType() const;
  bool IsMusicType() const;

  void SetMatchAllRules(bool matchAll) { m_ruleCombination.SetType(matchAll ? CSmartPlaylistRuleCombination::CombinationAnd : CSmartPlaylistRuleCombination::CombinationOr); }
  bool GetMatchAllRules() const { return m_ruleCombination.GetType() == CSmartPlaylistRuleCombination::CombinationAnd; }

  void SetLimit(unsigned int limit) { m_limit = limit; }
  unsigned int GetLimit() const { return m_limit; }

  void SetOrder(SortBy order) { m_orderField = order; }
  SortBy GetOrder() const { return m_orderField; }
  void SetOrderAscending(bool orderAscending)
  {
    m_orderDirection = orderAscending ? SortOrderAscending : SortOrderDescending;
  }
  bool GetOrderAscending() const { return m_orderDirection != SortOrderDescending; }
  SortOrder GetOrderDirection() const { return m_orderDirection; }
  void SetOrderAttributes(SortAttribute attributes) { m_orderAttributes = attributes; }
  SortAttribute GetOrderAttributes() const { return m_orderAttributes; }

  void SetGroup(const std::string &group) { m_group = group; }
  const std::string& GetGroup() const { return m_group; }
  void SetGroupMixed(bool mixed) { m_groupMixed = mixed; }
  bool IsGroupMixed() const { return m_groupMixed; }

  /*! \brief get the where clause for a playlist
   We handle playlists inside playlists separately in order to ensure we don't introduce infinite loops
   by playlist A including playlist B which also (perhaps via other playlists) then includes playlistA.

   \param db the database to use to format up results
   \param referencedPlaylists a set of playlists to know when we reach a cycle
   \param needWhere whether we need to prepend the where clause with "WHERE "
   */
  std::string GetWhereClause(const CDatabase &db, std::set<std::string> &referencedPlaylists) const;
  void GetVirtualFolders(std::vector<std::string> &virtualFolders) const;

  std::string GetSaveLocation() const;

  static void GetAvailableFields(const std::string &type, std::vector<std::string> &fieldList);

  static bool IsVideoType(const std::string &type);
  static bool IsMusicType(const std::string &type);
  static bool CheckTypeCompatibility(const std::string &typeLeft, const std::string &typeRight);

  bool IsEmpty(bool ignoreSortAndLimit = true) const;

  // rule creation
  CDatabaseQueryRule *CreateRule() const override;
  CDatabaseQueryRuleCombination *CreateCombination() const override;
private:
  friend class CGUIDialogSmartPlaylistEditor;
  friend class CGUIDialogMediaFilter;

  const TiXmlNode* readName(const TiXmlNode *root);
  const TiXmlNode* readNameFromPath(const CURL &url);
  const TiXmlNode* readNameFromXml(const std::string &xml);
  bool load(const TiXmlNode *root);

  CSmartPlaylistRuleCombination m_ruleCombination;
  std::string m_playlistName;
  std::string m_playlistType;

  // order information
  unsigned int m_limit;
  SortBy m_orderField;
  SortOrder m_orderDirection;
  SortAttribute m_orderAttributes;
  std::string m_group;
  bool m_groupMixed;

  CXBMCTinyXML m_xmlDoc;
};

