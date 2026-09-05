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
#include "threads/Timer.h"
#include "video/geometry/EffectiveGeometry.h"

#include <cstdint>
#include <utility>
#include <vector>

class CApplicationPlayer;

/*!
 * \brief The content rectangle in force, for everything that needs to know it.
 *
 * One resolution point rather than one per consumer, so the OSD, subtitle placement and any
 * external controller cannot disagree about what is on the screen.
 */
class CApplicationContentGeometry : public IApplicationComponent, public ISettingCallback
{
public:
  CApplicationContentGeometry();

  void RefreshAtRest();

  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  /*!
   * \brief Take the stored inputs for the file that is opening.
   *
   * \param cached what the detection cache holds, already verified against the file
   * \param sections the geometries the measurement was made of, dominant first
   * \param declaredAspect the ratio the user declared for this file, or zero for none
   */
  void SetFileInputs(const KODI::VIDEO::GEOMETRY::ContentGeometryLookup& cached,
                     std::vector<CRectInt> sections,
                     float declaredAspect);

  //! \brief Resolve again against the stream that is playing now.
  void Refresh();

  /*!
   * \brief Take a reading confirmed during playback, in coded space, and serve it.
   *
   * \param varies the player has confirmed more than one shape this playback; the stored
   *        measurement's own verdict is OR-ed in
   * \param found every shape this playback has seen, held for whoever writes them back rather
   *        than acted on here
   */
  void SetLive(const CRectInt& rect, bool varies, std::vector<CRectInt> found);

  //! \brief Leaves the stored inputs to answer as they did before playback.
  void ClearLive();

  //! \brief The shapes this playback saw, for whoever writes them back.
  std::vector<CRectInt> Discovered() const;

  //! \brief Take what an automation system has stated for this playback. Applied on top of the
  //! settings, and cleared as the next file opens.
  void SetOverrides(const KODI::VIDEO::GEOMETRY::GeometryOverrides& overrides);

  //! \brief Arm overrides for the file that is about to open. SetFileInputs() promotes them.
  void SetPendingOverrides(const KODI::VIDEO::GEOMETRY::GeometryOverrides& overrides);

  //! \brief Disarm without disturbing the playback in progress, for a request to play that never
  //! reaches a player and so is never announced as having stopped.
  void ClearPendingOverrides();

  KODI::VIDEO::GEOMETRY::GeometryOverrides GetOverrides() const;

  /*!
   * \brief The raster in force - the override if one was stated, otherwise the setting.
   *
   * \return the ratio, or zero for "the same as the display"
   */
  float RasterAspect() const;

  //! \brief Where the OSD is laid out, the override taking precedence over the setting.
  KODI::VIDEO::GEOMETRY::OsdPlacement OsdPlacementInForce() const;

  /*!
   * \brief Take where the renderer has just drawn the picture. Arrives once a frame, so it is
   *        rounded to whole pixels and published only when it moves.
   *
   * \param drawn the picture and the operating area, in screen pixels, or empty rectangles
   *        while nothing is drawn
   */
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

    //! \brief The widest shape the stored measurement found, which the masking sits at for the
    //! whole title, or zero when nothing was measured.
    float maskAspect{0.0f};

    float maintainAspect{0.0f}; //!< GetOverrides().maintainAspect
  };

  //! \brief The answers ManageRenderArea() needs, under one acquisition of the section.
  RenderInputs GetRenderInputs() const;

  /*!
   * \brief The ratio the measurement alone gives, ignoring any declaration. Recorded alongside a
   *        declaration, to tell a corrected detection from one that never had an answer.
   *
   * \return zero when nothing was measured
   */
  float DetectedAspect() const;

  //! \brief Declare \p ratio for the playing file, or withdraw the declaration for zero.
  //! The one write path for a declaration.
  void ApplyDeclaredAspect(CApplicationPlayer& player, float ratio);

private:
  //! \brief Publish \p geometry, announcing it only when it differs from what is already out.
  void Set(const KODI::VIDEO::GEOMETRY::EffectiveGeometry& geometry);

  //! \brief RenderInputs::maskAspect, for a caller that already holds the section.
  /*!
   rief Set the widen-only floor to the widest shape the stored measurement found.

   A title whose widest shape is known must not have the masking brought in by a scene composed
   narrower than it - including its opening scene, which arrives before any reading could have
   raised the floor.
   */
  void SeedLiveRatchetLocked();

  float MaskAspectLocked() const;

  //! \brief Carry a moved raster to the layout. Does nothing when the raster stated is already
  //! in force.
  void ApplyRaster();

  static void Announce(const KODI::VIDEO::GEOMETRY::EffectiveGeometry& geometry,
                       const KODI::VIDEO::GEOMETRY::DrawnGeometry& drawn);

  //! \brief Announce the change, or arrange for the latest one to be announced shortly. Changes
  //! within the interval are coalesced; what Get() answers is never held back.
  void AnnounceThrottled();

  void OnAnnounceTimer();

  //! \brief Announce the shape in force at this moment, freshly snapshotted.
  void AnnounceNow();

  std::pair<KODI::VIDEO::GEOMETRY::EffectiveGeometry, KODI::VIDEO::GEOMETRY::DrawnGeometry>
  Snapshot() const;

  mutable CCriticalSection m_section;
  KODI::VIDEO::GEOMETRY::GeometryInputs m_inputs;
  KODI::VIDEO::GEOMETRY::EffectiveGeometry m_current;
  KODI::VIDEO::GEOMETRY::DrawnGeometry m_drawn;
  KODI::VIDEO::GEOMETRY::GeometryOverrides m_overrides;
  KODI::VIDEO::GEOMETRY::GeometryOverrides m_pending;

  //! \brief What live detection has seen this playback that may be written back.
  std::vector<CRectInt> m_discovered;
  bool m_haveStream{false};

  //! The floor a live reading has to clear to be served, as a display ratio - see
  //! LiveReadingWidens(). Seeded from the stored measurement and raised by a wider reading;
  //! zero when nothing is known, and cleared with the title.
  float m_livePublishedAspect{0.0f};

  //! \brief A promotion moved the raster in force and the layout has not been told yet. Carried
  //! at the first resolve against a real stream - see Refresh().
  bool m_carryRaster{false};

  //! \brief Shortest interval between two announcements.
  static constexpr int64_t ANNOUNCE_INTERVAL_MS = 1000;

  CCriticalSection m_announceSection;
  int64_t m_lastAnnounceMs{0};
  bool m_announcePending{false};
  CTimer m_announceTimer{[this]() { OnAnnounceTimer(); }};
};
