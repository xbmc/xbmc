/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/test/TestI18nUtils.h"

#include "utils/StringUtils.h"
#include "utils/i18n/Bcp47Common.h"
#include "utils/i18n/Bcp47Parser.h"

namespace KODI::UTILS::I18N
{
std::ostream& operator<<(std::ostream& os, const Bcp47Extension& obj)
{
  os << obj.name << ": {" << StringUtils::Join(obj.segments, ",") << "}";
  return os;
}

std::ostream& operator<<(std::ostream& os, const ParsedBcp47Tag& obj)
{
  os << "BCP47 (language: " << obj.m_language << ", extended languages: {"
     << StringUtils::Join(obj.m_extLangs, ",") << "}, script: " << obj.m_script
     << ", region: " << obj.m_region << ", variants: {" << StringUtils::Join(obj.m_variants, ",")
     << "}, extensions: {";

  for (const auto& ext : obj.m_extensions)
    os << ext << " ";

  os << "}, private use: {" << StringUtils::Join(obj.m_privateUse, ", ") << "}, ";
  os << "grandfathered: " << obj.m_grandfathered << ")";

  return os;
}
} // namespace KODI::UTILS::I18N
