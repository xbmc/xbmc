/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/i18n/Iso3166_1.h"

#include "language/i18n/Iso3166_1_Table.h"

#include <algorithm>

using namespace KODI::LANGUAGE::I18N;

std::optional<std::string> CIso3166_1::Alpha2ToAlpha3(std::string_view code)
{
  const auto it = std::ranges::lower_bound(TableISO3166_1, code, {}, &ISO3166_1::alpha2);
  if (it != TableISO3166_1.end() && it->alpha2 == code)
    return std::string{it->alpha3};

  return std::nullopt;
}

bool CIso3166_1::ContainsAlpha2(std::string_view code)
{
  return std::ranges::binary_search(TableISO3166_1, code, {}, &ISO3166_1::alpha2);
}
