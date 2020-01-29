/*
 *  Copyright (C) 2012-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

class CDateTime;
class CFileItem;

namespace PVR
{
class CPVRChannel;
class CPVRChannelGroup;
class CPVREpgInfoTag;
class CPVRRecording;

class CPVRPlaybackState
{
public:
  /*!
   * @brief ctor.
   */
  CPVRPlaybackState();

  /*!
   * @brief dtor.
   */
  virtual ~CPVRPlaybackState();

  /*!
   * @brief Inform that playback of an item just started.
   * @param item The item that started to play.
   */
  void OnPlaybackStarted(const std::shared_ptr<CFileItem> item);

  /*!
   * @brief Inform that playback of an item was stopped due to user interaction.
   * @param item The item that stopped to play.
   * @return True, if the state has changed, false otherwise
   */
  bool OnPlaybackStopped(const std::shared_ptr<CFileItem> item);

  /*!
   * @brief Inform that playback of an item has stopped without user interaction.
   * @param item The item that ended to play.
   */
  void OnPlaybackEnded(const std::shared_ptr<CFileItem> item);

  /*!
   * @brief Check if a TV channel, radio channel or recording is playing.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlaying() const;

  /*!
   * @brief Check if a TV channel is playing.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlayingTV() const;

  /*!
   * @brief Check if a radio channel is playing.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlayingRadio() const;

  /*!
   * @brief Check if a an encrypted TV or radio channel is playing.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlayingEncryptedChannel() const;

  /*!
   * @brief Check if a recording is playing.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlayingRecording() const;

  /*!
   * @brief Check if an epg tag is playing.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlayingEpgTag() const;

  /*!
   * @brief Check whether playing channel matches given uids.
   * @param iClientID The client id.
   * @param iUniqueChannelID The channel uid.
   * @return True on match, false if there is no match or no channel is playing.
   */
  bool IsPlayingChannel(int iClientID, int iUniqueChannelID) const;

  /*!
   * @brief Check if the given channel is playing.
   * @param channel The channel to check.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlayingChannel(const std::shared_ptr<CPVRChannel>& channel) const;

  /*!
   * @brief Check if the given recording is playing.
   * @param recording The recording to check.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlayingRecording(const std::shared_ptr<CPVRRecording>& recording) const;

  /*!
   * @brief Check if the given epg tag is playing.
   * @param epgTag The tag to check.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlayingEpgTag(const std::shared_ptr<CPVREpgInfoTag>& epgTag) const;

  /*!
   * @brief Return the channel that is currently playing.
   * @return The channel or nullptr if none is playing.
   */
  std::shared_ptr<CPVRChannel> GetPlayingChannel() const;

  /*!
   * @brief Return the recording that is currently playing.
   * @return The recording or nullptr if none is playing.
   */
  std::shared_ptr<CPVRRecording> GetPlayingRecording() const;

  /*!
   * @brief Return the epg tag that is currently playing.
   * @return The tag or nullptr if none is playing.
   */
  std::shared_ptr<CPVREpgInfoTag> GetPlayingEpgTag() const;

  /*!
   * @brief Get the name of the playing client, if there is one.
   * @return The name of the client or an empty string if nothing is playing.
   */
  std::string GetPlayingClientName() const;

  /*!
   * @brief Get the ID of the playing client, if there is one.
   * @return The ID or -1 if no client is playing.
   */
  int GetPlayingClientID() const;

  /*!
   * @brief Check whether there are active recordings.
   * @return True if there are active recordings, false otherwise.
   */
  bool IsRecording() const;

  /*!
   * @brief Check whether there is an active recording on the currenlyt playing channel.
   * @return True if there is a playing channel and there is an active recording on that channel, false otherwise.
   */
  bool IsRecordingOnPlayingChannel() const;

  /*!
   * @brief Check if an active recording is playing.
   * @return True if an in-progress (active) recording is playing, false otherwise.
   */
  bool IsPlayingActiveRecording() const;

  /*!
   * @brief Check whether the currently playing channel can be recorded.
   * @return True if there is a playing channel that can be recorded, false otherwise.
   */
  bool CanRecordOnPlayingChannel() const;

  /*!
   * @brief Set the current playing group, used to load the right channel.
   * @param group The new group.
   */
  void SetPlayingGroup(const std::shared_ptr<CPVRChannelGroup>& group);

  /*!
   * @brief Get the current playing group, used to load the right channel.
   * @param bRadio True to get the current radio group, false to get the current TV group.
   * @return The current group or the group containing all channels if it's not set.
   */
  std::shared_ptr<CPVRChannelGroup> GetPlayingGroup(bool bRadio) const;

  /*!
   * @brief Get current playback time, taking timeshifting into account.
   * @return The playback time.
   */
  CDateTime GetPlaybackTime() const;

private:
  /*!
   * @brief Set the playing group to the first group the channel is in if the given channel is not part of the current playing group
   * @param channel The channel
   */
  void SetPlayingGroup(const std::shared_ptr<CPVRChannel>& channel);

  /*!
   * @brief Updates the last watched timestamps of the channel and group which are currently playing.
   * @param channel The channel which is updated
   * @param time The last watched time to set
   */
  void UpdateLastWatched(const std::shared_ptr<CPVRChannel>& channel, const CDateTime& time);

  std::shared_ptr<CPVRChannel> m_playingChannel;
  std::shared_ptr<CPVRRecording> m_playingRecording;
  std::shared_ptr<CPVREpgInfoTag> m_playingEpgTag;
  std::string m_strPlayingClientName;
  int m_playingClientId = -1;
  int m_playingChannelUniqueId = -1;

  class CLastWatchedUpdateTimer;
  std::unique_ptr<CLastWatchedUpdateTimer> m_lastWatchedUpdateTimer;
};
} // namespace PVR
