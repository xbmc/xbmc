/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <vector>

#include "addons/PVRClient.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "guilib/GUIDialog.h"
#include "view/GUIViewControl.h"

#include "pvr/channels/PVRChannelGroup.h"

namespace PVR
{
  class CGUIDialogPVRChannelManager : public CGUIDialog
  {
  public:
    CGUIDialogPVRChannelManager(void);
    ~CGUIDialogPVRChannelManager(void) override;
    bool OnMessage(CGUIMessage& message) override;
    bool OnAction(const CAction& action) override;
    void OnWindowLoaded(void) override;
    void OnWindowUnload(void) override;
    bool HasListItems() const override{ return true; }
    CFileItemPtr GetCurrentListItem(int offset = 0) override;

  protected:
    void OnInitWindow() override;
    void OnDeinitWindow(int nextWindowID) override;

  private:
    void Clear(void);
    void Update(void);
    void SaveList(void);
    void Renumber(void);
    void SetData(int iItem);
    void RenameChannel(const CFileItemPtr &pItem);

    bool OnPopupMenu(int iItem);
    bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
    bool OnActionMove(const CAction &action);
    bool OnMessageClick(CGUIMessage &message);
    bool OnClickListChannels(CGUIMessage &message);
    bool OnClickButtonOK(CGUIMessage &message);
    bool OnClickButtonApply(CGUIMessage &message);
    bool OnClickButtonCancel(CGUIMessage &message);
    bool OnClickButtonRadioTV(CGUIMessage &message);
    bool OnClickButtonRadioActive(CGUIMessage &message);
    bool OnClickButtonRadioParentalLocked(CGUIMessage &message);
    bool OnClickButtonEditName(CGUIMessage &message);
    bool OnClickButtonChannelLogo(CGUIMessage &message);
    bool OnClickButtonUseEPG(CGUIMessage &message);
    bool OnClickEPGSourceSpin(CGUIMessage &message);
    bool OnClickButtonGroupManager(CGUIMessage &message);
    bool OnClickButtonNewChannel();

    bool PersistChannel(const CFileItemPtr &pItem, const CPVRChannelGroupPtr &group, unsigned int *iChannelNumber);
    void SetItemsUnchanged(void);

    bool m_bIsRadio = false;
    bool m_bMovingMode = false;
    bool m_bContainsChanges = false;
    bool m_bAllowNewChannel = false;

    int m_iSelected = 0;
    CFileItemList* m_channelItems;
    CGUIViewControl m_viewControl;

    std::vector<CPVRClientPtr> m_clientsWithSettingsList;
  };
}
