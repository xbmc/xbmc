/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/i18n/Iso639_1.h"

#include "language/i18n/Iso639.h"
#include "language/i18n/Iso639_1_Table.h"
#include "utils/StringUtils.h"

#include <algorithm>

using namespace KODI::LANGUAGE::I18N;

bool CIso639_1::ListLanguages(std::map<std::string, std::string>& langMap)
{
  std::ranges::transform(TableISO639_1ByName, std::inserter(langMap, langMap.end()),
                         [](const LCENTRY& e)
                         { return std::make_pair(LongCodeToString(e.code), std::string{e.name}); });

  std::ranges::transform(TableISO639_1_DeprByName, std::inserter(langMap, langMap.end()),
                         [](const LCENTRY& e)
                         { return std::make_pair(LongCodeToString(e.code), std::string{e.name}); });

  return true;
}

bool CIso639_1::ListLanguageNames(std::map<std::string, std::string>& nameMap)
{
  std::ranges::transform(TableISO639_1ByName, std::inserter(nameMap, nameMap.end()),
                         [](const LCENTRY& e)
                         { return std::make_pair(std::string{e.name}, LongCodeToString(e.code)); });

  std::ranges::transform(TableISO639_1_DeprByName, std::inserter(nameMap, nameMap.end()),
                         [](const LCENTRY& e)
                         { return std::make_pair(std::string{e.name}, LongCodeToString(e.code)); });

  return true;
}
