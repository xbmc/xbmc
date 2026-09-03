/*
 *  Copyright (C) 2025-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "language/i18n/Bcp47Common.h"
#include "language/i18n/Bcp47SubTags.h"

#include <optional>
#include <string>
#include <vector>

namespace KODI::LANGUAGE::I18N
{
class CBcp47Formatter;
struct ParsedBcp47Tag;
class CSubTagRegistryManager;

class CBcp47
{
public:
  bool operator==(const ParsedBcp47Tag& other) const;

  /*!
   * \brief Parse a language tag into its subtags. The subtags are not altered or validated.
   * \param[in] str Text to parse 
   * \param[in] registry Subtag registry used. If not provided, the global registry will be used.
   * \return Object initialized with the subtags of a well-formed tag.
   *         std::nullopt is returned when the text parameter is not a well-formed language tag.
   */
  static std::optional<CBcp47> ParseTag(std::string str,
                                        const CSubTagRegistryManager* registry = nullptr);

  /*!
   * \brief Return the validity of the tag per RFC5646 validity rules
   * \return true for a valid tag, otherwise false.
   */
  bool IsValid() const { return m_isValid; }

  /*!
   * \brief Whether text is a region subtag RFC 5646 accepts.
   * \note A region means the same thing inside a language tag as it does standing on its own, so
   *       this answers for both.
   * \param[in] region Lower case candidate region subtag.
   * \param[in] registry Subtag registry used. If not provided, the global registry will be used.
   * \return true for an accepted region subtag.
   */
  static bool IsRegionSubtag(std::string_view region,
                             const CSubTagRegistryManager* registry = nullptr);

  /*!
   * \brief Return the primary language subtag of the tag.
   * \return The primary language subtag. Empty for grandfathered and private-use tags, which
   *         carry no primary language subtag.
   */
  const std::string& GetLanguage() const { return m_language; }

  /*!
   * \brief Return the region subtag of the tag.
   * \return The region subtag, lower case as the tag holds its subtags, where Format renders it
   *         the upper case BCP 47 recommends. Empty for a tag naming no region.
   */
  const std::string& GetRegion() const { return m_region; }

  /*!
   * \brief Transform the tag into its canonical from per RFC 5646 rules.
   */
  void Canonicalize();

  /*!
   * \brief Format the tag to text according to the provided format
   * \param[in] style Format of the output
   * \return The formatted tag
   */
  std::string Format(Bcp47FormattingStyle style = Bcp47FormattingStyle::FORMAT_BCP47) const;

  friend class CBcp47Formatter;

private:
  CBcp47() = default;

  void LoadRegistrySubTags(const CSubTagRegistryManager* registry);
  bool Validate(const CSubTagRegistryManager* registry);
  bool IsValidLanguage() const;
  bool IsValidExtLang() const;
  bool IsValidScript() const;
  bool IsValidRegion() const;
  bool HasDuplicateVariants() const;
  bool IsValidVariants() const;
  bool HasDuplicateExtensions() const;

  bool m_isValid{false};

  Bcp47TagType m_type = Bcp47TagType::WELL_FORMED;
  std::string m_language;
  std::vector<std::string> m_extLangs;
  std::string m_script;
  std::string m_region;
  std::vector<std::string> m_variants;
  std::vector<Bcp47Extension> m_extensions;
  std::vector<std::string> m_privateUse;
  std::string m_grandfathered;

  //! @todo when the implementation is mature, revisit whether to keep all fields of the subtags
  //! or just the descriptions, and whether to use std::optional or a smart pointer
  std::optional<TagSubTags> m_registrySubTags;

  //! The registry the tag was parsed against, so canonicalization resolves a replacement subtag
  //! against the same one. Not owned - a registry outlives the tags parsed from it.
  const CSubTagRegistryManager* m_registry{nullptr};
};
} // namespace KODI::LANGUAGE::I18N
