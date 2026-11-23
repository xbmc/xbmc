/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LibraryQuery.h"

#include "URL.h"
#include "Util.h"
#include "media/LibraryQueryReferences.h"
#include "music/MusicLibraryQuery.h"
#include "settings/MediaSettings.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "video/VideoLibraryQuery.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace KODI::MEDIA
{

constexpr char TAG_WATCHEDMODE[] = "watchedmode";

CLibraryQuery::CLibraryQuery()
{
  Reset();
}

bool CLibraryQuery::OpenAndReadName(const CURL& url)
{
  CXBMCTinyXML2 xmlDoc;
  if (readNameFromPath(url, xmlDoc) == nullptr)
    return false;

  return !m_playlistName.empty();
}

bool CLibraryQuery::SetTypeByName(const char* name, const std::string& source)
{
  const std::optional<LIBRARY_QUERY::TYPE> type{LIBRARY_QUERY::TypeFromName(name ? name : "")};
  if (!type)
  {
    if (name)
    {
      CLog::Log(LOGERROR, "Error loading Smart playlist {}: \"{}\" is not a type of smart playlist",
                source, name);
    }
    else
    {
      CLog::Log(LOGERROR, "Error loading Smart playlist {}: it has no type", source);
    }
    return false;
  }
  m_playlistType = *type;
  return true;
}

bool CLibraryQuery::SetGroupByName(const std::string& name)
{
  const std::optional<LIBRARY_QUERY::GROUP> group{LIBRARY_QUERY::GroupFromName(name)};
  if (!group)
  {
    CLog::Log(LOGERROR,
              "Error loading Smart playlist {}: \"{}\" is not something a smart playlist can be "
              "grouped by",
              m_playlistName, name);
    return false;
  }
  m_group = *group;
  return true;
}

const tinyxml2::XMLNode* CLibraryQuery::readName(const tinyxml2::XMLNode* root,
                                                 const std::string& file)
{
  if (root == nullptr)
    return nullptr;

  const tinyxml2::XMLElement* rootElem = root->ToElement();
  if (rootElem == nullptr)
    return nullptr;

  if (!StringUtils::EqualsNoCase(root->Value(), "smartplaylist"))
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist");
    return nullptr;
  }

  const char* type = rootElem->Attribute("type");
  if (!SetTypeByName(type, file))
  {
    return nullptr;
  }

  // load the playlist name
  XMLUtils::GetString(root, "name", m_playlistName);

  return root;
}

const tinyxml2::XMLNode* CLibraryQuery::readNameFromPath(const CURL& url, CXBMCTinyXML2& xmlDoc)
{
  if (!xmlDoc.LoadFile(url.Get()) && !xmlDoc.Error())
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist {} (failed to read file)", url.GetRedacted());
    return nullptr;
  }

  const tinyxml2::XMLNode* root = readName(xmlDoc.RootElement(), url.GetRedacted());
  if (m_playlistName.empty())
  {
    m_playlistName = CUtil::GetTitleFromPath(url.Get());
    if (URIUtils::HasExtension(m_playlistName, ".xsp"))
      URIUtils::RemoveExtension(m_playlistName);
  }

  return root;
}

bool CLibraryQuery::load(const tinyxml2::XMLNode* root)
{
  if (root == nullptr)
    return false;

  return LoadFromXML(root);
}

bool CLibraryQuery::Load(const CURL& url)
{
  CXBMCTinyXML2 xmlDoc;
  return load(readNameFromPath(url, xmlDoc));
}

bool CLibraryQuery::Load(const std::string& path)
{
  const CURL pathToUrl(path);
  CXBMCTinyXML2 xmlDoc;
  return load(readNameFromPath(pathToUrl, xmlDoc));
}

bool CLibraryQuery::Load(const CVariant& obj)
{
  if (!obj.isObject())
    return false;

  const bool named{obj.isMember("type") && obj["type"].isString()};
  if (!SetTypeByName(named ? obj["type"].asString().c_str() : nullptr, "given as JSON"))
  {
    return false;
  }

  // load the playlist name
  if (obj.isMember("name") && obj["name"].isString())
    m_playlistName = obj["name"].asString();

  if (obj.isMember("rules"))
    m_ruleCombination.Load(obj["rules"]);

  if (obj.isMember("group") && obj["group"].isMember("type") && obj["group"]["type"].isString())
  {
    if (!SetGroupByName(obj["group"]["type"].asString()))
    {
      return false;
    }
    if (obj["group"].isMember("mixed") && obj["group"]["mixed"].isBoolean())
      m_groupMixed = obj["group"]["mixed"].asBoolean();
  }

  // now any limits
  if (obj.isMember("limit") && (obj["limit"].isInteger() || obj["limit"].isUnsignedInteger()) &&
      obj["limit"].asUnsignedInteger() > 0)
    m_limit = (unsigned int)obj["limit"].asUnsignedInteger();

  // and order
  if (obj.isMember("order") && obj["order"].isMember("method") && obj["order"]["method"].isString())
  {
    const CVariant& order = obj["order"];
    if (order.isMember("direction") && order["direction"].isString())
      m_orderDirection = StringUtils::EqualsNoCase(order["direction"].asString(), "ascending")
                             ? SortOrder::ASCENDING
                             : SortOrder::DESCENDING;

    if (order.isMember("ignorefolders") && order["ignorefolders"].isBoolean())
    {
      m_orderAttributes =
          order["ignorefolders"].asBoolean() ? SortAttributeIgnoreFolders : SortAttributeNone;
    }

    m_orderField = CLibraryQueryRule::TranslateOrder(obj["order"]["method"].asString().c_str());
  }

  // load the watched mode
  m_watchedMode.reset();
  if (obj.isMember(TAG_WATCHEDMODE))
  {
    if (const CVariant v = obj[TAG_WATCHEDMODE]; v.isInteger())
    {
      m_watchedMode = CMediaSettings::ToWatchedMode(v.asInteger());
    }
  }

  return true;
}

bool CLibraryQuery::LoadFromXML(const tinyxml2::XMLNode* root)
{
  if (!root)
    return false;

  std::string tmp;
  if (XMLUtils::GetString(root, "match", tmp))
    m_ruleCombination.SetType(StringUtils::EqualsNoCase(tmp, "all")
                                  ? CLibraryQueryRuleGroup::Type::COMBINATION_AND
                                  : CLibraryQueryRuleGroup::Type::COMBINATION_OR);

  // now the rules
  const tinyxml2::XMLElement* ruleNode = root->FirstChildElement("rule");
  while (ruleNode)
  {
    const auto rule{std::make_shared<CLibraryQueryRule>()};
    if (rule->Load(ruleNode))
      m_ruleCombination.AddRule(rule);

    ruleNode = ruleNode->NextSiblingElement("rule");
  }

  const tinyxml2::XMLElement* groupElement = root->FirstChildElement("group");
  if (groupElement != nullptr && groupElement->FirstChild() != nullptr)
  {
    if (!SetGroupByName(groupElement->FirstChild()->Value()))
    {
      return false;
    }
    const char* mixed = groupElement->Attribute("mixed");
    m_groupMixed = mixed != nullptr && StringUtils::EqualsNoCase(mixed, "true");
  }

  // now any limits
  // format is <limit>25</limit>
  XMLUtils::GetUInt(root, "limit", m_limit);

  // and order
  // format is <order direction="ascending">field</order>
  const tinyxml2::XMLElement* order = root->FirstChildElement("order");
  if (order && order->FirstChild())
  {
    const char* direction = order->Attribute("direction");
    if (direction)
      m_orderDirection = StringUtils::EqualsNoCase(direction, "ascending") ? SortOrder::ASCENDING
                                                                           : SortOrder::DESCENDING;

    const char* ignorefolders = order->Attribute("ignorefolders");
    if (ignorefolders != nullptr)
      m_orderAttributes = StringUtils::EqualsNoCase(ignorefolders, "true")
                              ? SortAttributeIgnoreFolders
                              : SortAttributeNone;

    m_orderField = CLibraryQueryRule::TranslateOrder(order->FirstChild()->Value());
  }

  if (int wm; XMLUtils::GetInt(root, TAG_WATCHEDMODE, wm))
    m_watchedMode = CMediaSettings::ToWatchedMode(wm);

  return true;
}

bool CLibraryQuery::LoadFromJson(const std::string& json)
{
  if (json.empty())
    return false;

  CVariant obj;
  if (!CJSONVariantParser::Parse(json, obj))
    return false;

  return Load(obj);
}

bool CLibraryQuery::Save(const std::string& path) const
{
  CXBMCTinyXML2 doc;
  doc.InsertEndChild(doc.NewDeclaration(R"(xml version="1.0" encoding="UTF-8" standalone="yes")"));

  tinyxml2::XMLElement* xmlRootElement = doc.NewElement("smartplaylist");
  xmlRootElement->SetAttribute("type", std::string{LIBRARY_QUERY::NameOf(m_playlistType)}.c_str());
  tinyxml2::XMLNode* pRoot = doc.InsertEndChild(xmlRootElement);
  if (!pRoot)
    return false;

  // add the <name> tag
  XMLUtils::SetString(pRoot, "name", m_playlistName);

  // add the <match> tag
  XMLUtils::SetString(
      pRoot, "match",
      m_ruleCombination.GetType() == CLibraryQueryRuleGroup::Type::COMBINATION_AND ? "all" : "one");

  // add <rule> tags
  m_ruleCombination.Save(pRoot);

  // add <group> tag if necessary
  if (m_group != LIBRARY_QUERY::GROUP::DEFAULT)
  {
    tinyxml2::XMLElement* nodeGroup = doc.NewElement("group");
    if (m_groupMixed)
      nodeGroup->SetAttribute("mixed", "true");
    nodeGroup->InsertEndChild(
        doc.NewText(std::string{LIBRARY_QUERY::NameOf(m_group, m_playlistType)}.c_str()));
    pRoot->InsertEndChild(nodeGroup);
  }

  // add <limit> tag
  if (m_limit)
    XMLUtils::SetInt(pRoot, "limit", m_limit);

  // add <order> tag
  if (m_orderField != SortBy::NONE)
  {
    tinyxml2::XMLElement* nodeOrder = doc.NewElement("order");
    nodeOrder->SetAttribute("direction",
                            m_orderDirection == SortOrder::DESCENDING ? "descending" : "ascending");
    if (m_orderAttributes & SortAttributeIgnoreFolders)
      nodeOrder->SetAttribute("ignorefolders", "true");
    nodeOrder->InsertEndChild(doc.NewText(CLibraryQueryRule::TranslateOrder(m_orderField).c_str()));
    pRoot->InsertEndChild(nodeOrder);
  }

  // add the <watchedmode> tag
  if (m_watchedMode.has_value())
    XMLUtils::SetInt(pRoot, TAG_WATCHEDMODE, static_cast<int>(m_watchedMode.value()));

  return doc.SaveFile(path);
}

bool CLibraryQuery::Save(CVariant& obj, bool full /* = true */) const
{
  if (obj.type() == CVariant::VariantTypeConstNull)
    return false;

  obj.clear();
  // add "type"
  obj["type"] = std::string{LIBRARY_QUERY::NameOf(m_playlistType)};

  // add "rules"
  CVariant rulesObj = CVariant(CVariant::VariantTypeObject);
  if (m_ruleCombination.Save(rulesObj))
    obj["rules"] = rulesObj;

  // add "group"
  if (m_group != LIBRARY_QUERY::GROUP::DEFAULT)
  {
    obj["group"]["type"] = std::string{LIBRARY_QUERY::NameOf(m_group, m_playlistType)};
    obj["group"]["mixed"] = m_groupMixed;
  }

  // add "limit"
  if (full && m_limit)
    obj["limit"] = m_limit;

  // add "order"
  if (full && m_orderField != SortBy::NONE)
  {
    obj["order"] = CVariant(CVariant::VariantTypeObject);
    obj["order"]["method"] = CLibraryQueryRule::TranslateOrder(m_orderField);
    obj["order"]["direction"] =
        m_orderDirection == SortOrder::DESCENDING ? "descending" : "ascending";
    obj["order"]["ignorefolders"] = (m_orderAttributes & SortAttributeIgnoreFolders) != 0;
  }

  // add "watchedmode"
  if (m_watchedMode.has_value())
    obj[TAG_WATCHEDMODE] = static_cast<int>(m_watchedMode.value());

  return true;
}

bool CLibraryQuery::SaveAsJson(std::string& json, bool full /* = true */) const
{
  CVariant xsp(CVariant::VariantTypeObject);
  if (!Save(xsp, full))
    return false;

  return CJSONVariantWriter::Write(xsp, json, true) && !json.empty();
}

void CLibraryQuery::Reset()
{
  m_ruleCombination.clear();
  m_limit = 0;
  m_orderField = SortBy::NONE;
  m_orderDirection = SortOrder::NONE;
  m_orderAttributes = SortAttributeNone;
  m_playlistType = LIBRARY_QUERY::TYPE::SONGS;
  m_group = LIBRARY_QUERY::GROUP::DEFAULT;
  m_groupMixed = false;
  m_watchedMode.reset();
}

void CLibraryQuery::SetName(const std::string& name)
{
  m_playlistName = name;
}

std::optional<std::string> CLibraryQuery::GetWhereClause(const CDatabase& db,
                                                         MEDIA::TYPE itemType,
                                                         bool useOriginalDate) const
{
  const std::optional<CLibraryQueryReferences> references{CLibraryQueryReferences::Resolve(*this)};
  if (!references)
  {
    return std::nullopt;
  }
  return GetWhereClause(db, itemType, useOriginalDate, *references);
}

std::optional<std::string> CLibraryQuery::GetWhereClause(
    const CDatabase& db,
    MEDIA::TYPE itemType,
    bool useOriginalDate,
    const CLibraryQueryReferences& references) const
{
  return m_ruleCombination.GetWhereClause(
      db, CLibraryQueryContext{m_playlistType, itemType, useOriginalDate, &references});
}

void CLibraryQuery::ApplyOrder(SortDescription& sorting, bool ignoreArticle) const
{
  if (m_limit > 0)
  {
    sorting.limitEnd = m_limit;
  }
  if (m_orderField != SortBy::NONE)
  {
    sorting.sortBy = m_orderField;
  }
  if (m_orderDirection != SortOrder::NONE)
  {
    sorting.sortOrder = m_orderDirection;
  }
  sorting.sortAttributes =
      static_cast<SortAttribute>(sorting.sortAttributes | m_orderAttributes |
                                 (ignoreArticle ? SortAttributeIgnoreArticle : SortAttributeNone));
}

bool CLibraryQuery::IsGrouped() const
{
  return m_group != LIBRARY_QUERY::GROUP::DEFAULT && m_group != LIBRARY_QUERY::GROUP::NONE &&
         !m_groupMixed;
}

bool CLibraryQuery::AppliesTo(MediaSection section, std::string_view content, bool asFilter) const
{
  const std::optional<MEDIA::TYPE> itemType{LIBRARY_QUERY::ItemTypeIn(m_playlistType, section)};
  if (!itemType)
  {
    return false;
  }

  const std::optional<LIBRARY_QUERY::TYPE> listed{LIBRARY_QUERY::TypeOf(*itemType)};
  if (listed && content == LIBRARY_QUERY::NameOf(*listed))
  {
    return true;
  }
  if (asFilter)
  {
    return false;
  }

  if (IsGrouped())
  {
    const std::string_view groups{section == MediaSection::MUSIC
                                      ? MUSIC::LIBRARY_QUERY::GroupContent(m_group)
                                      : VIDEO::LIBRARY_QUERY::GroupContent(m_group)};
    if (!groups.empty() && content == groups)
    {
      return true;
    }
  }

  // Episodes are asked for by the path of the TV shows, which the season and episode are added to
  return *itemType == MEDIA::TYPE::EPISODE &&
         content == LIBRARY_QUERY::NameOf(LIBRARY_QUERY::TYPE::TV_SHOWS);
}

void CLibraryQuery::GetAvailableFields(LIBRARY_QUERY::TYPE type,
                                       std::vector<std::string>& fieldList)
{
  for (const Field field : CLibraryQueryRule::GetFields(type))
  {
    fieldList.emplace_back(CLibraryQueryRule::GetFieldName(field));
  }
}

bool CLibraryQuery::IsEmpty(bool ignoreSortAndLimit /* = true */) const
{
  bool empty = m_ruleCombination.empty();
  if (empty && !ignoreSortAndLimit)
    empty = m_limit == 0 && m_orderField == SortBy::NONE && m_orderDirection == SortOrder::NONE;

  return empty;
}

} // namespace KODI::MEDIA
