/*
 *  Copyright (C) 2025 Team Kodi
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
enum class Bcp47TagType
{
  WELL_FORMED, ///< The tag conforms to the syntax specified by RFC5646 and is not a special case.
  GRANDFATHERED, ///< The tag is a regular or irregular grandfathered tag.
  PRIVATE_USE, ///< The tag consists solely of private-use subtags.
};

struct ParsedBcp47Tag
{
  Bcp47TagType m_type = Bcp47TagType::WELL_FORMED;
  std::string m_language;
  std::vector<std::string> m_extLangs;
  std::string m_script;
  std::string m_region;
  std::vector<std::string> m_variants;
  std::vector<Bcp47Extension> m_extensions;
  std::vector<std::string> m_privateUse;
  std::string m_grandfathered;

  bool operator==(const ParsedBcp47Tag&) const = default;
};

class CBcp47Parser
{
public:
  /*!
   * \brief Parse a string as a BCP47 language tag
   * \param[in] str String to parse
   * \return parsed tag if it complies with RFC5646, std::nullopt otherwise.
   */
  static std::optional<ParsedBcp47Tag> Parse(std::string str);

private:
  CBcp47Parser() = delete;

  static std::optional<ParsedBcp47Tag> TryParseIso639(const std::string& str);
  static std::optional<ParsedBcp47Tag> TryGenericParse(const std::string& str);
  static std::optional<ParsedBcp47Tag> TryParseRegularGrandfathered(const std::string& str);
  static std::optional<ParsedBcp47Tag> TryParseIrregularGrandfathered(const std::string& str);
};
} // namespace KODI::UTILS::I18N
