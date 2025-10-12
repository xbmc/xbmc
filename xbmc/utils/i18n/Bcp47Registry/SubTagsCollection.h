/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/StringUtils.h"
#include "utils/i18n/Bcp47Registry/SubTagRegistryTypes.h"
#include "utils/log.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace KODI::UTILS::I18N
{
class CSubTagRegistryManager;

template<std::derived_from<BaseSubTag> T>
class CSubTagsCollection
{
public:
  friend class CSubTagRegistryManager;

  /*!
   * \brief Find a registry subtag by subtag name
   * \param[in] name The subtag name, in lower case
   * \return Matching registry record or std::nullopt when no subtags of the registry have the provided name.
   */
  std::optional<T> Lookup(const std::string& name) const;
  // possible future optimization: heterogeneous lookup from string_view

  /*!
   * \brief Find a registry subtag by description.
   *        The first matching subtag is returned when multiple subtags have a matching description.
   * \param[in] description The subtag description, in lower case
   * \return First matching registry record or std::nullopt when no subtags of the registry have the provided description.
   */
  std::optional<T> LookupByDescription(const std::string& description) const;

protected:
  bool EmplaceRecords(const std::vector<RegistryFileRecord>& records);

  void Reset();
  std::size_t Size() const { return m_subTags.size(); }
  std::size_t SizeDesc() const { return m_descToSubTags.size(); }

private:
  bool EmplaceRecord(const RegistryFileRecord& record);
  std::vector<std::string> SplitDescription(std::string&& description);
  void InsertDescLookupElem(std::string&& description, const T& subTag);

  /*!
   * \brief Main storage of the subtag type registry.
   *        key = tag/subtag value, value = tag/subtag object
   */
  std::unordered_map<std::string, T> m_subTags;

  /*!
   * \brief Description to tag/subtag mapping for efficient lookup from description
   *        key = description, value = tag/subtag value
   */
  std::unordered_map<std::string, std::string> m_descToSubTags;
};

template<std::derived_from<BaseSubTag> T>
std::optional<T> CSubTagsCollection<T>::Lookup(const std::string& name) const
{
  auto it = m_subTags.find(name);
  if (it != m_subTags.end())
    return it->second;
  else
    return std::nullopt;
}

template<std::derived_from<BaseSubTag> T>
std::optional<T> CSubTagsCollection<T>::LookupByDescription(const std::string& description) const
{
  auto it = m_descToSubTags.find(description);
  if (it == m_descToSubTags.end())
    return std::nullopt;

  return Lookup(it->second);
}

template<std::derived_from<BaseSubTag> T>
bool CSubTagsCollection<T>::EmplaceRecords(const std::vector<RegistryFileRecord>& records)
{
  bool error{false};

  m_subTags.reserve(m_subTags.size() + records.size());
  //! @todo maybe keep descriptions count in EmplaceRecord to enable mass-build of reverse lookup with reserve of the correct size
  //! may not be worth it
  m_descToSubTags.reserve(m_descToSubTags.size() + records.size());

  for (const RegistryFileRecord& record : records) [[likely]]
    if (!EmplaceRecord(record) && !error) [[unlikely]]
      error = true;

  return !error;
}

template<std::derived_from<BaseSubTag> T>
bool CSubTagsCollection<T>::EmplaceRecord(const RegistryFileRecord& record)
{
  const auto [newElemIt, inserted] = m_subTags.emplace(StringUtils::ToLower(record.m_subTag), T());
  if (!inserted) [[unlikely]]
  {
    CLog::LogF(LOGERROR, "ignoring unexpected redefinition of subtag [{}].", record.m_subTag);
  }
  else if (!newElemIt->second.Load(record)) [[unlikely]]
  {
    CLog::LogF(LOGERROR, "invalid registry file record not imported. Contents:\n{}", record);
    m_subTags.erase(newElemIt);
    return false;
  }

  // Take care of the reverse lookup map
  for (std::string description : record.m_descriptions)
  {
    StringUtils::ToLower(description);
    auto descriptions = SplitDescription(std::move(description));

    for (std::string& desc : descriptions)
      InsertDescLookupElem(std::move(desc), newElemIt->second);
  }

  return true;
}

// noop for the general case
template<std::derived_from<BaseSubTag> T>
std::vector<std::string> CSubTagsCollection<T>::SplitDescription(std::string&& description)
{
  return {std::move(description)};
}

// Specialization for grandfathered tags, some have multiple descriptions separated by commas
// or " or " in a single description field instead multiple description fields
template<>
inline std::vector<std::string> CSubTagsCollection<GrandfatheredTag>::SplitDescription(
    std::string&& description)
{
  // note - could be optimized with string_view versions and looking for separators before
  // splitting the string, but not worth it at this time due to the low count of grandfathered tags
  if (description.find(", ") == std::string::npos && description.find(" or ") == std::string::npos)
    return {std::move(description)};

  std::vector<std::string> splitDesc = StringUtils::Split(description, ", ");
  // A description with less than 2 commas is not considered multi-part.
  if (splitDesc.size() < 3)
  {
    auto indexOr = description.find(" or ");
    if (indexOr != std::string::npos)
      splitDesc = StringUtils::Split(description, " or ");
    else
      splitDesc = {std::move(description)};
  }
  else
  {
    for (std::string& desc : splitDesc)
      if (desc.starts_with("or "))
        desc = desc.substr(3, std::string::npos);
  }
  return splitDesc;
}

template<std::derived_from<BaseSubTag> T>
void CSubTagsCollection<T>::InsertDescLookupElem(std::string&& description, const T& subTag)
{
  auto [it, inserted] = m_descToSubTags.try_emplace(std::move(description), subTag.m_subTag);
  if (!inserted) [[unlikely]]
  {
    // The subtag has multiple identical descriptions that differ only by case
    if (subTag.m_subTag == it->second)
      return;

    // private use ranges are not needed in the registry.
    if (it->first == "private use")
      return;

    const auto orig = Lookup(it->second);
    if (!orig)
    {
      CLog::LogF(LOGERROR,
                 "subtag [{}] that inserted description [{}] for lookup cannot be retrieved to "
                 "resolve a conflict with [{}].",
                 it->second, it->first, subTag.m_subTag);
      return;
    }

    bool overwritten{false};

    // This subtag is valid and the subtag inserted earlier is deprecated > risk-free overwrite
    if (subTag.m_deprecated.empty() && !orig->m_deprecated.empty())
    {
      overwritten = true;
      it->second = subTag.m_subTag;
    }

    CLog::LogF(
        LOGDEBUG,
        "{}subtag [{}] redefines the decription [{}] previously defined by {}subtag [{}] - {}",
        subTag.m_deprecated.empty() ? "" : "deprecated ", subTag.m_subTag, it->first,
        orig->m_deprecated.empty() ? "" : "deprecated ", orig->m_subTag,
        overwritten ? "overwritten" : "ignored");
  }
}

template<std::derived_from<BaseSubTag> T>
void CSubTagsCollection<T>::Reset()
{
  m_subTags.clear();
  m_descToSubTags.clear();
}

} // namespace KODI::UTILS::I18N
