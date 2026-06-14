/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "video/FilenameAttributes.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <functional>
#include <utility>

constexpr std::string_view FILENAME_EDITION = "edition";

namespace
{
/*!
 * \brief Returns a compiled regular expression to retrieve filename attributes
 * \param[in] cache Optional regular expression cache
 * \return Valid pointer if the regular expression was compiled successfully, nullptr otherwise.
 */
std::shared_ptr<CRegExp> InitFilenameAttributesRegExp(KODI::REGEXP::RegExpCache* cache)
{
  std::shared_ptr<CRegExp> re;

  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();

  if (advancedSettings == nullptr)
    return re;

  re = KODI::REGEXP::GetRegExp(advancedSettings->m_videoFilenameAttributePairsRegExp, cache, true,
                               CRegExp::autoUtf8);
  if (re == nullptr)
  {
    CLog::LogF(LOGERROR, "Invalid filename attribute pairs RegExp:'{}'",
               advancedSettings->m_videoFilenameAttributePairsRegExp);
  }
  return re;
}

/*!
 * \brief Iterates over all filename attributes key=value pairs in @p fileName, invoking @p callback
 *        for each one.
 * @p fileName must not be modified by @p callback.
 * \param fileName [in] The filename to search for attribute pairs.
 * \param cache[in] Optional regular expression cache
 * \param callback[in] Invoked for each match with the absolute position, the match length, the value
 *                     of the key and value of the match.
 */
void ForEachFilenameAttribute(
    const std::string& fileName,
    KODI::REGEXP::RegExpCache* cache,
    std::function<void(int pos, int len, const std::string& key, const std::string& value)>
        callback)
{
  if (std::shared_ptr<CRegExp> re = InitFilenameAttributesRegExp(cache); re != nullptr)
  {
    unsigned int offset = 0;
    while (true)
    {
      int pos = re->RegFind(fileName, offset);
      if (pos < 0)
        break;

      const int matchLength = re->GetFindLen();
      callback(pos, matchLength, re->GetMatch("key"), re->GetMatch("value"));
      offset = pos + matchLength;
    }
  }
}
} // namespace

namespace KODI::VIDEO
{
CFilenameAttributes::CFilenameAttributes(const std::string& filename,
                                         KODI::REGEXP::RegExpCache* cache)
{
  m_attributes = GetFilenameAttributePairs(filename, cache);
}

AttributeMap CFilenameAttributes::GetFilenameAttributePairs(const std::string& fileName,
                                                            KODI::REGEXP::RegExpCache* cache)
{
  AttributeMap result;

  ForEachFilenameAttribute(fileName, cache,
                           [&result](int, int, std::string key, std::string value)
                           {
                             StringUtils::Trim(key);
                             StringUtils::ToLower(key);
                             StringUtils::Trim(value);
                             if (!key.empty() && !value.empty())
                               result.insert_or_assign(key, value);
                           });
  return result;
}

void CFilenameAttributes::CleanFilenameAttributePairs(std::string& fileName,
                                                      KODI::REGEXP::RegExpCache* cache)
{
  std::string result;
  result.reserve(fileName.size());
  int last = 0;

  // Collect the gaps between attribute tokens into the new result string. Reduces allocations
  // and in place erasure without modification of the string being iterated.
  ForEachFilenameAttribute(
      fileName, cache,
      [&result, &fileName, &last](int pos, int len, const std::string&, const std::string&)
      {
        result.append(fileName, last, pos - last);
        last = pos + len;
      });
  result.append(fileName, last, std::string::npos);

  fileName = std::move(result);
}

bool CFilenameAttributes::GetIdentifier(std::string& identifierType, std::string& identifier) const
{
  if (!m_hasIdentifier.has_value())
    m_hasIdentifier = GetIdentifierInternal(m_identifierType, m_identifier);

  if (m_hasIdentifier.value())
  {
    identifierType = m_identifierType;
    identifier = m_identifier;
  }
  return m_hasIdentifier.value();
}

bool CFilenameAttributes::GetIdentifierInternal(std::string& identifierType,
                                                std::string& identifier) const
{
  if (const std::shared_ptr<CAdvancedSettings> advancedSettings =
          CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
      advancedSettings != nullptr)
  {
    const std::unordered_set<std::string>& identifiers =
        advancedSettings->m_videoScannerMetadataSources;

    if (identifiers.empty())
      return false;

    // The attribute key must be a known identifier with an optional id suffix and the value
    // must be purely alphanumeric.

    // Projection to strip the "id" suffix
    auto proj = [](const std::pair<std::string, std::string>& attr)
    {
      constexpr std::string_view suffix = "id";
      if (!StringUtils::EndsWithNoCase(attr.first, suffix))
        return attr;
      return std::pair{attr.first.substr(0, attr.first.size() - suffix.size()), attr.second};
    };

    auto it = std::ranges::find_if(
        m_attributes,
        [&identifiers](const auto& attr)
        {
          if (identifiers.contains(attr.first) && !attr.second.empty() &&
              std::ranges::all_of(attr.second,
                                  [](char c) { return StringUtils::isasciialphanum(c); }))
            return true;

          return false;
        },
        proj);

    if (it != m_attributes.end())
    {
      auto stripped = proj(*it);
      identifierType = stripped.first;
      identifier = stripped.second;
      return true;
    }
  }
  return false;
}

bool CFilenameAttributes::HasIdentifier() const
{
  if (!m_hasIdentifier.has_value())
    m_hasIdentifier = GetIdentifierInternal(m_identifierType, m_identifier);

  return m_hasIdentifier.value();
}

std::string CFilenameAttributes::GetEdition() const
{
  auto it = m_attributes.find(FILENAME_EDITION);
  if (it != m_attributes.end())
    return it->second;

  return "";
}
} // namespace KODI::VIDEO
