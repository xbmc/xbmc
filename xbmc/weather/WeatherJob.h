/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WeatherManager.h"
#include "utils/StringUtils.h"

#include <map>
#include <string>

class CWeatherJob : public CJob
{
public:
  explicit CWeatherJob(int location);

  bool DoWork() override;

  const CWeatherInfo &GetInfo() const;
private:
  static std::string ConstructPath(std::string in);
  void LocalizeOverview(std::string &str);
  void LocalizeOverviewToken(std::string &str);
  void LoadLocalizedToken();
  static int ConvertSpeed(int speed);

  void SetFromProperties();

  /*! \brief Formats a celsius temperature into a string based on the users locale
   \param text the string to format
   \param temp the temperature (in degrees celsius).
   */
  static void FormatTemperature(std::string &text, double temp);

  struct ci_less
  {
    // May be able to use existing code IFF values are all ASCII and C Locale
    // is used for tolower. But for simplicity, consistency and more bullet-proof
    // solution it is recoded here to use FoldCase via CompareNoCase.

    bool operator()(const std::string_view s1, const std::string_view s2) const
    {
      return StringUtils::CompareNoCase(s1, s2) < 0;
    }
  };

  std::map<std::string, int, ci_less> m_localizedTokens;
  typedef std::map<std::string, int, ci_less>::const_iterator ilocalizedTokens;

  CWeatherInfo m_info;
  int m_location;

  static bool m_imagesOkay;
};
