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
#include "weather/WeatherTokenLocalizer.h"

class CWeatherJob : public CJob
{
public:
  static constexpr int MAX_HOURS_TO_FETCH = 72; // 3 days
  static constexpr int MAX_DAYS_TO_FETCH = 31; // ~1 month

  explicit CWeatherJob(int location);

  bool DoWork() override;

  int GetLocation() const;
  const WeatherInfo& GetInfo() const;
  const CWeatherManager::WeatherInfoV2& GetInfoV2() const;

private:
  void SetFromProperties();
  void SetFromPropertiesV2();

  WeatherInfo m_info{};
  CWeatherManager::WeatherInfoV2 m_infoV2{};
  CWeatherTokenLocalizer m_localizer;
  int m_location{-1};
};
