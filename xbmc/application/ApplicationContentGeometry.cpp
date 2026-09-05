/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationContentGeometry.h"

#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationSettingsHandling.h"
#include "cores/VideoPlayer/Interface/StreamInfo.h"
#include "cores/VideoSettings.h"
#include "interfaces/AnnouncementManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/AspectRatioVocabulary.h"
#include "utils/MathUtils.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "video/geometry/GeometryPublication.h"
#include "video/geometry/GeometrySettings.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <chrono>

using namespace KODI::UTILS;
using namespace KODI::VIDEO::GEOMETRY;

namespace
{

//! \brief The shape the interface is drawn at, zero unless the hold is on and the surround is
//! set to close to it.
float GuiShapeAspect()
{
  const auto settings = CServiceBroker::GetSettingsComponent();
  const auto values = settings ? settings->GetSettings() : nullptr;
  if (!values || !values->GetBool(CSettings::SETTING_VIDEOSCREEN_GUIKEEPSHAPE) ||
      values->GetInt(CSettings::SETTING_VIDEOSCREEN_GUISURROUND) != 3)
    return 0.0f;

  const auto winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
    return 0.0f;

  const CRect gui{winSystem->GetGfxContext().GetGuiKeepShapeRect()};
  return gui.Height() > 0.0f ? gui.Width() / gui.Height() : 0.0f;
}

//! \brief What is on the screen when nothing is playing: a ratio, no rectangle.
EffectiveGeometry AtRestGeometry()
{
  EffectiveGeometry gui;
  const float guiShape{GuiShapeAspect()};
  gui.aspect = guiShape > 0.0f ? guiShape : ContentGeometryAtRestFromSettings();
  gui.label = CAspectRatioVocabulary::Label(gui.aspect);
  gui.name = CAspectRatioVocabulary::Name(gui.aspect);
  gui.source = GeometrySource::Container;
  return gui;
}

CRect WholePixels(const CRect& rect)
{
  return {static_cast<float>(MathUtils::round_int(static_cast<double>(rect.x1))),
          static_cast<float>(MathUtils::round_int(static_cast<double>(rect.y1))),
          static_cast<float>(MathUtils::round_int(static_cast<double>(rect.x2))),
          static_cast<float>(MathUtils::round_int(static_cast<double>(rect.y2)))};
}

} // unnamed namespace

CApplicationContentGeometry::CApplicationContentGeometry() : m_current(AtRestGeometry())
{
}

void CApplicationContentGeometry::Announce(const EffectiveGeometry& geometry,
                                           const DrawnGeometry& drawn)
{
  const auto announcer = CServiceBroker::GetAnnouncementManager();
  if (!announcer)
    return;

  CVariant data{CVariant::VariantTypeObject};
  SerializeEffectiveGeometry(geometry, data["contentrect"]);

  if (drawn.Drawn())
    SerializeDrawnGeometry(drawn, data["screen"]);

  const int playlist = static_cast<int>(CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist());
  if (playlist >= 0)
    data["player"]["playerid"] = playlist;

  announcer->Announce(ANNOUNCEMENT::Player, "OnContentGeometryChange", data);
}

void CApplicationContentGeometry::Set(const EffectiveGeometry& geometry)
{
  bool differs = false;
  {
    std::unique_lock lock(m_section);
    differs = PublishedGeometryDiffers(m_current, geometry);
    m_current = geometry;
  }

  if (differs)
    AnnounceThrottled();
}

void CApplicationContentGeometry::AnnounceThrottled()
{
  bool now = false;
  {
    std::unique_lock lock(m_announceSection);

    const int64_t elapsed = CTimeUtils::MonotonicMs() - m_lastAnnounceMs;
    if (elapsed >= ANNOUNCE_INTERVAL_MS)
    {
      m_lastAnnounceMs = CTimeUtils::MonotonicMs();
      now = true;
    }
    else if (!m_announcePending)
    {
      m_announcePending = true;

      if (!m_announceTimer.Start(std::chrono::milliseconds(ANNOUNCE_INTERVAL_MS - elapsed)))
      {
        m_announcePending = false;
        m_lastAnnounceMs = CTimeUtils::MonotonicMs();
        now = true;
      }
    }
  }

  if (now)
    AnnounceNow();
}

void CApplicationContentGeometry::OnAnnounceTimer()
{
  {
    std::unique_lock lock(m_announceSection);
    m_announcePending = false;
    m_lastAnnounceMs = CTimeUtils::MonotonicMs();
  }

  AnnounceNow();
}

void CApplicationContentGeometry::AnnounceNow()
{
  const auto [geometry, drawn] = Snapshot();
  Announce(geometry, drawn);
}

void CApplicationContentGeometry::SetDrawn(const DrawnGeometry& drawn)
{
  const DrawnGeometry rounded{WholePixels(drawn.picture), WholePixels(drawn.raster)};

  {
    std::unique_lock lock(m_section);
    if (rounded.picture == m_drawn.picture && rounded.raster == m_drawn.raster)
      return;

    m_drawn = rounded;
  }

  AnnounceThrottled();
}

DrawnGeometry CApplicationContentGeometry::Drawn() const
{
  std::unique_lock lock(m_section);
  return m_drawn;
}

std::pair<EffectiveGeometry, DrawnGeometry> CApplicationContentGeometry::Snapshot() const
{
  std::unique_lock lock(m_section);
  return {m_current, m_drawn};
}

void CApplicationContentGeometry::SetFileInputs(const ContentGeometryLookup& cached,
                                                std::vector<CRectInt> sections,
                                                float declaredAspect)
{
  {
    std::unique_lock lock(m_section);

    m_inputs = {};
    m_discovered.clear();
    m_inputs.cached = cached;
    m_inputs.sections = std::move(sections);
    m_inputs.declaredAspect = declaredAspect;

    m_overrides = m_pending;
    m_pending = {};

    TakeMaskAspectLocked();
    SeedLiveRatchetLocked();
    m_haveStream = false;
    m_current = AtRestGeometry();

    m_carryRaster = true;
  }
}

void CApplicationContentGeometry::SetOverrides(const GeometryOverrides& overrides)
{
  {
    std::unique_lock lock(m_section);
    m_overrides = overrides;
  }

  ApplyRaster();

  Refresh();
}

void CApplicationContentGeometry::SetPendingOverrides(const GeometryOverrides& overrides)
{
  std::unique_lock lock(m_section);
  m_pending = overrides;
}

void CApplicationContentGeometry::ClearPendingOverrides()
{
  std::unique_lock lock(m_section);
  m_pending = {};
}

GeometryOverrides CApplicationContentGeometry::GetOverrides() const
{
  std::unique_lock lock(m_section);
  return m_overrides;
}

float CApplicationContentGeometry::RasterAspect() const
{
  {
    std::unique_lock lock(m_section);
    if (m_overrides.rasterAspect > 0.0f)
      return m_overrides.rasterAspect;
  }

  return RasterAspectFromSettings();
}

OsdPlacement CApplicationContentGeometry::OsdPlacementInForce() const
{
  {
    std::unique_lock lock(m_section);
    if (m_overrides.osdPlacement)
      return *m_overrides.osdPlacement;
  }

  return m_osdPlacement.load(std::memory_order_relaxed);
}

void CApplicationContentGeometry::RefreshOsdPlacement()
{
  OsdPlacement placement{OsdPlacement::Raster};

  const auto settings = CServiceBroker::GetSettingsComponent();
  const auto values = settings ? settings->GetSettings() : nullptr;
  if (values && values->GetInt(CSettings::SETTING_VIDEOSCREEN_OSDPLAYING) ==
                    static_cast<int>(OsdPlacement::Picture))
    placement = OsdPlacement::Picture;

  m_osdPlacement.store(placement, std::memory_order_relaxed);
}

void CApplicationContentGeometry::SetLive(const CRectInt& rect,
                                          bool varies,
                                          std::vector<CRectInt> found)
{
  bool republish{true};
  {
    std::unique_lock lock(m_section);

    m_discovered = std::move(found);

    const float par{PixelAspectRatio(m_inputs.stream)};
    const float reading{rect.Height() > 0 ? static_cast<float>(rect.Width()) * par /
                                                static_cast<float>(rect.Height())
                                          : 0.0f};
    const auto advanced = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
    const bool always{advanced && advanced->m_videoContentGeometryLiveRepublishes};
    if (always || LiveReadingWidens(reading, m_livePublishedAspect))
      m_livePublishedAspect = reading;
    else
      republish = false;

    if (republish)
    {
      m_inputs.live = {};
      m_inputs.live.rect = rect;
      m_inputs.live.envelope = rect;
      m_inputs.live.varies =
          varies || (m_inputs.cached.HasRecord() && m_inputs.cached.record.varies);
      m_inputs.live.hasReading = true;
      m_inputs.hasLive = true;
    }
  }

  if (republish)
    Refresh();
}

void CApplicationContentGeometry::ClearLive()
{
  {
    std::unique_lock lock(m_section);
    m_inputs.live = {};
    m_inputs.hasLive = false;

    SeedLiveRatchetLocked();
  }

  Refresh();
}

void CApplicationContentGeometry::RefreshAtRest()
{
  {
    std::unique_lock lock(m_section);
    if (m_haveStream)
      return;
  }

  Set(AtRestGeometry());
}

void CApplicationContentGeometry::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (!setting)
    return;

  const std::string& id = setting->GetId();

  if (id == CSettings::SETTING_VIDEOSCREEN_OSDPLAYING)
  {
    RefreshOsdPlacement();
    return;
  }

  if (id != CSettings::SETTING_VIDEOSCREEN_RASTERASPECT &&
      id != CSettings::SETTING_VIDEOSCREEN_VARIABLECONTENTGEOMETRY &&
      id != CSettings::SETTING_VIDEOSCREEN_GUIKEEPSHAPE &&
      id != CSettings::SETTING_VIDEOSCREEN_GUISURROUND)
    return;

  bool playing = false;
  {
    std::unique_lock lock(m_section);
    playing = m_haveStream;
  }

  if (playing)
    Refresh();
  else
    RefreshAtRest();
}

void CApplicationContentGeometry::Refresh()
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (!appPlayer)
    return;

  VideoStreamInfo stream;
  appPlayer->GetVideoStreamInfo(CURRENT_STREAM, stream);
  if (!stream.valid || stream.width <= 0 || stream.height <= 0)
  {
    {
      std::unique_lock lock(m_section);
      m_haveStream = false;
    }
    Set(AtRestGeometry());
    return;
  }

  const CVideoSettings videoSettings = appPlayer->GetVideoSettings();

  // Gathered before the section is taken: none of it depends on what the section guards, and
  // the settings store has a lock of its own.
  const VariableGeometryPolicy policy = ContentGeometryPolicyFromSettings();
  const float atRestAspect = ContentGeometryAtRestFromSettings();
  const int videoStream = appPlayer->GetVideoStream();

  EffectiveGeometry resolved;
  bool carryRaster = false;
  {
    std::unique_lock lock(m_section);

    carryRaster = m_carryRaster;
    m_carryRaster = false;

    m_inputs.stream =
        MeasuredStreamGeometry(stream.stereoMode, static_cast<unsigned int>(stream.width),
                               static_cast<unsigned int>(stream.height), stream.videoAspectRatio);

    m_inputs.stream.orientation = stream.orientation + videoSettings.m_Orientation;

    m_inputs.declaredAspect = videoSettings.m_declaredAspect;
    m_inputs.policy = policy;
    m_inputs.atRestAspect = atRestAspect;

    resolved = ResolveEffectiveGeometry(InputsForStream(m_inputs, videoStream));
    m_haveStream = true;
  }

  CLog::LogF(LOGDEBUG, "content rect {}x{} at {},{} in a {}x{} frame ({}, {}{}{})",
             resolved.displayRect.Width(), resolved.displayRect.Height(), resolved.displayRect.x1,
             resolved.displayRect.y1, resolved.displayFrame.Width(), resolved.displayFrame.Height(),
             resolved.label, GeometrySourceName(resolved.source), resolved.stale ? ", stale" : "",
             resolved.varies ? ", varies" : "");

  if (resolved.rejected)
  {
    CLog::LogF(LOGDEBUG,
               "a measurement was rejected as matching no real ratio, so the frame is served "
               "instead");
  }

  Set(resolved);

  if (carryRaster)
    ApplyRaster();
}

void CApplicationContentGeometry::Clear()
{
  {
    std::unique_lock lock(m_section);
    m_inputs = {};

    m_overrides = {};
    m_pending = {};

    m_livePublishedAspect = 0.0f;
    m_maskAspect = 0.0f;
    m_haveStream = false;

    m_drawn = {};

    m_carryRaster = false;
  }

  ApplyRaster();

  Set(AtRestGeometry());
}

void CApplicationContentGeometry::ApplyRaster()
{
  auto* const winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
    return;

  if (RasterAspect() == winSystem->GetGfxContext().GetRasterAspect())
    return;

  CApplicationSettingsHandling::ApplyRasterChange();
}

EffectiveGeometry CApplicationContentGeometry::Get() const
{
  std::unique_lock lock(m_section);
  return m_current;
}

std::vector<CRectInt> CApplicationContentGeometry::Discovered() const
{
  std::unique_lock lock(m_section);
  return m_discovered;
}

CApplicationContentGeometry::RenderInputs CApplicationContentGeometry::GetRenderInputs() const
{
  std::unique_lock lock(m_section);

  return {RenderGeometryOf(m_current), m_maskAspect, m_overrides.maintainAspect};
}

void CApplicationContentGeometry::TakeMaskAspectLocked()
{
  m_maskAspect = m_inputs.cached.HasRecord() ? WidestAspect(m_inputs.cached.record) : 0.0f;
}

void CApplicationContentGeometry::SeedLiveRatchetLocked()
{
  m_livePublishedAspect = m_maskAspect;
}

float CApplicationContentGeometry::DetectedAspect() const
{
  std::unique_lock lock(m_section);
  return m_haveStream ? ResolveDetectedAspect(m_inputs) : 0.0f;
}

void CApplicationContentGeometry::ApplyDeclaredAspect(CApplicationPlayer& player, float ratio)
{
  CVideoSettings vs = player.GetVideoSettings();
  if (ratio > 0.0f)
    vs.DeclareAspect(ratio, DetectedAspect());
  else
    vs.ClearDeclaredAspect();

  player.SetVideoSettings(vs);
}
