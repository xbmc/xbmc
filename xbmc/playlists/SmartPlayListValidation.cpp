/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartPlayList.h"
#include "XBDateTime.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/StringValidation.h"

using enum CDatabaseQueryRule::SearchOperator;

namespace KODI::PLAYLIST
{
bool CSmartPlaylistRule::ValidateRating(const std::string& input, void* data)
{
  char* end = NULL;
  std::string strRating = input;
  StringUtils::Trim(strRating);

  double rating = std::strtod(strRating.c_str(), &end);
  return (end == NULL || *end == '\0') && rating >= 0.0 && rating <= 10.0;
}

bool CSmartPlaylistRule::ValidateMyRating(const std::string& input, void* data)
{
  std::string strRating = input;
  StringUtils::Trim(strRating);

  int rating = atoi(strRating.c_str());
  return StringValidation::IsPositiveInteger(input, data) && rating <= 10;
}

bool CSmartPlaylistRule::ValidateDate(const std::string& input, void* data)
{
  if (!data)
    return false;

  const auto* rule = static_cast<CSmartPlaylistRule*>(data);

  //! @todo implement a validation for relative dates
  if (rule->m_operator == OPERATOR_IN_THE_LAST || rule->m_operator == OPERATOR_NOT_IN_THE_LAST)
    return true;

  // The date format must be YYYY-MM-DD
  CDateTime dt;
  return dt.SetFromRFC3339FullDate(input);
}

bool CSmartPlaylistRule::ValidateLanguage(const std::string& input, void* data)
{
  std::string dummy;
  return g_LangCodeExpander.ConvertToBcp47(input, dummy);
}
} // namespace KODI::PLAYLIST
