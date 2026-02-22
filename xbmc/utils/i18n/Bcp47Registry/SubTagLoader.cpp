/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Bcp47Registry/SubTagLoader.h"

#include "utils/i18n/Bcp47Registry/SubTagRegistryFile.h"
#include "utils/i18n/Bcp47Registry/SubTagRegistryTypes.h"

namespace KODI::UTILS::I18N
{
bool LoadBaseSubTag(BaseSubTag& subTag, const RegistryFileRecord& record)
{
  if (record.m_subTag.empty())
    return false;

  subTag.m_subTag = record.m_subTag;
  subTag.m_descriptions = record.m_descriptions;
  subTag.m_added = record.m_added;
  subTag.m_deprecated = record.m_deprecated;
  subTag.m_preferredValue = record.m_preferredValue;

  return true;
}

bool LoadLanguageSubTag(LanguageSubTag& subTag, const RegistryFileRecord& record)
{
  if (!LoadBaseSubTag(subTag, record))
    return false;

  subTag.m_suppressScript = record.m_suppressScript;
  subTag.m_macroLanguage = record.m_macroLanguage;
  subTag.m_scope = record.m_scope;

  return true;
}

bool LoadExtLangSubTag(ExtLangSubTag& subTag, const RegistryFileRecord& record)
{
  if (record.m_prefixes.size() != 1)
    return false;

  if (!LoadBaseSubTag(subTag, record))
    return false;

  subTag.m_prefix = record.m_prefixes.front();
  subTag.m_suppressScript = record.m_suppressScript;
  subTag.m_macroLanguage = record.m_macroLanguage;
  subTag.m_scope = record.m_scope;

  return true;
}

bool LoadVariantSubTag(VariantSubTag& subTag, const RegistryFileRecord& record)
{
  if (!LoadBaseSubTag(subTag, record))
    return false;

  subTag.m_prefixes = record.m_prefixes;

  return true;
}
} // namespace KODI::UTILS::I18N
