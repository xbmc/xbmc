/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Bcp47Registry/SubTagRegistryManager.h"

#include "filesystem/SpecialProtocol.h"
#include "utils/i18n/Bcp47Registry/SubTagLoader.h"
#include "utils/i18n/Bcp47Registry/SubTagRegistryFile.h"
#include "utils/log.h"

#include <concepts>
#include <functional>

// The default registry file is stored as [your code root dir]/system/language-subtag-registry.txt
//
// Upgrade instructions:
// Download the current registry file from
// https://www.iana.org/assignments/language-subtag-registry/language-subtag-registry
//
// Replace the in-tree copy if the file date (first line of the registry files) of the downloaded
// file is more recent.
// It will automatically be copied as needed for build and packaging.
//
// Note: The file is encoded in utf-8. The code can handle unix and windows line terminators.

using namespace KODI::UTILS::I18N;

static const std::string DEFAULTREGISTRYFILEPATH(
    "special://xbmc/system/language-subtag-registry.txt");

CSubTagRegistryManager::~CSubTagRegistryManager()
{
  Deinitialize();
}

bool CSubTagRegistryManager::Initialize(std::unique_ptr<IRegistryRecordProvider> provider)
{
  if (provider == nullptr)
  {
    const std::string filePath = CSpecialProtocol::TranslatePath(DEFAULTREGISTRYFILEPATH);
    provider = std::make_unique<CRegistryFile>(filePath);
  }

  return Load(provider.get());
}

bool CSubTagRegistryManager::Load(IRegistryRecordProvider* provider)
{
  if (provider == nullptr || !provider->Load())
    return false;

  bool error{false};

  for (const RegistryFileRecord& record : provider->GetRecords()) [[likely]]
    if (!ImportRecord(record) && !error) [[unlikely]]
      error = true;

  return !error;
}

bool CSubTagRegistryManager::ImportRecord(const RegistryFileRecord& record)
{
  switch (record.m_type)
  {
    case SubTagType::Language:
      return m_languageSubTags.EmplaceRecord(record);
    case SubTagType::ExtLang:
      return m_extLangSubTags.EmplaceRecord(record);
    case SubTagType::Script:
      return m_scriptSubTags.EmplaceRecord(record);
    case SubTagType::Region:
      return m_regionSubTags.EmplaceRecord(record);
    case SubTagType::Variant:
      return m_variantSubTags.EmplaceRecord(record);
    case SubTagType::Grandfathered:
      return m_grandfatheredTags.EmplaceRecord(record);
    case SubTagType::Redundant:
      return m_redundantTags.EmplaceRecord(record);
    default:
      CLog::LogF(LOGERROR,
                 "unknown subtag/tag type [{}], registry file record subtag [{}] ignored.",
                 record.m_type, record.m_subTag);
      return false;
  }
}

void CSubTagRegistryManager::Deinitialize()
{
  m_languageSubTags.Reset();
  m_extLangSubTags.Reset();
  m_scriptSubTags.Reset();
  m_regionSubTags.Reset();
  m_variantSubTags.Reset();
  m_grandfatheredTags.Reset();
  m_redundantTags.Reset();
}
