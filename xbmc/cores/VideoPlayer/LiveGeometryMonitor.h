/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Buffers/VideoBuffer.h"
#include "threads/CriticalSection.h"
#include "utils/Geometry.h"
#include "video/geometry/FrameReduction.h"
#include "video/geometry/GeometrySettings.h"
#include "video/geometry/LiveGeometrySelector.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class CDVDMessageQueue;
class CDVDStreamInfo;
class CProcessInfo;
struct VideoPicture;

namespace KODI::VIDEO::GEOMETRY
{
struct FrameRef;
} // namespace KODI::VIDEO::GEOMETRY

//! \brief Payload of CDVDMsg::PLAYER_CONTENT_GEOMETRY. The rectangle is in coded space, exactly
//! as a stored measurement is.
struct LiveGeometryUpdate
{
  //! \brief Withdraw the live reading rather than replace it.
  bool clear{false};

  CRectInt rect{};
  bool varies{false};

  //! \brief Every distinct shape served from outside the opening and closing exclusions, carried
  //! on each change - the video thread has no moment it can call the end of a playback.
  std::vector<CRectInt> found{};
};

/*!
 * \brief Reads the pictures a playing stream decodes and reports the shape on screen.
 *
 * Reads the frames playback is already decoding, in coded space at native bit depth, at around a
 * third of a millisecond a frame. The selector owns what is served; this class owns frame access
 * and delivery, posting a change to the player's message queue rather than applying it here,
 * resolution needing player state the video thread must not lock.
 */
class CLiveGeometryMonitor
{
public:
  CLiveGeometryMonitor(CDVDMessageQueue& messageParent, CProcessInfo& processInfo);

  //! \brief Forgets everything; whether sampling is permitted at all rides in on the hint.
  void OnStreamOpened(const CDVDStreamInfo& hint);

  //! \brief A seek, or a discontinuity treated like one.
  void OnFlush();

  /*!
   * \brief Read one decoded picture, on the video output thread.
   *
   * \param speed reading happens only at normal speed - trick play scrubs across the timeline
   *        and the shape must not chase it
   * \return the shape in force for this picture, to be carried on it to the renderer. Empty
   *         when nothing has read this stream, which leaves the stored measurement in charge.
   */
  CRectInt OnPicture(const VideoPicture& picture, const CDVDStreamInfo& hints, int speed);

  //! \brief One line of state for the player's debug overlay. Safe from any thread.
  std::string GetDebugInfo() const;

private:
  void Post(const LiveGeometryUpdate& update);

  //! \brief Does nothing when no reading is out.
  void Withdraw();

  //! \brief Takes the lock only when the state actually differs.
  void SetState(std::string_view state);

  //! \brief Asking clears it, so nothing is built while the overlay is closed.
  bool TakeDebugRequest();

  //! \brief What every picture is stamped with, including the ones no reading was taken from.
  CRectInt InForce() const;

  /*!
   * \brief Point \p frame at the picture's own planes, or failing that at a reduced copy its
   * decoder's device produced. \p reduced says which; a reduced reading is in the reduction's
   * coordinates.
   *
   * \return false when the stream is unreadable either way
   */
  bool AcquireFrame(const VideoPicture& picture,
                    const CDVDStreamInfo& hints,
                    KODI::VIDEO::GEOMETRY::FrameRef& frame,
                    bool& reduced);

  //! \brief Remember a newly served shape when it is recordable, and post it to the player.
  void PublishServed(const KODI::VIDEO::GEOMETRY::LiveGeometryReading& served);

  CDVDMessageQueue& m_messageParent;
  CProcessInfo& m_processInfo;

  KODI::VIDEO::GEOMETRY::CLiveGeometrySelector m_selector;

  bool m_allowed{false};

  //! \brief The rules in force, and when they were last re-read. Re-read on a slow cadence -
  //! reading the settings store is far dearer than reading the frame.
  KODI::VIDEO::GEOMETRY::LiveGeometrySettings m_settings;
  float m_declaredAspect{0.0f};
  int64_t m_settingsReadMs{0};

  //! \brief What the debounce was configured against; a mismatch reconfigures.
  unsigned int m_codedWidth{0};
  unsigned int m_codedHeight{0};
  std::string m_stereoMode;

  bool m_unreadableLogged{false};
  bool m_reducedLogged{false};

  //! \brief The reduced copy a hardware-decoded picture is read through. A member so its plane
  //! storage is allocated once and reused for every frame of the stream.
  KODI::VIDEO::GEOMETRY::ReducedFrame m_reduction;

  //! \brief What producing the reductions has cost, for the debug overlay.
  double m_reduceTotalMs{0.0};
  uint64_t m_reduceCount{0};

  //! \brief The distinct shapes served from outside the opening and closing exclusions, which is
  //! what the title may be recorded as containing.
  std::vector<CRectInt> m_found;

  mutable CCriticalSection m_section; //!< guards m_state, read by the render thread
  std::string m_state;

  //! \brief Bumped by every read of the state and sampled by the video thread - see
  //! TakeDebugRequest().
  mutable std::atomic<uint32_t> m_debugReads{0};
  uint32_t m_debugReadsSeen{0}; //!< video thread only
};
