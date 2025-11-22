/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/i18n/Bcp47.h"

#include <optional>
#include <string>
#include <vector>

namespace KODI::UTILS::I18N
{
enum class Bcp47TagType
{
  MALFORMED,
  REGULAR,
  GRANDFATHERED,
  PRIVATE_USE,
};

struct ParsedBcp47Tag
{
  Bcp47TagType type = Bcp47TagType::MALFORMED;
  std::string m_language;
  std::vector<std::string> m_extLangs;
  std::string m_script;
  std::string m_region;
  std::vector<std::string> m_variants;
  std::vector<Bcp47Extension> m_extensions;
  std::vector<std::string> m_privateUse;
  std::string m_grandfathered;
};

class CBcp47Parser
{
public:
  static ParsedBcp47Tag Parse(std::string str);

private:
  CBcp47Parser() = delete;

  static ParsedBcp47Tag TryParseIso639(const std::string& str);
  static ParsedBcp47Tag TryGenericParse(const std::string& str);
  static ParsedBcp47Tag TryParseRegularGrandfathered(const std::string& str);
  static ParsedBcp47Tag TryParseIrregularGrandfathered(const std::string& str);
};
} // namespace KODI::UTILS::I18N
