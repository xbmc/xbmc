/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <memory>

namespace PVR
{
  enum class ChannelSwitchMode
  {
    NO_SWITCH, // no channel switch
    INSTANT_OR_DELAYED_SWITCH // switch according to SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT
  };

  class CPVRChannelGroupMember;

  class CPVRGUIChannelNavigator
  {
  public:
    virtual ~CPVRGUIChannelNavigator() = default;

    /*!
     * @brief Select the next channel in currently playing channel group, relative to the currently selected channel.
     * @param eSwitchMode controls whether only the channel info OSD is triggered or whther additionally a (delayed) channel switch will be done.
     */
    void SelectNextChannel(ChannelSwitchMode eSwitchMode);

    /*!
     * @brief Select the previous channel in currently playing channel group, relative to the currently selected channel.
     * @param eSwitchMode controls whether only the channel info OSD is triggered or whther additionally a (delayed) channel switch will be done.
     */
    void SelectPreviousChannel(ChannelSwitchMode eSwitchMode);

    /*!
     * @brief Switch to the currently selected channel.
     */
    void SwitchToCurrentChannel();

    /*!
     * @brief Query the state of channel preview.
     * @return True, if the currently selected channel is different from the currently playing channel, False otherwise.
     */
    bool IsPreview() const;

    /*!
     * @brief Query the state of channel preview and channel info OSD.
     * @return True, if the currently selected channel is different from the currently playing channel and channel info OSD is active, False otherwise.
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
     * @brief Set a new playing channel group member and show the channel info OSD for the new channel.
     * @param groupMember The new playing channel group member
     */
    void SetPlayingChannel(const std::shared_ptr<CPVRChannelGroupMember>& groupMember);

    /*!
     * @brief Clear the currently playing channel and hide the channel info OSD.
     */
    void ClearPlayingChannel();

  private:
    /*!
     * @brief Get next or previous channel group member of the playing channel group, relative to the currently selected channel group member.
     * @param bNext True to get the next channel group member, false to get the previous channel group member.
     * @param return The channel or nullptr if not found.
     */
    std::shared_ptr<CPVRChannelGroupMember> GetNextOrPrevChannel(bool bNext);

    /*!
     * @brief Select a given channel group member, display channel info OSD, switch according to given switch mode.
     * @param groupMember The channel group member to select.
     * @param eSwitchMode The channel switch mode.
     */
    void SelectChannel(const std::shared_ptr<CPVRChannelGroupMember>& groupMember,
                       ChannelSwitchMode eSwitchMode);

    /*!
     * @brief Show the channel info OSD.
     * @param bForce True ignores value of SETTING_PVRMENU_DISPLAYCHANNELINFO and always activates the info, False acts aaccording settings value.
     */
    void ShowInfo(bool bForce);

    mutable CCriticalSection m_critSection;
    std::shared_ptr<CPVRChannelGroupMember> m_playingChannel;
    std::shared_ptr<CPVRChannelGroupMember> m_currentChannel;
    int m_iChannelEntryJobId = -1;
    int m_iChannelInfoJobId = -1;
  };

} // namespace PVR
