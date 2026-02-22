/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/i18n/Bcp47Registry/RegistryRecordProvider.h"
#include "utils/i18n/Bcp47Registry/SubTagRegistryTypes.h"
#include "utils/i18n/Bcp47Registry/SubTagsCollection.h"

#include <memory>

namespace KODI::UTILS::I18N
{
class CSubTagRegistryManager
{
public:
  ~CSubTagRegistryManager();

  bool Initialize(std::unique_ptr<IRegistryRecordProvider> provider = nullptr);
  void Deinitialize();

  const CSubTagsCollection<LanguageSubTag>& GetLanguageSubTags() const { return m_languageSubTags; }
  const CSubTagsCollection<ExtLangSubTag>& GetExtLangSubTags() const { return m_extLangSubTags; }
  const CSubTagsCollection<ScriptSubTag>& GetScriptSubTags() const { return m_scriptSubTags; }
  const CSubTagsCollection<RegionSubTag>& GetRegionSubTags() const { return m_regionSubTags; }
  const CSubTagsCollection<VariantSubTag>& GetVariantSubTags() const { return m_variantSubTags; }
  const CSubTagsCollection<GrandfatheredTag>& GetGrandfatheredTags() const
  {
    return m_grandfatheredTags;
  }
  const CSubTagsCollection<RedundantTag>& GetRedundantTags() const { return m_redundantTags; }

private:
  bool Load(IRegistryRecordProvider* provider);

  /*!
   * \brief Load the contents of \p record as a new subtag in the registry of the appropriate type.
   * \param[in] record The record information
   * \return true for success, false when \p record cannot be converted to a valid subtag.
   */
  bool ImportRecord(const RegistryFileRecord& record);

  CSubTagsCollection<LanguageSubTag> m_languageSubTags;
  CSubTagsCollection<ExtLangSubTag> m_extLangSubTags;
  CSubTagsCollection<ScriptSubTag> m_scriptSubTags;
  CSubTagsCollection<RegionSubTag> m_regionSubTags;
  CSubTagsCollection<VariantSubTag> m_variantSubTags;
  CSubTagsCollection<GrandfatheredTag> m_grandfatheredTags;
  CSubTagsCollection<RedundantTag> m_redundantTags;

  bool m_initialized{false};
};
} // namespace KODI::UTILS::I18N
