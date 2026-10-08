/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "application/IApplicationComponent.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"
#include "video/geometry/EffectiveGeometry.h"

#include <atomic>
#include <vector>

//! \brief The content rectangle in force, resolved once for the OSD, subtitle placement and
//! external controllers.
class CApplicationContentGeometry : public IApplicationComponent, public ISettingCallback
{
public:
  CApplicationContentGeometry();

  void RefreshAtRest();

  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  //! \brief Take the stored inputs for the file that is opening, \p cached already verified
  //! against it.
  void SetFileInputs(const KODI::VIDEO::GEOMETRY::ContentGeometryLookup& cached);

  //! \brief Resolve again against the stream that is playing now.
  void Refresh();

  //! \brief Take a reading confirmed during playback, in coded space, and serve it.
  void SetLive(const CRectInt& rect, bool varies);

  //! \brief Leaves the stored inputs to answer as they did before playback.
  void ClearLive();

  //! \brief Take what an automation has stated for this playback, over the settings. Cleared
  //! as the next file opens.
  void SetOverrides(const KODI::VIDEO::GEOMETRY::GeometryOverrides& overrides);

  //! \brief Arm overrides for the file that is about to open. SetFileInputs() promotes them.
  void SetPendingOverrides(const KODI::VIDEO::GEOMETRY::GeometryOverrides& overrides);

  //! \brief Disarm without disturbing the playback in progress.
  void ClearPendingOverrides();

  KODI::VIDEO::GEOMETRY::GeometryOverrides GetOverrides() const;

  //! \brief The raster in force, the override before the setting. Zero means the display's own.
  float RasterAspect() const;

  //! \brief Where the OSD is laid out, the override taking precedence over the setting.
  KODI::VIDEO::GEOMETRY::OsdPlacement OsdPlacementInForce() const;

  //! \brief Take the OSD placement setting. Call once the settings store is up; after that
  //! OnSettingChanged() keeps it current.
  void RefreshOsdPlacement();

  //! \brief Take where the renderer has just drawn the picture, in screen pixels, or empty
  //! rectangles while nothing is drawn. Announced only when it moves.
  void SetDrawn(const KODI::VIDEO::GEOMETRY::DrawnGeometry& drawn);

  //! \brief Where the picture was last drawn, empty while nothing is on the screen.
  KODI::VIDEO::GEOMETRY::DrawnGeometry Drawn() const;

  //! \brief Forget the file, leaving the answer describing the GUI rather than nothing.
  void Clear();

  KODI::VIDEO::GEOMETRY::EffectiveGeometry Get() const;

  //! \brief Everything the render path reads, scalars only.
  struct RenderInputs
  {
    KODI::VIDEO::GEOMETRY::RenderGeometry geometry;

    //! \brief The widest shape the stored measurement found, zero when nothing was measured.
    float maskAspect{0.0f};

    float maintainAspect{0.0f}; //!< GetOverrides().maintainAspect
  };

  RenderInputs GetRenderInputs() const;

private:
  void Set(const KODI::VIDEO::GEOMETRY::EffectiveGeometry& geometry);

  //! \brief The widest ratio the stored measurement holds, which the mask opens to. Zero when
  //! nothing was measured.
  float MaskAspectLocked() const;

  //! \brief Carry a moved raster to the layout; nothing when it is already in force.
  void ApplyRaster();

  mutable CCriticalSection m_section;
  KODI::VIDEO::GEOMETRY::GeometryInputs m_inputs;
  KODI::VIDEO::GEOMETRY::EffectiveGeometry m_current;
  KODI::VIDEO::GEOMETRY::DrawnGeometry m_drawn;
  KODI::VIDEO::GEOMETRY::GeometryOverrides m_overrides;
  KODI::VIDEO::GEOMETRY::GeometryOverrides m_pending;

  bool m_haveStream{false};

  //! \brief Where the OSD is laid out, mirrored out of the settings because the layout asks
  //! for it every frame. Refreshed by OnSettingChanged().
  std::atomic<KODI::VIDEO::GEOMETRY::OsdPlacement> m_osdPlacement{
      KODI::VIDEO::GEOMETRY::OsdPlacement::Raster};

  //! \brief A promotion moved the raster and the layout has not been told yet.
  bool m_carryRaster{false};
};
