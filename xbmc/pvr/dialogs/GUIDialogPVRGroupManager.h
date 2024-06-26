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
#include "view/GUIViewControl.h"

#include <memory>

class CFileItemList;
class CGUIMessage;

namespace PVR
{
class CPVRChannelGroup;

class CGUIDialogPVRGroupManager : public CGUIDialog
{
public:
  CGUIDialogPVRGroupManager();
  ~CGUIDialogPVRGroupManager() override;
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
  bool ActionButtonOk(const CGUIMessage& message);
  bool ActionButtonNewGroup(const CGUIMessage& message);
  bool ActionButtonDeleteGroup(const CGUIMessage& message);
  bool ActionButtonRenameGroup(const CGUIMessage& message);
  bool ActionButtonUngroupedChannels(const CGUIMessage& message);
  bool ActionButtonGroupMembers(const CGUIMessage& message);
  bool ActionButtonChannelGroups(const CGUIMessage& message);
  bool ActionButtonHideGroup(const CGUIMessage& message);
  bool ActionButtonToggleRadioTV(const CGUIMessage& message);
  bool ActionButtonRecreateThumbnail(const CGUIMessage& message);
  bool OnMessageClick(const CGUIMessage& message);
  bool OnPopupMenu(int itemNumber);
  bool OnContextButton(int itemNumber, int button);
  bool OnActionMove(const CAction& action);

  std::shared_ptr<CPVRChannelGroup> m_selectedGroup;
  bool m_bIsRadio;
  bool m_movingItem{false};
  bool m_allowReorder{false};

  int m_iSelectedUngroupedChannel = 0;
  int m_iSelectedGroupMember = 0;
  int m_iSelectedChannelGroup = 0;

  CFileItemList* m_ungroupedChannels;
  CFileItemList* m_groupMembers;
  CFileItemList* m_channelGroups;

  CGUIViewControl m_viewUngroupedChannels;
  CGUIViewControl m_viewGroupMembers;
  CGUIViewControl m_viewChannelGroups;

  CPVRThumbLoader m_thumbLoader;
};
} // namespace PVR
