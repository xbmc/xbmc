/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <algorithm>
#include <map>
#include <string>

class CWeatherTokenLocalizer
{
public:
  CWeatherTokenLocalizer() = default;
  virtual ~CWeatherTokenLocalizer() = default;

  std::string LocalizeOverview(const std::string& str);
  std::string LocalizeOverviewToken(const std::string& str);

private:
  void LoadLocalizedTokens();

  struct CaseInsensitiveCompare
  {
    using is_transparent = void; // Enables heterogeneous operations.

    bool operator()(const std::string_view& lhs, const std::string_view& rhs) const
    {
      return std::ranges::lexicographical_compare(lhs, rhs, [](char l, char r)
                                                  { return std::tolower(l) < std::tolower(r); });
    }
  };
  std::map<std::string, int, CaseInsensitiveCompare> m_localizedTokens;
};
