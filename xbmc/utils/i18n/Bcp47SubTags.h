/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// @todo include in SubTagRegistryTypes.h instead? where to locate this?
#include "utils/i18n/Bcp47Registry/SubTagRegistryTypes.h"

#include <optional>
#include <vector>

namespace KODI::UTILS::I18N
{
struct TagSubTags
{
  std::optional<LanguageSubTag> m_language;
  std::vector<ExtLangSubTag> m_extLangs;
  std::optional<ScriptSubTag> m_script;
  std::optional<RegionSubTag> m_region;
  std::vector<VariantSubTag> m_variants;
  std::optional<GrandfatheredTag> m_grandfathered;
};
} // namespace KODI::UTILS::I18N
