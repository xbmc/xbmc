/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>

#include "guilib/GUIDialog.h"
#include "utils/Observer.h"
#include "view/GUIViewControl.h"

#include "pvr/PVRChannelNumberInputHandler.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"

class CFileItemList;

namespace PVR
{
  class CGUIDialogPVRChannelsOSD : public CGUIDialog, public Observer, public CPVRChannelNumberInputHandler
  {
  public:
    CGUIDialogPVRChannelsOSD(void);
    ~CGUIDialogPVRChannelsOSD(void) override;
    bool OnMessage(CGUIMessage& message) override;
    bool OnAction(const CAction &action) override;
    void OnWindowLoaded() override;
    void OnWindowUnload() override;
    void Notify(const Observable &obs, const ObservableMessage msg) override;

    // CPVRChannelNumberInputHandler implementation
    void GetChannelNumbers(std::vector<std::string>& channelNumbers) override;
    void OnInputDone() override;

  protected:
    void OnInitWindow() override;
    void OnDeinitWindow(int nextWindowID) override;
    void RestoreControlStates() override;
    void SaveControlStates() override;
    void SetInvalid() override;
    CGUIControl *GetFirstFocusableControl(int id) override;

  private:
    void GotoChannel(int iItem);
    void ShowInfo(int item);
    void Clear();
    void Update();
    void SaveSelectedItemPath(int iGroupID);
    std::string GetLastSelectedItemPath(int iGroupID) const;

    CFileItemList *m_vecItems;
    CGUIViewControl m_viewControl;
    CPVRChannelGroupPtr m_group;
    std::map<int, std::string> m_groupSelectedItemPaths;
    XbmcThreads::EndTime m_refreshTimeout;
  };
}

