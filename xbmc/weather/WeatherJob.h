/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "jobs/Job.h"
#include "weather/WeatherManager.h"

#include <algorithm>
#include <map>
#include <string>
#include <string_view>

class CWeatherJob : public CJob
{
public:
  explicit CWeatherJob(int location);

  bool DoWork() override;

  const WeatherInfo& GetInfo() const;

private:
  void LocalizeOverview(std::string& str);
  void LocalizeOverviewToken(std::string& str);
  void LoadLocalizedToken();

  void SetFromProperties();

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

  WeatherInfo m_info;
  int m_location{-1};
};
