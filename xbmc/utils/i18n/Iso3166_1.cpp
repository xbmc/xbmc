/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Iso3166_1.h"

#include "utils/i18n/Iso3166_1_Table.h"

#include <algorithm>

using namespace KODI::UTILS::I18N;

std::optional<std::string> CIso3166_1::Alpha2ToAlpha3(std::string_view code)
{
  auto it = std::ranges::lower_bound(TableISO3166_1, code, {}, &ISO3166_1::alpha2);
  if (it != TableISO3166_1.end() && it->alpha2 == code)
    return std::string{it->alpha3};

  return std::nullopt;
}

std::optional<std::string> CIso3166_1::Alpha3ToAlpha2(std::string_view code)
{
  auto it = std::ranges::lower_bound(TableISO3166_1ByAlpha3, code, {}, &ISO3166_1::alpha3);
  if (it != TableISO3166_1ByAlpha3.end() && it->alpha3 == code)
    return std::string{it->alpha2};

  return std::nullopt;
}

bool CIso3166_1::ContainsAlpha3(std::string_view code)
{
  return std::ranges::binary_search(TableISO3166_1ByAlpha3, code, {}, &ISO3166_1::alpha3);
}
