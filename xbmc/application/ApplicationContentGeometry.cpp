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

/*!
 * \brief The shape the interface is actually drawn at, when the room has been asked to close to
 *        it rather than have the area around it filled.
 *
 * A skin that does not go wide is drawn at its own shape inside the raster, and what it leaves
 * is painted with a colour or an image. A room with masking has a better answer: told the shape
 * the interface occupies, its masking closes to that and there is nothing left to fill. Zero
 * unless that is what was asked for, and before the interface has been laid out.
 */
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

/*!
 * \brief The shape Kodi is sitting at, which is what is on the screen when nothing is playing.
 *
 * The viewer's own statement about their room - a 16:9 screen with masks in is a scope room, and
 * no hardware Kodi can interrogate says so. A ratio and no rectangle.
 */
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

//! \brief Round to the screen's own pixels, before the comparison SetDrawn() makes rather than
//! after, so what is published and what is compared against agree.
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

  // No item: this describes what is on the screen, which outlives any one item.
  CVariant data{CVariant::VariantTypeObject};
  SerializeEffectiveGeometry(geometry, data["contentrect"]);

  // Absent while nothing is drawn, rather than a rectangle of zeros a consumer would drive a
  // mask to.
  if (drawn.Drawn())
    SerializeDrawnGeometry(drawn, data["screen"]);

  // Omitted rather than sent as -1 when nothing is playing, TYPE_NONE being outside the
  // declared range of Player.Id - which is the case the last notification of a playback is.
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

  // Announced outside the lock, so a consumer of Get() cannot wait on this path.
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
      // Arrange for whatever the shape is when the window closes. Started once per window
      // rather than restarted per change, so a stream of changes cannot push it out.
      m_announcePending = true;

      // The timer refuses to start while its previous timeout is still running the callback,
      // and the callback clears the flag before it announces - so a change arriving in that
      // window would latch the flag for the rest of the session. Announcing now cannot latch.
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
  // Read now rather than captured when the change arrived, so what goes out is the shape in
  // force at this moment.
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

  // The picture has moved on the screen, which is a change in its own right - an instruction can
  // move it without moving what the content is. Throttled like every other announcement.
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

    // One promotion does both halves: what was stated for the outgoing title stops applying, and
    // what was armed with the request to play this one takes effect.
    m_overrides = m_pending;
    m_pending = {};

    SeedLiveRatchetLocked();
    m_haveStream = false;
    m_current = AtRestGeometry();

    // The promotion can have moved the raster in force. Noted rather than carried: this runs
    // while the file is still opening, and Refresh() says why a skin reload cannot go there.
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

  // The raster can change here, and it is what the picture is contained by.
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

  const auto settings = CServiceBroker::GetSettingsComponent();
  if (!settings)
    return OsdPlacement::Raster;

  return settings->GetSettings()->GetInt(CSettings::SETTING_VIDEOSCREEN_OSDPLAYING) ==
                 static_cast<int>(OsdPlacement::Picture)
             ? OsdPlacement::Picture
             : OsdPlacement::Raster;
}

void CApplicationContentGeometry::SetLive(const CRectInt& rect,
                                          bool varies,
                                          std::vector<CRectInt> found)
{
  bool republish{true};
  {
    std::unique_lock lock(m_section);

    // Kept whatever is decided below: what the watch saw is written back to the record when the
    // playback ends, and that is a different question from what the room is told mid-film.
    m_discovered = std::move(found);

    // Discarded rather than stored and left unannounced: Refresh() resolves from these inputs
    // and several paths reach it, so a reading kept here is published by the next of them to
    // run - which is how a scene composed inside the frame moved the masking anyway.
    //
    // Compared as a display ratio, which is what the floor and the vocabulary are; a coded
    // ratio would put the two in different units on anamorphic content.
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

    // Back to what the measurement knows, not to nothing. This runs mid-title as well as
    // between titles - live detection switched off, or the coded size changing - and zeroing
    // here would let the next reading of the same film bring the masking in.
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

  // Only while nothing is playing: the resting shape is where the room returns to, not
  // something that overrides what is on the screen.
  Set(AtRestGeometry());
}

void CApplicationContentGeometry::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (!setting)
    return;

  // Both change what the resolver answers, and a viewer switching the variable policy mid-film
  // is asking to see the difference now rather than at the next file.
  const std::string& id = setting->GetId();
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

  // The resting shape is also the direction a measurement resolves in, so during playback this
  // can move the served rectangle itself.
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
    // The stored inputs are loaded before the stream exists, so they are kept rather than
    // discarded between a file opening and its first picture.
    {
      std::unique_lock lock(m_section);
      m_haveStream = false;
    }
    Set(AtRestGeometry());
    return;
  }

  const CVideoSettings videoSettings = appPlayer->GetVideoSettings();

  EffectiveGeometry resolved;
  bool carryRaster = false;
  {
    std::unique_lock lock(m_section);

    carryRaster = m_carryRaster;
    m_carryRaster = false;

    // One view of a frame that packs two, which is the region both measurement paths measure.
    m_inputs.stream =
        MeasuredStreamGeometry(stream.stereoMode, static_cast<unsigned int>(stream.width),
                               static_cast<unsigned int>(stream.height), stream.videoAspectRatio);

    // The container's rotation and the viewer's both end up on the screen, so both belong in
    // the figure a mask or an inset is driven from.
    m_inputs.stream.orientation = stream.orientation + videoSettings.m_Orientation;

    m_inputs.declaredAspect = videoSettings.m_declaredAspect;
    m_inputs.policy = ContentGeometryPolicyFromSettings();
    m_inputs.atRestAspect = ContentGeometryAtRestFromSettings();

    resolved = ResolveEffectiveGeometry(InputsForStream(m_inputs, appPlayer->GetVideoStream()));
    m_haveStream = true;
  }

  CLog::LogF(LOGDEBUG, "content rect {}x{} at {},{} in a {}x{} frame ({}, {}{}{})",
             resolved.displayRect.Width(), resolved.displayRect.Height(), resolved.displayRect.x1,
             resolved.displayRect.y1, resolved.displayFrame.Width(), resolved.displayFrame.Height(),
             resolved.label, GeometrySourceName(resolved.source), resolved.stale ? ", stale" : "",
             resolved.varies ? ", varies" : "");

  if (resolved.rejected)
  {
    // Its own line: the rectangle logged above is the frame, so nothing in it says a
    // measurement was taken and thrown away.
    CLog::LogF(LOGDEBUG,
               "a measurement was rejected as matching no real ratio, so the frame is served "
               "instead");
  }

  Set(resolved);

  // Carried here rather than where it is promoted: SetFileInputs() runs while the file is still
  // opening, and a skin reload posted into that wedges the open.
  if (carryRaster)
    ApplyRaster();
}

void CApplicationContentGeometry::Clear()
{
  {
    std::unique_lock lock(m_section);
    m_inputs = {};

    // The armed slot goes too: a request to play that never reached a first frame must not land
    // on the next title.
    m_overrides = {};
    m_pending = {};

    m_livePublishedAspect = 0.0f;
    m_haveStream = false;

    // Dropped here rather than left for the next frame, so the announcement stopping produces
    // does not carry a screen the film has already left.
    m_drawn = {};

    // Nothing is opening, so there is no resolve left to carry it.
    m_carryRaster = false;
  }

  // Reverting is a raster change like any other, so the interface re-selects.
  ApplyRaster();

  // Kodi is showing its own interface now - see AtRestGeometry().
  Set(AtRestGeometry());
}

void CApplicationContentGeometry::ApplyRaster()
{
  auto* const winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
    return;

  // Checked here rather than left to the apply, which carries a skin reload: an automation
  // stating a placement or a maintained ratio must not reload for a raster it never mentioned.
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

  return {RenderGeometryOf(m_current), MaskAspectLocked(), m_overrides.maintainAspect};
}

void CApplicationContentGeometry::SeedLiveRatchetLocked()
{
  m_livePublishedAspect = m_inputs.cached.HasRecord() ? WidestAspect(m_inputs.cached.record) : 0.0f;
}

float CApplicationContentGeometry::MaskAspectLocked() const
{
  return m_inputs.cached.HasRecord() ? WidestAspect(m_inputs.cached.record) : 0.0f;
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
