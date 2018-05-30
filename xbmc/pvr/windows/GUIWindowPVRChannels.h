/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#pragma once

#include "pvr/PVRChannelNumberInputHandler.h"
#include "pvr/windows/GUIWindowPVRBase.h"

namespace PVR
{
  class CGUIWindowPVRChannelsBase : public CGUIWindowPVRBase, public CPVRChannelNumberInputHandler
  {
  public:
    CGUIWindowPVRChannelsBase(bool bRadio, int id, const std::string &xmlFile);
    ~CGUIWindowPVRChannelsBase() override;

    bool OnMessage(CGUIMessage& message) override;
    void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
    bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
    bool Update(const std::string &strDirectory, bool updateFilterPath = true) override;
    void UpdateButtons(void) override;
    bool OnAction(const CAction &action) override;

    // CPVRChannelNumberInputHandler implementation
    void GetChannelNumbers(std::vector<std::string>& channelNumbers) override;
    void OnInputDone() override;

  private:
    bool OnContextButtonManage(const CFileItemPtr &item, CONTEXT_BUTTON button);

    void ShowChannelManager();
    void ShowGroupManager();
    void UpdateEpg(const CFileItemPtr &item);

  protected:
    bool m_bShowHiddenChannels;
  };

  class CGUIWindowPVRTVChannels : public CGUIWindowPVRChannelsBase
  {
  public:
    CGUIWindowPVRTVChannels();

  protected:
    std::string GetDirectoryPath() override;
  };

  class CGUIWindowPVRRadioChannels : public CGUIWindowPVRChannelsBase
  {
  public:
    CGUIWindowPVRRadioChannels();

  protected:
    std::string GetDirectoryPath() override;
  };
}
