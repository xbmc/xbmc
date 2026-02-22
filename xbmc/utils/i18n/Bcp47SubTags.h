/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/i18n/Bcp47Registry/SubTagRegistryTypes.h"

#include <optional>
#include <vector>

namespace KODI::UTILS::I18N
{
// @todo what is the best place for this? inside SubTagRegistryTypes.h instead?
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
