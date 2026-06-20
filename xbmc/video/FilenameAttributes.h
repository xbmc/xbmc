/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/RegExp.h"

#include <optional>
#include <string>
#include <unordered_map>

class TestFilenameAttributes;

namespace KODI::VIDEO
{
/*!
 * \brief Transparent string hasher for std::unordered_map heterogeneous lookup
 */
struct StringHash
{
  using is_transparent = void; // Enables heterogeneous operations.
  std::size_t operator()(std::string_view sv) const { return std::hash<std::string_view>{}(sv); }
};

using AttributeMap = std::unordered_map<std::string, std::string, StringHash, std::equal_to<>>;

class CFilenameAttributes
{
public:
  CFilenameAttributes(const std::string& filename, KODI::REGEXP::RegExpCache* cache);

  /*!
   * \brief Retrieve unique identifier information from the attributes.
   *        in case multiple identifier exist, an undetermined identifier is returned.
   * \param[out] identifierType Type of unique identifier (ex. tmdb)
   * \param[out] identifier Unique identifier
   * \return true if a unique identifier attribute exists, false otherwise
   */
  bool GetIdentifier(std::string& identifierType, std::string& identifier) const;

  /*!
   * \brief Existence of a unique identifier attribute
   * \return true if a unique identifier attribute exists, false otherwise
   */
  bool HasIdentifier() const;

  /*!
   * \brief Retrieve the edition from the attributes.
   * \return Value of the edition attribute when present, empty otherwise.
   */
  std::string GetEdition() const;

  /*!
   * \brief Removes all filename attribute pairs from @p fileName
   * \param[in,out] fileName The filename to strip attributes from.
   * \param[in,out] cache Optional regular expression cache
   */
  static void CleanFilenameAttributePairs(std::string& fileName, KODI::REGEXP::RegExpCache* cache);

private:
  friend class ::TestFilenameAttributes;

  /*!
   * \brief Extracts all key/value attribute pairs encoded in @p fileName (ex. [key=value])
   *        Matches are found using a regular expression from advanced settings, which
   *        must contain named capture groups @c key and @c value. 
   *        The key is trimmed and lowercased before insertion. If the same key appears more than
   *        once, the last value wins.
   * \param fileName[in] The filename to extract attributes from.
   * \param cache[in,out] Optional regular expression cache.
   * \return A map of attribute keys and their values, or an empty map if the
   *         regular expression is invalid or no matches are found.
   */
  static AttributeMap GetFilenameAttributePairs(const std::string& fileName,
                                                KODI::REGEXP::RegExpCache* cache);

  /*!
   * \brief Retrieve and cache unique identifier information from the attributes.
   *        The attributes are compared against a list of known metadata providers and the first
   *        match wins. The list may be modified by users with advanced settings.
   * \param[out] identifierType Type of unique identifier (ex. tmdb)
   * \param[out] identifier Unique identifier
   * \return true if a unique identifier attribute exists, false otherwise
   */
  bool GetIdentifierInternal(std::string& identifierType, std::string& identifier) const;

  AttributeMap m_attributes;
  mutable std::string m_identifierType;
  mutable std::string m_identifier;
  mutable std::optional<bool> m_hasIdentifier;
};
} // namespace KODI::VIDEO
