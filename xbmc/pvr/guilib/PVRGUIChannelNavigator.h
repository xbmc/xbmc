/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "utils/EventStream.h"

#include <memory>

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{
struct PlayerShowInfoChangedEvent;
}
} // namespace GUILIB
} // namespace KODI

namespace PVR
{
enum class ChannelSwitchMode
{
  NO_SWITCH, // no channel switch
  INSTANT_OR_DELAYED_SWITCH // switch according to SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT
};

struct PVRPreviewAndPlayerShowInfoChangedEvent
{
  explicit PVRPreviewAndPlayerShowInfoChangedEvent(bool previewAndPlayerShowInfo)
    : m_previewAndPlayerShowInfo(previewAndPlayerShowInfo)
  {
  }
  virtual ~PVRPreviewAndPlayerShowInfoChangedEvent() = default;

  bool m_previewAndPlayerShowInfo{false};
};

class CPVRChannelGroupMember;

class CPVRGUIChannelNavigator
{
public:
  CPVRGUIChannelNavigator();
  virtual ~CPVRGUIChannelNavigator();

  /*!
   * @brief Subscribe to the event stream for changes of channel preview and player show info.
   * @param owner The subscriber.
   * @param fn The callback function of the subscriber for the events.
   */
  template<typename A>
  void Subscribe(A* owner, void (A::*fn)(const PVRPreviewAndPlayerShowInfoChangedEvent&))
  {
    SubscribeToShowInfoEventStream();
    m_events.Subscribe(owner, fn);
  }

  /*!
   * @brief Unsubscribe from the event stream for changes of channel preview and player show info.
   * @param obj The subscriber.
   */
  template<typename A>
  void Unsubscribe(A* obj)
  {
    m_events.Unsubscribe(obj);
  }

  /*!
   * @brief CEventStream callback for player show info flag changes.
   * @param event The event.
   */
  void Notify(const KODI::GUILIB::GUIINFO::PlayerShowInfoChangedEvent& event);

  /*!
   * @brief Select the next channel in currently playing channel group, relative to the currently
   * selected channel.
   * @param eSwitchMode controls whether only the channel info OSD is triggered or whether
   * additionally a (delayed) channel switch will be done.
   */
  void SelectNextChannel(ChannelSwitchMode eSwitchMode);

  /*!
   * @brief Select the previous channel in currently playing channel group, relative to the
   * currently selected channel.
   * @param eSwitchMode controls whether only the channel info OSD is triggered or whether
   * additionally a (delayed) channel switch will be done.
   */
  void SelectPreviousChannel(ChannelSwitchMode eSwitchMode);

  /*!
   * @brief Switch to the currently selected channel.
   */
  void SwitchToCurrentChannel();

  /*!
   * @brief Query the state of channel preview.
   * @return True, if the currently selected channel is different from the currently playing
   * channel, False otherwise.
   */
  bool IsPreview() const;

  /*!
   * @brief Query the state of channel preview and channel info OSD.
   * @return True, if the currently selected channel is different from the currently playing channel
   * and channel info OSD is active, False otherwise.
   */
  bool IsPreviewAndShowInfo() const;

  /*!
   * @brief Show the channel info OSD.
   */
  void ShowInfo();

  /*!
   * @brief Hide the channel info OSD.
   */
  void HideInfo();

  /*!
   * @brief Toggle the channel info OSD visibility.
   */
  void ToggleInfo();

  /*!
   * @brief Set a new playing channel group member and show the channel info OSD for the new
   * channel.
   * @param groupMember The new playing channel group member
   */
  void SetPlayingChannel(const std::shared_ptr<CPVRChannelGroupMember>& groupMember);

  /*!
   * @brief Clear the currently playing channel and hide the channel info OSD.
   */
  void ClearPlayingChannel();

private:
  /*!
   * @brief Get next or previous channel group member of the playing channel group, relative to the
   * currently selected channel group member.
   * @param bNext True to get the next channel group member, false to get the previous channel group
   * member.
   * @param return The channel or nullptr if not found.
   */
  std::shared_ptr<CPVRChannelGroupMember> GetNextOrPrevChannel(bool bNext);

  /*!
   * @brief Select a given channel group member, display channel info OSD, switch according to given
   * switch mode.
   * @param groupMember The channel group member to select.
   * @param eSwitchMode The channel switch mode.
   */
  void SelectChannel(const std::shared_ptr<CPVRChannelGroupMember>& groupMember,
                     ChannelSwitchMode eSwitchMode);

  /*!
   * @brief Show the channel info OSD.
   * @param bForce True ignores value of SETTING_PVRMENU_DISPLAYCHANNELINFO and always activates the
   * info, False acts aaccording settings value.
   */
  void ShowInfo(bool bForce);

  /*!
   * @brief Subscribe to the event stream for changes of player show info.
   */
  void SubscribeToShowInfoEventStream();

  /*!
   * @brief Check if property preview and show info value changed, inform subscribers in case.
   */
  void CheckAndPublishPreviewAndPlayerShowInfoChangedEvent();

  mutable CCriticalSection m_critSection;
  std::shared_ptr<CPVRChannelGroupMember> m_playingChannel;
  std::shared_ptr<CPVRChannelGroupMember> m_currentChannel;
  int m_iChannelEntryJobId = -1;
  int m_iChannelInfoJobId = -1;
  CEventSource<PVRPreviewAndPlayerShowInfoChangedEvent> m_events;
  bool m_playerShowInfo{false};
  bool m_previewAndPlayerShowInfo{false};
};
} // namespace PVR
