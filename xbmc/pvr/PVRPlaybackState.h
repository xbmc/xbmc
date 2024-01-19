/*
 *  Copyright (C) 2012-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "utils/ContentUtils.h"

#include <memory>
#include <string>

class CDateTime;
class CFileItem;

namespace PVR
{
class CPVRChannel;
class CPVRChannelGroup;
class CPVRChannelGroupMember;
class CPVREpgInfoTag;
class CPVRRecording;
class CPVRStreamProperties;

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
   * @brief clear instances, keep stored UIDs.
   */
  void Clear();

  /*!
   * @brief re-init using stored UIDs.
   */
  void ReInit();

  /*!
   * @brief Inform that playback of an item just started.
   * @param item The item that started to play.
   */
  void OnPlaybackStarted(const CFileItem& item);

  /*!
   * @brief Inform that playback of an item was stopped due to user interaction.
   * @param item The item that stopped to play.
   * @return True, if the state has changed, false otherwise
   */
  bool OnPlaybackStopped(const CFileItem& item);

  /*!
   * @brief Inform that playback of an item has stopped without user interaction.
   * @param item The item that ended to play.
   * @return True, if the state has changed, false otherwise
   */
  bool OnPlaybackEnded(const CFileItem& item);

  /*!
   * @brief Start playback of the given item.
   * @param item containing a channel, a recording or an epg tag.
   * @param mode playback mode.
   */
  void StartPlayback(
      CFileItem* item,
      ContentUtils::PlayMode mode = ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM) const;

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
  bool IsPlayingChannel(const std::shared_ptr<const CPVRChannel>& channel) const;

  /*!
   * @brief Check if the given recording is playing.
   * @param recording The recording to check.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlayingRecording(const std::shared_ptr<const CPVRRecording>& recording) const;

  /*!
   * @brief Check if the given epg tag is playing.
   * @param epgTag The tag to check.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlayingEpgTag(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const;

  /*!
   * @brief Return the channel that is currently playing.
   * @return The channel or nullptr if none is playing.
   */
  std::shared_ptr<CPVRChannel> GetPlayingChannel() const;

  /*!
   * @brief Return the channel group member that is currently playing.
   * @return The channel group member or nullptr if none is playing.
   */
  std::shared_ptr<CPVRChannelGroupMember> GetPlayingChannelGroupMember() const;

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
   * @brief Return playing channel unique identifier
   * @return The channel id or -1 if not present
   */
  int GetPlayingChannelUniqueID() const;

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
   * @brief Set the active channel group.
   * @param group The new group.
   */
  void SetActiveChannelGroup(const std::shared_ptr<CPVRChannelGroup>& group);

  /*!
   * @brief Get the active channel group.
   * @param bRadio True to get the active radio group, false to get the active TV group.
   * @return The current group or the group containing all channels if it's not set.
   */
  std::shared_ptr<CPVRChannelGroup> GetActiveChannelGroup(bool bRadio) const;

  /*!
   * @brief Get the last played channel group member.
   * @param bRadio True to get the radio group member, false to get the TV group member.
   * @return The last played group member or nullptr if it's not available.
   */
  std::shared_ptr<CPVRChannelGroupMember> GetLastPlayedChannelGroupMember(bool bRadio) const;

  /*!
   * @brief Get the channel group member that was played before the last played member.
   * @param bRadio True to get the radio group member, false to get the TV group member.
   * @return The previous played group member or nullptr if it's not available.
   */
  std::shared_ptr<CPVRChannelGroupMember> GetPreviousToLastPlayedChannelGroupMember(
      bool bRadio) const;

  /*!
   * @brief Get current playback time for the given channel, taking timeshifting and playing
   * epg tags into account.
   * @param iClientID The client id.
   * @param iUniqueChannelID The channel uid.
   * @return The playback time or 'now' if not playing.
   */
  CDateTime GetPlaybackTime(int iClientID, int iUniqueChannelID) const;

  /*!
   * @brief Get current playback time for the given channel, taking timeshifting into account.
   * @param iClientID The client id.
   * @param iUniqueChannelID The channel uid.
   * @return The playback time or 'now' if not playing.
   */
  CDateTime GetChannelPlaybackTime(int iClientID, int iUniqueChannelID) const;

private:
  void ClearData();

  /*!
   * @brief Return the next item to play automatically, if any.
   * @param item The item which just finished playback.
   * @return The item to play next, if any, nullptr otherwise.
   */
  std::unique_ptr<CFileItem> GetNextAutoplayItem(const CFileItem& item);

  /*!
   * @brief Set the active group to the group of the supplied channel group member.
   * @param channel The channel group member
   */
  void SetActiveChannelGroup(const std::shared_ptr<CPVRChannelGroupMember>& channel);

  /*!
   * @brief Updates the last watched timestamps of the channel and group which are currently playing.
   * @param channel The channel which is updated
   * @param time The last watched time to set
   */
  void UpdateLastWatched(const std::shared_ptr<CPVRChannelGroupMember>& channel,
                         const CDateTime& time);

  mutable CCriticalSection m_critSection;

  std::shared_ptr<CPVRChannelGroupMember> m_playingChannel;
  std::shared_ptr<CPVRRecording> m_playingRecording;
  std::shared_ptr<CPVREpgInfoTag> m_playingEpgTag;
  std::shared_ptr<CPVRChannelGroupMember> m_lastPlayedChannelTV;
  std::shared_ptr<CPVRChannelGroupMember> m_lastPlayedChannelRadio;
  std::shared_ptr<CPVRChannelGroupMember> m_previousToLastPlayedChannelTV;
  std::shared_ptr<CPVRChannelGroupMember> m_previousToLastPlayedChannelRadio;
  std::string m_strPlayingClientName;
  int m_playingGroupId = -1;
  int m_playingClientId = -1;
  int m_playingChannelUniqueId = -1;
  std::string m_strPlayingRecordingUniqueId;
  int m_playingEpgTagChannelUniqueId = -1;
  unsigned int m_playingEpgTagUniqueId = 0;
  std::shared_ptr<CPVRChannelGroup> m_activeGroupTV;
  std::shared_ptr<CPVRChannelGroup> m_activeGroupRadio;

  class CLastWatchedUpdateTimer;
  std::unique_ptr<CLastWatchedUpdateTimer> m_lastWatchedUpdateTimer;
};
} // namespace PVR
