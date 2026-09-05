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
  const auto settings = CServiceBroker::GetSettingsComponent();
  return settings ? settings->GetSettings() : nullptr;
}

std::shared_ptr<CAdvancedSettings> Advanced()
{
  const auto settings = CServiceBroker::GetSettingsComponent();
  return settings ? settings->GetAdvancedSettings() : nullptr;
}

} // unnamed namespace

bool ContentGeometryEnabledFromSettings()
{
  const auto values = Settings();
  return values && values->GetBool(CSettings::SETTING_VIDEOSCREEN_EXTRACTCONTENTGEOMETRY);
}

bool ContentGeometryNonLiveFromSettings()
{
  const auto values = Settings();
  return ContentGeometryEnabledFromSettings() && values &&
         values->GetBool(CSettings::SETTING_VIDEOSCREEN_CONTENTGEOMETRYONSCAN);
}

VariableGeometryPolicy ContentGeometryPolicyFromSettings()
{
  const auto values = Settings();
  if (!values)
    return VariableGeometryPolicy::Envelope;

  return values->GetInt(CSettings::SETTING_VIDEOSCREEN_VARIABLECONTENTGEOMETRY) == 1
             ? VariableGeometryPolicy::Dominant
             : VariableGeometryPolicy::Envelope;
}

float RasterAspectFromSettings()
{
  const auto values = Settings();
  if (!values)
    return 0.0f;

  return KODI::UTILS::CAspectRatioVocabulary::RatioForKey(
      values->GetInt(CSettings::SETTING_VIDEOSCREEN_RASTERASPECT));
}

float ContentGeometryAtRestFromSettings()
{
  // What Kodi assumed before the viewer could say, and the answer whenever what they said
  // no longer names a ratio the definition in force holds.
  constexpr float DEFAULT_AT_REST_ASPECT = 1.78f;

  const float ratio = RasterAspectFromSettings();

  // Zero is "the same as the display", the default, and answers the 16:9 Kodi has always
  // assumed rather than the ratio of whatever display a room happens to drive.
  return ratio > 0.0f ? ratio : DEFAULT_AT_REST_ASPECT;
}

SamplingParams ContentGeometrySamplingFromSettings(SamplingDepth depth)
{
  SamplingParams sampling;

  if (depth == SamplingDepth::Thorough)
  {
    // 240 is a point every half minute of a two-hour film, which is the density
    // SamplingDepth::Thorough promises.
    sampling.points = 240;
    sampling.escalatedPoints = 240;
    return sampling;
  }

  const auto values = Settings();
  if (!values)
    return sampling;

  sampling.points = static_cast<unsigned int>(
      std::max(1, values->GetInt(CSettings::SETTING_VIDEOSCREEN_CONTENTGEOMETRYSAMPLES)));

  // Escalation stays proportional to what was asked for, deciding that a title varies being
  // sampling-density limited. Three is the factor escalatedPoints was measured at.
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

  const auto advanced = Advanced();
  if (!advanced)
    return combining;

  combining.variesShare = advanced->m_videoContentGeometryVariesShare;
  return combining;
}

LiveGeometrySettings LiveGeometryFromSettings()
{
  LiveGeometrySettings live;

  const auto values = Settings();
  if (!values)
    return live;

  live.enabled = ContentGeometryEnabledFromSettings() &&
                 values->GetBool(CSettings::SETTING_VIDEOSCREEN_LIVECONTENTGEOMETRY);

  live.selector.narrowFrames = static_cast<unsigned int>(
      std::max(1, values->GetInt(CSettings::SETTING_VIDEOSCREEN_LIVEGEOMETRYNARROW)));
  live.leadInSeconds =
      60.0 * std::max(0, values->GetInt(CSettings::SETTING_VIDEOSCREEN_LIVEGEOMETRYLEADIN));
  live.leadOutSeconds =
      60.0 * std::max(0, values->GetInt(CSettings::SETTING_VIDEOSCREEN_LIVEGEOMETRYLEADOUT));

  if (const auto advanced = Advanced())
    live.reductionWidth = static_cast<unsigned int>(advanced->m_videoContentGeometryReductionWidth);

  return live;
}

} // namespace KODI::VIDEO::GEOMETRY
