/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/PVRChannelNumberInputHandler.h"
#include "pvr/dialogs/GUIDialogPVRItemsViewBase.h"
#include "threads/SystemClock.h"

#include <map>
#include <memory>
#include <string>

namespace PVR
{
enum class PVREvent;

class CPVRChannelGroup;

class CGUIDialogPVRChannelsOSD : public CGUIDialogPVRItemsViewBase,
                                 public CPVRChannelNumberInputHandler
{
public:
  CGUIDialogPVRChannelsOSD();
  ~CGUIDialogPVRChannelsOSD() override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;

  /*!
   * @brief CEventStream callback for PVR events.
   * @param event The event.
   */
  void Notify(const PVREvent& event);

  // CPVRChannelNumberInputHandler implementation
  void GetChannelNumbers(std::vector<std::string>& channelNumbers) override;
  void OnInputDone() override;

protected:
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;
  void RestoreControlStates() override;
  void SaveControlStates() override;
  void SetInvalid() override;

private:
  void GotoChannel(int iItem);
  void Update();
  void SaveSelectedItemPath(int iGroupID);
  std::string GetLastSelectedItemPath(int iGroupID) const;

  std::shared_ptr<CPVRChannelGroup> m_group;
  std::map<int, std::string> m_groupSelectedItemPaths;
  XbmcThreads::EndTime<> m_refreshTimeout;
};
} // namespace PVR
