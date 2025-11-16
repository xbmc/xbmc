/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/i18n/RegistryRecordProvider.h"
#include "utils/i18n/SubTagRegistry.h"
#include "utils/i18n/SubTagRegistryTypes.h"

#include <memory>

namespace KODI::UTILS::I18N
{
class CSubTagRegistryManager
{
public:
  ~CSubTagRegistryManager();

  bool Initialize(std::unique_ptr<IRegistryRecordProvider> provider = nullptr);
  void Deinitialize();

  const CSubTagRegistry<LanguageSubTag>& GetLanguageSubTags() const { return m_languageSubTags; }
  const CSubTagRegistry<ExtLangSubTag>& GetExtLangSubTags() const { return m_extLangSubTags; }
  const CSubTagRegistry<ScriptSubTag>& GetScriptSubTags() const { return m_scriptSubTags; }
  const CSubTagRegistry<RegionSubTag>& GetRegionSubTags() const { return m_regionSubTags; }
  const CSubTagRegistry<VariantSubTag>& GetVariantSubTags() const { return m_variantSubTags; }
  const CSubTagRegistry<GrandfatheredTag>& GetGrandfatheredTags() const
  {
    return m_grandfatheredTags;
  }
  const CSubTagRegistry<RedundantTag>& GetRedundantTags() const { return m_redundantTags; }

private:
  bool Load(IRegistryRecordProvider* provider);

  /*!
   * \brief Load the contents of \p record as a new subtag in the registry of the appropriate type.
   * \param[in] record The record information
   * \return true for success, false when \p record cannot be converted to a valid subtag.
   */
  bool ImportRecord(const RegistryFileRecord& record);

  CSubTagRegistry<LanguageSubTag> m_languageSubTags;
  CSubTagRegistry<ExtLangSubTag> m_extLangSubTags;
  CSubTagRegistry<ScriptSubTag> m_scriptSubTags;
  CSubTagRegistry<RegionSubTag> m_regionSubTags;
  CSubTagRegistry<VariantSubTag> m_variantSubTags;
  CSubTagRegistry<GrandfatheredTag> m_grandfatheredTags;
  CSubTagRegistry<RedundantTag> m_redundantTags;
};
} // namespace KODI::UTILS::I18N