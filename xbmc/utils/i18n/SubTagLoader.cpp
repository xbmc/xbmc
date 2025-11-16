/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/SubTagLoader.h"

#include "utils/i18n/SubTagRegistryFile.h"
#include "utils/i18n/SubTagRegistryTypes.h"

namespace KODI::UTILS::I18N
{
bool LoadBaseSubTag(BaseSubTag& subTag, const RegistryFileRecord& record)
{
  if (record.m_subTag.empty())
    return false;

  subTag.m_subTag = record.m_subTag;
  subTag.m_description = record.m_description;
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
  if (record.m_prefix.size() != 1)
    return false;

  if (!LoadBaseSubTag(subTag, record))
    return false;

  subTag.m_prefix = record.m_prefix.front();
  subTag.m_suppressScript = record.m_suppressScript;
  subTag.m_macroLanguage = record.m_macroLanguage;
  subTag.m_scope = record.m_scope;

  return true;
}

bool LoadVariantSubTag(VariantSubTag& subTag, const RegistryFileRecord& record)
{
  if (!LoadBaseSubTag(subTag, record))
    return false;

  subTag.m_prefix = record.m_prefix;

  return true;
}
} // namespace KODI::UTILS::I18N