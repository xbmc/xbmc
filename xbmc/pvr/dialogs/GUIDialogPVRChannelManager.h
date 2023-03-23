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
    void SetRadio(bool bIsRadio);

  protected:
    void OnInitWindow() override;
    void OnDeinitWindow(int nextWindowID) override;

  private:
    void Clear();
    void Update();
    void PromptAndSaveList();
    void SaveList();
    void Renumber();
    void SetData(int iItem);
    void RenameChannel(const CFileItemPtr& pItem);

    void ClearChannelOptions();
    void EnableChannelOptions(bool bEnable);

    bool OnPopupMenu(int iItem);
    bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
    bool OnActionMove(const CAction& action);
    bool OnMessageClick(const CGUIMessage& message);
    bool OnClickListChannels(const CGUIMessage& message);
    bool OnClickButtonOK();
    bool OnClickButtonApply();
    bool OnClickButtonCancel();
    bool OnClickButtonRadioTV();
    bool OnClickButtonRadioActive();
    bool OnClickButtonRadioParentalLocked();
    bool OnClickButtonEditName();
    bool OnClickButtonChannelLogo();
    bool OnClickButtonUseEPG();
    bool OnClickEPGSourceSpin();
    bool OnClickButtonGroupManager();
    bool OnClickButtonNewChannel();
    bool OnClickButtonRefreshChannelLogos();

    bool UpdateChannelData(const std::shared_ptr<CFileItem>& pItem,
                           const std::shared_ptr<CPVRChannelGroup>& group);

    bool HasChangedItems() const;
    void SetItemChanged(const CFileItemPtr& pItem);

    bool m_bIsRadio = false;
    bool m_bMovingMode = false;
    bool m_bAllowNewChannel = false;
    bool m_bAllowRenumber = false;
    bool m_bAllowReorder = false;

    std::shared_ptr<CFileItem> m_initialSelection;
    int m_iSelected = 0;
    CFileItemList* m_channelItems;
    CGUIViewControl m_viewControl;

    std::vector<std::shared_ptr<CPVRClient>> m_clientsWithSettingsList;
  };
}
