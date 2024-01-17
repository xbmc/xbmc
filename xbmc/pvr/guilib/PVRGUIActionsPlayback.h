/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/IPVRComponent.h"
#include "pvr/settings/PVRSettings.h"
#include "utils/ContentUtils.h"

#include <string>

class CFileItem;

namespace PVR
{
enum PlaybackType
{
  PlaybackTypeAny = 0,
  PlaybackTypeTV,
  PlaybackTypeRadio
};

class CPVRGUIActionsPlayback : public IPVRComponent
{
public:
  CPVRGUIActionsPlayback();
  ~CPVRGUIActionsPlayback() override = default;

  /*!
   * @brief Resume a previously not completely played recording.
   * @param item containing a recording or an epg tag.
   * @param bFallbackToPlay controls whether playback of the recording should be started at the
   * beginning ig no resume data are available.
   * @return true on success, false otherwise.
   */
  bool ResumePlayRecording(const CFileItem& item, bool bFallbackToPlay) const;

  /*!
   * @brief Play recording.
   * @param item containing a recording or an epg tag.
   * @param bCheckResume controls resume check.
   * @return true on success, false otherwise.
   */
  bool PlayRecording(const CFileItem& item, bool bCheckResume) const;

  /*!
   * @brief Play a recording folder.
   * @param item containing a recording folder.
   * @param bCheckResume controls resume check.
   * @return true on success, false otherwise.
   */
  bool PlayRecordingFolder(const CFileItem& item, bool bCheckResume) const;

  /*!
   * @brief Play EPG tag.
   * @param item containing an epg tag.
   * @param mode playback mode.
   * @return true on success, false otherwise.
   */
  bool PlayEpgTag(
      const CFileItem& item,
      ContentUtils::PlayMode mode = ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM) const;

  /*!
   * @brief Switch channel.
   * @param item containing a channel or an epg tag.
   * @param bCheckResume controls resume check in case a recording for the current epg event is
   * present.
   * @return true on success, false otherwise.
   */
  bool SwitchToChannel(const CFileItem& item, bool bCheckResume) const;

  /*!
   * @brief Playback the given file item.
   * @param item containing a channel or a recording.
   * @return True if the item could be played, false otherwise.
   */
  bool PlayMedia(const CFileItem& item) const;

  /*!
   * @brief Start playback of the last played channel, and if there is none, play first channel in
   * the current channelgroup.
   * @param type The type of playback to be started (any, radio, tv). See PlaybackType enum
   * @return True if playback was started, false otherwise.
   */
  bool SwitchToChannel(PlaybackType type) const;

  /*!
   * @brief Plays the last played channel or the first channel of TV or Radio on startup.
   * @return True if playback was started, false otherwise.
   */
  bool PlayChannelOnStartup() const;

  /*!
   * @brief Seek to the start of the next epg event in timeshift buffer, relative to the currently
   * playing event. If there is no next event, seek to the end of the currently playing event (to
   * the 'live' position).
   */
  void SeekForward();

  /*!
   * @brief Seek to the start of the previous epg event in timeshift buffer, relative to the
   * currently playing event or if there is no previous event or if playback time is greater than
   * given threshold, seek to the start of the playing event.
   * @param iThreshold the value in seconds to trigger seek to start of current event instead of
   * start of previous event.
   */
  void SeekBackward(unsigned int iThreshold);

private:
  CPVRGUIActionsPlayback(const CPVRGUIActionsPlayback&) = delete;
  CPVRGUIActionsPlayback const& operator=(CPVRGUIActionsPlayback const&) = delete;

  /*!
   * @brief Check whether resume play is possible for a given item, display "resume from ..."/"play
   * from start" context menu in case.
   * @param item containing a recording or an epg tag.
   * @return true, to play/resume the item, false otherwise.
   */
  bool CheckResumeRecording(const CFileItem& item) const;

  /*!
   * @brief Check "play minimized" settings value and switch to fullscreen if not set.
   * @param bFullscreen switch to fullscreen or set windowed playback.
   */
  void CheckAndSwitchToFullscreen(bool bFullscreen) const;

  CPVRSettings m_settings;
};

namespace GUI
{
// pretty scope and name
using Playback = CPVRGUIActionsPlayback;
} // namespace GUI

} // namespace PVR
