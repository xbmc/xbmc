/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WeatherManager.h"

#include <map>
#include <string>
#include "utils/StringUtils.h"
#include "utils/UnicodeUtils.h"

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
    // TODO: Unicode: Consider using UnicodUtils::Collate. It takes locale into account.
    // Could add API to modify behavior.
    // Does this need to support multibyte characters?

    // case-independent (ci) compare_less binary function
    struct nocase_compare
    {
      bool operator() (const unsigned char& c1, const unsigned char& c2) const {
        const char * p1 = (const char*) &c1;
        const char * p2 = (const char*) &c2;
        return UnicodeUtils::CompareNoCase(p1, p2) < 0;
      }
    };
    bool operator()(const std::string & s1, const std::string & s2) const {
      return std::lexicographical_compare
      (s1.begin(), s1.end(),
        s2.begin(), s2.end(),
        nocase_compare());
    }
  };

  std::map<std::string, int, ci_less> m_localizedTokens;
  typedef std::map<std::string, int, ci_less>::const_iterator ilocalizedTokens;

  CWeatherInfo m_info;
  int m_location;

  static bool m_imagesOkay;
};
