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

//! \brief Payload of CDVDMsg::PLAYER_CONTENT_GEOMETRY, in coded space as a stored measurement
//! is.
struct LiveGeometryUpdate
{
  bool clear{false}; //!< withdraw the live reading rather than replace it
  CRectInt rect{};
  bool varies{false};

  //! \brief Every shape served from outside the exclusions, carried on each change.
  std::vector<CRectInt> found{};
};

/*!
 * \brief Reads the pictures a playing stream decodes and reports the shape on screen.
 *
 * Owns frame access and delivery, posting changes to the player's message queue rather than
 * resolving them on the video thread.
 */
class CLiveGeometryMonitor
{
public:
  CLiveGeometryMonitor(CDVDMessageQueue& messageParent, CProcessInfo& processInfo);

  //! \brief Forget everything. Whether sampling is permitted rides in on \p hint.
  void OnStreamOpened(const CDVDStreamInfo& hint);

  //! \brief A seek, or a discontinuity treated like one.
  void OnFlush();

  //! \brief Read one decoded picture, on the video output thread, and return the shape in
  //! force for it. Reads only at normal \p speed. Empty leaves the stored measurement in
  //! charge.
  CRectInt OnPicture(const VideoPicture& picture, const CDVDStreamInfo& hints, int speed);

  //! \brief One line of state for the player's debug overlay. Safe from any thread.
  std::string GetDebugInfo() const;

private:
  void Post(const LiveGeometryUpdate& update);

  //! \brief Does nothing when no reading is out.
  void Withdraw();

  void SetState(std::string_view state);

  //! \brief Asking clears it.
  bool TakeDebugRequest();

  //! \brief What every picture is stamped with, including the ones no reading was taken from.
  CRectInt InForce() const;

  //! \brief Point \p frame at the picture's own planes, or at a reduced copy its device
  //! produced. \p reduced says which, and a reduced reading is in the reduction's coordinates.
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

  //! \brief The rules in force, and when they were last re-read.
  KODI::VIDEO::GEOMETRY::LiveGeometrySettings m_settings;
  float m_declaredAspect{0.0f};
  int64_t m_settingsReadMs{0};

  //! \brief What the debounce was configured against; a mismatch reconfigures.
  unsigned int m_codedWidth{0};
  unsigned int m_codedHeight{0};
  std::string m_stereoMode;

  bool m_unreadableLogged{false};
  bool m_reducedLogged{false};

  //! \brief The reduced copy a hardware-decoded picture is read through; its plane storage is
  //! allocated once for the stream.
  KODI::VIDEO::GEOMETRY::ReducedFrame m_reduction;

  //! \brief What producing the reductions has cost, for the debug overlay.
  double m_reduceTotalMs{0.0};
  uint64_t m_reduceCount{0};

  //! \brief The shapes served from outside the exclusions, which the title may be recorded as
  //! containing.
  std::vector<CRectInt> m_found;

  mutable CCriticalSection m_section; //!< guards m_state, read by the render thread
  std::string m_state;

  //! \brief The last state written, video thread only.
  std::string m_stateWritten;

  //! \brief Bumped by every read of the state and sampled by the video thread - see
  //! TakeDebugRequest().
  mutable std::atomic<uint32_t> m_debugReads{0};
  uint32_t m_debugReadsSeen{0}; //!< video thread only
};
