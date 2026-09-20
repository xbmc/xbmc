/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayerControllerActions.h"

namespace KODI::VIDEO
{
SubtitleTrackResult CPlayerControllerActions::ExecSubtitleTrackAction(int streamCount,
                                                                      int currentIndex,
                                                                      bool visible,
                                                                      SubtitleTrackAction action)
{
  int newIndex = currentIndex;
  bool newVisible = visible;

  if (streamCount == 0 || currentIndex >= streamCount)
    return {newIndex, newVisible};

  if (visible)
  {
    const int delta = (action == SubtitleTrackAction::PREV) ? -1 : 1;
    if (currentIndex == 0 && delta < 0)
    {
      newVisible = false;
    }
    else
    {
      newIndex += delta;
      if (newIndex >= streamCount)
      {
        newIndex = 0;
        if (action != SubtitleTrackAction::CYCLE)
          newVisible = false;
      }
    }
  }
  else
  {
    if (action != SubtitleTrackAction::CYCLE)
    {
      if (currentIndex == 0 && action == SubtitleTrackAction::PREV)
        newIndex = streamCount - 1;
      newVisible = true;
    }
  }

  return {newIndex, newVisible};
}
} // namespace KODI::VIDEO
