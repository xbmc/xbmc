/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dialogs/GUIDialogContextMenu.h"
#include "guilib/GUIDialog.h"
#include "view/GUIViewControl.h"

#include <memory>
#include <vector>

class CAction;
class CFileItemList;
class CGUIMessage;

namespace PVR
{
  class CPVRChannelGroup;
  class CPVRClient;

  class CGUIDialogPVRChannelManager : public CGUIDialog
  {
  public:
    CGUIDialogPVRChannelManager();
    ~CGUIDialogPVRChannelManager() override;
    bool OnMessage(CGUIMessage& message) override;
    bool OnAction(const CAction& action) override;
    void OnWindowLoaded() override;
    void OnWindowUnload() override;
    bool HasListItems() const override{ return true; }
    CFileItemPtr GetCurrentListItem(int offset = 0) override;

    void Open(const std::shared_ptr<CFileItem>& initialSelection);

  protected:
    void OnInitWindow() override;
    void OnDeinitWindow(int nextWindowID) override;

  private:
    void Clear();
    void Update();
    void SaveList();
    void Renumber();
    void SetData(int iItem);
    void RenameChannel(const CFileItemPtr& pItem);

    bool OnPopupMenu(int iItem);
    bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
    bool OnActionMove(const CAction& action);
    bool OnMessageClick(CGUIMessage& message);
    bool OnClickListChannels(CGUIMessage& message);
    bool OnClickButtonOK(CGUIMessage& message);
    bool OnClickButtonApply(CGUIMessage& message);
    bool OnClickButtonCancel(CGUIMessage& message);
    bool OnClickButtonRadioTV(CGUIMessage& message);
    bool OnClickButtonRadioActive(CGUIMessage& message);
    bool OnClickButtonRadioParentalLocked(CGUIMessage& message);
    bool OnClickButtonEditName(CGUIMessage& message);
    bool OnClickButtonChannelLogo(CGUIMessage& message);
    bool OnClickButtonUseEPG(CGUIMessage& message);
    bool OnClickEPGSourceSpin(CGUIMessage& message);
    bool OnClickButtonGroupManager(CGUIMessage& message);
    bool OnClickButtonNewChannel();

    bool PersistChannel(const CFileItemPtr& pItem, const std::shared_ptr<CPVRChannelGroup>& group, unsigned int* iChannelNumber);
    void SetItemsUnchanged();

    bool m_bIsRadio = false;
    bool m_bMovingMode = false;
    bool m_bContainsChanges = false;
    bool m_bAllowNewChannel = false;
    bool m_bAllowRenumber = false;

    std::shared_ptr<CFileItem> m_initialSelection;
    int m_iSelected = 0;
    CFileItemList* m_channelItems;
    CGUIViewControl m_viewControl;

    std::vector<std::shared_ptr<CPVRClient>> m_clientsWithSettingsList;
  };
}
