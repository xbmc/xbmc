#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <set>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "dbwrappers/DatabaseQuery.h"
#include "utils/SortUtils.h"
#include "utils/StdString.h"
#include "utils/XBMCTinyXML.h"

class CVariant;

class CSmartPlaylistRule : public CDatabaseQueryRule
{
public:
  CSmartPlaylistRule();
  virtual ~CSmartPlaylistRule() { }

  CStdString                  GetLocalizedRule() const;

  static SortBy               TranslateOrder(const char *order);
  static CStdString           TranslateOrder(SortBy order);
  static Field                TranslateGroup(const char *group);
  static CStdString           TranslateGroup(Field group);

  static CStdString           GetLocalizedField(int field);
  static CStdString           GetLocalizedGroup(Field group);
  static bool                 CanGroupMix(Field group);

  static std::vector<Field>   GetFields(const CStdString &type);
  static std::vector<SortBy>  GetOrders(const CStdString &type);
  static std::vector<Field>   GetGroups(const CStdString &type);
  virtual FIELD_TYPE          GetFieldType(int field) const;
  static bool                 IsFieldBrowseable(int field);

  static bool Validate(const std::string &input, void *data);
  static bool ValidateRating(const std::string &input, void *data);

protected:
  virtual CStdString          GetField(int field, const CStdString& type) const;
  virtual int                 TranslateField(const char *field) const;
  virtual CStdString          TranslateField(int field) const;
  virtual CStdString          FormatParameter(const CStdString &negate, const CStdString &oper, const CDatabase &db, const CStdString &type) const;
  virtual CStdString          FormatWhereClause(const CStdString &negate, const CStdString &oper, const CStdString &param,
                                                const CDatabase &db, const CStdString &type) const;
  virtual SEARCH_OPERATOR     GetOperator(const CStdString &type) const;
  virtual CStdString          GetBooleanQuery(const CStdString &negate, const CStdString &strType) const;

private:
  CStdString GetVideoResolutionQuery(const CStdString &parameter) const;
};

class CSmartPlaylistRuleCombination : public CDatabaseQueryRuleCombination
{
public:
  CSmartPlaylistRuleCombination() { }
  virtual ~CSmartPlaylistRuleCombination() { }

  CStdString GetWhereClause(const CDatabase &db, const CStdString& strType, std::set<CStdString> &referencedPlaylists) const;
  void GetVirtualFolders(const CStdString& strType, std::vector<CStdString> &virtualFolders) const;

  void AddRule(const CSmartPlaylistRule &rule);
};

class CSmartPlaylist : public IDatabaseQueryRuleFactory
{
public:
  CSmartPlaylist();
  virtual ~CSmartPlaylist() { }

  bool Load(const CStdString &path);
  bool Load(const CVariant &obj);
  bool LoadFromXml(const CStdString &xml);
  bool LoadFromJson(const CStdString &json);
  bool Save(const CStdString &path) const;
  bool Save(CVariant &obj, bool full = true) const;
  bool SaveAsJson(CStdString &json, bool full = true) const;

  bool OpenAndReadName(const CStdString &path);
  bool LoadFromXML(const TiXmlNode *root, const CStdString &encoding = "UTF-8");

  void Reset();

  void SetName(const CStdString &name);
  void SetType(const CStdString &type); // music, video, mixed
  const CStdString& GetName() const { return m_playlistName; };
  const CStdString& GetType() const { return m_playlistType; };
  bool IsVideoType() const;
  bool IsMusicType() const;

  void SetMatchAllRules(bool matchAll) { m_ruleCombination.SetType(matchAll ? CSmartPlaylistRuleCombination::CombinationAnd : CSmartPlaylistRuleCombination::CombinationOr); }
  bool GetMatchAllRules() const { return m_ruleCombination.GetType() == CSmartPlaylistRuleCombination::CombinationAnd; }

  void SetLimit(unsigned int limit) { m_limit = limit; };
  unsigned int GetLimit() const { return m_limit; };

  void SetOrder(SortBy order) { m_orderField = order; };
  SortBy GetOrder() const { return m_orderField; };
  void SetOrderAscending(bool orderAscending) { m_orderDirection = orderAscending ? SortOrderAscending : SortOrderDescending; };
  bool GetOrderAscending() const { return m_orderDirection != SortOrderDescending; };
  SortOrder GetOrderDirection() const { return m_orderDirection; }
  void SetOrderAttributes(SortAttribute attributes) { m_orderAttributes = attributes; }
  SortAttribute GetOrderAttributes() const { return m_orderAttributes; }

  void SetGroup(const CStdString &group) { m_group = group; }
  const CStdString& GetGroup() const { return m_group; }
  void SetGroupMixed(bool mixed) { m_groupMixed = mixed; }
  bool IsGroupMixed() const { return m_groupMixed; }

  /*! \brief get the where clause for a playlist
   We handle playlists inside playlists separately in order to ensure we don't introduce infinite loops
   by playlist A including playlist B which also (perhaps via other playlists) then includes playlistA.
   
   \param db the database to use to format up results
   \param referencedPlaylists a set of playlists to know when we reach a cycle
   \param needWhere whether we need to prepend the where clause with "WHERE "
   */
  CStdString GetWhereClause(const CDatabase &db, std::set<CStdString> &referencedPlaylists) const;
  void GetVirtualFolders(std::vector<CStdString> &virtualFolders) const;

  CStdString GetSaveLocation() const;

  static void GetAvailableFields(const std::string &type, std::vector<std::string> &fieldList);

  static bool IsVideoType(const CStdString &type);
  static bool IsMusicType(const CStdString &type);
  static bool CheckTypeCompatibility(const CStdString &typeLeft, const CStdString &typeRight);

  bool IsEmpty(bool ignoreSortAndLimit = true) const;

  // rule creation
  virtual CDatabaseQueryRule *CreateRule() const;
  virtual CDatabaseQueryRuleCombination *CreateCombination() const;
private:
  friend class CGUIDialogSmartPlaylistEditor;
  friend class CGUIDialogMediaFilter;

  const TiXmlNode* readName(const TiXmlNode *root);
  const TiXmlNode* readNameFromPath(const CStdString &path);
  const TiXmlNode* readNameFromXml(const CStdString &xml);
  bool load(const TiXmlNode *root);

  CSmartPlaylistRuleCombination m_ruleCombination;
  CStdString m_playlistName;
  CStdString m_playlistType;

  // order information
  unsigned int m_limit;
  SortBy m_orderField;
  SortOrder m_orderDirection;
  SortAttribute m_orderAttributes;
  CStdString m_group;
  bool m_groupMixed;

  CXBMCTinyXML m_xmlDoc;
};

