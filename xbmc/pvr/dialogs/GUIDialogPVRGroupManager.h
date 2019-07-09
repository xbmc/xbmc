/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "pvr/PVRThumbLoader.h"
#include "pvr/PVRTypes.h"
#include "view/GUIViewControl.h"

class CFileItemList;
class CGUIMessage;

namespace PVR
{
  class CGUIDialogPVRGroupManager : public CGUIDialog
  {
  public:
    CGUIDialogPVRGroupManager(void);
    ~CGUIDialogPVRGroupManager(void) override;
    bool OnMessage(CGUIMessage& message) override;
    bool OnAction(const CAction& action) override;
    void OnWindowLoaded() override;
    void OnWindowUnload() override;

    void SetRadio(bool bIsRadio);

  protected:
    void OnInitWindow() override;
    void OnDeinitWindow(int nextWindowID) override;

  private:
    void Clear();
    void ClearSelectedGroupsThumbnail();
    void Update();
    bool PersistChanges(void);
    bool ActionButtonOk(CGUIMessage &message);
    bool ActionButtonNewGroup(CGUIMessage &message);
    bool ActionButtonDeleteGroup(CGUIMessage &message);
    bool ActionButtonRenameGroup(CGUIMessage &message);
    bool ActionButtonUngroupedChannels(CGUIMessage &message);
    bool ActionButtonGroupMembers(CGUIMessage &message);
    bool ActionButtonChannelGroups(CGUIMessage &message);
    bool ActionButtonHideGroup(CGUIMessage &message);
    bool ActionButtonToggleRadioTV(CGUIMessage &message);
    bool ActionButtonRecreateThumbnail(CGUIMessage& message);
    bool OnMessageClick(CGUIMessage &message);
    bool OnActionMove(const CAction& action);

    CPVRChannelGroupPtr m_selectedGroup;
    bool              m_bIsRadio;

    int m_iSelectedUngroupedChannel = 0;
    int m_iSelectedGroupMember = 0;
    int m_iSelectedChannelGroup = 0;

    CFileItemList *   m_ungroupedChannels;
    CFileItemList *   m_groupMembers;
    CFileItemList *   m_channelGroups;

    CGUIViewControl   m_viewUngroupedChannels;
    CGUIViewControl   m_viewGroupMembers;
    CGUIViewControl   m_viewChannelGroups;

    CPVRThumbLoader m_thumbLoader;
  };
}
