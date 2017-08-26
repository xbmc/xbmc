#pragma once
/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "threads/CriticalSection.h"

#include "pvr/PVRTypes.h"

namespace PVR
{
  enum class ChannelSwitchMode
  {
    NO_SWITCH, // no channel switch
    INSTANT_OR_DELAYED_SWITCH // switch according to SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT
  };

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
     * @brief Set a new playing channel and show the channel info OSD for the new channel.
     * @param channel The new playing channel
     */
    void SetPlayingChannel(const CPVRChannelPtr channel);

    /*!
     * @brief Clear the currently playing channel and hide the channel info OSD.
     */
    void ClearPlayingChannel();

  private:
    /*!
     * @brief Get next or previous channel of the playing channel group, relative to the currently selected channel.
     * @param bNext True to get the next channel, false to get the previous channel.
     * @param return The channel or nullptr if not found.
     */
    CPVRChannelPtr GetNextOrPrevChannel(bool bNext);

    /*!
     * @brief Select a given channel, display channel info OSD, switch according to given switch mode.
     * @param item The channel to select.
     * @param eSwitchMode The channel switch mode.
     */
    void SelectChannel(const CPVRChannelPtr channel, ChannelSwitchMode eSwitchMode);

    /*!
     * @brief Show the channel info OSD.
     * @param bForce True ignores value of SETTING_PVRMENU_DISPLAYCHANNELINFO and always activates the info, False acts aaccording settings value.
     */
    void ShowInfo(bool bForce);

    CCriticalSection m_critSection;
    CPVRChannelPtr m_playingChannel;
    CPVRChannelPtr m_currentChannel;
    int m_iChannelEntryJobId = -1;
    int m_iChannelInfoJobId = -1;
  };

} // namespace PVR
