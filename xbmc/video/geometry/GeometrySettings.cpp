/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GeometrySettings.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/AspectRatioVocabulary.h"

#include <algorithm>

namespace KODI::VIDEO::GEOMETRY
{

namespace
{

std::shared_ptr<CSettings> Settings()
{
  return CServiceBroker::GetSettingsComponent()->GetSettings();
}

std::shared_ptr<CAdvancedSettings> Advanced()
{
  return CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
}

} // unnamed namespace

bool ContentGeometryEnabledFromSettings()
{
  return Settings()->GetBool(CSettings::SETTING_VIDEOSCREEN_EXTRACTCONTENTGEOMETRY);
}

bool ContentGeometryNonLiveFromSettings()
{
  const auto values = Settings();
  return values->GetBool(CSettings::SETTING_VIDEOSCREEN_EXTRACTCONTENTGEOMETRY) &&
         values->GetBool(CSettings::SETTING_VIDEOSCREEN_CONTENTGEOMETRYONSCAN);
}

VariableGeometryPolicy ContentGeometryPolicyFromSettings()
{
  return Settings()->GetInt(CSettings::SETTING_VIDEOSCREEN_VARIABLECONTENTGEOMETRY) == 1
             ? VariableGeometryPolicy::Dominant
             : VariableGeometryPolicy::Envelope;
}

float RasterAspectFromSettings()
{
  return KODI::UTILS::CAspectRatioVocabulary::RatioForKey(
      Settings()->GetInt(CSettings::SETTING_VIDEOSCREEN_RASTERASPECT));
}

float ContentGeometryAtRestFromSettings()
{
  // The answer whenever what the viewer said no longer names a ratio the definition holds.
  constexpr float DEFAULT_AT_REST_ASPECT = 1.78f;

  const float ratio = RasterAspectFromSettings();

  // Zero is "the same as the display", which answers 16:9.
  return ratio > 0.0f ? ratio : DEFAULT_AT_REST_ASPECT;
}

SamplingParams ContentGeometrySamplingFromSettings(SamplingDepth depth)
{
  SamplingParams sampling;

  if (depth == SamplingDepth::Thorough)
  {
    // A point every half minute of a two-hour film, which is what Thorough promises.
    sampling.points = 240;
    sampling.escalatedPoints = 240;
    return sampling;
  }

  const auto values = Settings();
  sampling.points = static_cast<unsigned int>(
      std::max(1, values->GetInt(CSettings::SETTING_VIDEOSCREEN_CONTENTGEOMETRYSAMPLES)));

  // Escalation stays proportional to what was asked for.
  sampling.escalatedPoints = sampling.points * 3;

  sampling.leadInSeconds =
      60.0 * values->GetInt(CSettings::SETTING_VIDEOSCREEN_CONTENTGEOMETRYLEADIN);
  sampling.leadOutSeconds =
      60.0 * values->GetInt(CSettings::SETTING_VIDEOSCREEN_CONTENTGEOMETRYLEADOUT);
  return sampling;
}

CombinerParams ContentGeometryCombiningFromSettings()
{
  CombinerParams combining;
  combining.variesShare = Advanced()->m_videoContentGeometryVariesShare;
  return combining;
}

LiveGeometrySettings LiveGeometryFromSettings()
{
  LiveGeometrySettings live;

  const auto values = Settings();
  live.enabled = values->GetBool(CSettings::SETTING_VIDEOSCREEN_EXTRACTCONTENTGEOMETRY) &&
                 values->GetBool(CSettings::SETTING_VIDEOSCREEN_LIVECONTENTGEOMETRY);

  live.selector.narrowFrames = static_cast<unsigned int>(
      std::max(1, values->GetInt(CSettings::SETTING_VIDEOSCREEN_LIVEGEOMETRYNARROW)));

  return live;
}

} // namespace KODI::VIDEO::GEOMETRY
