/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI::VIDEO
{
enum class SubtitleTrackAction
{
  NEXT,
  PREV,
  CYCLE
};

struct SubtitleTrackResult
{
  int newIndex;
  bool newVisible;
};

class CPlayerControllerActions
{
private:
  CPlayerControllerActions() = delete;

public:
  /*!
   * \brief Pure navigation logic invoked for subtitle actions.
   * \param[in] streamCount Count of streams
   * \param[in] currentIndex Current index
   * \param[in] visible Current visibility
   * \param[in] action Action
   * \return New state
   */
  static SubtitleTrackResult ExecSubtitleTrackAction(int streamCount,
                                                     int currentIndex,
                                                     bool visible,
                                                     SubtitleTrackAction action);
};
} // namespace KODI::VIDEO
