/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/Territory.h"

#include "language/i18n/Bcp47.h"
#include "language/i18n/Iso3166_1.h"
#include "utils/StringUtils.h"

using namespace KODI::LANGUAGE;
using namespace KODI::LANGUAGE::I18N;

CTerritory CTerritory::FromCode(std::string_view code)
{
  std::string subtag{StringUtils::ToLower(code)};
  StringUtils::Trim(subtag);

  // What counts as a region is CBcp47's answer, not a second one
  if (!CBcp47::IsRegionSubtag(subtag))
    return {};

  StringUtils::ToUpper(subtag);

  return CTerritory(std::move(subtag));
}

bool CTerritory::IsCountry() const
{
  // Not the same question as being a region. BCP 47 accepts areas ISO 3166-1 assigns no current
  // code to - the UN M.49 groupings, the codes it reserves such as EU and UN, and the ones it
  // has withdrawn - and a caller contracting to supply a country can use none of them.
  return CIso3166_1::ContainsAlpha2(StringUtils::ToLower(m_code));
}

std::string CTerritory::AsIso3166_1Alpha2() const
{
  return IsCountry() ? m_code : std::string{};
}

std::string CTerritory::AsIso3166_1Alpha3() const
{
  if (!IsCountry())
    return {};

  const std::string alpha3{
      CIso3166_1::Alpha2ToAlpha3(StringUtils::ToLower(m_code)).value_or(std::string{})};
  return StringUtils::ToUpper(alpha3);
}
