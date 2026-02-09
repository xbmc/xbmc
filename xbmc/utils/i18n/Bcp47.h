/*
 *  Copyright (C) 2025-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/i18n/Bcp47Common.h"

#include <optional>
#include <string>
#include <vector>

namespace KODI::UTILS::I18N
{
class CBcp47Formatter;
struct ParsedBcp47Tag;

class CBcp47
{
public:
  bool operator==(const ParsedBcp47Tag& other) const;

  /*!
   * \brief Parse a language tag into its subtags. The subtags are not altered or validated.
   * \param[in] str Text to parse 
   * \return Object initialized with the subtags of a well-formed tag.
   *         std::nullopt is returned when the text parameter is not a well-formed language tag.
   */
  static std::optional<CBcp47> ParseTag(std::string str);

  /*!
   * \brief Return the validity of the tag per RFC5646 validity rules
   * \return true for a valid tag, otherwise false.
   */
  bool IsValid() const { return m_isValid; }

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

  // Accessors
  //! @todo consider returning const references
  Bcp47TagType GetType() const { return m_type; }
  const std::string& GetLanguage() const { return m_language; }
  const std::vector<std::string>& GetExtLangs() const { return m_extLangs; }
  const std::string& GetScript() const { return m_script; }
  const std::string& GetRegion() const { return m_region; }
  const std::vector<std::string>& GetVariants() const { return m_variants; }
  const std::vector<Bcp47Extension>& GetExtensions() const { return m_extensions; }
  const std::vector<std::string>& GetPrivateUse() const { return m_privateUse; }
  const std::string& GetGrandfathered() const { return m_grandfathered; }

private:
  CBcp47() = default;

  bool Validate();
  bool IsValidLanguage() const;
  bool HasMultipleExtLang() const;
  bool IsValidRegion() const;
  bool HasDuplicateVariants() const;
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
};
} // namespace KODI::UTILS::I18N
